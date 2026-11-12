#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/feapp.h"
#include "frontend/common/fecheats.h"
#include "frontend/common/fedialog.h"
#include "frontend/common/fememcard.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/femenuoptions.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetracks.h"
#include "frontend/common/screenaudio.h"
#include "frontend/common/screencarselect.h"
#include "frontend/common/screencongrats.h"
#include "frontend/common/screencontroller.h"
#include "frontend/common/screendisplay.h"
#include "frontend/common/screenmain.h"
#include "frontend/common/screenpinkslips.h"
#include "frontend/common/screenpost.h"
#include "frontend/common/screentournselect.h"
#include "frontend/common/screentrackinfo.h"
#include "frontend/common/screentrackrecords.h"
#include "frontend/common/screentracks.h"
#include "frontend/common/screentrophyinfo.h"
#include "frontend/common/screentrophyroom.h"
#include "frontend/common/screenusername.h"
#include "frontend/common/statchk.h"
#include "game/common/audiocmn.h"
#include "game/common/car.h"
#include "game/common/cars.h"
#include "game/common/gmesetup.h"
#include "game/common/nfs3.h"
#include "game/common/textsys.h"

struct tTrackInformation
{
    char fTrackID;
    uint8 fSimNumber;
    uint8 fTrackDifficulty;
    uint8 fAvailable;
    uint8 fIsEgg;
    uint8 fLengthKM;
    uint8 fLengthMiles;
    uint8 fNumMoments;
    char fShapeName[8];
    char fSplineName[8];
    char fCountry;
    char fDispatch;
    char fReverseCall;
    char fLanguage;
    char fTrafficCars[6];
    sint16 fTX;
    sint16 fTY;
    sint16 fSX;
    sint16 fSY;
    uint8 fSpeedoCountry;
    uint8 fPad;
    sint32 fRotate;
};

static tCarModels gCarActivation[6][5] =
{
    { cm_CorvetteCop, cm_CorvetteCop, cm_CorvetteCop, cm_CorvetteCop, cm_CorvetteCop },
    { cm_BMWM5Cop, cm_BMWM5Cop, cm_BMWM5Cop, cm_BMWM5Cop, cm_BMWM5Cop },
    { cm_PorscheCop, cm_PorscheCop, cm_PorscheCop, cm_DiabloCop, cm_PorscheCop },
    { cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop },
    { cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop },
    { cm_Helicopter, cm_Helicopter, cm_Helicopter, cm_Helicopter, cm_Helicopter },
};

sint16 SelectListNull[2];
sint16 SelectListOffOn[3] = { 103, 102, 0 };
sint16 SelectListTrackDirection[3] = { 228, 229, 0 };
sint16 SelectListNormal[3] = { 227, 225, 0 };
sint16 SelectListOpponentUpgrades[3] = { 271, 272, 0 };
sint16 SelectListTransmission[3] = { 276, 275, 0 };
sint16 SelectListDisplaySpeed[5] = { 500, 498, 499, 103, 0 };
sint16 SelectListDisplayMap[4] = { 103, 488, 489, 0 };
sint16 SelectListDisplayOpponentID[4] = { 103, 501, 502, 0 };
sint16 SelectListSplitTime[4] = { 103, 490, 491, 0 };
sint16 SelectListSplitDisplay[3] = { 492, 493, 0 };
sint16 SelectListUpgrades[4] = { 150, 151, 152, 0 };
sint16 SelectListControllerConfig[4] = { 539, 540, 541, 0 };
sint16 SelectListAudioMode[4] = { 466, 467, 468, 0 };
tGlobalMenuDefs *menuDefs;

sint32 AskTheUserToSaveTheGame(void)
{
    FUNCTION_MARKER(0x8002C41C, "FRONT.BIN");
    if (FECheat::IsTheUserACryBabyCheater())
        return 0;
    return ::SaveGame(0);
}

bool GenericMenuSaveGame(sint32)
{
    FUNCTION_MARKER(0x8002C9A0, "FRONT.BIN");
    return ::SaveGame(0);
}

bool PinkSlipsPreSave(void)
{
    FUNCTION_MARKER(0x8002CABC, "FRONT.BIN");
    if (FECheat::IsTheUserACryBabyCheater())
        return true;
    return GenericMenuSaveGame(0);
}

void GenericMenuLoadGame(sint32 player)
{
    FUNCTION_MARKER(0x8002D83C, "FRONT.BIN");
    if (CURRENTLYUSINGMEMCARD == 0)
        ::LoadGame((sint16)player, false, false);
}

void MenuExtended::SetOnePlayer(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002C0E0, "FRONT.BIN");
    if ((uint8)FEApp->fInputPlayer == 1)
        command.type = kMenu_Command_None;
    else {
        frontEnd.gameMode = 0;
        screenMain->SwapBackground(-1);
    }
}

void MenuExtended::SetTwoPlayer(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002C12C, "FRONT.BIN");
    if (gPadinfo.buf[4].nopad != 0)
        command.type = kMenu_Command_None;
    else {
        frontEnd.gameMode = 1;
        frontEnd.oppNumber = 0;
        screenMain->SwapBackground(-1);
    }
}

void MenuExtended::SetTestDrive(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002C17C, "FRONT.BIN");
    frontEnd.carListType = 0;
    frontEnd.oppNumber = 0;
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.raceType = 0;
}

void MenuExtended::SetSingleRace(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002C198, "FRONT.BIN");
    frontEnd.carListType = 1;
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.raceType = 0;
    screenMain->SwapBackground(-1);
}

void MenuExtended::SetTournament(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002C1D8, "FRONT.BIN");
    frontEnd.carListType = 1;
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.tier = 0;
    frontEnd.raceType = 2;
    screenMain->SwapBackground(-1);
}

void MenuExtended::SetSpecialEvent(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002C220, "FRONT.BIN");
    frontEnd.carListType = 1;
    frontEnd.tier = 1;
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.raceType = 2;
    screenMain->SwapBackground(-1);
}

void MenuExtended::SetSoloRace(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002C268, "FRONT.BIN");
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.raceType = 0;
    frontEnd.oppNumber = 0;
    frontEnd.carListType = 1;
}

void MenuExtended::GoToTwoPlayerSingleRace(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002C288, "FRONT.BIN");
    tDialogYesNoTri YesNoDialog;
    if ((carManager.GetNumOwnedCars(0)) <= 0) {
        YesNoDialog.string = TextSys::Word(0x42);
        YesNoDialog.yesnowords[0] = 0x321;
        YesNoDialog.yesnowords[1] = 0x322;
        YesNoDialog.fDefault = 0;
        if (YesNoDialog.Run() == 1) {
            tGlobalMenuDefs *menus = menuDefs;
            frontEnd.raceType = 0;
            command.type = kMenu_Command_GoToMenu;
            menus->iteratorDealerCar.Decrement(kPlayerBoth);
            menuDefs->iteratorDealerCar.Increment(kPlayerBoth);
            command.nextMenu = &menuDefs->menuCarDealer;
            screenCarSelect->SetState(2);
        }
    } else {
        SetSoloRace(command);
        command.type = kMenu_Command_GoToMenu;
        command.nextMenu = &menuDefs->menuSingleTrackSelect;
    }
}

void MenuExtended::SetDuelRace(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002C39C, "FRONT.BIN");
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.raceType = 0;
    frontEnd.oppNumber = 1;
    screenMain->SwapBackground(-1);
}

void MenuExtended::SetFullGrid(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002C3DC, "FRONT.BIN");
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.raceType = 0;
    frontEnd.oppNumber = 2;
    screenMain->SwapBackground(-1);
}

void MenuExtended::TransitionFromPostGameToMainMenu(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002C494, "FRONT.BIN");
    tGlobalMenuDefs *menus = menuDefs;
    command.type = kMenu_Command_GoToMenuOneWay;
    command.nextMenu = &menus->menuMain;
}

void MenuExtended::TransitionFromPostGameToMainMenuAndSaveGame(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002C4B0, "FRONT.BIN");
    AskTheUserToSaveTheGame();
    TransitionFromPostGameToMainMenu(command);
}

void MenuExtended::GoToCarSelect(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002C4FC, "FRONT.BIN");

    tDialogMessageString *popUp = &FEApp->messagePopup;
    tTrackInformation trackInfo;
    uint8 trackIndex = (uint8)frontEnd.pinkSlipsTrackIndex;
    uint8 *frontBytes = reinterpret_cast<uint8 *>(&frontEnd);
    trackManager.GetTrack(frontBytes[offsetof(tfrontEnd, track) + trackIndex], trackInfo);
    if (trackInfo.fAvailable == 0)
    {
        popUp->string = TextSys::Word(235);
        popUp->Display();
        return;
    }
    if (frontEnd.gameMode == 1)
    {
        command.type = kMenu_Command_GoToMenuTwoPlayer;
        if (frontEnd.raceType == 6)
            command.nextMenu = &menuDefs->menuPlayerOnePinkSlipCarSelect;
        else if (frontEnd.carListType == 0)
            command.nextMenu = &menuDefs->menuPlayerOneCarSelect;
        else
            command.nextMenu = &menuDefs->menuPlayerOneGarage;
    }
    else
    {
        command.type = kMenu_Command_GoToMenu;
        if (frontEnd.oppNumber == 1)
        {
            if (frontEnd.raceType == 1)
                command.nextMenu = &menuDefs->menuHPDuelCarSelect;
            else
                command.nextMenu = &menuDefs->menuDuelCarSelect;
            screenCarSelectDuel->SetState(7);
        }
        else
        {
            tScreenCarSelect *screen = screenCarSelect;
            bool garage = frontEnd.carListType != 0;
            if (garage)
                command.nextMenu = &menuDefs->menuCarGarage;
            else
                command.nextMenu = &menuDefs->menuSingleCarSelect;
            screen->SetState(garage);
        }
    }
    if (frontEnd.raceType == 1)
    {
        if (frontEnd.oppNumber == 0 || frontEnd.gameMode == 1)
            menuDefs->iteratorCar1.fCarListFilter = 9;
        else
            menuDefs->iteratorCar1.fCarListFilter = 1;
    }
    else if (frontEnd.raceType == 6)
        menuDefs->iteratorPinkSlipsCar.fCarListFilter = 0x20;
    else if (frontEnd.carListType == 0)
        menuDefs->iteratorCar1.fCarListFilter = 1;
    else if (frontEnd.raceType == 2)
        menuDefs->iteratorGarageCar.fCarListFilter = 0x40;
    else
        menuDefs->iteratorGarageCar.fCarListFilter = 2;
    if (frontEnd.carListType == 0)
    {
        menuDefs->iteratorCar1.Decrement(kPlayerOne);
        menuDefs->iteratorCar1.Increment(kPlayerOne);
        menuDefs->iteratorCar1.Decrement(kPlayerTwo);
        menuDefs->iteratorCar1.Increment(kPlayerTwo);
    }
    else if (carManager.GetNumOwnedCars(0) > 0 || (frontEnd.raceType == 1 && frontEnd.oppNumber != 1))
    {
        menuDefs->iteratorGarageCar.Decrement(kPlayerOne);
        menuDefs->iteratorGarageCar.Increment(kPlayerOne);
        if (frontEnd.gameMode == 1)
        {
            menuDefs->iteratorGarageCar.Decrement(kPlayerTwo);
            menuDefs->iteratorGarageCar.Increment(kPlayerTwo);
        }
    }
    if (frontEnd.oppNumber == 1 && frontEnd.raceType != 2)
    {
        menuDefs->iteratorOpponentCar.Decrement(kPlayerBoth);
        menuDefs->iteratorOpponentCar.Increment(kPlayerBoth);
        menuDefs->iteratorOpponentCar.fCarListFilter = 1;
    }
}

void MenuExtended::GoToDealer(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002C890, "FRONT.BIN");
    tScreenCarSelect *screen = screenCarSelect;
    tGlobalMenuDefs *menus = menuDefs;
    command.type = kMenu_Command_GoToMenu;
    command.nextMenu = &menus->menuCarDealer;
    screen->SetState(2);
    menuDefs->iteratorDealerCar.Decrement(kPlayerBoth);
    menuDefs->iteratorDealerCar.Increment(kPlayerBoth);
}

void MenuExtended::GoToSeller(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002C8F8, "FRONT.BIN");
    tScreenCarSelect *screen = screenCarSelect;
    tGlobalMenuDefs *menus = menuDefs;
    command.type = kMenu_Command_GoToMenu;
    command.nextMenu = &menus->menuCarSeller;
    screen->SetState(3);
    menuDefs->iteratorSellerCar.Decrement(kPlayerBoth);
    menuDefs->iteratorSellerCar.Increment(kPlayerBoth);
}

void MenuExtended::GoToUpgrades(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002C960, "FRONT.BIN");
    command.type = kMenu_Command_GoToMenu;
    command.nextMenu = &menuDefs->menuCarUpgrades;
    screenCarSelect->SetState(4);
}

void MenuExtended::GoToBestOfOne(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002CB84, "FRONT.BIN");

    if (PinkSlipsPreSave())
    {
        tGlobalMenuDefs *defs = menuDefs;
        command.type = kMenu_Command_GoToMenu;
        command.nextMenu = &defs->menuSingleTrackSelect;
    }
    else
        command.type = kMenu_Command_None;
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.pinkSlipsNumTracks = 1;
}

void MenuExtended::GoToBestOfThree(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002CBE0, "FRONT.BIN");

    if (PinkSlipsPreSave())
    {
        tGlobalMenuDefs *defs = menuDefs;
        command.type = kMenu_Command_GoToMenu;
        command.nextMenu = &defs->menuPinkSlipsBestOfThree;
    }
    else
        command.type = kMenu_Command_None;
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.pinkSlipsNumTracks = 3;
}

void MenuExtended::GoToBestOfFive(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002CC3C, "FRONT.BIN");

    if (PinkSlipsPreSave())
    {
        tGlobalMenuDefs *defs = menuDefs;
        command.type = kMenu_Command_GoToMenu;
        command.nextMenu = &defs->menuPinkSlipsBestOfFive;
    }
    else
        command.type = kMenu_Command_None;
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.pinkSlipsNumTracks = 5;
}

void MenuExtended::GoToRace(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002CC98, "FRONT.BIN");

    tDialogMessageString *popUp = &FEApp->messagePopup;
    command.type = kMenu_Command_StartRace;
    tCarInfo carInfo;
    sint32 message = 170;
    if (frontEnd.carListType != 1 || carManager.GetNumOwnedCars(0) > 0 || frontEnd.raceType == 1 || frontEnd.raceType == 6)
    {
        if (frontEnd.raceType == 2 && carManager.GetNumTourneyCars(0) <= 0)
            message = 241;
        else if (frontEnd.raceType == 1 && (carManager.GetStockCar((uint8)frontEnd.playerCar[0], carInfo), carInfo.fPursuitAvailable == 0))
            message = 242;
        else
        {
            if (frontEnd.carListType != 0)
                return;
            carManager.GetStockCar((uint8)frontEnd.playerCar[0], carInfo);
            message = 243;
            if (carInfo.fAvailable != 0)
                return;
        }
    }
    popUp->string = TextSys::Word(message);
    popUp->Display();
    command.type = kMenu_Command_None;
}

void MenuExtended::GoTo2PlayerRace(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002CDF4, "FRONT.BIN");

    tDialogMessageString *popUp = &FEApp->messagePopup;
    command.type = kMenu_Command_Start2PlayerRace;
    if (frontEnd.raceType == 6)
        return;
    tCarInfo carInfo;
    sint32 message = 170;
    if (frontEnd.carListType != 1 || carManager.GetNumOwnedCars(0) > 0 || frontEnd.raceType == 1)
    {
        if (frontEnd.carListType == 0)
        {
            uint8 player = (uint8)FEApp->fPlayer;
            uint8 *frontBytes = reinterpret_cast<uint8 *>(&frontEnd);
            carManager.GetStockCar(frontBytes[offsetof(tfrontEnd, playerCar) + player], carInfo);
            if (carInfo.fAvailable == 0)
            {
                message = 243;
                goto showError;
            }
        }
        if (frontEnd.raceType != 1)
            return;
        uint8 player = (uint8)FEApp->fPlayer;
        uint8 *frontBytes = reinterpret_cast<uint8 *>(&frontEnd);
        carManager.GetStockCar(frontBytes[offsetof(tfrontEnd, playerCar) + player], carInfo);
        message = 242;
        if (carInfo.fPursuitAvailable != 0)
            return;
    }
showError:
    popUp->string = TextSys::Word(message);
    popUp->Display();
    command.type = kMenu_Command_None;
}

void MenuExtended::GoToTournTrackInfo(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002CF40, "FRONT.BIN");

    tTournamentDefinition *definition = tournamentManager.fDefinition;
    uint8 tournament = frontEnd.tournament;
    frontEnd.tier = 0;
    tTourneyInfo *tourn = &definition->fTournaments[definition->fTiers[0].fTournOffset + tournament];
    sint32 entranceFee = tourn->fEntranceFee;
    if (entranceFee > 0)
    {
        if (tournamentManager.fMoney < entranceFee)
        {
            tDialogMessageString *popUp = &FEApp->messagePopup;
            popUp->string = TextSys::Word(246);
            popUp->Display();
            return;
        }
        tDialogYesNo popUp;
        popUp.string = TextSys::Word(247);
        popUp.yesnowords[0] = 802;
        popUp.yesnowords[1] = 801;
        popUp.fDefault = 0;
        if (popUp.Run() == 0)
            return;
        AudioCmn::PlayFESFX(26);
        sint32 amount = tourn->fEntranceFee;
        tournamentManager.fMoney = (tournamentManager.fMoney - amount);
    }
    tournamentManager.StartNewTournament(0, (uint8)frontEnd.tournament);
    tGlobalMenuDefs *defs = menuDefs;
    command.type = kMenu_Command_GoToMenu;
    command.nextMenu = &defs->menuTrackInfo;
}

void MenuExtended::GoToSpecialEventTrackInfo(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002D0A8, "FRONT.BIN");

    tTournamentDefinition *definition = tournamentManager.fDefinition;
    frontEnd.tier = 1;
    tTourneyInfo *tourn = &definition->fTournaments[definition->fTiers[1].fTournOffset + (uint8)frontEnd.specialevent];
    sint32 entranceFee = tourn->fEntranceFee;
    if (entranceFee > 0)
    {
        if (tournamentManager.fMoney < entranceFee)
        {
            tDialogMessageString *popUp = &FEApp->messagePopup;
            popUp->string = TextSys::Word(246);
            popUp->Display();
            return;
        }
        tDialogYesNo popUp;
        popUp.string = TextSys::Word(247);
        popUp.yesnowords[0] = 801;
        popUp.yesnowords[1] = 802;
        popUp.fDefault = 0;
        if (popUp.Run() == 0)
            return;
        AudioCmn::PlayFESFX(26);
        sint32 amount = tourn->fEntranceFee;
        tournamentManager.fMoney = (tournamentManager.fMoney - amount);
    }
    tournamentManager.StartNewTournament(1, (uint8)frontEnd.specialevent);
    tGlobalMenuDefs *defs = menuDefs;
    command.type = kMenu_Command_GoToMenu;
    command.nextMenu = &defs->menuTrackInfo;
}

void MenuExtended::EnterUserName(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002D214, "FRONT.BIN");

    tGlobalMenuDefs *defs = menuDefs;
    uint8 player = (uint8)FEApp->fInputPlayer;
    uint8 namePlayer = (uint8)FEApp->fInputPlayer;
    tUserNameMenuItem &item = defs->menuItemUserName;
    item.fMaxStringLength = 7;
    char *data = frontEnd.playerNameList[namePlayer];
    tScreenUserName *screen = screenUserName;
    tOptionsMenu *m = &defs->menuUserName;
    item.fCurrentRow = 0;
    item.fCurrentColumn = 0;
    item.fPlayer = player;
    item.fData = data;
    screen->callingMenu = m;
    command.type = kMenu_Command_GoToMenu;
    command.nextMenu = m;
}

void MenuExtended::GoToShowroom(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002D278, "FRONT.BIN");
    command.type = kMenu_Command_GoToMenu;
    command.nextMenu = &menuDefs->menuShowroom;
    screenCarSelect->SetState(5);
}

void MenuExtended::GoToDealerShowroom(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002D2B8, "FRONT.BIN");
    command.type = kMenu_Command_GoToMenu;
    command.nextMenu = &menuDefs->menuShowroom;
    screenCarSelect->SetState(6);
}

void MenuExtended::SetHPSoloRace(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002D2F8, "FRONT.BIN");
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.raceType = 1;
    frontEnd.oppNumber = 0;
    menuDefs->iteratorCar1.fCarListFilter = 0xB;
}

void MenuExtended::SetHPDuelRace(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002D324, "FRONT.BIN");
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.raceType = 1;
    frontEnd.oppNumber = 1;
    menuDefs->iteratorCar1.fCarListFilter = 1;
}

void MenuExtended::SetHotPursuit(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002D350, "FRONT.BIN");
    frontEnd.carListType = 0;
    frontEnd.pinkSlipsTrackIndex = 0;
    frontEnd.raceType = 1;
    if ((uint8)frontEnd.gameMode == 1)
        menuDefs->iteratorCar1.fCarListFilter = 9;
    else
        menuDefs->iteratorCar1.fCarListFilter = 1;
    frontEnd.oppNumber = 0;
}

void MenuExtended::SellCar(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002D3A0, "FRONT.BIN");

    sint32 money = tournamentManager.fMoney;
    sint32 usedPrice = carManager.CalcUsedPrice((uint8)frontEnd.garageCar[0]);
    money = (money + usedPrice);
    bool canSell = carManager.GetNumOwnedCars(0) >= 2;
    if (!canSell)
        canSell = money >= carManager.CheapestCarStockPrice();
    if (canSell)
    {
        tDialogYesNo popUp;
        popUp.string = TextSys::Word(165);
        popUp.yesnowords[0] = 801;
        popUp.yesnowords[1] = 802;
        popUp.fDefault = 0;
        if (popUp.Run() != 0)
        {
            sint32 price = carManager.SellCar((uint8)frontEnd.sellerCar, 0);
            tournamentManager.fMoney = (tournamentManager.fMoney + price);
            menuDefs->iteratorSellerCar.Decrement(kPlayerOne);
            menuDefs->iteratorSellerCar.Increment(kPlayerOne);
            AudioCmn::PlayFESFX(26);
        }
    }
    else
    {
        tDialogMessageString *popUp = &FEApp->messagePopup;
        popUp->string = TextSys::Word(169);
        popUp->Display();
    }
}

void MenuExtended::BuyCar(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002D4F8, "FRONT.BIN");

    tCarInfo carInfo;
    tDialogMessageString *popUp = &FEApp->messagePopup;
    carManager.GetStockCar((uint8)frontEnd.dealerCar, carInfo);
    if (carManager.GetNumOwnedCars(0) >= 32)
    {
        popUp->string = TextSys::Word(75);
        popUp->Display();
        return;
    }
    if (tournamentManager.fMoney < carInfo.fPrices[0])
    {
        AudioCmn::PlayFESFX(10);
        popUp->string = TextSys::Word(167);
        popUp->Display();
        return;
    }
    tDialogYesNo yesNo;
    yesNo.string = TextSys::Word(164);
    yesNo.yesnowords[0] = 801;
    yesNo.yesnowords[1] = 802;
    yesNo.fDefault = 0;
    if (yesNo.Run() != 0)
    {
        sint16 carID = (sint8)carInfo.fCarID;
        sint32 price = carManager.PurchaseCar(carID, (uint8)frontEnd.carColors[carID / 48][carID % 48], 0);
        tournamentManager.fMoney = (tournamentManager.fMoney - price);
        AudioCmn::PlayFESFX(26);
    }
}

void MenuExtended::PurchaseUpgrade(sint32 upgradeNumber)
{
    FUNCTION_MARKER(0x8002D64C, "FRONT.BIN");
    uint32 upgradeFlag = 1U << (upgradeNumber & 31);
    tCarInfo carInfo;
    carManager.GetGarageCar((uint8)frontEnd.garageCar[0], carInfo, 0);
    if ((carInfo.fUpgrades & upgradeFlag) != 0)
        return;
    if (tournamentManager.fMoney < carInfo.fPrices[upgradeNumber + 1]) {
        tDialogMessageString *message = &FEApp->messagePopup;
        message->string = TextSys::Word(168);
        message->Display();
    } else {
        tDialogYesNo popUp;
        popUp.string = TextSys::Word(166);
        popUp.yesnowords[0] = 801;
        popUp.yesnowords[1] = 802;
        popUp.fDefault = 0;
        if (popUp.Run() != 0) {
            sint32 cost = carManager.PurchaseUpgrade((uint8)frontEnd.garageCar[0], (sint16)upgradeFlag, 0);
            tournamentManager.fMoney = (tournamentManager.fMoney - cost);
            AudioCmn::PlayFESFX(26);
        }
    }
}

void MenuExtended::PurchaseUpgrade1(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002D78C, "FRONT.BIN");
    PurchaseUpgrade(1);
}

void MenuExtended::PurchaseUpgrade2(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002D7AC, "FRONT.BIN");
    PurchaseUpgrade(2);
}

void MenuExtended::PurchaseUpgrade3(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002D7CC, "FRONT.BIN");
    PurchaseUpgrade(0);
}

void MenuExtended::SaveGame(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002D7EC, "FRONT.BIN");
    if (CURRENTLYUSINGMEMCARD == 0 && (menuDefs->itemSaveGame.fFlags & 1) == 0)
        GenericMenuSaveGame(0);
}

void MenuExtended::LoadGame(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002D8D0, "FRONT.BIN");
    if ((menuDefs->itemLoadGame.fFlags & 1) == 0)
        GenericMenuLoadGame(0);
}

void MenuExtended::TierFinished(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002D968, "FRONT.BIN");
    command.type = kMenu_Command_GoToMenuOneWay;
    tAwardInformation award;
    tournamentManager.GetAwardInformation(award);
    if (award.fCompletedTier != 0)
        command.nextMenu = &menuDefs->menuTierCompleteCongrats;
    else
    {
        AskTheUserToSaveTheGame();
        command.nextMenu = &menuDefs->menuMain;
    }
}

bool MenuExtended::DidUserWinBeTheCop(void)
{
    FUNCTION_MARKER(0x8002D9E8, "FRONT.BIN");
    tCarInfo carInfo;
    carManager.GetStockCar((uint8)frontEnd.playerCar[0], carInfo);
    if (carInfo.fCarClass != 7 || frontEnd.raceType != 1 || frontEnd.gameMode == 1)
        return false;
    if (!(GameSetup::gData.numPerps < GameSetup::gData.finalPerpArrests))
        return false;
    sint32 carID = (sint8)carInfo.fCarID;
    sint32 country = (sint8)frontEnd.carCountry[0][carID];
    tCarInfo *activateCar = carManager.GetCarFromID((sint16)gCarActivation[carID - 22][country]);
    if (activateCar->fAvailable != 0)
        return false;
    carManager.SetCarAvailable((tCarModels)(sint8)activateCar->fCarID, true);
    frontEnd.congratsCopCar = activateCar->fCarID;
    frontEnd.congratsCopCountry = frontEnd.carCountry[0][(sint8)carInfo.fCarID];
    return true;
}

void MenuExtended::PostGameMenu(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002DB04, "FRONT.BIN");
    StatChk::ClearNewRecords();
    command.type = kMenu_Command_GoToMenuOneWay;
    if (frontEnd.raceType == 2)
    {
        if (tournamentManager.IsTournamentFinished() != 0)
        {
            command.nextMenu = &menuDefs->menuTournamentFinished;
            screenTournamentStandings->fDrawMoney = 1;
        }
        else
        {
            command.nextMenu = &menuDefs->menuTournamentStandings;
            screenTournamentStandings3item->fDrawMoney = 1;
        }
    }
    else if (frontEnd.raceType == 6)
        command.nextMenu = &menuDefs->menuPinkSlipStandings;
    else if (frontEnd.raceType == 1 && DidUserWinBeTheCop())
        command.nextMenu = &menuDefs->menuBeTheCopCongrats;
    else
        command.nextMenu = &menuDefs->menuMain;
}

void MenuExtended::FinishedPlayer1GetName(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002DC04, "FRONT.BIN");
    command.type = kMenu_Command_GoToMenuOneWay;
    if (FEApp->needName[1] != 0 && FEApp->gotName[1] == 0)
    {
        menuDefs->menuItemUserName2.fPlayer = 1;
        menuDefs->menuItemUserName2.fData = frontEnd.playerNameList[1];
        menuDefs->menuItemUserName2.fMaxStringLength = 7;
        menuDefs->menuItemUserName2.fCurrentRow = 0;
        menuDefs->menuItemUserName2.fCurrentColumn = 0;
        screenUserName->callingMenu = &menuDefs->menuPostGamePlayer2Name;
        command.nextMenu = &menuDefs->menuPostGamePlayer2Name;
    }
    else
    {
        Car::tStats *dummyCars = Cars::gNewCarStatsList;
        sint16 nBestCarIndex;
        if (StatChk::IsRecordLapTime(dummyCars, Cars::gNumRaceCars, &nBestCarIndex))
            StatChk::SaveRecordLapTime(dummyCars, Cars::gNumRaceCars, nBestCarIndex);
        if (StatChk::IsTopTime(dummyCars, Cars::gNumRaceCars) != 0)
            StatChk::SaveTopTime(dummyCars, Cars::gNumRaceCars);
        command.nextMenu = &menuDefs->menuPostGameTrackRecords;
    }
}

void MenuExtended::FinishedPlayer2GetName(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002DD14, "FRONT.BIN");
    Car::tStats *dummyCars = Cars::gNewCarStatsList;
    sint16 nBestCarIndex;
    if (StatChk::IsRecordLapTime(dummyCars, Cars::gNumRaceCars, &nBestCarIndex))
        StatChk::SaveRecordLapTime(dummyCars, Cars::gNumRaceCars, nBestCarIndex);
    if (StatChk::IsTopTime(dummyCars, Cars::gNumRaceCars) != 0)
        StatChk::SaveTopTime(dummyCars, Cars::gNumRaceCars);
    command.type = kMenu_Command_GoToMenuOneWay;
    command.nextMenu = &menuDefs->menuPostGameTrackRecords;
}

void MenuExtended::SetPinkSlips(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002DDB4, "FRONT.BIN");

    screenMain->SwapBackground(-1);
    frontEnd.raceType = 6;
    frontEnd.gameMode = 1;
    frontEnd.oppNumber = 0;
    frontEnd.pinkSlipsForfeit = -1;
    frontEnd.pinkSlipsTrackIndex = 0;
    for (sint16 i = 0; i < 2; ++i)
        frontEnd.pinkSlipsWins[i] = 0;
    for (sint16 i = 0; i < 5; ++i)
        frontEnd.pinkSlipsWinner[i] = -1;
}

void MenuExtended::AwardPinkSlipsCar(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002DE74, "FRONT.BIN");
    sint32 fWinner = screenPinkSlipCongrats->fWinner;
    tCarInfo carInfo;
    carManager.GetPinkSlipsCar(frontEnd.pinkSlipsCar[1 - fWinner], carInfo, 1 - fWinner);
    carManager.AddToPinkSlipsList((sint8)carInfo.fCarID, carInfo.fColor, fWinner);
    carManager.AddUpgradesToPinkSlipsList(frontEnd.pinkSlipsCar[fWinner], carInfo.fUpgrades, fWinner);
    SavePinkSlipsCarsWithErrorDialogs(fWinner, 2, -1);
    command.type = kMenu_Command_GoToMenuOneWay;
    command.nextMenu = &menuDefs->menuMain;
    GenericMenuLoadGame(0);
    tGlobalMenuDefs *defs = menuDefs;
    command.type = kMenu_Command_GoToMenuOneWay;
    command.nextMenu = &defs->menuMain;
}

void MenuExtended::GoToGarage(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002E09C, "FRONT.BIN");
    frontEnd.carListType = 1;
    menuDefs->iteratorGarageCar.fCarListFilter = 0x40;
    menuDefs->iteratorGarageCar.Decrement(kPlayerBoth);
    menuDefs->iteratorGarageCar.Increment(kPlayerBoth);
    command.type = kMenu_Command_GoToMenu;
    if (tournamentManager.fCurrentTrack == 0)
        command.nextMenu = &menuDefs->menuCarGarage;
    else
        command.nextMenu = &menuDefs->menuPostCarGarage;
    screenCarSelect->SetState(1);
}

void MenuExtended::GoToTournTrophyRoom(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002E144, "FRONT.BIN");
    frontEnd.tier = 0;
}

void MenuExtended::GoToSETrophyRoom(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002E150, "FRONT.BIN");
    frontEnd.tier = 1;
}

void MenuExtended::SetBeginner(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002E160, "FRONT.BIN");
    frontEnd.skillLevel = 0;
}

void MenuExtended::SetIntermediate(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002E16C, "FRONT.BIN");
    frontEnd.skillLevel = 1;
}

void MenuExtended::SetExpert(tMenuCommand &)
{
    FUNCTION_MARKER(0x8002E17C, "FRONT.BIN");
    frontEnd.skillLevel = 2;
}

void MenuExtended::ExitTourney(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002E18C, "FRONT.BIN");

    tDialogYesNo AreYouSure;
    AreYouSure.yesnowords[0] = 801;
    AreYouSure.yesnowords[1] = 802;
    AreYouSure.fDefault = 0;
    AreYouSure.string = TextSys::Word(157);
    if (AreYouSure.Run() != 0)
    {
        tGlobalMenuDefs *defs = menuDefs;
        command.type = kMenu_Command_GoToMenuOneWay;
        command.nextMenu = &defs->menuMain;
    }
    else
        command.type = kMenu_Command_None;
}

void MenuExtended::ExitPinkSlipsEarly(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002E21C, "FRONT.BIN");
    tDialogYesNo AreYouSure;
    AreYouSure.yesnowords[0] = 801;
    AreYouSure.yesnowords[1] = 802;
    AreYouSure.fDefault = 0;
    AreYouSure.string = TextSys::Word(157);
    if (AreYouSure.Run() != 0)
    {
        for (sint32 player = 0; player < 2; ++player)
            SavePinkSlipsCarsWithErrorDialogs(player, 1, -1);
        tGlobalMenuDefs *defs = menuDefs;
        command.type = kMenu_Command_GoToMenuOneWay;
        command.nextMenu = &defs->menuMain;
        frontEnd.raceType = 0;
    }
}

void MenuExtended::PinkSlipsContinue(tMenuCommand &command)
{
    FUNCTION_MARKER(0x8002E34C, "FRONT.BIN");

    sint32 winsNeeded = ((uint8)frontEnd.pinkSlipsNumTracks >> 1) + 1;
    if ((uint8)frontEnd.pinkSlipsWins[0] < winsNeeded && (uint8)frontEnd.pinkSlipsWins[1] < winsNeeded)
    {
        frontEnd.pinkSlipsTrackIndex = (char)((uint8)frontEnd.pinkSlipsTrackIndex + 1);
        command.type = kMenu_Command_StartRace;
        return;
    }
    tGlobalMenuDefs *defs = menuDefs;
    command.type = kMenu_Command_GoToMenuOneWay;
    command.nextMenu = &defs->menuPinkSlipCongrats;
}

tGlobalMenuDefs::tGlobalMenuDefs(void) :
    itemMainOnePlayerRace(91, &menuOnePlayer, MenuExtended::SetOnePlayer, 30, 10),
    itemMainTwoPlayerRace(92, &menuTwoPlayer, MenuExtended::SetTwoPlayer, 40, 10),
    itemMainTrophyRoom(94, &menuTrophyRoomSelect, 0, 50, 10),
    itemMainOptions(93, &menuOptions, 0, 60, 10),
    menuMain(4116, screenMain, 0, 0, 0, 180, static_cast<tMenuItem *>(&itemMainOnePlayerRace), static_cast<tMenuItem *>(&itemMainTwoPlayerRace), static_cast<tMenuItem *>(&itemMainOptions), static_cast<tMenuItem *>(&itemMainTrophyRoom), static_cast<tMenuItem *>(0)),
    itemTournTrophyRoom(99, &menuTrophyRoom, MenuExtended::GoToTournTrophyRoom, 50, 10),
    itemSETrophyRoom(105, &menuTrophyRoom, MenuExtended::GoToSETrophyRoom, 50, 10),
    menuTrophyRoomSelect(4116, screenMain, 0, 0, 0, 95, static_cast<tMenuItem *>(&itemTournTrophyRoom), static_cast<tMenuItem *>(&itemSETrophyRoom), static_cast<tMenuItem *>(0)),
    menuTrophyInfo(16384, screenTrophyInfo, 0, 0, 0, -1),
    itemSkillBeginner(161, &menuSingleTrackSelect, MenuExtended::SetBeginner, 140, 10),
    itemSkillIntermediate(162, &menuSingleTrackSelect, MenuExtended::SetIntermediate, 150, 10),
    itemSkillExpert(163, &menuSingleTrackSelect, MenuExtended::SetExpert, 160, 10),
    menuSkillLevel(4100, screenMain, 0, 0, 0, 183, static_cast<tMenuItem *>(&itemSkillBeginner), static_cast<tMenuItem *>(&itemSkillIntermediate), static_cast<tMenuItem *>(&itemSkillExpert), static_cast<tMenuItem *>(0)),
    itemOnePlayerTestDrive(96, &menuTestDriveTrackSelect, MenuExtended::SetTestDrive, 240, 10),
    itemOnePlayerSingleRace(114, &menuSingleRace, MenuExtended::SetSingleRace, 70, 10),
    itemOnePlayerPursuit(113, &menuHotPursuit, MenuExtended::SetHotPursuit, 80, 10),
    itemOnePlayerTournament(99, &menuTournament, MenuExtended::SetTournament, 90, 10),
    itemOnePlayerSpecialEvents(105, &menuSpecialEvent, MenuExtended::SetSpecialEvent, 100, 10),
    menuOnePlayer(4100, screenMain, 0, 0, 0, 181, static_cast<tMenuItem *>(&itemOnePlayerTestDrive), static_cast<tMenuItem *>(&itemOnePlayerSingleRace), static_cast<tMenuItem *>(&itemOnePlayerPursuit), static_cast<tMenuItem *>(&itemOnePlayerTournament), static_cast<tMenuItem *>(&itemOnePlayerSpecialEvents), static_cast<tMenuItem *>(0)),
    itemSingleRaceSolo(108, &menuSingleTrackSelect, MenuExtended::SetSoloRace, 110, 10),
    itemSingleRaceDuel(109, &menuSkillLevel, MenuExtended::SetDuelRace, 120, 10),
    itemSingleRaceFullGrid(111, &menuSkillLevel, MenuExtended::SetFullGrid, 130, 10),
    menuSingleRace(4100, screenMain, 0, 0, 0, 182, static_cast<tMenuItem *>(&itemSingleRaceSolo), static_cast<tMenuItem *>(&itemSingleRaceDuel), static_cast<tMenuItem *>(&itemSingleRaceFullGrid), static_cast<tMenuItem *>(0)),
    itemHotPursuitSolo(108, &menuSkillLevel, MenuExtended::SetHPSoloRace, 110, 10),
    itemHotPursuitDuel(109, &menuSkillLevel, MenuExtended::SetHPDuelRace, 120, 10),
    menuHotPursuit(4100, screenMain, 0, 0, 0, 182, static_cast<tMenuItem *>(&itemHotPursuitSolo), static_cast<tMenuItem *>(&itemHotPursuitDuel), static_cast<tMenuItem *>(0)),
    iteratorTournament(&frontEnd.tournament, &tournamentManager),
    itemTournamentContinue(90, 0, MenuExtended::GoToTournTrackInfo, 34, 10),
    itemTournamentSelect(148, &iteratorTournament, 44, 10),
    menuTournament(4096, screenTournSelect, 0, 0, MenuExtended::GoToTournTrackInfo, 101, static_cast<tMenuItem *>(&itemTournamentContinue), static_cast<tMenuItem *>(&itemTournamentSelect), static_cast<tMenuItem *>(0)),
    iteratorSpecialEvent(&frontEnd.specialevent, &tournamentManager),
    itemSpecialEventContinue(90, 0, MenuExtended::GoToSpecialEventTrackInfo, 34, 10),
    itemSpecialEventSelect(105, &iteratorSpecialEvent, 54, 10),
    menuSpecialEvent(4096, screenTournSelect, 0, 0, MenuExtended::GoToSpecialEventTrackInfo, 100, static_cast<tMenuItem *>(&itemSpecialEventContinue), static_cast<tMenuItem *>(&itemSpecialEventSelect), static_cast<tMenuItem *>(0)),
    itemTwoPlayerTestDrive(96, &menuTestDriveTrackSelect, MenuExtended::SetTestDrive, 240, 10),
    itemTwoPlayerDuel(114, 0, MenuExtended::GoToTwoPlayerSingleRace, 70, 10),
    itemTwoPlayerHotPursuit(113, &menuSkillLevel, MenuExtended::SetHotPursuit, 80, 10),
    itemTwoPlayerPinkSlips(106, &menuPinkSlipSelect, MenuExtended::SetPinkSlips, 170, 10),
    menuTwoPlayer(4100, screenMain, 0, 0, 0, 181, static_cast<tMenuItem *>(&itemTwoPlayerTestDrive), static_cast<tMenuItem *>(&itemTwoPlayerDuel), static_cast<tMenuItem *>(&itemTwoPlayerHotPursuit), static_cast<tMenuItem *>(&itemTwoPlayerPinkSlips), static_cast<tMenuItem *>(0)),
    itemBestOfOne(192, &menuSingleTrackSelect, MenuExtended::GoToBestOfOne, 170, 10),
    itemBestOfThree(193, &menuPinkSlipsBestOfThree, MenuExtended::GoToBestOfThree, 170, 10),
    itemBestOfFive(194, &menuPinkSlipsBestOfFive, MenuExtended::GoToBestOfFive, 170, 10),
    menuPinkSlipSelect(4096, screenMain, 0, 0, 0, 182, static_cast<tMenuItem *>(&itemBestOfOne), static_cast<tMenuItem *>(&itemBestOfThree), static_cast<tMenuItem *>(&itemBestOfFive), static_cast<tMenuItem *>(0)),
    itemPinkSlipsContinue(90, 0, MenuExtended::GoToCarSelect, 36, 10),
    itemTrack1(195, &iteratorTrack, 46, 10),
    itemTrack2(196, &iteratorTrack, 46, 10),
    itemTrack3(197, &iteratorTrack, 46, 10),
    itemTrack4(198, &iteratorTrack, 46, 10),
    itemTrack5(199, &iteratorTrack, 46, 10),
    menuPinkSlipsBestOfThree(5120, screenPinkSlips, 0, &menuTrackOptions, MenuExtended::GoToCarSelect, 190, static_cast<tMenuItem *>(&itemPinkSlipsContinue), static_cast<tMenuItem *>(&itemTrack1), static_cast<tMenuItem *>(&itemTrack2), static_cast<tMenuItem *>(&itemTrack3), static_cast<tMenuItem *>(0)),
    menuPinkSlipsBestOfFive(5120, screenPinkSlips, 0, &menuTrackOptions, MenuExtended::GoToCarSelect, 191, static_cast<tMenuItem *>(&itemPinkSlipsContinue), static_cast<tMenuItem *>(&itemTrack1), static_cast<tMenuItem *>(&itemTrack2), static_cast<tMenuItem *>(&itemTrack3), static_cast<tMenuItem *>(&itemTrack4), static_cast<tMenuItem *>(&itemTrack5), static_cast<tMenuItem *>(0)),
    iteratorTrack(frontEnd.track, &frontEnd.pinkSlipsTrackIndex, &trackManager),
    itemTrackContinue(90, 0, MenuExtended::GoToCarSelect, 28, 10),
    itemTrack(147, &iteratorTrack, 38, 10),
    itemTrackRecords(212, &menuTrackRecords, 0, 58, 10),
    menuSingleTrackSelect(5632, screenTrackSelect, 0, &menuTrackOptions, MenuExtended::GoToCarSelect, 200, static_cast<tMenuItem *>(&itemTrackContinue), static_cast<tMenuItem *>(&itemTrack), static_cast<tMenuItem *>(&itemTrackRecords), static_cast<tMenuItem *>(0)),
    menuTestDriveTrackSelect(5632, screenTrackSelect, 0, 0, MenuExtended::GoToCarSelect, 200, static_cast<tMenuItem *>(&itemTrackContinue), static_cast<tMenuItem *>(&itemTrack), static_cast<tMenuItem *>(&itemTrackRecords), static_cast<tMenuItem *>(0)),
    iteratorLaps(SelectListNormal, frontEnd.lapind, &frontEnd.pinkSlipsTrackIndex),
    iteratorTrackDirection(SelectListTrackDirection, frontEnd.trackdirection, &frontEnd.pinkSlipsTrackIndex),
    iteratorTrackMirrored(SelectListOffOn, frontEnd.trackmirrored, &frontEnd.pinkSlipsTrackIndex),
    iteratorTimeOfDay(SelectListOffOn, frontEnd.timeOfDay, &frontEnd.pinkSlipsTrackIndex),
    iteratorWeather(SelectListOffOn, frontEnd.weather, &frontEnd.pinkSlipsTrackIndex),
    iteratorTraffic(SelectListOffOn, frontEnd.traffic, &frontEnd.pinkSlipsTrackIndex),
    iteratorLocalSpeech(SelectListOffOn, &frontEnd.localSpeech),
    itemLaps(202, &iteratorLaps),
    itemTrackDirection(204, &iteratorTrackDirection),
    itemTrackMirrored(205, &iteratorTrackMirrored),
    itemTimeOfDay(206, &iteratorTimeOfDay),
    itemWeather(207, &iteratorWeather),
    itemTraffic(208, &iteratorTraffic),
    itemLocalSpeech(210, &iteratorLocalSpeech),
    menuTrackOptions(4096, 0, 0, 0, 0, 185, -1, static_cast<tMenuItem *>(&itemLaps), static_cast<tMenuItem *>(&itemTrackDirection), static_cast<tMenuItem *>(&itemTrackMirrored), static_cast<tMenuItem *>(&itemTimeOfDay), static_cast<tMenuItem *>(&itemWeather), static_cast<tMenuItem *>(&itemTraffic), static_cast<tMenuItem *>(&itemLocalSpeech), static_cast<tMenuItem *>(0)),
    menuTrackRecordsItem(0, 0, 0, -1, -1),
    menuTrackRecords(4096, screenTrackRecords, 0, 0, 0, 212, 1, 10, static_cast<tMenuItem *>(0)),
    itemTrackInfoContinue(90, 0, MenuExtended::GoToGarage, 33, 10),
    menuTrackInfo(4100, screenTrackInfo, 0, 0, 0, 249, static_cast<tMenuItem *>(&itemTrackInfoContinue), static_cast<tMenuItem *>(0)),
    iteratorCar1(frontEnd.playerCar, &carManager),
    iteratorColor(reinterpret_cast<char *>(&frontEnd.carColors), &FEApp->fPlayer, frontEnd.playerCar, 48, &carManager),
    itemCarSelectRace(189, 0, MenuExtended::GoToRace, 128, 10),
    itemCar(146, &iteratorCar1, 28, 10),
    itemColor(288, &iteratorColor, 38, 10),
    itemShowcase(274, 0, MenuExtended::GoToShowroom, 48, 10),
    menuSingleCarSelect(6656, screenCarSelect, 0, &menuCarOptions, MenuExtended::GoToRace, 186, static_cast<tMenuItem *>(&itemCarSelectRace), static_cast<tMenuItem *>(&itemCar), static_cast<tMenuItem *>(&itemColor), static_cast<tMenuItem *>(&itemShowcase), static_cast<tMenuItem *>(0)),
    iteratorGarageCar(frontEnd.garageCar, &carManager),
    itemGarageCar(146, &iteratorGarageCar, 28, 10),
    itemCarDealer(116, &menuGoToCarDealer, 0, 58, 10),
    itemUpgradeCar(145, 0, MenuExtended::GoToUpgrades, 68, 10),
    menuCarGarage(6656, screenCarSelect, 0, &menuCarOptions, MenuExtended::GoToRace, 143, static_cast<tMenuItem *>(&itemCarSelectRace), static_cast<tMenuItem *>(&itemGarageCar), static_cast<tMenuItem *>(&itemCarDealer), static_cast<tMenuItem *>(&itemUpgradeCar), static_cast<tMenuItem *>(0)),
    menuPostCarGarage(6656, screenCarSelect, 0, &menuCarOptions, MenuExtended::GoToRace, 143, static_cast<tMenuItem *>(&itemCarSelectRace), static_cast<tMenuItem *>(&itemUpgradeCar), static_cast<tMenuItem *>(0)),
    iteratorOpponentCar(&frontEnd.oppCar, &carManager),
    itemDuelRace(189, 0, MenuExtended::GoToRace, 42, 10),
    itemCar2(146, &iteratorCar1, 12, 10),
    itemColor2(288, &iteratorColor, 22, 10),
    itemGoToDuelBuyCar(120, &menuCarDealer, MenuExtended::GoToDealer, -1, 10),
    itemOpponentCar(188, &iteratorOpponentCar, 32, 10),
    menuDuelCarSelect(6144, screenCarSelectDuel, 0, &menuCarOptions, MenuExtended::GoToRace, 186, static_cast<tMenuItem *>(&itemDuelRace), static_cast<tMenuItem *>(&itemGarageCar), static_cast<tMenuItem *>(&itemGoToDuelBuyCar), static_cast<tMenuItem *>(&itemOpponentCar), static_cast<tMenuItem *>(0)),
    menuHPDuelCarSelect(6144, screenCarSelectDuel, 0, &menuCarOptions, MenuExtended::GoToRace, 186, static_cast<tMenuItem *>(&itemDuelRace), static_cast<tMenuItem *>(&itemCar), static_cast<tMenuItem *>(&itemColor), static_cast<tMenuItem *>(&itemOpponentCar), static_cast<tMenuItem *>(0)),
    itemPlayerOneRace(189, 0, MenuExtended::GoTo2PlayerRace, 42, 10),
    itemCarP1(146, &iteratorCar1, 12, 10),
    itemColorP1(288, &iteratorColor, 22, 10),
    menuPlayerOneCarSelect(4104, screenCarSelectTwoPlayer, 0, &menuCarOptionsPlayerOne, MenuExtended::GoTo2PlayerRace, 186, static_cast<tMenuItem *>(&itemPlayerOneRace), static_cast<tMenuItem *>(&itemCarP1), static_cast<tMenuItem *>(&itemColorP1), static_cast<tMenuItem *>(0)),
    itemPlayerTwoRace(189, 0, MenuExtended::GoTo2PlayerRace, 42, 10),
    itemCarP2(146, &iteratorCar1, 12, 10),
    itemColorP2(288, &iteratorColor, 22, 10),
    menuPlayerTwoCarSelect(4104, screenCarSelectPlayerTwo, 0, &menuCarOptionsPlayerTwo, MenuExtended::GoTo2PlayerRace, 186, static_cast<tMenuItem *>(&itemPlayerTwoRace), static_cast<tMenuItem *>(&itemCarP2), static_cast<tMenuItem *>(&itemColorP2), static_cast<tMenuItem *>(0)),
    itemPlayerOneGarageRace(189, 0, MenuExtended::GoTo2PlayerRace, 42, 10),
    itemGarageCarP1(146, &iteratorGarageCar, 12, 10),
    menuPlayerOneGarage(4104, screenCarSelectTwoPlayer, 0, &menuCarOptionsPlayerOne, MenuExtended::GoTo2PlayerRace, 186, static_cast<tMenuItem *>(&itemPlayerOneRace), static_cast<tMenuItem *>(&itemGarageCarP1), static_cast<tMenuItem *>(0)),
    itemPlayerTwoGarageRace(189, 0, MenuExtended::GoTo2PlayerRace, 42, 10),
    itemGarageCarP2(146, &iteratorGarageCar, 12, 10),
    menuPlayerTwoGarage(4104, screenCarSelectPlayerTwo, 0, &menuCarOptionsPlayerTwo, MenuExtended::GoTo2PlayerRace, 186, static_cast<tMenuItem *>(&itemPlayerTwoGarageRace), static_cast<tMenuItem *>(&itemGarageCarP2), static_cast<tMenuItem *>(0)),
    iteratorPinkSlipsCar(frontEnd.pinkSlipsCar, &carManager),
    itemPlayerOnePinkSlipRace(189, 0, MenuExtended::GoTo2PlayerRace, 42, 10),
    itemPinkSlipCarP1(146, &iteratorPinkSlipsCar, 12, 10),
    menuPlayerOnePinkSlipCarSelect(4104, screenPinkSlipsCarSelectTwoPlayer, 0, &menuPinkSlipCarOptionsPlayerOne, MenuExtended::GoTo2PlayerRace, 186, static_cast<tMenuItem *>(&itemPlayerOnePinkSlipRace), static_cast<tMenuItem *>(&itemPinkSlipCarP1), static_cast<tMenuItem *>(0)),
    itemPlayerTwoPinkSlipRace(189, 0, MenuExtended::GoTo2PlayerRace, 42, 10),
    itemPinkSlipCarP2(146, &iteratorPinkSlipsCar, 12, 10),
    menuPlayerTwoPinkSlipCarSelect(4104, screenPinkSlipsCarSelectPlayerTwo, 0, &menuPinkSlipCarOptionsPlayerTwo, MenuExtended::GoTo2PlayerRace, 186, static_cast<tMenuItem *>(&itemPlayerTwoPinkSlipRace), static_cast<tMenuItem *>(&itemPinkSlipCarP2), static_cast<tMenuItem *>(0)),
    itemGoToBuyCar(120, &menuCarDealer, MenuExtended::GoToDealer, 88, 10),
    itemGoToSellCar(121, &menuCarSeller, MenuExtended::GoToSeller, 78, 10),
    menuGoToCarDealer(4608, screenCarSelect, 0, 0, 0, 144, static_cast<tMenuItem *>(&itemGoToBuyCar), static_cast<tMenuItem *>(&itemGoToSellCar), static_cast<tMenuItem *>(0)),
    iteratorDealerCar(&frontEnd.dealerCar, &carManager),
    iteratorDealerColor(reinterpret_cast<char *>(&frontEnd.carColors), &FEApp->fPlayer, &frontEnd.dealerCar, 48, &carManager),
    itemDealerCar(146, &iteratorDealerCar, 28, 10),
    itemDealerColor(288, &iteratorDealerColor, 38, 10),
    itemBuyCar(117, 0, MenuExtended::BuyCar, 88, 10),
    itemDealerShowroom(274, 0, MenuExtended::GoToDealerShowroom, 48, 10),
    menuCarDealer(8768, screenCarSelect, 0, 0, 0, 144, static_cast<tMenuItem *>(&itemDealerCar), static_cast<tMenuItem *>(&itemDealerColor), static_cast<tMenuItem *>(&itemBuyCar), static_cast<tMenuItem *>(&itemDealerShowroom), static_cast<tMenuItem *>(0)),
    iteratorSellerCar(&frontEnd.sellerCar, &carManager),
    itemSellerCar(146, &iteratorSellerCar, 28, 10),
    itemSellCar(119, 0, MenuExtended::SellCar, 78, 10),
    menuCarSeller(8704, screenCarSelect, 0, 0, 0, 144, static_cast<tMenuItem *>(&itemSellerCar), static_cast<tMenuItem *>(&itemSellCar), static_cast<tMenuItem *>(0)),
    itemPurchaseUpgrade1(150, 0, MenuExtended::PurchaseUpgrade1, 98, 10),
    itemPurchaseUpgrade2(151, 0, MenuExtended::PurchaseUpgrade2, 108, 10),
    itemPurchaseUpgrade3(152, 0, MenuExtended::PurchaseUpgrade3, 118, 10),
    menuCarUpgrades(8704, screenCarSelect, 0, 0, 0, 145, static_cast<tMenuItem *>(&itemPurchaseUpgrade1), static_cast<tMenuItem *>(&itemPurchaseUpgrade2), static_cast<tMenuItem *>(&itemPurchaseUpgrade3), static_cast<tMenuItem *>(0)),
    iteratorTransmission(SelectListTransmission, frontEnd.transmission, &FEApp->fPlayer),
    iteratorABS(SelectListOffOn, frontEnd.ABS, &FEApp->fPlayer),
    iteratorDamage(SelectListOffOn, &frontEnd.damage),
    iteratorOpponentUpgrades(SelectListOpponentUpgrades, &frontEnd.opponentUpgrades),
    itemTransmission(266, &iteratorTransmission),
    itemABS(267, &iteratorABS),
    itemDamage(273, &iteratorDamage),
    itemTransmission2(266, &iteratorTransmission),
    itemABS2(267, &iteratorABS),
    itemDamage2(273, &iteratorDamage),
    itemOpponentUpgrades(270, &iteratorOpponentUpgrades),
    menuCarOptions(4096, 0, 0, 0, 0, 187, -1, static_cast<tMenuItem *>(&itemTransmission), static_cast<tMenuItem *>(&itemABS), static_cast<tMenuItem *>(&itemDamage), static_cast<tMenuItem *>(&itemOpponentUpgrades), static_cast<tMenuItem *>(0)),
    menuCarOptionsPlayerOne(4104, screenCarSelectTwoPlayer, 0, 0, 0, 187, 0, static_cast<tMenuItem *>(&itemTransmission), static_cast<tMenuItem *>(&itemABS), static_cast<tMenuItem *>(&itemDamage), static_cast<tMenuItem *>(0)),
    menuCarOptionsPlayerTwo(4104, screenCarSelectPlayerTwo, 0, 0, 0, 187, 1, static_cast<tMenuItem *>(&itemTransmission2), static_cast<tMenuItem *>(&itemABS2), static_cast<tMenuItem *>(&itemDamage2), static_cast<tMenuItem *>(0)),
    menuPinkSlipCarOptionsPlayerOne(4104, screenPinkSlipsCarSelectTwoPlayer, 0, 0, 0, 187, 0, static_cast<tMenuItem *>(&itemTransmission), static_cast<tMenuItem *>(&itemABS), static_cast<tMenuItem *>(&itemDamage), static_cast<tMenuItem *>(0)),
    menuPinkSlipCarOptionsPlayerTwo(4104, screenPinkSlipsCarSelectPlayerTwo, 0, 0, 0, 187, 1, static_cast<tMenuItem *>(&itemTransmission2), static_cast<tMenuItem *>(&itemABS2), static_cast<tMenuItem *>(&itemDamage2), static_cast<tMenuItem *>(0)),
    menuShowroom(16384, 0, 0, 0, 0, -1),
    itemOptionsAudio(453, &menuAudio, 0, 180, 10),
    itemOptionsDisplay(454, &menuDisplayOptions, 0, 190, 10),
    itemOptionsControllers(456, &menuControllerConfig, 0, 200, 10),
    itemSaveGame(646, 0, MenuExtended::SaveGame, -1, -1),
    itemLoadGame(647, 0, MenuExtended::LoadGame, -1, -1),
    itemOptionsUsername(458, 0, MenuExtended::EnterUserName, 220, 10),
    itemOptionsCredits(460, &menuCredits, 0, 230, 10),
    menuOptions(4116, screenMain, 0, 0, 0, 93, static_cast<tMenuItem *>(&itemOptionsAudio), static_cast<tMenuItem *>(&itemOptionsDisplay), static_cast<tMenuItem *>(&itemOptionsControllers), static_cast<tMenuItem *>(&itemSaveGame), static_cast<tMenuItem *>(&itemLoadGame), static_cast<tMenuItem *>(&itemOptionsUsername), static_cast<tMenuItem *>(&itemOptionsCredits), static_cast<tMenuItem *>(0)),
    iteratorMusicVolume(0, 127, &frontEnd.musicVolume),
    iteratorSoundEffectsVolume(0, 127, &frontEnd.sfxVolume),
    iteratorEngineVolume(0, 127, &frontEnd.engineVolume),
    iteratorSpeechVolume(0, 127, &frontEnd.narrationVolume),
    iteratorAmbientVolume(0, 127, &frontEnd.ambientVolume),
    iteratorAudioMode(SelectListAudioMode, &frontEnd.audioMode),
    itemMusicVolume(469, &iteratorMusicVolume, 0),
    itemSoundEffectsVolume(470, &iteratorSoundEffectsVolume, 1),
    itemEngineVolume(471, &iteratorEngineVolume, 2),
    itemSpeechVolume(472, &iteratorSpeechVolume, 3),
    itemAmbientVolume(473, &iteratorAmbientVolume, 4),
    itemAudioMode(474, &iteratorAudioMode),
    itemSlidingPlayList(475, 350, 43, -102, 13, 0),
    menuPlayListMenu(4096, 0, 0, 0, 0, 0, static_cast<tMenuItem *>(0)),
    menuAudio(4112, screenAudio, 0, 0, 0, 611, 38, 10, static_cast<tMenuItem *>(&itemMusicVolume), static_cast<tMenuItem *>(&itemSoundEffectsVolume), static_cast<tMenuItem *>(&itemEngineVolume), static_cast<tMenuItem *>(&itemSpeechVolume), static_cast<tMenuItem *>(&itemAmbientVolume), static_cast<tMenuItem *>(&itemAudioMode), static_cast<tMenuItem *>(&itemSlidingPlayList), static_cast<tMenuItem *>(0)),
    iteratorDisplaySpeedometer(SelectListDisplaySpeed, frontEnd.displaySpeed, &FEApp->fInputPlayer),
    iteratorDisplayMap(SelectListDisplayMap, frontEnd.displayMap, &FEApp->fInputPlayer),
    iteratorDisplayOpponentID(SelectListDisplayOpponentID, frontEnd.displayOpponentID, &FEApp->fInputPlayer),
    iteratorDisplayTime(SelectListOffOn, frontEnd.displayTime, &FEApp->fInputPlayer),
    iteratorDisplayPosition(SelectListOffOn, frontEnd.displayPosition, &FEApp->fInputPlayer),
    iteratorDisplayLapNumber(SelectListOffOn, frontEnd.displayLapNumber, &FEApp->fInputPlayer),
    iteratorDisplaySplitTime(SelectListSplitTime, &frontEnd.checkPointType),
    iteratorDisplaySplitDisplay(SelectListSplitDisplay, frontEnd.checkPointDisplay, &FEApp->fInputPlayer),
    itemDisplaySpeedometer(479, &iteratorDisplaySpeedometer),
    itemDisplayMap(481, &iteratorDisplayMap),
    itemDisplayOpponentID(482, &iteratorDisplayOpponentID),
    itemDisplayTime(483, &iteratorDisplayTime),
    itemDisplayPosition(486, &iteratorDisplayPosition),
    itemDisplayLapNumber(487, &iteratorDisplayLapNumber),
    itemDisplaySplitTime(484, &iteratorDisplaySplitTime),
    itemDisplaySplitDisplay(485, &iteratorDisplaySplitDisplay),
    menuDisplayOptions(4128, screenDisplay, 0, 0, 0, 477, 1, 10, static_cast<tMenuItem *>(&itemDisplaySpeedometer), static_cast<tMenuItem *>(&itemDisplayMap), static_cast<tMenuItem *>(&itemDisplayOpponentID), static_cast<tMenuItem *>(&itemDisplayTime), static_cast<tMenuItem *>(&itemDisplaySplitTime), static_cast<tMenuItem *>(&itemDisplaySplitDisplay), static_cast<tMenuItem *>(&itemDisplayPosition), static_cast<tMenuItem *>(&itemDisplayLapNumber), static_cast<tMenuItem *>(0)),
    iteratorControllerConfigSelected(SelectListControllerConfig, frontEnd.controlConfig, &FEApp->fInputPlayer),
    itemControllerConfigSelected(521, &iteratorControllerConfigSelected),
    itemControllerSettings(522, 172, 72, 0, 13, 1),
    menuControllerConfig(4128, screenControllerConfig, 0, 0, 0, 520, 0, 10, static_cast<tMenuItem *>(&itemControllerConfigSelected), static_cast<tMenuItem *>(&itemControllerSettings), static_cast<tMenuItem *>(0)),
    iteratorControllerShockMode(0, 127, frontEnd.shockMode, &FEApp->fInputPlayer),
    itemControllerShockMode(526, &iteratorControllerShockMode),
    iteratorControllerShockImpact(0, 127, frontEnd.shockImpact, &FEApp->fInputPlayer),
    itemControllerShockImpact(527, &iteratorControllerShockImpact),
    iteratorControllerSteeringRange1(0, 127, frontEnd.J1MAX, &FEApp->fInputPlayer),
    itemControllerSteeringRange1(529, &iteratorControllerSteeringRange1, 0),
    iteratorControllerDeadSpot1(0, 127, frontEnd.J1MIN, &FEApp->fInputPlayer),
    itemControllerDeadSpot1(531, &iteratorControllerDeadSpot1, 1),
    iteratorControllerSteeringRange2(0, 127, frontEnd.J2MAX, &FEApp->fInputPlayer),
    itemControllerSteeringRange2(528, &iteratorControllerSteeringRange2, 2),
    iteratorControllerDeadSpot2(0, 127, frontEnd.J2MIN, &FEApp->fInputPlayer),
    itemControllerDeadSpot2(530, &iteratorControllerDeadSpot2, 3),
    iteratorControllerJoyRange(0, 127, frontEnd.steeringRange, &FEApp->fInputPlayer),
    itemControllerJoyRange(532, &iteratorControllerJoyRange, 0),
    iteratorControllerCenterPoint(0, 127, frontEnd.deadSpot, &FEApp->fInputPlayer),
    itemControllerCenterPoint(533, &iteratorControllerCenterPoint, 1),
    iteratorControllerIMax(0, 255, frontEnd.ImaxRange, &FEApp->fInputPlayer),
    itemControllerIMax(534, &iteratorControllerIMax, 2),
    iteratorControllerIIMax(0, 255, frontEnd.IImaxRange, &FEApp->fInputPlayer),
    itemControllerIIMax(535, &iteratorControllerIIMax, 3),
    menuControllerDualShock(4096, 0, 0, 0, 0, 0, static_cast<tMenuItem *>(&itemControllerShockMode), static_cast<tMenuItem *>(&itemControllerShockImpact), static_cast<tMenuItem *>(0)),
    menuControllerAnalog(4096, 0, 0, 0, 0, 0, static_cast<tMenuItem *>(&itemControllerSteeringRange1), static_cast<tMenuItem *>(&itemControllerDeadSpot1), static_cast<tMenuItem *>(&itemControllerSteeringRange2), static_cast<tMenuItem *>(&itemControllerDeadSpot2), static_cast<tMenuItem *>(0)),
    menuControllerDualShockAnalog(4096, 0, 0, 0, 0, 0, static_cast<tMenuItem *>(&itemControllerShockMode), static_cast<tMenuItem *>(&itemControllerShockImpact), static_cast<tMenuItem *>(&itemControllerDeadSpot1), static_cast<tMenuItem *>(&itemControllerDeadSpot2), static_cast<tMenuItem *>(0)),
    menuControllerNegcon(4096, 0, 0, 0, 0, 0, static_cast<tMenuItem *>(&itemControllerJoyRange), static_cast<tMenuItem *>(&itemControllerCenterPoint), static_cast<tMenuItem *>(&itemControllerIMax), static_cast<tMenuItem *>(&itemControllerIIMax), static_cast<tMenuItem *>(0)),
    menuItemUserName(504),
    menuItemUserName1(504),
    menuItemUserName2(504),
    menuUserName(4384, screenUserName, 0, 0, 0, -1, 32, 10, static_cast<tMenuItem *>(&menuItemUserName), static_cast<tMenuItem *>(0)),
    iteratorChangeTrophy(screenTrophyRoom->fTrophyList, &screenTrophyRoom->thisisuseless),
    itemChangeTrophy(94, &iteratorChangeTrophy, -1, 0),
    menuTrophyRoom(16400, screenTrophyRoom, 0, 0, 0, -1, static_cast<tMenuItem *>(&itemChangeTrophy), static_cast<tMenuItem *>(0)),
    itemPinkSlipStandingsForward(90, 0, MenuExtended::PinkSlipsContinue, 0, 10),
    itemPinkSlipStandingsExit(156, 0, MenuExtended::ExitPinkSlipsEarly, 0, 10),
    menuPinkSlipStandings(4100, screenPinkSlipStandings, 0, 0, 0, 756, static_cast<tMenuItem *>(&itemPinkSlipStandingsForward), static_cast<tMenuItem *>(&itemPinkSlipStandingsExit), static_cast<tMenuItem *>(0)),
    itemTournStandingsForward(90, &menuTrackInfo, 0, 0, 10),
    itemTournStandingsExit(155, 0, MenuExtended::ExitTourney, 0, 10),
    menuTournamentStandings(4100, screenTournamentStandings3item, 0, 0, 0, 756, static_cast<tMenuItem *>(&itemTournStandingsForward), static_cast<tMenuItem *>(&itemTournStandingsExit), static_cast<tMenuItem *>(0)),
    itemTournamentFinishedHome(90, &menuTournamentTrophy, 0, 0, 10),
    menuTournamentFinished(4100, screenTournamentStandings, 0, 0, 0, 756, static_cast<tMenuItem *>(&itemTournamentFinishedHome), static_cast<tMenuItem *>(0)),
    menuTournamentTrophy(32768, screenTournamentTrophy, 0, 0, MenuExtended::TierFinished, -1),
    menuPostGamePlayer1Name(4160, screenUserName, 0, 0, MenuExtended::FinishedPlayer1GetName, -1, 32, 10, static_cast<tMenuItem *>(&menuItemUserName1), static_cast<tMenuItem *>(0)),
    menuPostGamePlayer2Name(4224, screenUserName, 0, 0, MenuExtended::FinishedPlayer2GetName, -1, 32, 10, static_cast<tMenuItem *>(&menuItemUserName2), static_cast<tMenuItem *>(0)),
    itemPostGameTrackRecordsContinue(90, 0, MenuExtended::PostGameMenu, -1, -1),
    menuPostGameTrackRecords(135172, screenTrackRecords, 0, 0, 0, 212, 1, 10, static_cast<tMenuItem *>(&itemPostGameTrackRecordsContinue), static_cast<tMenuItem *>(0)),
    menuPinkSlipCongrats(32768, screenPinkSlipCongrats, 0, 0, MenuExtended::AwardPinkSlipsCar, -1),
    menuBeTheCopCongrats(32768, screenBeTheCopCongrats, 0, 0, MenuExtended::TransitionFromPostGameToMainMenuAndSaveGame, -1),
    menuTierCompleteCongrats(32768, screenTournamentCongrats, 0, 0, MenuExtended::TransitionFromPostGameToMainMenuAndSaveGame, -1),
    menuCredits(4112, screenMain, 0, 0, 0, -1)
{
    FUNCTION_MARKER(0x8002E3B8, "FRONT.BIN");
    menuPlayerOneCarSelect.fChildMenu = &menuPlayerTwoCarSelect;
    menuPlayerOneGarage.fChildMenu = &menuPlayerTwoGarage;
    menuPlayerOnePinkSlipCarSelect.fChildMenu = &menuPlayerTwoPinkSlipCarSelect;
    itemMusicVolume.SetDimensions(0, 0, 120, 5);
    itemSoundEffectsVolume.SetDimensions(0, 0, 120, 5);
    itemEngineVolume.SetDimensions(0, 0, 120, 5);
    itemSpeechVolume.SetDimensions(0, 0, 120, 5);
    itemAmbientVolume.SetDimensions(0, 0, 120, 5);
    uint32 flags = itemTournamentFinishedHome.fFlags;
    iteratorPinkSlipsCar.fCarListFilter = 32;
    iteratorGarageCar.fCarListFilter = 2;
    iteratorDealerCar.fCarListFilter = 1;
    iteratorSellerCar.fCarListFilter = 2;
    menuAudio.VertHelp = 0;
    menuDisplayOptions.VertHelp = 0;
    menuControllerConfig.VertHelp = 1;
    itemTournamentFinishedHome.fFlags = flags | 0x40;
    menuUserName.VertHelp = 0;
    menuTrackRecords.VertHelp = 1;
    menuTrophyInfo.VertHelp = 0;
}

tGlobalMenuDefs::~tGlobalMenuDefs(void)
{
    FUNCTION_MARKER(0x800315D4, "FRONT.BIN");
}
