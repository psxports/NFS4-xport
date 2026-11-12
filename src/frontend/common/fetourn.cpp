#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "frontend/common/fecheats.h"
#include "frontend/common/femenu.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetracks.h"
#include "frontend/common/front.h"
#include "game/common/car.h"
#include "game/common/cars.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "lib/psx/blkmov.h"
#include "lib/psx/fixdmult.h"

struct tTrackInfo;

struct tSaveTournament;

struct tSaveTournament
{
    sint32 fSaveMoney;
    uint8 fSaveTier;
    uint8 fSaveTournament;
    uint8 fSaveTrack;
    uint8 fSaveNumRacers;
    tCompetitor fSaveCompetitors[6];
    uint8 fSaveBestPlacement[64];
    sint16 fSaveTierFinishPrize[4];
};

static sint32 retbill;
static sint32 retbonus;

static sint32 tournPointsCompare(const void *p1, const void *p2)
{
    FUNCTION_MARKER(0x80032A50, "FRONT.BIN");
    uint8 first = *static_cast<const uint8 *>(p1);
    uint8 second = *static_cast<const uint8 *>(p2);
    sint32 result = tournamentManager.fCompetitors[second].fPoints - tournamentManager.fCompetitors[first].fPoints;
    if (result == 0) {
        result = (sint8)tournamentManager.fCompetitors[second].fIsPlayerCar - (sint8)tournamentManager.fCompetitors[first].fIsPlayerCar;
        if (result == 0) {
            Car::tStats *dummyCars = Cars::gNewCarStatsList;
            result = (dummyCars[first].finalPosition - dummyCars[second].finalPosition);
        }
    }
    return result;
}

void tTournamentManager::Initialize(void)
{
    FUNCTION_MARKER(0x80032228, "FRONT.BIN");
    fMoney = 20000;
    fCurrentTrack = 0;
    fNumTiers = 0;
    fDefinition = 0;
    fTournamentList[0] = 0;
    fTrackList[0] = 0;
    for (sint16 i = 0; i < 6; ++i)
        fRanking[i] = 0;
    for (sint16 i = 0; i < 64; ++i)
        fBestPlacement[i] = 7;
    for (sint16 i = 0; i < 3; ++i) {
        fTierFinishPrize[i] = 6;
        fTierFinishPrizeChange[i] = 0;
    }
    char selectedCar = (char)(carManager.fNumCars - 1);
    frontEnd.garageCar[0] = selectedCar;
    frontEnd.garageCar[1] = selectedCar;
}

void tTournamentManager::LoadDescription(void)
{
    FUNCTION_MARKER(0x800322FC, "FRONT.BIN");
    char filename[80];
    sprintf(filename, "%s%s", Paths_Paths[37], "tourn.trn");
    ReleaseDescription();
    char *input = static_cast<char *>(loadfileadr(filename, 16));
    blockmove(input, fFinishPoints, 6);
    fNumTiers = input[6];
    fDefinition = static_cast<tTournamentDefinition *>(reservememadr("Tourney", 10532, 0));
    char *data = input + 7;
    for (sint16 tier = 0; tier < (uint8)fNumTiers; ++tier) {
        blockmove(data, &fDefinition->fTiers[tier], 12);
        sint16 firstTourney = fDefinition->fTiers[tier].fTournOffset;
        data += 12;
        for (sint16 tourney = firstTourney; tourney < firstTourney + fDefinition->fTiers[tier].fNumTournaments; ++tourney) {
            blockmove(data, &fDefinition->fTournaments[tourney], 84);
            sint16 firstTrack = fDefinition->fTournaments[tourney].fTrackOffset;
            data += 84;
            for (sint16 track = firstTrack; track < firstTrack + fDefinition->fTournaments[tourney].fNumTracks; ++track) {
                blockmove(data, &fDefinition->fTracks[track], 40);
                data += 40;
            }
        }
    }
    purgememadr(input);
}

void tTournamentManager::ReleaseDescription(void)
{
    FUNCTION_MARKER(0x80032548, "FRONT.BIN");
    if (fDefinition != 0) {
        purgememadr(fDefinition);
        fDefinition = 0;
    }
}

void tTournamentManager::UpdateTrackList(sint16 tier, sint16 tournament)
{
    FUNCTION_MARKER(0x80032584, "FRONT.BIN");
    tTourneyInfo *currentTourney = &fDefinition->fTournaments[fDefinition->fTiers[tier].fTournOffset + tournament];
    sint16 numTracks = currentTourney->fNumTracks;
    sint16 trackOffset = currentTourney->fTrackOffset;
    for (sint16 i = 0; i < numTracks; ++i) {
        fTrackList[i] = (sint8)fDefinition->fTracks[trackOffset].fTrackNumber + 213;
        ++trackOffset;
    }
    fTrackList[numTracks] = 0;
}

sint16 *tTournamentManager::GetTrackList(sint16 tier, sint16 tournament)
{
    FUNCTION_MARKER(0x80032654, "FRONT.BIN");
    UpdateTrackList(tier, tournament);
    return fTrackList;
}

void tTournamentManager::GetTrackToRace(tTrackInfo &track)
{
    FUNCTION_MARKER(0x8003268C, "FRONT.BIN");
    tTourneyInfo *tourn = &fDefinition->fTournaments[fDefinition->fTiers[fTier].fTournOffset + fTournament];
    blockmove(&fDefinition->fTracks[tourn->fTrackOffset + fCurrentTrack], &track, 40);
    track.fDirection = fDirection[fCurrentTrack];
    track.fMirrored = fMirror[fCurrentTrack];
    track.fTimeOfDay = fTimeOfDay[fCurrentTrack];
    track.fWeather = fWeather[fCurrentTrack];
}

void tTournamentManager::StartNewTournament(uint8 tier, uint8 tournament)
{
    FUNCTION_MARKER(0x80032784, "FRONT.BIN");
    fTier = tier;
    fTournament = tournament;
    fCurrentTrack = 0;
    fNumRacers = GetNumCompetitors();
    tTourneyInfo *tourn = &fDefinition->fTournaments[fDefinition->fTiers[fTier].fTournOffset + fTournament];
    for (sint16 i = 0; i < fNumRacers; ++i) {
        fCompetitors[i].fPoints = 0;
        fCompetitors[i].fEliminated = 0;
        fCompetitors[i].fIsPlayerCar = 0;
        if (i == 0)
            fCompetitors[i].fPersonality = kPersonalityNemesis;
        else
            fCompetitors[i].fPersonality = (tPersonalities)tourn->fPersonalities[i - 1];
        if (i == 0)
            fCompetitors[i].fPosition = (uint8)fNumRacers;
        else
            fCompetitors[i].fPosition = (uint8)i;
    }
    for (sint16 i = 0; i < tourn->fNumTracks; ++i) {
        tTrackInfo *track = &fDefinition->fTracks[tourn->fTrackOffset + i];
        fDirection[i] = track->fDirection;
        if (track->fDirection > 1)
            fDirection[i] = (char)(randPSX() & 1);
        fMirror[i] = track->fMirrored;
        if (track->fMirrored > 1)
            fMirror[i] = (char)(randPSX() & 1);
        fTimeOfDay[i] = track->fTimeOfDay;
        if (track->fTimeOfDay > 1)
            fTimeOfDay[i] = (char)(randPSX() & 1);
        fWeather[i] = track->fWeather;
        if (track->fWeather > 1)
            fWeather[i] = (char)(randPSX() & 1);
    }
    fCompetitors[0].fIsPlayerCar = 1;
}

sint16 tTournamentManager::IsTournamentFinished(void)
{
    FUNCTION_MARKER(0x800329BC, "FRONT.BIN");
    tTourneyInfo *tourn = &fDefinition->fTournaments[fDefinition->fTiers[fTier].fTournOffset + fTournament];
    if (fCurrentTrack < tourn->fNumTracks && (tourn->fKnockout == 0 || fCurrentTrack <= fCompetitors[0].fPoints))
        return 0;
    return 1;
}

void tTournamentManager::UpdateTournFinishMoney(void)
{
    FUNCTION_MARKER(0x80032ADC, "FRONT.BIN");
    tTourneyInfo *tourn = &fDefinition->fTournaments[fDefinition->fTiers[fTier].fTournOffset + fTournament];
    fPrevBestPlacement = fBestPlacement[(sint8)tourn->fTournamentID];
    if (tourn->fKnockout != 0) {
        sint32 prize = GetTournamentFinishPrize(fCompetitors[0].fPosition - 1);
        fAwards.fTournMoney = (fAwards.fTournMoney + prize);
    } else {
        for (uint8 i = 0; i < 6; ++i) {
            if (fRanking[i] == 0) {
                sint32 prize = GetTournamentFinishPrize(i);
                fAwards.fTournMoney = (fAwards.fTournMoney + prize);
                if (i < (sint8)fBestPlacement[(sint8)tourn->fTournamentID])
                    fBestPlacement[(sint8)tourn->fTournamentID] = (char)(i + 1);
            }
        }
    }
}

void tTournamentManager::UpdateTrackFinishMoney(void)
{
    FUNCTION_MARKER(0x80032C0C, "FRONT.BIN");
    Car::tStats *dummyCars = Cars::gNewCarStatsList;
    if (dummyCars[0].finalFinishType == 2) {
        sint32 prize = GetTrackFinishPrize((sint16)((uint16)dummyCars[0].finalPosition - 1));
        fAwards.fMoney = (fAwards.fMoney + prize);
    }
}

void Front::ResetOverlayAwards(void)
{
    retbill = 0;
    retbonus = 0;
}

void tTournamentManager::CalcTrackFinishDamageBill(bool recalculate, sint32 &bill, sint32 &bonus)
{
    FUNCTION_MARKER(0x80032C6C, "FRONT.BIN");
    if (recalculate) {
        Car::tStats *dummyCars = Cars::gNewCarStatsList;
        tCarInfo carInfo;
        carManager.GetGarageCar((uint8)frontEnd.garageCar[0], carInfo, 0);
        sint32 totalcarprice = carInfo.fPrices[0];
        for (sint32 i = 0; i < 2; ++i) {
            if ((carInfo.fUpgrades & (1U << i)) != 0)
                totalcarprice = (totalcarprice + carInfo.fPrices[i + 1]);
        }
        sint32 damage = dummyCars[0].finalDamage / 65536;
        if (damage == 0 && dummyCars[0].finalPosition < 4) {
            retbill = 0;
            retbonus = totalcarprice / 20;
            if (dummyCars[0].finalPosition == 2)
                retbonus = (sint32)((uint32)totalcarprice * 3UL) / 100;
            else if (dummyCars[0].finalPosition == 3)
                retbonus = totalcarprice / 100;
        } else {
            retbonus = 0;
            retbill = (sint32)(totalcarprice * (uint32)damage * 3UL) / 10000;
        }
    }
    bill = retbill;
    bonus = retbonus;
}

void tTournamentManager::UpdateTrackFinishPoints(void)
{
    FUNCTION_MARKER(0x80032E30, "FRONT.BIN");
    sint16 numCompetitors = GetNumCompetitors();
    Car::tStats *dummyCars = Cars::gNewCarStatsList;
    tTourneyInfo *tourn = &fDefinition->fTournaments[fDefinition->fTiers[fTier].fTournOffset + fTournament];
    if (tourn->fKnockout != 0) {
        sint16 k = 0;
        for (sint32 i = 0; i < numCompetitors; ++i) {
            if (fCompetitors[i].fEliminated == 0) {
                if (dummyCars[k].finalPosition - 1U < 6 && dummyCars[k].finalFinishType == 2) {
                    if (dummyCars[k].finalPosition < fNumRacers)
                        fCompetitors[i].fPoints = (uint16)(fCompetitors[i].fPoints + 1);
                    else
                        fCompetitors[i].fEliminated = 1;
                    fCompetitors[i].fPosition = (uint8)dummyCars[k].finalPosition;
                }
                ++k;
            }
        }
        fNumRacers = (fNumRacers - 1);
    } else {
        for (sint32 i = 0; i < numCompetitors; ++i) {
            uint32 place = dummyCars[i].finalPosition - 1U;
            if (place < 6 && dummyCars[i].finalFinishType == 2)
                fCompetitors[i].fPoints = (uint16)(fCompetitors[i].fPoints + fFinishPoints[place]);
        }
        for (sint32 i = 5; i >= 0; --i)
            fRanking[i] = (uint8)i;
        qsortPSX(fRanking, numCompetitors, 1, tournPointsCompare);
        for (sint32 i = 0; i < 6; ++i)
            fCompetitors[fRanking[i]].fPosition = (uint8)(i + 1);
    }
}

sint16 tTournamentManager::AdvanceToNextTrack(void)
{
    FUNCTION_MARKER(0x80033048, "FRONT.BIN");
    tTourneyInfo *currentTourney = &fDefinition->fTournaments[fDefinition->fTiers[fTier].fTournOffset + fTournament];
    if (fCurrentTrack > currentTourney->fNumTracks - 1)
        return -1;
    fAwards.fMoney = 0;
    fAwards.fTournMoney = 0;
    fAwards.fActivateFlags = 0;
    fAwards.fAwardCar = 0;
    UpdateTrackFinishMoney();
    UpdateTrackFinishPoints();
    if (fCurrentTrack == currentTourney->fNumTracks - 1) {
        UpdateTournFinishMoney();
        CalcTierFinishPrize();
    }
    fCurrentTrack = (fCurrentTrack + 1);
    if (fCurrentTrack >= currentTourney->fNumTracks) {
        fAwards.fAwardCarGarageFull = 0;
        fAwards.fCompletedGarageFull = 0;
        if (fCompetitors[0].fPosition < 4) {
            if ((currentTourney->fActivateFlags & 1) != 0) {
                fAwards.fActivateFlags |= 1;
                fAwards.fActivateTrack = currentTourney->fActivatedTrack;
            }
            if ((currentTourney->fActivateFlags & 2) != 0) {
                fAwards.fActivateFlags |= 2;
                fAwards.fActivateCarClass = (tCarClassType)currentTourney->fActivatedCarClass;
            }
            if ((currentTourney->fActivateFlags & 4) != 0) {
                fAwards.fActivateFlags |= 4;
                fAwards.fActivateCar = (tCarModels)currentTourney->fActivatedCar;
            }
            if ((currentTourney->fActivateFlags & 8) != 0) {
                fAwards.fActivateFlags |= 8;
                fAwards.fActivateTrackClass = (tTrackClassType)currentTourney->fActivatedTrackClass;
            }
            fAwards.fCompletedTier = 0;
        }
        if (fCompetitors[0].fPosition < 2) {
            if (currentTourney->fAwardCar != 0) {
                tCarInfo *carInfo = carManager.GetCarFromID(currentTourney->fAwardCarModel);
                fAwards.fAwardCar = 1;
                fAwards.fAwardCarModel = (tCarModels)currentTourney->fAwardCarModel;
                fAwards.fAwardCarColor = carInfo->fDefaultColor;
                fAwards.fAwardCarUpgrades = currentTourney->fAwardCarUpgrades;
                if (carManager.GetNumOwnedCars(0) >= 32) {
                    fAwards.fAwardCarGarageFull = 1;
                    fAwards.fTournMoney = (fAwards.fTournMoney + carInfo->fPrices[0]);
                    fAwards.fAwardCarBonusMoney = carInfo->fPrices[0];
                }
            }
            if ((currentTourney->fActivateFlags & 16) != 0) {
                fAwards.fActivateFlags |= 16;
                fAwards.fActivateCheat = currentTourney->fActivatedCheat;
            }
            fAwards.fCompletedTier = 0;
            fBestPlacement[(sint8)currentTourney->fTournamentID] = 1;
            if (FECheat::IsCheatEnabled((tCheatCode)(fTier + cheat_FinishedTournament)) != 1) {
                fAwards.fCompletedTier = 1;
                tTournamentDefinition *definition = fDefinition;
                tTierInfo *currentTier = &definition->fTiers[fTier];
                for (sint16 i = 0; i < currentTier->fNumTournaments; ++i) {
                    tTourneyInfo *tourn = &definition->fTournaments[currentTier->fTournOffset + i];
                    if ((sint8)fBestPlacement[(sint8)tourn->fTournamentID] != 1)
                        fAwards.fCompletedTier = 0;
                }
                if (fAwards.fCompletedTier != 0) {
                    sint32 numGarageCars = carManager.GetNumOwnedCars(0);
                    if (fAwards.fAwardCar != 0)
                        ++numGarageCars;
                    FECheat::ActivateBonus((tCheatCode)(fTier + cheat_FinishedTournament));
                    fAwards.fCompletedCar = (tCarModels)(fTier + cm_BonusCar1);
                    tCarInfo *carInfo = carManager.GetCarFromID((sint16)fAwards.fCompletedCar);
                    carManager.SetCarAvailable(fAwards.fCompletedCar, true);
                    carManager.SetCarViewable(fAwards.fCompletedCar, true);
                    if (numGarageCars < 32) {
                        carManager.PurchaseCar((sint16)fAwards.fCompletedCar, carInfo->fDefaultColor, 0);
                    } else {
                        fAwards.fCompletedGarageFull = 1;
                        fAwards.fCompletedBonusMoney = carInfo->fPrices[0];
                        fMoney = (fMoney + carInfo->fPrices[0]);
                    }
                    fAwards.fCompletedText = (sint16)((uint16)fTier + 985);
                }
            }
        }
    }
    return (sint16)fCurrentTrack;
}

sint16 tTournamentManager::GetLastTrackRaced(void)
{
    FUNCTION_MARKER(0x8003348C, "FRONT.BIN");
    tTourneyInfo *currentTourney = &fDefinition->fTournaments[fDefinition->fTiers[fTier].fTournOffset + fTournament];
    return (sint8)fDefinition->fTracks[currentTourney->fTrackOffset + fCurrentTrack - 1].fTrackNumber;
}

void tTournamentManager::SaveTournament(tSaveTournament &save)
{
    FUNCTION_MARKER(0x800334FC, "FRONT.BIN");
    save.fSaveMoney = fMoney;
    for (sint16 i = 0; i < 64; ++i)
        save.fSaveBestPlacement[i] = fBestPlacement[i];
}

void tTournamentManager::LoadTournament(tSaveTournament &load)
{
    FUNCTION_MARKER(0x80033544, "FRONT.BIN");
    fMoney = load.fSaveMoney;
    for (sint16 i = 0; i < 64; ++i)
        fBestPlacement[i] = load.fSaveBestPlacement[i];
}

sint16 tTournamentManager::GetNumCompetitors(void)
{
    FUNCTION_MARKER(0x8003358C, "FRONT.BIN");
    return fDefinition->fTournaments[fDefinition->fTiers[fTier].fTournOffset + fTournament].fNumCars;
}

void tTournamentManager::UpdateCarLineup(void)
{
    FUNCTION_MARKER(0x800335D8, "FRONT.BIN");
    sint16 numCompetitors = GetNumCompetitors();
    sint16 k = 0;
    for (sint16 i = 0; i < numCompetitors; ++i) {
        if (fCompetitors[i].fEliminated == 0) {
            fCarLineup[k].isPlayerCar = fCompetitors[i].fIsPlayerCar != 0;
            fCarLineup[k].personality = fCompetitors[i].fPersonality;
            fCarLineup[k].position = fCompetitors[i].fPosition;
            ++k;
        }
    }
}

sint32 tTournamentManager::GetTrackFinishPrize(sint16 position)
{
    FUNCTION_MARKER(0x80033688, "FRONT.BIN");
    sint32 openClassAdjust[7][6] = {
        { 1310, 1310, 1310, 1310, 1310, 1310 },
        { 1310, 1310, 1310, 1310, 1310, 1310 },
        { 1966, 1966, 1966, 1966, 1966, 1966 },
        { 2621, 2621, 2621, 2621, 2621, 2621 },
        { 1966, 1966, 1966, 1966, 1966, 1966 },
        { 1966, 1966, 1966, 1966, 1966, 1966 },
        { 1966, 1966, 1966, 1966, 1966, 1966 }
    };
    if ((uint16)position >= 6)
        return 0;
    tTourneyInfo *currentTourney = &fDefinition->fTournaments[fDefinition->fTiers[fTier].fTournOffset + fTournament];
    sint32 result = 0;
    if (currentTourney->fOpponentCarClass == 10) {
        tCarInfo carInfo;
        carManager.GetGarageCar((uint8)frontEnd.garageCar[0], carInfo, 0);
        if (carInfo.fCarClass < 7) {
            sint32 carPrice = carInfo.fPrices[0];
            for (sint32 i = 0; i < 3; ++i) {
                if ((carInfo.fUpgrades & (1U << i)) != 0)
                    carPrice = (carPrice + carInfo.fPrices[i + 1]);
            }
            result = fixedmult(openClassAdjust[carInfo.fCarClass][position], carPrice);
        }
    }
    return (result + fDefinition->fTracks[currentTourney->fTrackOffset + fCurrentTrack].fPrize[position]);
}

sint32 tTournamentManager::GetTournamentFinishPrize(sint16 position)
{
    FUNCTION_MARKER(0x80033860, "FRONT.BIN");
    if ((uint16)position >= 6)
        return 0;
    return fDefinition->fTournaments[fDefinition->fTiers[fTier].fTournOffset + fTournament].fPrize[position];
}

void tTournamentManager::GetAwardInformation(tAwardInformation &info)
{
    FUNCTION_MARKER(0x800338D4, "FRONT.BIN");
    const uint8 *source = reinterpret_cast<const uint8 *>(&fAwards);
    uint8 *destination = reinterpret_cast<uint8 *>(&info);
    for (sint32 offset = 0; offset < 64; offset += 16) {
        uint32 words[4];
        memcpy(words, source + offset, 16);
        memcpy(destination + offset, words, 16);
    }
    uint32 last;
    memcpy(&last, source + 64, 4);
    memcpy(destination + 64, &last, 4);
}

void tTournamentManager::UpdateAwardInformation(void)
{
    FUNCTION_MARKER(0x80033914, "FRONT.BIN");
    sint32 bill;
    sint32 bonus;
    CalcTrackFinishDamageBill(true, bill, bonus);
    fMoney = (fMoney - bill);
    fMoney = (fMoney + bonus);
    fMoney = (fMoney + fAwards.fMoney);
    fMoney = (fMoney + fAwards.fTournMoney);
    if ((fAwards.fActivateFlags & 2) != 0)
        FECheat::ActivateBonus((tCheatCode)fAwards.fActivateCarClass);
    if ((fAwards.fActivateFlags & 4) != 0)
        carManager.SetCarAvailable(fAwards.fActivateCar, true);
    if ((fAwards.fActivateFlags & 1) != 0)
        trackManager.SetTrackAvailable((uint8)fAwards.fActivateTrack, true);
    if ((fAwards.fActivateFlags & 8) != 0)
        trackManager.SetClassAvailable(fAwards.fActivateTrackClass, true);
    if ((fAwards.fActivateFlags & 16) != 0)
        FECheat::ActivateBonus((tCheatCode)fAwards.fActivateCheat);
    if (fAwards.fAwardCar != 0 && fAwards.fAwardCarGarageFull == 0) {
        tCarInfo *carInfo = carManager.GetCarFromID((sint16)fAwards.fAwardCarModel);
        carManager.PurchaseCar((sint16)fAwards.fAwardCarModel, (sint8)carInfo->fColorOrder[(uint8)fAwards.fAwardCarColor], 0);
        carManager.PurchaseUpgrade((uint8)frontEnd.garageCar[0], (uint8)fAwards.fAwardCarUpgrades, 0);
    }
}

sint16 tTournamentManager::TournPointTotal(sint16 *p)
{
    FUNCTION_MARKER(0x80033A8C, "FRONT.BIN");
    return fCompetitors[*p].fPoints;
}

sint16 tTournamentManager::PlayerRanking(sint16 pos)
{
    FUNCTION_MARKER(0x80033AA8, "FRONT.BIN");
    sint16 numCompetitors = GetNumCompetitors();
    for (sint16 i = 0; i < numCompetitors; ++i) {
        if (fCompetitors[i].fPosition == pos)
            return i;
    }
    return 0;
}

void tTournamentManager::CalcTierFinishPrize(void)
{
    FUNCTION_MARKER(0x80033B38, "FRONT.BIN");
    fTierFinishPrize[fTier] = 7;
    fTierFinishPrizeChange[fTier] = 0;
}

void tTournamentManager::GetTrophyName(tTourneyInfo *tourn, tTrophySize size, char *buffer, sint32 place)
{
    FUNCTION_MARKER(0x80033B64, "FRONT.BIN");
    char trophySizeLetter[3] = { 'S', 'M', 'L' };
    char trophyPlacementLetter[4] = { 'W', 'G', 'S', 'B' };
    sint32 showplace = (sint8)tournamentManager.fBestPlacement[(sint8)tourn->fTournamentID];
    if (place == -1)
        place = showplace >= 1 && showplace <= 3 ? showplace : 0;
    sprintf(buffer, "TR%c%02d%c", (uint8)trophySizeLetter[size], tourn->fTrophyID, (uint8)trophyPlacementLetter[place]);
}

bool tTournamentManager::ValidCar(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x80033C24, "FRONT.BIN");
    uint32 index = (uint32)fTournament + fDefinition->fTiers[fTier].fTournOffset;
    tTourneyInfo *tourney = &fDefinition->fTournaments[index];
    bool result = true;
    if (tourney->fOpponentCarClass != cct_OpenClass)
    {
        result = carInfo.fCarClass == tourney->fOpponentCarClass;
        if (FECheat::IsCheatEnabled(cheat_FinishedTournament) && fTier == 0)
            result = true;
    }
    if (!(tourney->fRequiredFlags & 4))
        return result;
    if ((sint8)carInfo.fCarID != tourney->fRequiredCar)
        return false;
    switch (tourney->fRequiredUpgrades)
    {
    case 0:
        return carInfo.fUpgrades == 0 ? result : false;
    case 1:
        return result;
    case 2:
        return carInfo.fUpgrades != 0 ? result : false;
    case 3:
        return (carInfo.fUpgrades & tourney->fSpecificUpgrades) == tourney->fSpecificUpgrades ? result : false;
    default:
        return result;
    }
}

tListIteratorTournament::tListIteratorTournament(char *valPtr, tTournamentManager *tournManager) : tListIterator(0, valPtr)
{
    FUNCTION_MARKER(0x80033D9C, "FRONT.BIN");
    fTournamentManager = tournManager;
}

tListIteratorTournament::~tListIteratorTournament(void)
{
    FUNCTION_MARKER(0x80033DEC, "FRONT.BIN");
}

char tListIteratorTournament::Value(tPlayer)
{
    FUNCTION_MARKER(0x80033E14, "FRONT.BIN");
    return *fValue;
}

sint16 tListIteratorTournament::TextValue(tPlayer)
{
    FUNCTION_MARKER(0x80033E28, "FRONT.BIN");
    tTournamentDefinition *definition = fTournamentManager->fDefinition;
    sint16 tournIndex = definition->fTiers[(uint8)frontEnd.tier].fTournOffset + (uint8)*fValue;
    return (sint8)definition->fTournaments[tournIndex].fTournamentID + 833;
}

void tListIteratorTournament::Increment(tPlayer)
{
    FUNCTION_MARKER(0x80033E80, "FRONT.BIN");
    tTierInfo *tier = &fTournamentManager->fDefinition->fTiers[(uint8)frontEnd.tier];
    do {
        char *value = fValue;
        *value = (char)((uint8)*value + 1);
        value = fValue;
        if ((uint8)*value >= tier->fNumTournaments) *value = 0;
    } while (ValidTournament(*fValue) != 1);
}

void tListIteratorTournament::Decrement(tPlayer)
{
    FUNCTION_MARKER(0x80033F20, "FRONT.BIN");
    tTierInfo *tier = &fTournamentManager->fDefinition->fTiers[(uint8)frontEnd.tier];
    do {
        char *value = fValue;
        uint8 current = (uint8)*value;
        if (current == 0)
            current = tier->fNumTournaments;
        *value = (char)(current - 1);
    } while (ValidTournament(*fValue) != 1);
}

bool tListIteratorTournament::ValidTournament(char tourn)
{
    FUNCTION_MARKER(0x80033FB0, "FRONT.BIN");
    tTournamentManager *manager = fTournamentManager;
    tTierInfo *currentTier = &manager->fDefinition->fTiers[(uint8)frontEnd.tier];
    tTourneyInfo *currentTourn = &manager->fDefinition->fTournaments[currentTier->fTournOffset + (uint8)tourn];
    uint16 requiredFlags = currentTourn->fRequiredFlags;
    bool result = true;
    if ((requiredFlags & 1) != 0)
        result = (sint8)manager->fBestPlacement[currentTourn->fRequiredTournamentID] < 4;
    if ((requiredFlags & 2) != 0 && (sint8)manager->fBestPlacement[currentTourn->fRequiredTournamentID] >= 2)
        result = false;
    return result;
}
