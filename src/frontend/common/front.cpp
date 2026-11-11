#include <new>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/feapp.h"
#include "frontend/common/feaudio.h"
#include "frontend/common/fecheats.h"
#include "frontend/common/fecredits.h"
#include "frontend/common/fedialog.h"
#include "frontend/common/fememcard.h"
#include "frontend/common/femenu.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/femission.h"
#include "frontend/common/fescreen.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetools.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetracks.h"
#include "frontend/common/front.h"
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
#include "frontend/common/stattool.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/audiomus.h"
#include "game/common/car.h"
#include "game/common/cars.h"
#include "game/common/gmesetup.h"
#include "game/common/new.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "game/common/stats.h"
#include "game/common/textsys.h"
#include "game/psx/audio.h"
#include "game/psx/cario.h"
#include "game/psx/platform.h"
#include "game/psx/psxcontroller.h"
#include "lib/psx/blkmov.h"
#include "lib/psx/random.h"

struct tFEStream;

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

struct tPerpModelList
{
    tCarModels carModel;
    char carColor;
};

struct tFEStream
{
    sint16 totalCars;
    sint16 totalModels;
    sint16 currentCar;
    sint16 numPlayers;
    tCarInfo playerCars[2];
    sint16 numOpponents;
    tCarLineup carLineup[6];
    sint16 numCops;
    sint16 numSuperCops;
    tCarModels copCars[6];
    sint16 copCountry[6];
    sint16 numTraffic;
    sint16 trafficCars[6];
    sint16 numPerpObjects;
    sint16 numPerps;

    tMissionInfo *pMission;

    tStageInfo *pStages;
    tPerpModelList perps[6];
    tTrackInformation trackInfo;
    tTrackInfo track;
};

class tAllScreens

{
public:
    tAllScreens(void);

    ~tAllScreens(void);

    tScreenMain screenMain;

    tScreenCarSelect screenCarSelect;

    tScreenCarSelectDuel screenCarSelectDuel;

    tScreenCarSelectTwoPlayer screenCarSelectTwoPlayer;

    tScreenCarSelectTwoPlayer screenCarSelectPlayerTwo;

    tScreenPinkSlipsCarSelect screenPinkSlipsCarSelectTwoPlayer;

    tScreenPinkSlipsCarSelect screenPinkSlipsCarSelectPlayerTwo;

    tScreenTrackRecords screenTrackRecords;

    tScreenTrackInfo screenTrackInfo;

    tScreenTrackSelect screenTrackSelect;

    tScreenTournSelect screenTournSelect;

    tScreenTournamentStandings screenTournamentStandings;

    tScreenTournamentTrophy screenTournamentTrophy;

    tScreenTrophyRoom screenTrophyRoom;

    tScreenTrophyInfo screenTrophyInfo;

    tScreenControllerConfig screenControllerConfig;

    tScreenDisplay screenDisplay;

    tScreenAudio screenAudio;

    tScreenUserName screenUserName;

    tScreenPinkSlipCongrats screenPinkSlipCongrats;

    tScreenPinkSlipStandings screenPinkSlipStandings;

    tScreenTournamentStandings3item screenTournamentStandings3item;

    tScreenPinkSlips screenPinkSlips;

    tScreenBeTheCopCongrats screenBeTheCopCongrats;

    tScreenTournamentCongrats screenTournamentCongrats;
};

struct tCarInLineup
{
    char isPlayerCar;
    char isAlive;
    char AIPersonality;
    char LineupPosition;
};

sint32 memCardReadOK;

static tCarModels regularCopModels[7][5] =
{
    { cm_BMWM5Cop, cm_BMWM5Cop, cm_BMWM5Cop, cm_HoldenHSVTCop, cm_CapriceCop },
    { cm_BMWM5Cop, cm_BMWM5Cop, cm_BMWM5Cop, cm_HoldenHSVTCop, cm_CapriceCop },
    { cm_BMWM5Cop, cm_BMWM5Cop, cm_BMWM5Cop, cm_BMWM5Cop, cm_CapriceCop },
    { cm_BMWM5Cop, cm_BMWM5Cop, cm_BMWM5Cop, cm_BMWM5Cop, cm_CorvetteCop },
    { cm_PorscheCop, cm_PorscheCop, cm_PorscheCop, cm_BMWM5Cop, cm_CorvetteCop },
    { cm_PorscheCop, cm_PorscheCop, cm_PorscheCop, cm_BMWM5Cop, cm_CorvetteCop },
    { cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop },
};

static tCarModels superCopModels[7][5] =
{
    { cm_PorscheCop, cm_PorscheCop, cm_PorscheCop, cm_BMWM5Cop, cm_CorvetteCop },
    { cm_PorscheCop, cm_PorscheCop, cm_PorscheCop, cm_BMWM5Cop, cm_CorvetteCop },
    { cm_PorscheCop, cm_PorscheCop, cm_PorscheCop, cm_BMWM5Cop, cm_CorvetteCop },
    { cm_PorscheCop, cm_PorscheCop, cm_PorscheCop, cm_DiabloCop, cm_DiabloCop },
    { cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop },
    { cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop },
    { cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop, cm_DiabloCop },
};

sint32 gPSXMemCardFull;
sint32 overRide;
sint32 ComingIntoTheFrontEndTheVeryFirstTime;
tAllScreens *gAllScreens;
char gFE_Cheats[5];
sint32 colourChosen[8];
char picked[11];
tCarInLineup CarLineup[9];

tAllScreens::tAllScreens(void)
{
    FUNCTION_MARKER(0x8002BAF4, "FRONT.BIN");
}

tAllScreens::~tAllScreens(void)
{
    FUNCTION_MARKER(0x8002BF0C, "FRONT.BIN");
}

sint32 GetPSXPadValue(sint32 value, sint32 player)
{
    FUNCTION_MARKER(0x80027194, "FRONT.BIN");
    PAD_update();
    PAD_COMMON *pad = &gPadinfo.buf[player * 4];
    sint32 type = pad->nopad == 0 ? pad->ID : 0;
    uint32 analogPlayer = (uint32)player << 30;
    uint32 newControl;
    if (type == 0x23)
    {
        switch ((uint32)value)
        {
        case 0x8000U:
            return (sint32)(analogPlayer | ((uint32)(uint8)frontEnd.IImaxRange[player] << 8) | 0x02000001U);
        case 0x4000U:
            return (sint32)(analogPlayer | ((uint32)(uint8)frontEnd.ImaxRange[player] << 8) | 0x01000001U);
        case 0x400U:
            return (sint32)(analogPlayer | 0x030AFF01U);
        case 0x800000U:
            return (sint32)(analogPlayer | ((127U - (uint8)frontEnd.deadSpot[player]) << 16) |
                ((127U - (uint8)frontEnd.steeringRange[player]) << 8) | 1U);
        case 0x200000U:
            return (sint32)(analogPlayer | (((uint8)frontEnd.deadSpot[player] + 128U) << 16) |
                (((uint8)frontEnd.steeringRange[player] + 128U) << 8) | 1U);
        }
    }
    else if (type == 0x53 || type == 0x73)
    {
        switch ((uint32)value)
        {
        case 0x400000U:
            newControl = (((uint8)frontEnd.J1MIN[player] + 128U) << 16) | 0x01000000U;
            return (sint32)(analogPlayer | newControl | (((uint8)frontEnd.J1MAX[player] + 128U) << 8) | 1U);
        case 0x100000U:
            newControl = ((127U - (uint8)frontEnd.J1MIN[player]) << 16) | 0x01000000U;
            return (sint32)(analogPlayer | newControl | ((127U - (uint8)frontEnd.J1MAX[player]) << 8) | 1U);
        case 0x200000U:
            return (sint32)(analogPlayer | (((uint8)frontEnd.J1MIN[player] + 128U) << 16) |
                (((uint8)frontEnd.J1MAX[player] + 128U) << 8) | 1U);
        case 0x80000000U:
            newControl = ((127U - (uint8)frontEnd.J2MIN[player]) << 16) | 0x02000000U;
            return (sint32)(analogPlayer | newControl | ((127U - (uint8)frontEnd.J2MAX[player]) << 8) | 1U);
        case 0x10000000U:
            newControl = ((127U - (uint8)frontEnd.J2MIN[player]) << 16) | 0x03000000U;
            return (sint32)(analogPlayer | newControl | ((127U - (uint8)frontEnd.J2MAX[player]) << 8) | 1U);
        case 0x20000000U:
            newControl = (((uint8)frontEnd.J2MIN[player] + 128U) << 16) | 0x02000000U;
            return (sint32)(analogPlayer | newControl | (((uint8)frontEnd.J2MAX[player] + 128U) << 8) | 1U);
        case 0x40000000U:
            newControl = (((uint8)frontEnd.J2MIN[player] + 128U) << 16) | 0x03000000U;
            return (sint32)(analogPlayer | newControl | (((uint8)frontEnd.J2MAX[player] + 128U) << 8) | 1U);
        case 0x800000U:
            return (sint32)(analogPlayer | ((127U - (uint8)frontEnd.J1MIN[player]) << 16) |
                ((127U - (uint8)frontEnd.J2MAX[player]) << 8) | 1U);
        }
    }
    return (sint32)(((uint32)player << 26) | ((uint32)value << 8) | 2U);
}

void SetPads(void)
{
    FUNCTION_MARKER(0x8002750C, "FRONT.BIN");
    for (sint32 j = 0; j < 2; ++j)
    {
        sint32 pad = j << 4;
        sint32 LookingFor = frontEnd.AnalogOn[j] != 0 ? 7 : 4;
        sint32 starttick = ticks;
        while ((sint32)((uint32)ticks - (uint32)starttick) < 128 && PadGetState(pad) != 6)
        {
        }
        if (PadGetState(pad) == 6)
        {
            sint32 theanalogoffset = 0;
            bool gotone = false;
            sint32 numoffsets = PadInfoMode(pad, 4, -1);
            for (sint32 i = 0; i < numoffsets; ++i)
            {
                if (PadInfoMode(pad, 4, i) == LookingFor)
                {
                    gotone = true;
                    theanalogoffset = i;
                }
            }
            if (gotone)
                PadSetMainMode(pad, theanalogoffset, 0);
        }
    }
}

void Front::ConstructAll(void)
{
    FUNCTION_MARKER(0x80026E58, "FRONT.BIN");

    void *storage = __builtin_new(sizeof(tAllScreens));
    tAllScreens *screens = ::new (storage) tAllScreens;
    gAllScreens = screens;
    screenMain = &screens->screenMain;
    screenCarSelect = &screens->screenCarSelect;
    screenCarSelectDuel = &screens->screenCarSelectDuel;
    screenCarSelectTwoPlayer = &screens->screenCarSelectTwoPlayer;
    screenCarSelectPlayerTwo = &screens->screenCarSelectPlayerTwo;
    screenPinkSlipsCarSelectTwoPlayer = &screens->screenPinkSlipsCarSelectTwoPlayer;
    screenPinkSlipsCarSelectPlayerTwo = &screens->screenPinkSlipsCarSelectPlayerTwo;
    screenTrackRecords = &screens->screenTrackRecords;
    screenTrackInfo = &screens->screenTrackInfo;
    screenTrackSelect = &screens->screenTrackSelect;
    screenTournSelect = &screens->screenTournSelect;
    screenTournamentStandings = &screens->screenTournamentStandings;
    screenTournamentTrophy = &screens->screenTournamentTrophy;
    screenTrophyRoom = &screens->screenTrophyRoom;
    screenTrophyInfo = &screens->screenTrophyInfo;
    screenControllerConfig = &screens->screenControllerConfig;
    screenDisplay = &screens->screenDisplay;
    screenAudio = &screens->screenAudio;
    screenUserName = &screens->screenUserName;
    screenPinkSlipCongrats = &screens->screenPinkSlipCongrats;
    screenPinkSlipStandings = &screens->screenPinkSlipStandings;
    screenTournamentStandings3item = &screens->screenTournamentStandings3item;
    screenPinkSlips = &screens->screenPinkSlips;
    screenBeTheCopCongrats = &screens->screenBeTheCopCongrats;
    screenTournamentCongrats = &screens->screenTournamentCongrats;
    storage = __builtin_new(sizeof(tFEApplication));
    FEApp = ::new (storage) tFEApplication;
    storage = __builtin_new(sizeof(tGlobalMenuDefs));
    menuDefs = ::new (storage) tGlobalMenuDefs;
}

void Front::DeleteAll(void)
{
    FUNCTION_MARKER(0x80026FE8, "FRONT.BIN");

    tAllScreens *screens = gAllScreens;
    if (screens != 0)
    {
        screens->~tAllScreens();
        purgememadr(screens);
    }
    tFEApplication *application = FEApp;
    if (application != 0)
    {
        application->~tFEApplication();
        purgememadr(application);
    }
    tGlobalMenuDefs *defs = menuDefs;
    if (defs != 0)
    {
        defs->~tGlobalMenuDefs();
        purgememadr(defs);
    }
}

void Front::ResetOverlayScreenGraph(void)
{
    tDialogBase::HideAllDialogs();
    gAllScreens = 0;
    FEApp = 0;
    menuDefs = 0;
    screenMain = 0;
    screenCarSelect = 0;
    screenCarSelectDuel = 0;
    screenCarSelectTwoPlayer = 0;
    screenCarSelectPlayerTwo = 0;
    screenPinkSlipsCarSelectTwoPlayer = 0;
    screenPinkSlipsCarSelectPlayerTwo = 0;
    screenTrackRecords = 0;
    screenTrackInfo = 0;
    screenTrackSelect = 0;
    screenTournSelect = 0;
    screenTournamentStandings = 0;
    screenTournamentTrophy = 0;
    screenTrophyRoom = 0;
    screenTrophyInfo = 0;
    screenControllerConfig = 0;
    screenDisplay = 0;
    screenAudio = 0;
    screenUserName = 0;
    screenPinkSlipCongrats = 0;
    screenPinkSlipStandings = 0;
    screenTournamentStandings3item = 0;
    screenPinkSlips = 0;
    screenBeTheCopCongrats = 0;
    screenTournamentCongrats = 0;
}

void Front::ResetOverlayMenuState(void)
{
    ComingIntoTheFrontEndTheVeryFirstTime = 0;
    gLargestUnused = 0;
    gMenu_SubMenuPlayer = kPlayerBoth;
    gSemiTransText = 0;
    tScreen::fSuppressLoadingText = 0;
    FeTools::gScrollTicksOut = 30;
    memCardReadOK = 0;
    menu_kUserNameRows = 0;
    MENUUSERNAME_STARTY = 0;
    StatChk::ClearNewRecords();
    carManager.InitializeIngameCarList();

    ResetOverlayCarSelect();
    ResetOverlayDemo();
    ResetOverlayCredits();
    ResetOverlayOptions();
    ResetOverlayAwards();
    ResetOverlayAudioScreen();
    ResetOverlayCongrats();
    ResetOverlayPinkSlips();
    ResetOverlayController();
}

void Front::ResetSettingsForCar(sint32 player, sint32 car)
{
    FUNCTION_MARKER(0x8002704C, "FRONT.BIN");
    frontEnd.settingsActive[player] = (sint16)((uint16)frontEnd.settingsActive[player] &
        ~(1U << ((uint32)car & 31)));
    frontEnd.carDownforce[player][car] = 50;
    frontEnd.suspension[player][car] = 50;
    frontEnd.engineTuning[player][car] = 3;
}

void Front::ResetSettingsMenu(sint32 player)
{
    FUNCTION_MARKER(0x8002709C, "FRONT.BIN");
    for (sint32 i = 0; i < 48; ++i)
        ResetSettingsForCar(player, i);
}

void Front::ResetPSXController(sint32 player, sint32 config)
{
    FUNCTION_MARKER(0x800270E4, "FRONT.BIN");
    timedwait(10);
    PAD_update();
    frontEnd.controlType[player] = gPadinfo.buf[player * 4].ID;
    frontEnd.controlConfig[player] = (char)config;
}

void Front::ResetPSXAnalogs(sint32 player)
{
    FUNCTION_MARKER(0x80027150, "FRONT.BIN");
    frontEnd.steeringRange[player] = 121;
    frontEnd.deadSpot[player] = 10;
    frontEnd.ImaxRange[player] = (char)0xFF;
    frontEnd.IImaxRange[player] = (char)0xFF;
    frontEnd.J1MAX[player] = 127;
    frontEnd.J1MIN[player] = 6;
    frontEnd.J2MAX[player] = 127;
    frontEnd.J2MIN[player] = 6;
}

void InitFrontEndStructure(void)
{
    FUNCTION_MARKER(0x80027648, "FRONT.BIN");
    frontEnd.randomSeed = 0;
    frontEnd.catchup = 1;
    frontEnd.currentPlayer = 0;
    frontEnd.cheatFlags = 0;
    frontEnd.cheatFlagsActive = 0;
    frontEnd.secretFlags = 0;
    frontEnd.gameMode = 0;
    frontEnd.raceType = 0;
    frontEnd.skillLevel = 0;
    frontEnd.headstart = 20;
    uint8 *frontBytes = reinterpret_cast<uint8 *>(&frontEnd);
    for (sint32 i = 0; i < 5; ++i)
    {
        frontEnd.track[i] = 0;
        frontEnd.lapind[i] = 0;
        frontEnd.trackmirrored[i] = 0;
        frontEnd.trackdirection[i] = 0;
        frontEnd.timeOfDay[i] = 0;
        frontEnd.weather[i] = 0;
        frontEnd.traffic[i] = 0;
        frontEnd.situations[i] = 0;

        frontBytes[offsetof(tfrontEnd, checkPointDisplay) + i] = 0;
        frontBytes[offsetof(tfrontEnd, playerCar) + i] = 0;
    }
    frontEnd.localSpeech = 0;
    frontEnd.opponentUpgrades = 0;
    frontEnd.dealerCar = 0;
    frontEnd.sellerCar = 0;
    frontEnd.damage = 1;
    frontEnd.laps = 0;
    frontEnd.recordlaptime = Stattool::ReturnRecordLapTime((uint8)frontEnd.track[0]);
    frontEnd.tractionassist = 1;
    frontEnd.bestline = 1;
    frontEnd.checkPointType = 1;
    frontEnd.tournament = 0;
    frontEnd.specialevent = 0;
    frontEnd.tier = 0;
    frontEnd.upgrade = 0;
    frontEnd.policeTier = 0;
    frontEnd.policeMission = 0;
    frontEnd.measurement = 0;
    for (sint32 i = 0; i < 2; ++i)
    {
        frontEnd.AnalogOn[i] = 1;
        frontEnd.ABS[i] = 0;
        frontEnd.transmission[i] = 1;
        frontEnd.displaySpeed[i] = 0;
        frontEnd.displayTach[i] = 1;
        frontEnd.displayMap[i] = 1;
        frontEnd.displayOpponentID[i] = 1;
        frontEnd.displayTime[i] = 1;
        frontEnd.displayPosition[i] = 1;
        frontEnd.displayLapNumber[i] = 1;
        frontEnd.displayMirror[i] = 0;
        frontEnd.pinkSlipsWins[i] = 0;
        frontEnd.pinkSlipsCash[i] = 0;
        frontEnd.pinkSlipsForfeit = -1;
        frontEnd.cameraType[i][1] = 3;
        frontEnd.cameraType[i][2] = 6;
        frontEnd.cameraType[i][3] = 7;
        frontEnd.cameraType[i][0] = 0;
        frontEnd.playerNameList[i][0] = 0;
        for (sint32 j = 0; j < 4; ++j)
            frontEnd.cameraSway[i][j] = 0;
        frontEnd.rampSteer[i] = 1;
        frontEnd.rampGas[i] = 1;
        frontEnd.rampBrake[i] = 1;
        for (sint32 j = 0; j < 48; ++j)
        {
            const tCarInfo *car = carManager.GetCarFromID((sint16)j);
            frontEnd.carColors[i][j] = car ? (char)car->fDefaultColor : 0;
        }
        frontEnd.controlType[i] = 0x41;
        frontEnd.controlConfig[i] = 0;
        frontEnd.defaultedPlayerName[i] = 1;
        frontEnd.shockMode[i] = 85;
        frontEnd.shockImpact[i] = 85;
        Front::ResetPSXAnalogs(i);
        Front::ResetPSXController(i, 0);
    }
    frontEnd.numBTracksActivated = 0;
    Front::ResetSettingsMenu(0);
    Front::ResetSettingsMenu(1);
    frontEnd.oppNumber = 1;
    frontEnd.audioMode = 2;
    frontEnd.oppCar = 0;
    frontEnd.language = 0;
    frontEnd.musicVolume = 85;
    frontEnd.sfxVolume = 85;
    frontEnd.narrationVolume = 85;
    frontEnd.engineVolume = 85;
    frontEnd.ambientVolume = 85;
    frontEnd.song = 0;
    gMasterMusicLevel = 85;
    gMasterSFXLevel = 85;
    gMasterFENarrationLevel = 85;
    gMasterAmbientLevel = 85;
    frontEnd.sensitivity = 0;
    frontEnd.GotAPlayList = 0;
    CreditManager.Setup();
}

sint32 LoadConfig(void)
{
    FUNCTION_MARKER(0x80027940, "FRONT.BIN");

    return 0;
}

sint32 *OutputDisplaySettings(sint32 *d, sint32 c, sint32 player, tTrackInformation &trackInfo)
{
    FUNCTION_MARKER(0x80027DA8, "FRONT.BIN");
    sint32 valtopass;
    uint8 displaySpeed = (uint8)frontEnd.displaySpeed[player];
    if (displaySpeed == 0)
        valtopass = CountryMeasurement[trackInfo.fSpeedoCountry];
    else if (displaySpeed == 1)
        valtopass = 0;
    else if (displaySpeed == 2)
        valtopass = 1;
    else
        valtopass = 2;
    d[0] = 0x11A;
    d[1] = c;
    d[2] = valtopass;
    d[3] = 0x11B;
    d[4] = c;
    d[5] = (uint8)frontEnd.displaySpeed[player] != 3;
    d[6] = 0x11C;
    d[7] = c;
    d[8] = (uint8)frontEnd.displayMap[player];
    d[9] = 0x11D;
    d[10] = c;
    d[11] = (uint8)frontEnd.displayOpponentID[player];
    d[12] = 0x11E;
    d[13] = c;
    d[14] = (uint8)frontEnd.displayTime[player];
    d[15] = 0x11F;
    d[16] = c;
    d[17] = (uint8)frontEnd.displayLapNumber[player];
    d[18] = 0x120;
    d[19] = c;
    d[20] = (uint8)frontEnd.displayPosition[player];
    d[21] = 0x121;
    d[22] = c;
    d[23] = (uint8)frontEnd.displayMirror[player];
    d[24] = 0x126;
    d[25] = c;
    d[26] = (uint8)frontEnd.cameraType[player][0];
    d[27] = 0x127;
    d[28] = c;
    d[29] = (uint8)frontEnd.cameraType[player][1];
    d[30] = 0x128;
    d[31] = c;
    d[32] = (uint8)frontEnd.cameraType[player][2];
    d[33] = 0x129;
    d[34] = c;
    d[35] = (uint8)frontEnd.cameraType[player][3];
    return d + 36;
}

void Front::InitialMemCardCheck(void)
{
    FUNCTION_MARKER(0x80027948, "FRONT.BIN");
    MEMCARDFRONTENDISINITTED = false;
    gPSXMemCardFull = 0;
    memCardReadOK = 0;
    Stattool::GetAllDefaultRecords(Stats::gTrackRecords, false);
    InitFrontEndStructure();
    frontEnd.language = 0;
}

void Front::SecondaryMemCardCheck(void)
{
    FUNCTION_MARKER(0x80027994, "FRONT.BIN");
    MEMCARDFRONTENDISINITTED = false;
    for (sint32 player = 0; player < 2; ++player)
    {
        if (memCardReadOK == 0)
            LoadGame((sint16)player, false, false);
        memCardReadOK = 0;
    }
    SetPads();
    MEMCARDFRONTENDISINITTED = true;
}

void Front::InitGraphics(void)
{
    FUNCTION_MARKER(0x80027A5C, "FRONT.BIN");
    initlinkmode(0, 1, 1);
    char buffer[40];
    sprintf(buffer, "%szzFE.viv", Paths_Paths[35]);
    if (XPORT_UNPLACED::FILE::addbigsync(buffer, 0, 100, &gFEBigHandle) == 0)
        for (;;)
        {
        }
    Platform::ResetDCTBuffer();
    Init_PSX_FrontEnd();
}

sint32 Front::Menu(tFront::ProcessingType role)
{
    FUNCTION_MARKER(0x80027AF4, "FRONT.BIN");
    sint32 result = kApp_Command_StartRace;
    tScreen::fSuppressLoadingText = 1;
    gLargestUnused = (uint32)largestunused();
    FeAudio::InitCommentary((uint8)frontEnd.language, 0);
    InitializeSpinningCars();
    ConstructAll();
    if (carManager.GetNumOwnedCars(0) <= 0 && tournamentManager.fMoney < 1)
    {
        sint32 extraMoney = carManager.CheapestCarStockPrice();
        tournamentManager.fMoney = (sint32)((uint32)tournamentManager.fMoney + (uint32)extraMoney + 1U);
    }
    if (role == tFront::kFront_InitialLoad || role == tFront::kFront_QuitToGameSetup)
    {
        if (role == tFront::kFront_InitialLoad)
            LoadConfig();
        if (gUseFrontend != 0)
        {
            tMenuCommand tempCommand;
            MenuExtended::TransitionFromPostGameToMainMenu(tempCommand);
            result = FEApp->RunFrontEnd();
        }
    }
    else if (role == tFront::kFront_QuitToPostGame)
    {
        gCalculateVictory = 1;
        if ((uint8)frontEnd.raceType == 2 && GameSetup::gData.replayMode == 0)
        {
            tournamentManager.AdvanceToNextTrack();
            tournamentManager.UpdateAwardInformation();
        }
        else if ((uint8)frontEnd.raceType == 6 && GameSetup::gData.replayMode == 0)
        {
            sint16 winner = ((Cars::gNewCarStatsList[0].finalPosition < 2 && frontEnd.pinkSlipsForfeit != 0) ||
                frontEnd.pinkSlipsForfeit == 1) ? 0 : 1;
            uint32 index = (uint8)frontEnd.pinkSlipsTrackIndex;
            memcpy(reinterpret_cast<char *>(&frontEnd) + offsetof(tfrontEnd, pinkSlipsWinner) + index * 2U,
                &winner, sizeof(winner));
            frontEnd.pinkSlipsWins[winner] = (char)((uint8)frontEnd.pinkSlipsWins[winner] + 1U);
        }
        result = FEApp->RunPostGame();
    }
    DeleteAll();
    FeAudio::DeInitCommentary();
    Audio::FECleanUp();
    TextSys::UnloadWords();
    FeTools::deinit();
    PSXExitFrontend();
    XPORT_UNPLACED::FILE::delbigsync(gFEBigHandle, 100);
    sint16 track = (uint8)frontEnd.track[0];
    gFEBigHandle = 0;
    frontEnd.recordlaptime = Stattool::ReturnRecordLapTime(track);
    gLargestUnused = (uint32)largestunused();
    return result;
}

sint32 Front::GetLapsForType(void)
{
    FUNCTION_MARKER(0x80027FC0, "FRONT.BIN");
    sint16 lapconv[2] = { 2, 4 };
    if ((uint8)frontEnd.raceType == 2) {
        tTournamentDefinition *definition = tournamentManager.fDefinition;
        return definition->fTournaments[definition->fTiers[tournamentManager.fTier].fTournOffset + tournamentManager.fTournament].fNumLaps;
    }
    return lapconv[(uint8)frontEnd.lapind[(uint8)frontEnd.pinkSlipsTrackIndex]];
}

void Front::InitStream(tFEStream &streamData)
{
    FUNCTION_MARKER(0x80028068, "FRONT.BIN");
    streamData.totalCars = 0;
    streamData.totalModels = 0;
    streamData.currentCar = 0;
    carManager.InitializeIngameCarList();
}

void Front::InitPlayerCars(tFEStream &streamData)
{
    FUNCTION_MARKER(0x80028098, "FRONT.BIN");
    streamData.numPlayers = 0;
    if ((uint8)frontEnd.raceType == 2) {
        carManager.GetGarageCar((uint8)frontEnd.garageCar[0], streamData.playerCars[0], 0);
        tCarInfo *carInfo = &streamData.playerCars[streamData.numPlayers];
        carInfo->fColor = carInfo->fColorOrder[carInfo->fColor];
        ++streamData.numPlayers;
    } else if ((uint8)frontEnd.raceType == 6) {
        carManager.GetPinkSlipsCar((uint8)frontEnd.pinkSlipsCar[0], streamData.playerCars[0], 0);
        tCarInfo *carInfo = &streamData.playerCars[streamData.numPlayers];
        carInfo->fColor = carInfo->fColorOrder[carInfo->fColor];
        ++streamData.numPlayers;
        carManager.GetPinkSlipsCar((uint8)frontEnd.pinkSlipsCar[1], streamData.playerCars[streamData.numPlayers], 1);
        carInfo = &streamData.playerCars[streamData.numPlayers];
        carInfo->fColor = carInfo->fColorOrder[carInfo->fColor];
        ++streamData.numPlayers;
    } else {
        sint16 i = 0;
        do {
            tCarInfo *carInfo;
            if ((uint8)frontEnd.carListType == 0) {
                carManager.GetStockCar((uint8)frontEnd.playerCar[i], streamData.playerCars[streamData.numPlayers]);
                carInfo = &streamData.playerCars[streamData.numPlayers];
                carInfo->fColor = carInfo->fColorOrder[(uint8)frontEnd.carColors[i][(sint8)carInfo->fCarID]];
            } else {
                carManager.GetGarageCar((uint8)frontEnd.garageCar[i], streamData.playerCars[streamData.numPlayers], i);
                carInfo = &streamData.playerCars[streamData.numPlayers];
                carInfo->fColor = carInfo->fColorOrder[carInfo->fColor];
            }
            if (carInfo->fCarClass == 7) {
                uint8 country = (uint8)frontEnd.carCountry[streamData.numPlayers][(sint8)carInfo->fCarID];
                carInfo->fColor = 0;
                carInfo->fCountry = country;
            }
            ++streamData.numPlayers;
            ++i;
        } while ((uint8)frontEnd.gameMode == 1 && i < 2);
    }
    for (sint16 i = 0; i < streamData.numPlayers; ++i) {
        tCarModels carModel = (tCarModels)(sint8)streamData.playerCars[i].fCarID;
        char carColor = streamData.playerCars[i].fColor;
        if (carManager.IsCarAnAddedModel(carModel, carColor) != 1 && streamData.totalModels < 13)
            streamData.totalModels = (sint16)(streamData.totalModels + 6);
        carManager.AddCarToIngameList(carModel, carColor);
        streamData.totalCars = (sint16)(streamData.totalCars + 2);
        streamData.carLineup[i].isPlayerCar = 1;
        streamData.carLineup[i].carModel = carModel;
        streamData.carLineup[i].carColor = carColor;
        streamData.carLineup[i].carUpgrades = streamData.playerCars[i].fUpgrades;
    }
}

void Front::InitTourneyTraffic(tFEStream &streamData)
{
    FUNCTION_MARKER(0x8002845C, "FRONT.BIN");
    char carColor = 0;
    sint32 firstTourney = tournamentManager.fDefinition->fTiers[tournamentManager.fTier].fTournOffset;
    tTourneyInfo *tourn = &tournamentManager.fDefinition->fTournaments[firstTourney + tournamentManager.fTournament];
    streamData.numTraffic = 0;
    if ((uint8)frontEnd.raceType == 2 && tourn->fTraffic != 0)
    {
        sint16 i = 0;
        do
        {
            tCarModels carModel = (tCarModels)(uint8)streamData.trackInfo.fTrafficCars[i];
            ++i;
            if (i > 5)
                i = 0;
            if (carManager.IsCarAnAddedModel(carModel, carColor) != 1)
            {
                streamData.totalModels = (sint16)((uint16)streamData.totalModels + 1);
                carManager.AddCarToIngameList(carModel, carColor);
            }
            streamData.trafficCars[streamData.numTraffic] = (sint16)carModel;
            streamData.numTraffic = (sint16)((uint16)streamData.numTraffic + 1);
            streamData.totalCars = (sint16)((uint16)streamData.totalCars + 1);
        } while (streamData.numTraffic < 3);
    }
}

void Front::InitOpponentCars(tFEStream &streamData)
{
    FUNCTION_MARKER(0x800285D0, "FRONT.BIN");
    streamData.numOpponents = 0;
    if ((uint8)frontEnd.raceType == 2 ||
        ((uint8)frontEnd.raceType == 0 && (uint8)frontEnd.oppNumber == 2))
    {
        tCarClassType opponentClass = cct_OpenClass;
        if ((uint8)frontEnd.raceType == 2)
            opponentClass = (tCarClassType)tournamentManager.fDefinition->fTournaments[
                tournamentManager.fDefinition->fTiers[tournamentManager.fTier].fTournOffset +
                tournamentManager.fTournament].fOpponentCarClass;
        bool usePlayerUpgrades = opponentClass == cct_OpenClass;
        tTourneyInfo *tourn;
        if (usePlayerUpgrades)
            tourn = &tournamentManager.fDefinition->fTournaments[
                tournamentManager.fDefinition->fTiers[2].fTournOffset +
                (uint8)streamData.playerCars[0].fCarID];
        else
            tourn = &tournamentManager.fDefinition->fTournaments[
                tournamentManager.fDefinition->fTiers[tournamentManager.fTier].fTournOffset +
                tournamentManager.fTournament];
        sint16 numOpponents = 5;
        if ((uint8)frontEnd.raceType == 2)
            numOpponents = (sint16)((uint32)tournamentManager.fNumRacers - 1U);
        for (sint16 i = 0; i < numOpponents; ++i)
        {
            tCarModels carModel = (tCarModels)tourn->fOpponentCar[i];
            tCarInfo *carInfo = carManager.GetCarFromID((sint16)carModel);
            char carColor = (char)carInfo->fDefaultColor;
            if (carManager.IsCarAnAddedModel(carModel, carColor) != 1 && streamData.totalModels < 16)
                streamData.totalModels = (sint16)((uint16)streamData.totalModels + 3);
            carManager.FindSimilarCar(carModel, carColor, 0, 0);
            carManager.AddCarToIngameList(carModel, carColor);
            tCarLineup *carLineup = &streamData.carLineup[i + 1];
            carLineup->isPlayerCar = 0;
            carLineup->carModel = carModel;
            carLineup->carColor = carInfo->fColorOrder[(uint8)carColor];
            if (usePlayerUpgrades)
                carLineup->carUpgrades = streamData.playerCars[0].fUpgrades;
            else
                carLineup->carUpgrades = (char)tourn->fOpponentUpgrades[i];
            if ((uint8)frontEnd.raceType == 2 && (uint8)frontEnd.tier == 0)
            {
                if (!FECheat::IsCheatEnabled(cheat_FinishedTournament) || (uint8)frontEnd.opponentUpgrades == 0)
                    carLineup->carUpgrades = 0;
                else
                    carLineup->carUpgrades = (char)tourn->fOpponentUpgrades[i];
            }
            streamData.totalCars = (sint16)((uint16)streamData.totalCars + 2);
            streamData.numOpponents = (sint16)((uint16)streamData.numOpponents + 1);
        }
        if ((uint8)frontEnd.raceType == 2)
        {
            tournamentManager.UpdateCarLineup();
            streamData.numOpponents = (sint16)((uint32)tournamentManager.fNumRacers - 1U);
            for (sint16 i = 0; i < (sint32)numOpponents + 1; ++i)
            {
                streamData.carLineup[i].personality = tournamentManager.fCarLineup[i].personality;
                streamData.carLineup[i].position = tournamentManager.fCarLineup[i].position;
            }
        }
        else
        {
            for (sint16 i = 0; i < 5; ++i)
            {
                streamData.carLineup[i + 1].personality = (tPersonalities)i;
                streamData.carLineup[i + 1].position = (char)(i + 1);
            }
            streamData.carLineup[0].position = 6;
        }
    }
    else if ((uint8)frontEnd.raceType < 2 && (uint8)frontEnd.oppNumber == 1)
    {
        tCarInfo carInfo;
        carManager.GetStockCar((uint8)frontEnd.oppCar, carInfo);
        tCarModels carModel = (tCarModels)(sint8)carInfo.fCarID;
        char carColor = carInfo.fColorOrder[carInfo.fDefaultColor];
        if (carManager.IsCarAnAddedModel(carModel, carColor) != 1)
        {
            if (streamData.totalModels < 16)
                streamData.totalModels = (sint16)((uint16)streamData.totalModels + 3);
            else
            {
                tCarModels modelList[3];
                carManager.GetClassList((tCarClassType)carInfo.fCarClass, 3, modelList);
                carManager.FindSimilarCar(carModel, carColor, 3, modelList);
            }
        }
        streamData.carLineup[1].isPlayerCar = 0;
        streamData.carLineup[1].personality = kPersonalityNemesis;
        streamData.numOpponents = (sint16)((uint16)streamData.numOpponents + 1);
        streamData.carLineup[1].position = 1;
        streamData.carLineup[1].carUpgrades = streamData.playerCars[0].fUpgrades;
        streamData.carLineup[1].carModel = carModel;
        streamData.carLineup[1].carColor = carColor;
        streamData.carLineup[0].position = (char)((uint8)streamData.numOpponents + 1);
        carManager.AddCarToIngameList(carModel, carColor);
        streamData.totalCars = (sint16)((uint16)streamData.totalCars + 2);
    }
    else
    {
        streamData.carLineup[0].position = 1;
        streamData.carLineup[1].position = 2;
    }
}

void Front::InitMissions(tFEStream &streamData)
{
    FUNCTION_MARKER(0x80028B64, "FRONT.BIN");
    streamData.pMission = 0;
    streamData.pStages = 0;
    tCarModels fBestCar = (tCarModels)0;
    if ((uint8)frontEnd.raceType == 1)
    {
        for (sint16 i = 0; i < streamData.numPlayers; ++i)
        {
            if (streamData.playerCars[i].fCarClass != 7)
                return;
            if ((sint8)streamData.playerCars[i].fCarID > (sint32)fBestCar)
                fBestCar = (tCarModels)(sint8)streamData.playerCars[i].fCarID;
        }
        char oldTier = frontEnd.policeTier;
        char oldMission = frontEnd.policeMission;
        frontEnd.policeTier = (char)((sint32)fBestCar - 22);
        frontEnd.policeMission = 0;
        missionManager.LoadDescription(true);
        missionManager.GetMissionToRace(&streamData.pMission);
        missionManager.GetMissionStages((uint8)frontEnd.policeTier,
            (uint8)frontEnd.policeMission, &streamData.pStages);
        frontEnd.policeTier = oldTier;
        frontEnd.policeMission = oldMission;
    }
}

static void PutStreamPair(sint32 *&d, sint32 command, sint32 value)
{
    *d++ = command;
    *d++ = value;
}

static void PutStreamPad(sint32 *&d, sint32 command, sint32 value, sint32 player)
{
    *d++ = command;
    *d++ = GetPSXPadValue(value, player);
}

static void PutStreamPlayerControls(sint32 *&d, sint32 player, bool night, bool copCar)
{
    const sint32 type = frontEnd.controlType[player] == 0x23 ? 0 :
        (frontEnd.controlType[player] == 0x53 || frontEnd.controlType[player] == 0x73 ? 1 : 2);
    const sint32 config = (uint8)frontEnd.controlConfig[player];
    const sint32 (&mapping)[13][3] = mappings[config];
    static const sint32 commands[2][9] = {
        { 0x4F, 0x50, 0x51, 0x52, 0xFB, 0x53, 0x66, 0x67, 0x68 },
        { 0x9B, 0x9C, 0x9D, 0x9E, 0xFC, 0x9F, 0xB2, 0xB3, 0xB4 }
    };
    static const sint32 actions[9] = { 0, 1, 2, 3, 8, 4, 5, 6, 12 };
    for (sint32 j = 0; j < 9; ++j)
        PutStreamPad(d, commands[player][j], mapping[actions[j]][type], player);

    const sint32 analogFlags = type == 1 ? 6 : 0;
    const sint32 lightCommand = player == 0 ? 0x73 : 0xBF;
    if (night)
        PutStreamPad(d, lightCommand, mapping[9][type], player);
    else
        PutStreamPair(d, lightCommand, 0);
    PutStreamPad(d, player == 0 ? 0x54 : 0xA0, (night ? 0 : mapping[9][type]) | analogFlags, player);
    PutStreamPad(d, player == 0 ? 0x4D : 0x99, mapping[11][type], player);
    PutStreamPad(d, player == 0 ? 0x75 : 0xC1, mapping[7][type], player);
    PutStreamPad(d, player == 0 ? 0x65 : 0xB1, mapping[7][type], player);
    PutStreamPad(d, player == 0 ? 0x85 : 0xD1, 0x800, player);

    const sint32 finalCommand = player == 0 ? 0x81 : 0xCD;
    if (copCar)
    {
        if (player == 0 && (uint8)frontEnd.gameMode != 1)
        {
            PutStreamPad(d, 0x7A, 0x8000, 0);
            PutStreamPad(d, 0x7B, 0x2000, 0);
            PutStreamPad(d, 0x7C, 0x1000, 0);
        }
        PutStreamPad(d, finalCommand, mapping[10][type] | analogFlags, player);
    }
    else
    {
        PutStreamPad(d, player == 0 ? 0x82 : 0xCE, mapping[10][type] | analogFlags, player);
        PutStreamPad(d, player == 0 ? 0x7D : 0xC9, mapping[0][type], player);
        PutStreamPad(d, player == 0 ? 0x7E : 0xCA, mapping[1][type], player);
        PutStreamPad(d, finalCommand, 0x40, player);
    }
}

void SetLicensePlate(void)
{
    FUNCTION_MARKER(0x8002B7CC, "FRONT.BIN");
    for (sint16 i = 0; i < 2; ++i) {
        char *name = PlayerName(i);
        char *plate = frontEnd.licensePlate[i];
        sprintf(plate, name);
        Stattool::UpperCaseItKeepingInMindThoseBloodySpecialCharacters(plate);
        s_lower(plate);
        CarIO::CleanUpLicense(i);
        CarIO::CreateLicense(plate, 0, i);
    }
}

void SetPlayList(sint32 ivealreadygotone)
{
    FUNCTION_MARKER(0x8002B874, "FRONT.BIN");

    static char gameBank[] = "ymus";
    static char gamePattern[] = "game*";
    static char menuBank[] = "amus";
    AudioMus::SysCleanUp();
    AudioMus::SysStartUp(0xC000, 0x18000, gameBank);
    AudioMus::tSongList *songlist = AudioMus::GetSongList(gamePattern, 0);
    if (ivealreadygotone == 0)
    {
        for (sint32 i = 39; i >= 0; --i)
            frontEnd.FEPlayList[i] = 0;
        for (sint32 i = 0; i < songlist->numsongs; ++i)
            frontEnd.FEPlayList[songlist->song[i].index] = 1;
    }
    frontEnd.GotAPlayList = 1;
    purgememadr(songlist);
    AudioMus::SysCleanUp();
    AudioMus::SysStartUp(0xD800, 0x18000, menuBank);
}

bool PlayerNameExist(sint32 player)
{
    FUNCTION_MARKER(0x8002B9D0, "FRONT.BIN");
    return strlen(frontEnd.playerNameList[player]) != 0;
}

char *PlayerName(sint32 player)
{
    FUNCTION_MARKER(0x8002B9FC, "FRONT.BIN");
    char *string = frontEnd.allUpperCasedPlayerNameList[player];
    sprintf(string, frontEnd.playerNameList[player]);
    if (strlen(string) != 0) {
        Stattool::UpperCaseItKeepingInMindThoseBloodySpecialCharacters(string);
        return string;
    }
    sint32 word = 0x4E;
    if ((uint8)frontEnd.gameMode == 1)
        word = (sint32)((uint32)player + 0x50);
    return TextSys::Word(word);
}

char *PlayerNameMixedCase(sint32 player)
{
    FUNCTION_MARKER(0x8002BA88, "FRONT.BIN");
    char *string = frontEnd.playerNameList[player];
    if (strlen(string) != 0)
        return string;
    sint32 word = 0x4E;
    if ((uint8)frontEnd.gameMode == 1)
        word = (sint32)((uint32)player + 0x50);
    return TextSys::Word(word);
}

void Front::InitCopCars(tFEStream &streamData)
{
    FUNCTION_MARKER(0x80028C94, "FRONT.BIN");
    tCarModels fBestModel = cm_MercedesSLK;
    tCarClassType fBestClass = cct_Roadster;
    tCarModels copModel = cm_CapriceCop;
    char copColor = 0;
    streamData.numCops = 0;
    streamData.numSuperCops = 0;
    for (sint16 i = 0; i < streamData.numPlayers; ++i)
    {
        if ((sint8)streamData.playerCars[i].fCarID > (sint32)fBestModel)
            fBestModel = (tCarModels)(sint8)streamData.playerCars[i].fCarID;
        if (streamData.playerCars[i].fCarClass > (sint32)fBestClass)
            fBestClass = (tCarClassType)streamData.playerCars[i].fCarClass;
    }
    if (streamData.pMission && streamData.pStages)
    {
        if ((uint8)frontEnd.gameMode != 1)
        {
            for (sint16 i = 0; i < streamData.pMission->fNumStages; ++i)
            {
                if (streamData.pStages[i].fWingman >= 0 || streamData.pStages[i].fBlockadeCop >= 0)
                {
                    streamData.copCars[streamData.numCops] = fBestModel;
                    streamData.copCountry[streamData.numCops] = streamData.playerCars[0].fCountry;
                    streamData.numCops = (sint16)((uint16)streamData.numCops + 1);
                    streamData.totalCars = (sint16)((uint16)streamData.totalCars + 2);
                    return;
                }
            }
        }
        return;
    }
    if ((uint8)frontEnd.raceType != 1)
        return;
    for (sint16 i = 0; i < streamData.numPlayers; ++i)
        if (streamData.playerCars[i].fCarClass == 7)
            return;
    if (streamData.numPlayers == 1 && streamData.numOpponents == 0)
    {
        streamData.numCops = 2;
        streamData.numSuperCops = 1;
    }
    else if (streamData.numPlayers == 2)
    {
        streamData.numCops = 0;
        streamData.numSuperCops = 2;
    }
    else
    {
        streamData.numCops = 4;
        streamData.numSuperCops = 0;
    }
    for (sint16 i = 0; i < streamData.numCops + streamData.numSuperCops; ++i)
    {
        if (i < streamData.numSuperCops)
            copModel = superCopModels[fBestClass][(uint8)streamData.trackInfo.fCountry];
        else
            copModel = regularCopModels[fBestClass][(uint8)streamData.trackInfo.fCountry];
        if (carManager.IsCarAnAddedModel(copModel, copColor) != 1)
        {
            streamData.totalModels = (sint16)((uint16)streamData.totalModels + 3);
            carManager.AddCarToIngameList(copModel, copColor);
        }
        streamData.copCars[i] = copModel;
        streamData.totalCars = (sint16)((uint16)streamData.totalCars + 2);
        streamData.copCountry[i] = (uint8)streamData.trackInfo.fCountry;
    }
}

void Front::InitPerps(tFEStream &streamData)
{
    FUNCTION_MARKER(0x80029054, "FRONT.BIN");
    streamData.numPerpObjects = 0;
    streamData.numPerps = 0;
    if (!streamData.pMission)
        return;
    for (sint16 i = 0; i < streamData.pMission->fNumStages; ++i)
    {
        tCarModels carModel = (tCarModels)streamData.pStages[i].fCarModel;
        char carColor = (char)streamData.pStages[i].fColor;
        tCarInfo *carInfo = carManager.GetCarFromID((sint16)carModel);
        sint16 j = 0;
        for (; j < 16; ++j)
            if ((sint8)carInfo->fColorOrder[j] == (uint8)carColor)
                break;
        carColor = (char)j;
        if (carManager.IsCarAnAddedModel(carModel, carColor) != 1)
        {
            if (streamData.totalModels < 16)
                streamData.totalModels = (sint16)((uint16)streamData.totalModels + 6);
            carManager.AddCarToIngameList(carModel, carColor);
            streamData.totalCars = (sint16)((uint16)streamData.totalCars + 2);
            streamData.perps[streamData.numPerpObjects].carModel = carModel;
            streamData.perps[streamData.numPerpObjects].carColor = (char)streamData.pStages[i].fColor;
            streamData.numPerpObjects = (sint16)((uint16)streamData.numPerpObjects + 1);
        }
        streamData.numPerps = (sint16)((uint16)streamData.numPerps + 1);
    }
}

void Front::InitTrack(tFEStream &streamData)
{
    FUNCTION_MARKER(0x8002921C, "FRONT.BIN");
    if ((uint8)frontEnd.raceType == 2)
    {
        tournamentManager.GetTrackToRace(streamData.track);
        tTrackInformation *trackInfo = trackManager.GetTrackByID((sint8)streamData.track.fTrackNumber);
        blockmove(trackInfo, &streamData.trackInfo, 48);
    }
    else
    {
        trackManager.GetTrack((uint8)frontEnd.track[(uint8)frontEnd.pinkSlipsTrackIndex], streamData.trackInfo);
        streamData.track.fTrackNumber = streamData.trackInfo.fTrackID;
        if ((uint8)frontEnd.carListType == 1 || (uint8)frontEnd.raceType == 1)
        {
            streamData.track.fDirection = (uint8)frontEnd.trackdirection[(uint8)frontEnd.pinkSlipsTrackIndex];
            streamData.track.fMirrored = (uint8)frontEnd.trackmirrored[(uint8)frontEnd.pinkSlipsTrackIndex];
            streamData.track.fTimeOfDay = (uint8)frontEnd.timeOfDay[(uint8)frontEnd.pinkSlipsTrackIndex];
            streamData.track.fWeather = (uint8)frontEnd.weather[(uint8)frontEnd.pinkSlipsTrackIndex];
        }
        else
        {
            streamData.track.fWeather = 0;
            streamData.track.fTimeOfDay = 0;
            streamData.track.fMirrored = 0;
            streamData.track.fDirection = 0;
        }
        streamData.track.fDifficulty = 0x10000;
    }
    if (streamData.track.fDirection > 1)
        streamData.track.fDirection = (uint8)(randPSX() & 1);
    if (streamData.track.fMirrored > 1)
        streamData.track.fMirrored = (uint8)(randPSX() & 1);
    if (streamData.track.fTimeOfDay > 1)
        streamData.track.fTimeOfDay = (uint8)(randPSX() & 1);
    if (streamData.track.fWeather > 1)
        streamData.track.fWeather = (uint8)(randPSX() & 1);
}

void Front::InitTraffic(tFEStream &streamData)
{
    FUNCTION_MARKER(0x800293E0, "FRONT.BIN");
    char carColor = 0;
    sint16 maxTraffic = (uint8)frontEnd.gameMode == 1 ? 3 : 6;
    if ((uint8)frontEnd.raceType == 1)
        maxTraffic = streamData.track.fTimeOfDay == 1 ? 1 : 2;
    bool bTraffic = (uint8)frontEnd.traffic[(uint8)frontEnd.pinkSlipsTrackIndex] != 0;
    if ((uint8)frontEnd.raceType < 2)
    {
        if ((uint8)frontEnd.carListType == 0 && (uint8)frontEnd.raceType == 0)
            bTraffic = true;
        else if ((uint8)frontEnd.raceType == 1)
            bTraffic = (uint8)frontEnd.traffic[0] != 0;
        if (streamData.trackInfo.fTrackDifficulty > 2)
            bTraffic = false;
        if (streamData.trackInfo.fIsEgg != 0)
            bTraffic = false;
        if ((uint8)frontEnd.gameMode == 1)
        {
            if ((uint8)frontEnd.raceType == 1)
                bTraffic = false;
        }
        else if ((uint8)frontEnd.oppNumber == 2)
            bTraffic = false;
    }
    else if ((uint8)frontEnd.raceType != 6)
        bTraffic = false;
    if (bTraffic)
    {
        streamData.numTraffic = 0;
        if (maxTraffic != 0)
        {
            sint16 i = 0;
            do
            {
                tCarModels carModel = (tCarModels)(uint8)streamData.trackInfo.fTrafficCars[i];
                ++i;
                if (i > 5)
                    i = 0;
                if (carManager.IsCarAnAddedModel(carModel, carColor) != 1)
                    carManager.AddCarToIngameList(carModel, carColor);
                streamData.trafficCars[streamData.numTraffic] = (sint16)carModel;
                streamData.numTraffic = (sint16)((uint16)streamData.numTraffic + 1);
            } while (streamData.numTraffic < maxTraffic);
        }
    }
}

sint32 * Front::AppendPlayerCarData(sint32 *stream, tFEStream &streamData)
{
    FUNCTION_MARKER(0x80029618, "FRONT.BIN");
    for (sint16 i = 0; i < streamData.numPlayers; ++i) {
        tCarInfo *carInfo = &streamData.playerCars[i];
        tCarLineup *carLineup = &streamData.carLineup[i];
        stream[0] = 0x119;
        stream[1] = streamData.currentCar;
        stream[2] = (sint8)carLineup->position;
        stream[3] = 0x104;
        stream[4] = streamData.currentCar;
        stream[5] = carInfo->fSimNumber;
        stream[6] = 0x106;
        stream[7] = streamData.currentCar;
        stream[8] = (uint8)frontEnd.transmission[i];
        stream[9] = 0x10A;
        stream[10] = streamData.currentCar;
        stream[11] = carInfo->fColor;
        stream[12] = 0x10B;
        stream[13] = streamData.currentCar;
        stream[14] = 0;
        stream[15] = 0x111;
        stream[16] = streamData.currentCar;
        stream[17] = ((uint8)frontEnd.ABS[i] != 0 && carInfo->fABSAvailable != 0) || (sint8)carInfo->fCarID == 28;
        stream[18] = 0x115;
        stream[19] = streamData.currentCar;
        stream[20] = ((uint8)frontEnd.ABS[i] != 0 && carInfo->fTractionAvailable != 0) || (sint8)carInfo->fCarID == 28;
        stream[21] = 0x110;
        stream[22] = streamData.currentCar;
        stream[23] = ((uint8)carLineup->carUpgrades >> 2) & 1;
        stream[24] = 0x112;
        stream[25] = streamData.currentCar;
        stream[26] = ((uint8)carLineup->carUpgrades >> 1) & 1;
        stream[27] = 0x10D;
        stream[28] = streamData.currentCar;
        stream[29] = (uint8)carLineup->carUpgrades & 1;
        stream[30] = 0x10C;
        stream[31] = streamData.currentCar;
        stream[32] = 0;
        stream[33] = 0x122;
        stream[34] = streamData.currentCar;
        uint32 color = carInfo->fColorList[carInfo->fColor];
        stream[35] = (sint32)(((color >> 16) & 0xFF) | (color & 0xFF00) | ((color & 0xFF) << 16));
        stream[36] = 0x123;
        stream[37] = streamData.currentCar;
        stream[38] = (sint32)carInfo->fHudColor[streamData.track.fTimeOfDay];
        stream[39] = 0x124;
        stream[40] = streamData.currentCar;
        stream[41] = carInfo->fSpeechColors[carInfo->fColor];
        stream[42] = 0x125;
        stream[43] = streamData.currentCar;
        stream[44] = carInfo->fCountry;
        stream[45] = 0x105;
        stream[46] = streamData.currentCar;
        stream[47] = carInfo->fCarClass == 7 ? 0x41 : 1;
        stream[48] = 0x113;
        stream[49] = streamData.currentCar;
        stream[50] = (carInfo->fUpgrades & 2) != 0 ? 2 : carInfo->fDefaultTires;
        stream[51] = 0x107;
        stream[52] = streamData.currentCar;
        stream[53] = (uint8)frontEnd.rampSteer[i];
        stream[54] = 0x108;
        stream[55] = streamData.currentCar;
        stream[56] = (uint8)frontEnd.rampGas[i];
        stream[57] = 0x109;
        stream[58] = streamData.currentCar;
        stream[59] = (uint8)frontEnd.rampBrake[i];
        stream = OutputDisplaySettings(stream + 60, streamData.currentCar, i, streamData.trackInfo);
        streamData.currentCar = (sint16)(streamData.currentCar + 1);
    }
    stream[0] = 29;
    stream[1] = streamData.numPlayers;
    stream[2] = 30;
    stream[3] = streamData.numOpponents;
    stream[4] = 14;
    stream[5] = 0;
    return stream + 6;
}

sint32 * Front::AppendOpponentData(sint32 *stream, tFEStream &streamData)
{
    FUNCTION_MARKER(0x80029AFC, "FRONT.BIN");
    for (sint16 i = 0; i < streamData.numOpponents; ++i)
    {
        tCarLineup *carLineup = &streamData.carLineup[i + streamData.numPlayers];
        tCarInfo *carInfo = carManager.GetCarFromID((sint16)carLineup->carModel);
        stream[0] = 0x119;
        stream[1] = streamData.currentCar;
        stream[2] = (sint8)carLineup->position;
        stream[3] = 0x104;
        stream[4] = streamData.currentCar;
        stream[5] = carInfo->fSimNumber;
        stream[6] = 0x106;
        stream[7] = streamData.currentCar;
        stream[8] = 1;
        stream[9] = 0x105;
        stream[10] = streamData.currentCar;
        stream[11] = 2;
        stream[12] = 0x114;
        stream[13] = streamData.currentCar;
        stream[14] = carLineup->personality;
        stream[15] = 0x118;
        stream[16] = streamData.currentCar;
        stream[17] = carLineup->personality;
        stream[18] = 0x10A;
        stream[19] = streamData.currentCar;
        stream[20] = (uint8)carLineup->carColor;
        stream[21] = 0x10B;
        stream[22] = streamData.currentCar;
        stream[23] = 0;
        stream[24] = 0x110;
        stream[25] = streamData.currentCar;
        stream[26] = (uint8)carLineup->carUpgrades & 1;
        stream[27] = 0x112;
        stream[28] = streamData.currentCar;
        stream[29] = ((uint8)carLineup->carUpgrades >> 1) & 1;
        stream[30] = 0x10D;
        stream[31] = streamData.currentCar;
        stream[32] = ((uint8)carLineup->carUpgrades >> 2) & 1;
        stream[33] = 0x10C;
        stream[34] = streamData.currentCar;
        stream[35] = 0;
        stream[36] = 0x125;
        stream[37] = streamData.currentCar;
        stream[38] = 0;
        stream[39] = 0x122;
        stream[40] = streamData.currentCar;
        uint32 color = carInfo->fColorList[(uint8)carLineup->carColor];
        stream[41] = (sint32)(((color >> 16) & 0xFF) | (color & 0xFF00) | ((color & 0xFF) << 16));
        stream[42] = 0x123;
        stream[43] = streamData.currentCar;
        stream[44] = (sint32)carInfo->fHudColor[streamData.track.fTimeOfDay];
        stream[45] = 0x124;
        stream[46] = streamData.currentCar;
        stream[47] = carInfo->fSpeechColors[(uint8)carLineup->carColor];
        streamData.currentCar = (sint16)((uint16)streamData.currentCar + 1);
        stream += 48;
    }
    return stream;
}

sint32 * Front::AppendCopData(sint32 *stream, tFEStream &streamData)
{
    FUNCTION_MARKER(0x80029E30, "FRONT.BIN");
    if (streamData.numCops + streamData.numSuperCops > 0)
    {
        stream[0] = 0x0C;
        stream[1] = 1;
        stream += 2;
    }
    for (sint16 i = 0; i < streamData.numCops + streamData.numSuperCops; ++i)
    {
        tCarInfo *carInfo = carManager.GetCarFromID((sint16)streamData.copCars[i]);
        stream[0] = 0x104;
        stream[1] = streamData.currentCar;
        stream[2] = carInfo->fSimNumber;
        stream[3] = 0x106;
        stream[4] = streamData.currentCar;
        stream[5] = 1;
        stream[6] = 0x105;
        stream[7] = streamData.currentCar;
        stream[8] = i < streamData.numSuperCops ? 0x10 : 8;
        stream[9] = 0x118;
        stream[10] = streamData.currentCar;
        stream[11] = (uint8)frontEnd.skillLevel + 5;
        stream[12] = 0x10A;
        stream[13] = streamData.currentCar;
        stream[14] = 0;
        stream[15] = 0x10B;
        stream[16] = streamData.currentCar;
        stream[17] = 0;
        stream[18] = 0x10C;
        stream[19] = streamData.currentCar;
        stream[20] = 0;
        stream[21] = 0x125;
        stream[22] = streamData.currentCar;
        stream[23] = streamData.copCountry[i];
        stream[24] = 0x110;
        stream[25] = streamData.currentCar;
        stream[26] = 0;
        stream[27] = 0x112;
        stream[28] = streamData.currentCar;
        stream[29] = 0;
        stream[30] = 0x10D;
        stream[31] = streamData.currentCar;
        stream[32] = 0;
        stream += 33;
        streamData.currentCar = (sint16)((uint16)streamData.currentCar + 1);
    }
    return stream;
}

sint32 * Front::AppendPerpData(sint32 *stream, tFEStream &streamData)
{
    FUNCTION_MARKER(0x8002A084, "FRONT.BIN");
    if (streamData.pMission)
    {
        stream[0] = 0x25;
        stream[1] = streamData.pMission->fNumStages;
        stream[2] = 0x26;
        stream[3] = streamData.pMission->fStageOffset;
    }
    else
    {
        stream[0] = 0x25;
        stream[1] = 0;
        stream[2] = 0x26;
        stream[3] = 0;
    }
    stream += 4;
    for (sint16 i = 0; i < streamData.numPerpObjects; ++i)
    {
        tCarInfo *carInfo = carManager.GetCarFromID((sint16)streamData.perps[i].carModel);
        stream[0] = 0x104;
        stream[1] = streamData.currentCar;
        stream[2] = carInfo->fSimNumber;
        stream[3] = 0x106;
        stream[4] = streamData.currentCar;
        stream[5] = 1;
        stream[6] = 0x10A;
        stream[7] = streamData.currentCar;
        stream[8] = (uint8)streamData.perps[i].carColor;
        stream[9] = 0x10B;
        stream[10] = streamData.currentCar;
        stream[11] = 0;
        stream[12] = 0x105;
        stream[13] = streamData.currentCar;
        stream[14] = 2;
        stream[15] = 0x118;
        stream[16] = streamData.currentCar;
        stream[17] = 0;
        stream[18] = 0x10C;
        stream[19] = streamData.currentCar;
        stream[20] = 1;
        stream[21] = 0x125;
        stream[22] = streamData.currentCar;
        stream[23] = 0;
        stream[24] = 0x110;
        stream[25] = streamData.currentCar;
        stream[26] = 0;
        stream[27] = 0x112;
        stream[28] = streamData.currentCar;
        stream[29] = 0;
        stream[30] = 0x10D;
        stream[31] = streamData.currentCar;
        stream[32] = 0;
        streamData.currentCar = (sint16)((uint16)streamData.currentCar + 1);
        stream += 33;
    }
    stream[0] = 0x25;
    stream[1] = streamData.numPerps;
    return stream + 2;
}

sint32 * Front::AppendTrafficData(sint32 *stream, tFEStream &streamData)
{
    FUNCTION_MARKER(0x8002A31C, "FRONT.BIN");
    for (sint16 i = 0; i < streamData.numTraffic; ++i)
    {
        tCarInfo *carInfo = carManager.GetCarFromID(streamData.trafficCars[i]);
        stream[0] = 0x104;
        stream[1] = streamData.currentCar;
        stream[2] = carInfo->fSimNumber;
        stream[3] = 0x106;
        stream[4] = streamData.currentCar;
        stream[5] = 1;
        stream[6] = 0x10A;
        stream[7] = streamData.currentCar;
        stream[8] = 0;
        stream[9] = 0x10B;
        stream[10] = streamData.currentCar;
        stream[11] = 0;
        stream[12] = 0x105;
        stream[13] = streamData.currentCar;
        stream[14] = 4;
        stream[15] = 0x118;
        stream[16] = streamData.currentCar;
        stream[17] = 8;
        stream[18] = 0x10C;
        stream[19] = streamData.currentCar;
        stream[20] = 1;
        stream[21] = 0x125;
        stream[22] = streamData.currentCar;
        stream[23] = 0;
        stream[24] = 0x110;
        stream[25] = streamData.currentCar;
        stream[26] = 0;
        stream[27] = 0x112;
        stream[28] = streamData.currentCar;
        stream[29] = 0;
        stream[30] = 0x10D;
        stream[31] = streamData.currentCar;
        stream[32] = 0;
        streamData.currentCar = (sint16)((uint16)streamData.currentCar + 1);
        stream += 33;
    }
    sint32 density = streamData.numTraffic / 3;
    if (streamData.numTraffic > 0 && density < 1)
        density = 1;
    stream[0] = 0x0D;
    stream[1] = density;
    return stream + 2;
}

sint32 * Front::AppendTrackData(sint32 *stream, tFEStream &streamData)
{
    FUNCTION_MARKER(0x8002A56C, "FRONT.BIN");
    tTrackInformation trackInfo;
    trackManager.GetTrack((uint8)frontEnd.track[(uint8)frontEnd.pinkSlipsTrackIndex], trackInfo);
    sint32 valtopass;
    if ((uint8)frontEnd.displaySpeed[0] == 1)
        valtopass = 0;
    else if ((uint8)frontEnd.displaySpeed[0] == 2)
        valtopass = 1;
    else
        valtopass = CountryMeasurement[trackInfo.fSpeedoCountry];
    stream[0] = 0x1A;
    stream[1] = valtopass;
    stream[2] = 0x18;
    stream[3] = streamData.track.fMirrored;
    stream[4] = 0x19;
    stream[5] = streamData.track.fDirection;
    stream[6] = 0x14;
    stream[7] = streamData.track.fWeather;
    stream[8] = 0x17;
    stream[9] = streamData.track.fTimeOfDay;
    stream[10] = 0x12;
    stream[11] = streamData.trackInfo.fSimNumber;
    stream[12] = 0x0B;
    stream[13] = (sint32)streamData.track.fDifficulty;
    return stream + 14;
}

bool Front::EnableLocalSpeech(void)
{
    FUNCTION_MARKER(0x8002A6AC, "FRONT.BIN");
    bool result = false;
    if ((uint8)frontEnd.raceType == 1)
    {
        tTrackInformation trackInfo;
        trackManager.GetTrack((uint8)frontEnd.track[(uint8)frontEnd.pinkSlipsTrackIndex], trackInfo);
        uint8 trackLanguage = (uint8)trackInfo.fLanguage;
        result = trackLanguage != (uint8)frontEnd.language &&
            (trackLanguage < 3 || trackLanguage == 6);
    }
    return result;
}

sint32 *Front::BuildStream(sint32 *stream)
{
    FUNCTION_MARKER(0x8002A738, "FRONT.BIN");
    tFEStream streamData;
    InitStream(streamData);
    InitPlayerCars(streamData);
    InitTrack(streamData);
    InitTourneyTraffic(streamData);
    InitOpponentCars(streamData);
    InitMissions(streamData);
    InitCopCars(streamData);
    InitPerps(streamData);
    InitTraffic(streamData);
    frontEnd.randomSeed = (sint16)ticks;
    seedrandom(frontEnd.randomSeed);
    for (sint32 colourLoop = 7; colourLoop >= 0; --colourLoop)
        colourChosen[colourLoop] = 0;
    Controller_SetRamp();
    resizememadr(stream, 0x10000);

    sint32 *d = stream;
    for (sint32 player = 0; player < 2; ++player)
    {
        sint32 command = player == 0 ? 0x34 : 0x3F;
        PutStreamPair(d, command++, (uint8)frontEnd.controlConfig[player]);
        PutStreamPair(d, command++, (uint8)frontEnd.deadSpot[player]);
        PutStreamPair(d, command++, (uint8)frontEnd.steeringRange[player]);
        PutStreamPair(d, command++, (uint8)frontEnd.IImaxRange[player]);
        PutStreamPair(d, command++, (uint8)frontEnd.ImaxRange[player]);
        PutStreamPair(d, command++, (uint8)frontEnd.J1MAX[player]);
        PutStreamPair(d, command++, (uint8)frontEnd.J1MIN[player]);
        PutStreamPair(d, command++, (uint8)frontEnd.J2MAX[player]);
        PutStreamPair(d, command++, (uint8)frontEnd.J2MIN[player]);
        PutStreamPair(d, command++, (uint8)frontEnd.shockMode[player]);
        PutStreamPair(d, command, (uint8)frontEnd.shockImpact[player]);
    }
    PutStreamPair(d, 0x21, -1);
    PutStreamPair(d, 0x03, 1);
    PutStreamPair(d, 0x1B, frontEnd.randomSeed);
    PutStreamPair(d, 0x4A, 0);
    sint32 gameLang = (uint8)frontEnd.language;
    sint32 trackLang = (uint8)streamData.trackInfo.fLanguage;
    PutStreamPair(d, 0x24, frontEnd.localSpeech != 0 && EnableLocalSpeech() ? trackLang : gameLang);
    PutStreamPair(d, 0x27, (uint8)frontEnd.language);
    PutStreamPair(d, 0x28, (uint8)frontEnd.measurement);
    PutStreamPair(d, 0x11, 0);
    PutStreamPair(d, 0x09, (uint8)frontEnd.skillLevel);
    PutStreamPair(d, 0x0A, (uint8)frontEnd.gameMode);
    PutStreamPair(d, 0x07, (uint8)frontEnd.raceType);
    PutStreamPair(d, 0x115, 0);
    *d++ = 0;
    PutStreamPair(d, 0x117, 0);
    *d++ = 0;
    PutStreamPair(d, 0x0F, (uint8)frontEnd.catchup);
    d = AppendTrackData(d, streamData);
    PutStreamPair(d, 0x13, (uint8)frontEnd.song);
    PutStreamPair(d, 0x2A, (uint8)frontEnd.audioMode);
    PutStreamPair(d, 0x2B, (uint8)frontEnd.musicVolume);
    PutStreamPair(d, 0x2C, (uint8)frontEnd.sfxVolume);
    PutStreamPair(d, 0x2D, (uint8)frontEnd.engineVolume);
    PutStreamPair(d, 0x2E, (uint8)frontEnd.narrationVolume);
    PutStreamPair(d, 0x2F, (uint8)frontEnd.ambientVolume);
    sint32 damage = streamData.playerCars[0].fCarID == 0x1C ? 0 :
        (frontEnd.raceType == 2 ? 1 : (uint8)frontEnd.damage);
    PutStreamPair(d, 0x16, damage);

    sint32 numplaylistsongs = 0;
    for (sint32 j = 0; j < 40; ++j)
        if (frontEnd.FEPlayList[j] != 0)
            ++numplaylistsongs;
    PutStreamPair(d, 0x30, numplaylistsongs);
    PutStreamPair(d, 0x4C, numplaylistsongs);
    for (sint32 j = 0; j < 40; ++j)
        if (frontEnd.FEPlayList[j] != 0)
            *d++ = j;
    PutStreamPair(d, 0x29, Stattool::ReturnRecordLapTime((sint8)streamData.track.fTrackNumber));
    PutStreamPair(d, 0x08, frontEnd.raceType != 0 || frontEnd.carListType != 0 ? GetLapsForType() : 1);

    const bool firstCopCar = streamData.playerCars[0].fCarClass == cct_CopCar;
    const bool secondCopCar = streamData.numPlayers > 1 && streamData.playerCars[1].fCarClass == cct_CopCar;
    if (firstCopCar || secondCopCar ||
        (frontEnd.raceType == 0 && frontEnd.oppNumber == 0 && frontEnd.gameMode == 0))
    {
        PutStreamPair(d, 0x20, 0);
        PutStreamPair(d, 0x32, 0);
        PutStreamPair(d, 0x33, 0);
    }
    else
    {
        PutStreamPair(d, 0x20, (uint8)frontEnd.checkPointType);
        for (sint32 player = 0; player < 2; ++player)
        {
            sint32 display = 0;
            if (frontEnd.checkPointDisplay[player] == 1)
                display = CountryMeasurement[streamData.trackInfo.fSpeedoCountry] != 0 ? 1 : 2;
            PutStreamPair(d, 0x32 + player, display);
        }
    }

    d = AppendPlayerCarData(d, streamData);
    d = AppendOpponentData(d, streamData);
    d = AppendPerpData(d, streamData);
    d = AppendCopData(d, streamData);
    d = AppendTrafficData(d, streamData);
    PutStreamPair(d, 0x1C, streamData.numPlayers + streamData.numOpponents + streamData.numCops +
        streamData.numSuperCops + streamData.numPerpObjects + streamData.numTraffic);
    PutStreamPad(d, 0xE5, 1, 0);
    PutStreamPad(d, 0xEB, 8, 0);
    PutStreamPad(d, 0xE8, 0x80, 0);
    PutStreamPad(d, 0xE9, 0x20, 0);
    PutStreamPad(d, 0xE6, 0x10, 0);
    PutStreamPad(d, 0xE7, 0x40, 0);
    PutStreamPad(d, 0xEA, 0x4000, 0);
    PutStreamPlayerControls(d, 0, streamData.track.fTimeOfDay != 0, firstCopCar);
    if (frontEnd.gameMode == 1)
        PutStreamPlayerControls(d, 1, streamData.track.fTimeOfDay != 0, secondCopCar);

    *d++ = 0;
    resizememadr(stream, (sint32)((d - stream) * sizeof(*d)));
    return stream;
}

void Front::GetInGameVars(void)
{
    FUNCTION_MARKER(0x8002B6D8, "FRONT.BIN");
    uint8 musicLevel = (uint8)gMasterMusicLevel;
    frontEnd.musicVolume = (char)musicLevel;
    AudioMus::Volume((musicLevel * 35) >> 6);
    frontEnd.sfxVolume = (char)gMasterSFXLevel;
    frontEnd.engineVolume = (char)gMasterEngineLevel;
    frontEnd.narrationVolume = (char)gMasterFENarrationLevel;
    frontEnd.ambientVolume = (char)gMasterAmbientLevel;
    frontEnd.pinkSlipsForfeit = (sint16)(uint32)GameSetup::gData.pinkSlipsForfeit;
    for (sint32 i = 0; i < 2; ++i)
    {
        frontEnd.controlConfig[i] = (char)GameSetup::gData.controllerData.controllerConfig[i];
        frontEnd.deadSpot[i] = (char)GameSetup::gData.controllerData.deadSpot[i];
        frontEnd.steeringRange[i] = (char)GameSetup::gData.controllerData.steeringRange[i];
        frontEnd.IImaxRange[i] = (char)GameSetup::gData.controllerData.IImaxRange[i];
        frontEnd.ImaxRange[i] = (char)GameSetup::gData.controllerData.ImaxRange[i];
        frontEnd.shockMode[i] = (char)GameSetup::gData.controllerData.shockMode[i];
        frontEnd.shockImpact[i] = (char)GameSetup::gData.controllerData.shockImpact[i];
    }
}

sint16 Front::GetTrackRaced(void)
{
    FUNCTION_MARKER(0x8002B960, "FRONT.BIN");
    sint16 trackraced;
    if ((uint8)frontEnd.raceType == 2) {
        trackraced = tournamentManager.GetLastTrackRaced();
    } else {
        tTrackInformation trackInfo;
        trackManager.GetTrack((uint8)frontEnd.track[(uint8)frontEnd.pinkSlipsTrackIndex], trackInfo);
        trackraced = (sint8)trackInfo.fTrackID;
    }
    return trackraced;
}
