#ifndef FETOURN_H
#define FETOURN_H
#include "xport.h"
#include "frontend/common/fecars.h"
#include "frontend/common/personalities.h"
#include "frontend/common/trackclass.h"
#include "frontend/common/trackinfo.h"

struct tTierInfo
{
    uint8 fNumTournaments;
    uint8 fDescriptionID;
    uint8 fTournOffset;
    uint8 fPad;
    uint8 fReserved[8];
};

struct tTourneyInfo
{
    char fTournamentID;
    uint8 fNumTracks;
    uint8 fTrackOffset;
    uint8 fOpponentCarClass;
    uint8 fTraffic;
    uint8 fKnockout;
    uint8 fNumCars;
    uint8 fAwardCar;
    uint8 fAwardCarModel;
    uint8 fAwardCarUpgrades;
    uint16 fActivateFlags;
    uint16 fRequiredFlags;
    uint8 fActivatedTrack;
    uint8 fActivatedCarClass;
    uint8 fActivatedCar;
    uint8 fRequiredTournamentID;
    uint8 fRequiredCar;
    uint8 fRequiredUpgrades;
    uint8 fSpecificUpgrades;
    uint8 fRequiredCheatID;
    uint8 fTrophyID;
    uint8 fPad;
    sint32 fPrize[6];
    sint32 fEntranceFee;
    uint8 fPersonalities[5];
    uint8 fOpponentCar[5];
    uint8 fOpponentUpgrades[5];
    uint8 fActivatedTrackClass;
    uint8 fActivatedCheat;
    uint8 fNumLaps;
    uint8 fReserved[14];
};

struct tTournamentDefinition
{
    tTierInfo fTiers[3];
    tTourneyInfo fTournaments[64];
    tTrackInfo fTracks[128];
};

struct tTrackInfo;

struct tSaveTournament;

struct tTourneyInfo;

struct tTournamentDefinition;

enum tTrophySize
{
    ts_Small = 0,
    ts_Medium = 1,
    ts_Large = 2,
};

struct tCompetitor
{
    tPersonalities fPersonality;
    uint8 fVariation;
    char fIsPlayerCar;
    sint32 fEliminated;
    uint16 fPoints;
    uint8 fPosition;
};

struct tCarLineup
{
    sint32 isPlayerCar;
    tPersonalities personality;
    char position;
    tCarModels carModel;
    char carColor;
    char carUpgrades;
};

struct tAwardInformation
{
    sint32 fMoney;
    sint32 fTournMoney;
    uint16 fActivateFlags;
    char fActivateTrack;
    tCarClassType fActivateCarClass;
    tCarModels fActivateCar;
    tTrackClassType fActivateTrackClass;
    sint32 fActivateCheat;
    sint32 fAwardCar;
    sint32 fAwardCarGarageFull;
    sint32 fAwardCarBonusMoney;
    tCarModels fAwardCarModel;
    char fAwardCarColor;
    char fAwardCarUpgrades;
    sint32 fCompletedTier;
    sint16 fCompletedText;
    tCarModels fCompletedCar;
    sint32 fCompletedGarageFull;
    sint32 fCompletedBonusMoney;
};

class tTournamentManager
{
public:
    char fNumTiers;
    sint32 fTier;
    sint32 fTournament;
    sint32 fCurrentTrack;
    sint32 fNumRacers;
    sint32 fMoney;
    tTournamentDefinition *fDefinition;
    sint16 fTierList[4];
    sint16 fTierFinishPrize[4];
    sint16 fTierFinishPrizeChange[4];
    sint16 fTournamentList[65];
    sint16 fTrackList[17];
    char fDirection[16];
    char fMirror[16];
    char fTimeOfDay[16];
    char fWeather[16];
    tCompetitor fCompetitors[6];
    tCarLineup fCarLineup[6];
    char fBestPlacement[64];
    char fPrevBestPlacement;
    uint8 fFinishPoints[6];
    uint8 fRanking[6];
    tAwardInformation fAwards;
    void Initialize(void);
    void LoadDescription(void);
    void ReleaseDescription(void);
    void UpdateTrackList(sint16 tier, sint16 tournament);
    sint16 *GetTrackList(sint16 tier, sint16 tournament);
    void GetTrackToRace(tTrackInfo &track);
    void StartNewTournament(uint8 tier, uint8 tournament);
    sint16 IsTournamentFinished(void);
    void UpdateTournFinishMoney(void);
    void UpdateTrackFinishMoney(void);
    void CalcTrackFinishDamageBill(bool recalculate, sint32 &bill, sint32 &bonus);
    void UpdateTrackFinishPoints(void);
    sint16 AdvanceToNextTrack(void);
    sint16 GetLastTrackRaced(void);
    void SaveTournament(tSaveTournament &save);
    void LoadTournament(tSaveTournament &load);
    sint16 GetNumCompetitors(void);
    void UpdateCarLineup(void);
    sint32 GetTrackFinishPrize(sint16 position);
    sint32 GetTournamentFinishPrize(sint16 position);
    void GetAwardInformation(tAwardInformation &info);
    void UpdateAwardInformation(void);
    sint16 TournPointTotal(sint16 *p);
    sint16 PlayerRanking(sint16 pos);
    void CalcTierFinishPrize(void);
    void GetTrophyName(tTourneyInfo *tourn, tTrophySize size, char *buffer, sint32 place);
    bool ValidCar(tCarInfo &carInfo);
};

extern tTournamentManager tournamentManager;

#endif
