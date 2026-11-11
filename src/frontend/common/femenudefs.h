#ifndef FEMENUDEFS_H
#define FEMENUDEFS_H

#include "xport.h"
#include "frontend/common/femenuoptions.h"

class MenuExtended
{
public:
    static void SetOnePlayer(tMenuCommand &command);
    static void SetTwoPlayer(tMenuCommand &command);
    static void SetTestDrive(tMenuCommand &);
    static void SetSingleRace(tMenuCommand &);
    static void SetTournament(tMenuCommand &);
    static void SetSpecialEvent(tMenuCommand &);
    static void SetSoloRace(tMenuCommand &);
    static void GoToTwoPlayerSingleRace(tMenuCommand &command);
    static void SetDuelRace(tMenuCommand &);
    static void SetFullGrid(tMenuCommand &);
    static void TransitionFromPostGameToMainMenu(tMenuCommand &command);
    static void TransitionFromPostGameToMainMenuAndSaveGame(tMenuCommand &command);
    static void GoToCarSelect(tMenuCommand &command);
    static void GoToDealer(tMenuCommand &command);
    static void GoToSeller(tMenuCommand &command);
    static void GoToUpgrades(tMenuCommand &command);
    static void GoToBestOfOne(tMenuCommand &command);
    static void GoToBestOfThree(tMenuCommand &command);
    static void GoToBestOfFive(tMenuCommand &command);
    static void GoToRace(tMenuCommand &command);
    static void GoTo2PlayerRace(tMenuCommand &command);
    static void GoToTournTrackInfo(tMenuCommand &command);
    static void GoToSpecialEventTrackInfo(tMenuCommand &command);
    static void EnterUserName(tMenuCommand &command);
    static void GoToShowroom(tMenuCommand &command);
    static void GoToDealerShowroom(tMenuCommand &command);
    static void SetHPSoloRace(tMenuCommand &);
    static void SetHPDuelRace(tMenuCommand &);
    static void SetHotPursuit(tMenuCommand &);
    static void SellCar(tMenuCommand &);
    static void BuyCar(tMenuCommand &);
    static void PurchaseUpgrade(sint32 upgradeNumber);
    static void PurchaseUpgrade1(tMenuCommand &);
    static void PurchaseUpgrade2(tMenuCommand &);
    static void PurchaseUpgrade3(tMenuCommand &);
    static void SaveGame(tMenuCommand &);
    static void LoadGame(tMenuCommand &);
    static void TierFinished(tMenuCommand &command);
    static bool DidUserWinBeTheCop(void);
    static void PostGameMenu(tMenuCommand &command);
    static void FinishedPlayer1GetName(tMenuCommand &command);
    static void FinishedPlayer2GetName(tMenuCommand &command);
    static void SetPinkSlips(tMenuCommand &);
    static void AwardPinkSlipsCar(tMenuCommand &command);
    static void GoToGarage(tMenuCommand &command);
    static void GoToTournTrophyRoom(tMenuCommand &);
    static void GoToSETrophyRoom(tMenuCommand &);
    static void SetBeginner(tMenuCommand &);
    static void SetIntermediate(tMenuCommand &);
    static void SetExpert(tMenuCommand &);
    static void ExitTourney(tMenuCommand &command);
    static void ExitPinkSlipsEarly(tMenuCommand &command);
    static void PinkSlipsContinue(tMenuCommand &command);
};

class tGlobalMenuDefs

{
public:
    tGlobalMenuDefs(void);

    ~tGlobalMenuDefs(void);

    tMenuItemGoToMenuNFS4Button itemMainOnePlayerRace;

    tMenuItemGoToMenuNFS4Button itemMainTwoPlayerRace;

    tMenuItemGoToMenuNFS4Button itemMainTrophyRoom;

    tMenuItemGoToMenuNFS4Button itemMainOptions;

    tMenuNFS4 menuMain;

    tMenuItemGoToMenuNFS4Button itemTournTrophyRoom;

    tMenuItemGoToMenuNFS4Button itemSETrophyRoom;

    tMenuNFS4 menuTrophyRoomSelect;

    tMenuBlank menuTrophyInfo;

    tMenuItemGoToMenuNFS4Button itemSkillBeginner;

    tMenuItemGoToMenuNFS4Button itemSkillIntermediate;

    tMenuItemGoToMenuNFS4Button itemSkillExpert;

    tMenuNFS4 menuSkillLevel;

    tMenuItemGoToMenuNFS4Button itemOnePlayerTestDrive;

    tMenuItemGoToMenuNFS4Button itemOnePlayerSingleRace;

    tMenuItemGoToMenuNFS4Button itemOnePlayerPursuit;

    tMenuItemGoToMenuNFS4Button itemOnePlayerTournament;

    tMenuItemGoToMenuNFS4Button itemOnePlayerSpecialEvents;

    tMenuNFS4 menuOnePlayer;

    tMenuItemGoToMenuNFS4Button itemSingleRaceSolo;

    tMenuItemGoToMenuNFS4Button itemSingleRaceDuel;

    tMenuItemGoToMenuNFS4Button itemSingleRaceFullGrid;

    tMenuNFS4 menuSingleRace;

    tMenuItemGoToMenuNFS4Button itemHotPursuitSolo;

    tMenuItemGoToMenuNFS4Button itemHotPursuitDuel;

    tMenuNFS4 menuHotPursuit;

    tListIteratorTournament iteratorTournament;

    tMenuItemGoToMenuNFS4Button itemTournamentContinue;

    tMenuItemNFS4LeftRightChoice itemTournamentSelect;

    tMenuNFS4 menuTournament;

    tListIteratorTournament iteratorSpecialEvent;

    tMenuItemGoToMenuNFS4Button itemSpecialEventContinue;

    tMenuItemNFS4LeftRightChoice itemSpecialEventSelect;

    tMenuNFS4 menuSpecialEvent;

    tMenuItemGoToMenuNFS4Button itemTwoPlayerTestDrive;

    tMenuItemGoToMenuNFS4Button itemTwoPlayerDuel;

    tMenuItemGoToMenuNFS4Button itemTwoPlayerHotPursuit;

    tMenuItemGoToMenuNFS4Button itemTwoPlayerPinkSlips;

    tMenuNFS4 menuTwoPlayer;

    tMenuItemGoToMenuNFS4Button itemBestOfOne;

    tMenuItemGoToMenuNFS4Button itemBestOfThree;

    tMenuItemGoToMenuNFS4Button itemBestOfFive;

    tMenuNFS4 menuPinkSlipSelect;

    tMenuItemGoToMenuNFS4Button itemPinkSlipsContinue;

    tMenuItemNFS4LeftRightChoice itemTrack1;

    tMenuItemNFS4LeftRightChoice itemTrack2;

    tMenuItemNFS4LeftRightChoice itemTrack3;

    tMenuItemNFS4LeftRightChoice itemTrack4;

    tMenuItemNFS4LeftRightChoice itemTrack5;

    tMenuNFS4 menuPinkSlipsBestOfThree;

    tMenuNFS4 menuPinkSlipsBestOfFive;

    tListIteratorTrack iteratorTrack;

    tMenuItemGoToMenuNFS4Button itemTrackContinue;

    tMenuItemNFS4LeftRightChoice itemTrack;

    tMenuItemGoToMenuNFS4Button itemTrackRecords;

    tMenuNFS4 menuSingleTrackSelect;

    tMenuNFS4 menuTestDriveTrackSelect;

    tListIteratorIndexed iteratorLaps;

    tListIteratorIndexed iteratorTrackDirection;

    tListIteratorIndexed iteratorTrackMirrored;

    tListIteratorIndexed iteratorTimeOfDay;

    tListIteratorIndexed iteratorWeather;

    tListIteratorIndexed iteratorTraffic;

    tListIterator iteratorLocalSpeech;

    tMenuItemOptionsLeftRightChoice itemLaps;

    tMenuItemOptionsLeftRightChoice itemTrackDirection;

    tMenuItemOptionsTwoItemChoice itemTrackMirrored;

    tMenuItemOptionsTwoItemChoice itemTimeOfDay;

    tMenuItemOptionsTwoItemChoice itemWeather;

    tMenuItemOptionsTwoItemChoice itemTraffic;

    tMenuItemOptionsTwoItemChoice itemLocalSpeech;

    tMenuOptions menuTrackOptions;

    tBlankMenuItemGoToMenuNFS4Button menuTrackRecordsItem;

    tOptionsMenu menuTrackRecords;

    tMenuItemGoToMenuNFS4Button itemTrackInfoContinue;

    tMenuNFS4 menuTrackInfo;

    tListIteratorCar iteratorCar1;

    tListIteratorCarColor iteratorColor;

    tMenuItemGoToMenuNFS4Button itemCarSelectRace;

    tMenuItemNFS4LeftRightChoice itemCar;

    tMenuItemNFS4LeftRightChoice itemColor;

    tMenuItemGoToMenuNFS4Button itemShowcase;

    tMenuNFS4 menuSingleCarSelect;

    tListIteratorCar iteratorGarageCar;

    tMenuItemNFS4LeftRightChoice itemGarageCar;

    tMenuItemGoToMenuNFS4Button itemCarDealer;

    tMenuItemGoToMenuNFS4Button itemUpgradeCar;

    tMenuNFS4 menuCarGarage;

    tMenuNFS4 menuPostCarGarage;

    tListIteratorCar iteratorOpponentCar;

    tMenuItemGoToMenuNFS4Button itemDuelRace;

    tMenuItemNFS4LeftRightChoice itemCar2;

    tMenuItemNFS4LeftRightChoice itemColor2;

    tMenuItemGoToMenuNFS4Button itemGoToDuelBuyCar;

    tMenuItemNFS4LeftRightChoice itemOpponentCar;

    tMenuNFS4 menuDuelCarSelect;

    tMenuNFS4 menuHPDuelCarSelect;

    tMenuItemGoToMenuNFS4Button itemPlayerOneRace;

    tMenuItemNFS4LeftRightChoice itemCarP1;

    tMenuItemNFS4LeftRightChoice itemColorP1;

    tMenuNFS4TwoPlayer menuPlayerOneCarSelect;

    tMenuItemGoToMenuNFS4Button itemPlayerTwoRace;

    tMenuItemNFS4LeftRightChoice itemCarP2;

    tMenuItemNFS4LeftRightChoice itemColorP2;

    tMenuNFS4TwoPlayer menuPlayerTwoCarSelect;

    tMenuItemGoToMenuNFS4Button itemPlayerOneGarageRace;

    tMenuItemNFS4LeftRightChoice itemGarageCarP1;

    tMenuNFS4TwoPlayer menuPlayerOneGarage;

    tMenuItemGoToMenuNFS4Button itemPlayerTwoGarageRace;

    tMenuItemNFS4LeftRightChoice itemGarageCarP2;

    tMenuNFS4TwoPlayer menuPlayerTwoGarage;

    tListIteratorCar iteratorPinkSlipsCar;

    tMenuItemGoToMenuNFS4Button itemPlayerOnePinkSlipRace;

    tMenuItemNFS4LeftRightChoice itemPinkSlipCarP1;

    tMenuNFS4TwoPlayer menuPlayerOnePinkSlipCarSelect;

    tMenuItemGoToMenuNFS4Button itemPlayerTwoPinkSlipRace;

    tMenuItemNFS4LeftRightChoice itemPinkSlipCarP2;

    tMenuNFS4TwoPlayer menuPlayerTwoPinkSlipCarSelect;

    tMenuItemGoToMenuNFS4Button itemGoToBuyCar;

    tMenuItemGoToMenuNFS4Button itemGoToSellCar;

    tMenuNFS4 menuGoToCarDealer;

    tListIteratorCar iteratorDealerCar;

    tListIteratorCarColor iteratorDealerColor;

    tMenuItemNFS4LeftRightChoice itemDealerCar;

    tMenuItemNFS4LeftRightChoice itemDealerColor;

    tMenuItemGoToMenuNFS4Button itemBuyCar;

    tMenuItemGoToMenuNFS4Button itemDealerShowroom;

    tMenuNFS4 menuCarDealer;

    tListIteratorCar iteratorSellerCar;

    tMenuItemNFS4LeftRightChoice itemSellerCar;

    tMenuItemGoToMenuNFS4Button itemSellCar;

    tMenuNFS4 menuCarSeller;

    tMenuItemGoToMenuNFS4Button itemPurchaseUpgrade1;

    tMenuItemGoToMenuNFS4Button itemPurchaseUpgrade2;

    tMenuItemGoToMenuNFS4Button itemPurchaseUpgrade3;

    tMenuNFS4 menuCarUpgrades;

    tListIteratorIndexed iteratorTransmission;

    tListIteratorIndexed iteratorABS;

    tListIterator iteratorDamage;

    tListIterator iteratorOpponentUpgrades;

    tMenuItemOptionsLeftRightChoice itemTransmission;

    tMenuItemOptionsLeftRightChoice itemABS;

    tMenuItemOptionsLeftRightChoice itemDamage;

    tMenuItemOptionsLeftRightChoice itemTransmission2;

    tMenuItemOptionsLeftRightChoice itemABS2;

    tMenuItemOptionsLeftRightChoice itemDamage2;

    tMenuItemOptionsLeftRightChoice itemOpponentUpgrades;

    tMenuOptions menuCarOptions;

    tMenuOptions menuCarOptionsPlayerOne;

    tMenuOptions menuCarOptionsPlayerTwo;

    tMenuOptions menuPinkSlipCarOptionsPlayerOne;

    tMenuOptions menuPinkSlipCarOptionsPlayerTwo;

    tMenuBlank menuShowroom;

    tMenuItemGoToMenuNFS4Button itemOptionsAudio;

    tMenuItemGoToMenuNFS4Button itemOptionsDisplay;

    tMenuItemGoToMenuNFS4Button itemOptionsControllers;

    tMenuItemGoToMenuNFS4Button itemSaveGame;

    tMenuItemGoToMenuNFS4Button itemLoadGame;

    tMenuItemGoToMenuNFS4Button itemOptionsUsername;

    tMenuItemGoToMenuNFS4Button itemOptionsCredits;

    tMenuNFS4 menuOptions;

    tListIteratorRange iteratorMusicVolume;

    tListIteratorRange iteratorSoundEffectsVolume;

    tListIteratorRange iteratorEngineVolume;

    tListIteratorRange iteratorSpeechVolume;

    tListIteratorRange iteratorAmbientVolume;

    tListIterator iteratorAudioMode;

    tMenuItemLeftRightAudioSlider itemMusicVolume;

    tMenuItemLeftRightAudioSlider itemSoundEffectsVolume;

    tMenuItemLeftRightAudioSlider itemEngineVolume;

    tMenuItemLeftRightAudioSlider itemSpeechVolume;

    tMenuItemLeftRightAudioSlider itemAmbientVolume;

    tMenuItemDisplayLeftRightChoice itemAudioMode;

    tMenuItemSlidingActivated itemSlidingPlayList;

    tInsideBoxSongMenu menuPlayListMenu;

    tOptionsMenu menuAudio;

    tListIteratorIndexed iteratorDisplaySpeedometer;

    tListIteratorIndexed iteratorDisplayMap;

    tListIteratorIndexed iteratorDisplayOpponentID;

    tListIteratorIndexed iteratorDisplayTime;

    tListIteratorIndexed iteratorDisplayPosition;

    tListIteratorIndexed iteratorDisplayLapNumber;

    tListIterator iteratorDisplaySplitTime;

    tListIteratorIndexed iteratorDisplaySplitDisplay;

    tMenuItemDisplayLeftRightChoice itemDisplaySpeedometer;

    tMenuItemDisplayLeftRightChoice itemDisplayMap;

    tMenuItemDisplayLeftRightChoice itemDisplayOpponentID;

    tMenuItemOnOffLeftRightChoice itemDisplayTime;

    tMenuItemOnOffLeftRightChoice itemDisplayPosition;

    tMenuItemOnOffLeftRightChoice itemDisplayLapNumber;

    tMenuItemDisplayLeftRightChoice itemDisplaySplitTime;

    tMenuItemDisplayLeftRightChoice itemDisplaySplitDisplay;

    tOptionsMenu menuDisplayOptions;

    tListIteratorIndexed iteratorControllerConfigSelected;

    tMenuItemControllerLeftRightChoice itemControllerConfigSelected;

    tMenuItemSlidingMenu itemControllerSettings;

    tOptionsMenu menuControllerConfig;

    tListIteratorRangeIndexed iteratorControllerShockMode;

    tInsideBoxControllerLeftRightSlider itemControllerShockMode;

    tListIteratorRangeIndexed iteratorControllerShockImpact;

    tInsideBoxControllerLeftRightSlider itemControllerShockImpact;

    tListIteratorRangeIndexed iteratorControllerSteeringRange1;

    tInsideBoxTwoWaySlider itemControllerSteeringRange1;

    tListIteratorRangeIndexed iteratorControllerDeadSpot1;

    tInsideBoxTwoWaySlider itemControllerDeadSpot1;

    tListIteratorRangeIndexed iteratorControllerSteeringRange2;

    tInsideBoxTwoWaySlider itemControllerSteeringRange2;

    tListIteratorRangeIndexed iteratorControllerDeadSpot2;

    tInsideBoxTwoWaySlider itemControllerDeadSpot2;

    tListIteratorRangeIndexed iteratorControllerJoyRange;

    tInsideBoxTwoWaySlider itemControllerJoyRange;

    tListIteratorRangeIndexed iteratorControllerCenterPoint;

    tInsideBoxTwoWaySlider itemControllerCenterPoint;

    tListIteratorRangeIndexed iteratorControllerIMax;

    tInsideBoxTwoWaySlider itemControllerIMax;

    tListIteratorRangeIndexed iteratorControllerIIMax;

    tInsideBoxTwoWaySlider itemControllerIIMax;

    tInsideBoxMenu menuControllerDualShock;

    tInsideBoxMenu menuControllerAnalog;

    tInsideBoxMenu menuControllerDualShockAnalog;

    tInsideBoxMenu menuControllerNegcon;

    tUserNameMenuItem menuItemUserName;

    tUserNameMenuItem menuItemUserName1;

    tUserNameMenuItem menuItemUserName2;

    tOptionsMenu menuUserName;

    tListIterator iteratorChangeTrophy;

    tBlankMenuItemNFS4LeftRightChoice itemChangeTrophy;

    tMenuNFS4Bottom menuTrophyRoom;

    tMenuItemGoToMenuNFS4Button itemPinkSlipStandingsForward;

    tMenuItemGoToMenuNFS4Button itemPinkSlipStandingsExit;

    tMenuNFS4 menuPinkSlipStandings;

    tMenuItemGoToMenuNFS4Button itemTournStandingsForward;

    tMenuItemGoToMenuNFS4Button itemTournStandingsExit;

    tMenuNFS4 menuTournamentStandings;

    tMenuItemGoToMenuNFS4Button itemTournamentFinishedHome;

    tMenuNFS4 menuTournamentFinished;

    tMenuBlank menuTournamentTrophy;

    tOptionsMenu menuPostGamePlayer1Name;

    tOptionsMenu menuPostGamePlayer2Name;

    tBlankMenuItemGoToMenuNFS4Button itemPostGameTrackRecordsContinue;

    tOptionsMenu menuPostGameTrackRecords;

    tMenuBlank menuPinkSlipCongrats;

    tMenuBlank menuBeTheCopCongrats;

    tMenuBlank menuTierCompleteCongrats;

    tMenuBlank menuCredits;

};

extern tGlobalMenuDefs *menuDefs;
extern sint16 SelectListOffOn[3];
extern sint16 SelectListTrackDirection[3];

#endif
