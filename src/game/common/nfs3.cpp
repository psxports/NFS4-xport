#include <new>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/nfile.h"
#include "eaclib/psx/textcrnt.h"
#include "frontend/common/feaudio.h"
#include "frontend/common/fecars.h"
#include "frontend/common/fecheats.h"
#include "frontend/common/fecredits.h"
#include "frontend/common/feinput.h"
#include "frontend/common/femission.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetracks.h"
#include "frontend/common/front.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audioclc.h"
#include "game/common/audiocmn.h"
#include "game/common/audiomus.h"
#include "game/common/bworld.h"
#include "game/common/camera.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/clock.h"
#include "game/common/copspeak.h"
#include "game/common/dashhud.h"
#include "game/common/gmesetup.h"
#include "game/common/minfront.h"
#include "game/common/mpause.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "game/common/physics.h"
#include "game/common/render.h"
#include "game/common/replay.h"
#include "game/common/sim.h"
#include "game/common/simqueue.h"
#include "game/common/speech.h"
#include "game/common/stats.h"
#include "game/psx/audio.h"
#include "game/psx/hrzsku.h"
#include "game/psx/hud.h"
#include "game/psx/loading.h"
#include "game/psx/night.h"
#include "game/psx/platform.h"
#include "game/psx/texture.h"
#include "game/psx/textureprocess.h"
#include "game/psx/weather.h"
#include "lib/psx/shpsubs.h"

struct shapetbl;

struct tCredit;

struct tAcademyDefinition;

struct tTrackInfo;

struct tSaveTournament;

struct tTrackInformation;

struct tSaveTrackInfo;

struct tRecordBuffer
{
    char sName[8];
    sint32 nCar;
    sint32 nTime;
    sint32 nBestLap;
};

bool MEMCARDFRONTENDISINITTED;
sint32 gUseFrontend = 1;
char gMusicInSimIsOn = 1;
uint32 gFECheats;
uint32 gFEBonus;
NativeFileSlot *gFEBigHandle;
char gCheckTotalTime = 1;
char gCheckLapTime = 1;
char gUpdateTournamentInfoFlag = 1;
char gUpdateKnockoutInfoFlag = 1;
char gKnockOutSpeechFlag = 1;
char gCalculateVictory = 1;
tCarManager carManager;
tfrontEnd frontEnd;
tTrackManager trackManager;
tTournamentManager tournamentManager;
tCreditManager CreditManager;
tNfsSystemInfo nfs_sysInfo;
tMissionManager missionManager;
char gTournamentFinishOrder[12];
char befuddleTemp[10];

char befuddleExtensions[14][4] = {
    { 113, 112, 108, 0 },
    { 97, 105, 105, 0 },
    { 115, 112, 100, 0 },
    { 113, 97, 108, 0 },
    { 115, 97, 108, 0 },
    { 100, 116, 97, 0 },
    { 113, 112, 115, 0 },
    { 105, 110, 102, 0 },
    { 113, 100, 97, 0 },
    { 99, 97, 114, 0 },
    { 113, 97, 115, 0 },
    { 113, 100, 116, 0 },
    { 115, 112, 108, 0 },
    { 115, 112, 101, 0 },
};

char finishOrder[8];
char gPlayerEnteredNameAlready[2];
tRecordBuffer Stats::gTrackRecords[187];

void LoadFrontendOverlay(void)
{
    FUNCTION_MARKER(0x800A44F8, "NFS4.EXE");

    Front::ResetOverlayScreenGraph();
    PSXFront::ResetOverlayGraphics();
    FEInput::ResetOverlayInput();
    FeAudio::ResetOverlayState();
    Front::ResetOverlayMenuState();
}

void LoadOverlay(void)
{
    FUNCTION_MARKER(0x800A454C, "NFS4.EXE");
    LoadFrontendOverlay();

    char fname[60];
    sprintf(fname, "%sDCT.BIN", Paths_Paths[32]);
    uint32 handle = asyncloadfileat(fname, Platform::GetDCTBufferBase());
    while (getasyncreadstatus(handle) == 0)
        systemtask(0);
}

void Nfs2::SystemNLibStartUp(void)
{
    FUNCTION_MARKER(0x800A3DEC, "NFS4.EXE");
    Platform::SysStartUp();
    Loading::GetInitialMemory();
    if (Speech::fgUndefined == 0)
    {
        Speech::Speaker *speaker = new Speech::Speaker;
        speaker->fSub = 0;
        Speech::fgUndefined = speaker;
    }
    Render::InitLibRender();
}

void Nfs2::BefuddleCode(void)
{
    FUNCTION_MARKER(0x800A3E50, "NFS4.EXE");

    for (sint32 loop = 0; loop < 14; ++loop)
        strcpy(befuddleTemp, befuddleExtensions[loop]);
}

void Nfs2::ResetGame(void)
{
    FUNCTION_MARKER(0x800A3EA8, "NFS4.EXE");

    Replay::ResetReplay();
    BWorld::Restart();
    Sim::Restart();
    Camera::Init();
    Weather::Restart();
    AudioCmn::Reset();
    Speech::Reset();
    AudioClc::StartUp();
    DashHUD::ResetHUD();
}

void NFS4::LoadPerps(void)
{
    FUNCTION_MARKER(0x800A3F08, "NFS4.EXE");
    if (GameSetup::gData.numPerps <= 0)
        return;
    char filename[64];
    const char *mission = GameSetup::gData.commMode == 1 ? "zHPurs2.mis" : "zHPurs.mis";
    sprintf(filename, "%s%s", Paths_Paths[37], mission);
    char *buffer = static_cast<char *>(loadfileadr(filename, 16));
    sprintf(filename, "%s%s", Paths_Paths[37], "fecars.car");
    char *cars = static_cast<char *>(loadfileadr(filename, 16));
    uint32 numTiers;
    uint32 numMissions;
    uint32 numCars;
    memcpy(&numTiers, buffer, 4);
    memcpy(&numMissions, buffer + 4, 4);
    memcpy(&numCars, cars, 4);
    uint32 stageOffset = 12 + numTiers * 4 + numMissions * 20 + (uint32)GameSetup::gData.stageOffset * 44;
    tStageInfo *data = reinterpret_cast<tStageInfo *>(buffer + stageOffset);
    tCarInfo *carData = reinterpret_cast<tCarInfo *>(cars + 4);
    GameSetup::tPerpData *perpInfo = GameSetup::gData.perpInfo;
    if (GameSetup::gData.numPerps > 0)
    {
        uint32 i = 0;
        do
        {
            uint32 j = 0;
            for (; j < numCars; ++j)
            {
                if ((sint8)carData[j].fCarID == data->fCarModel)
                    break;
            }
            if (j >= numCars)
                j = 0;
            perpInfo->CarType = carData[j].fSimNumber;
            perpInfo->Colour = data->fColor;
            perpInfo->Personality = data->fAIPersonality;
            perpInfo->TimeLimit = data->fTimeLimit;
            if (GameSetup::gData.commMode != 1)
            {
                perpInfo->WingmanTime = data->fWingman;
                perpInfo->SpikeBeltTime = data->fSpikeBelt;
                perpInfo->BlockadeCopTime = data->fBlockadeCop;
            }
            else
            {
                perpInfo->WingmanTime = -1;
                perpInfo->SpikeBeltTime = -1;
                perpInfo->BlockadeCopTime = -1;
            }
            perpInfo->Distance = data->fDistance;
            perpInfo->SpeedFactor = data->fSpeed;
            perpInfo->WeightFactor = data->fWeight;
            perpInfo->GlueFactor = data->fGlue;
            perpInfo->SpeechColour = carData[j].fSpeechColors[data->fColor];
            uint32 color = carData[j].fColorList[data->fColor];
            perpInfo->HudColour = (color & 0xFF00) | ((color & 0xFF) << 16) | ((color >> 16) & 0xFF);
            ++data;
            ++perpInfo;
            ++i;
        } while ((sint16)i < GameSetup::gData.numPerps);
    }
    purgememadr(buffer);
    purgememadr(cars);
}

void Nfs2::GameModuleStartUp(sint32 *FrontEndDataStream)
{
    FUNCTION_MARKER(0x800A41A8, "NFS4.EXE");
    Audio::InitDriver(0, 0);
    restoretextdraw();
    Platform::InitMemory();
    Platform::ResetDCTBuffer();
    GameSetup::StartUp(FrontEndDataStream);
    Replay::InitReplay();
    BefuddleCode();
    Loading::DrawLoadingScreen();
    Loading::UpdateLoadingScreen(1);
    NFS4::LoadPerps();
    Loading::UpdateLoadingScreen(2);
    Physics::CheckGamedata();
    Clock::SystemStartUp();
    AudioCmn::LoadGameSamples();
    CopSpeak::StartUp();
    if (GameSetup::gData.raceType == 1 && Speech::fgSpeech == 0)
    {
        void *storage = Speech::operator new(sizeof(Speech));
        Speech::fgSpeech = ::new (storage) Speech;
    }
    MPause::InitMPause();
    Render::InitTrackRender();
    Loading::UpdateLoadingScreen(4);
    BWorld::Init();
    Fog::InitFogPositions();
    Loading::UpdateLoadingScreen(6);
    Hrz::InitHorizon();
    Night::InitNightDriving();
    Weather::Init();
    Loading::UpdateLoadingScreen(7);
    Sim::StartUp();
    Render::InitTrackRenderPostSim();
    Hud::InitMap();
    SimQueue::StartUp();
    sint32 available = largestunused();
    sint32 threshold = GameSetup::gData.commMode == 1 ? 0x13000 : 0xB000;
    if (available > threshold)
    {
        static char songs[] = "ymus";
        AudioMus::SysStartUp(0x6000, 0x14000, songs);
    }
    AudioMus::BuildPlayList(GameSetup::gData.userSetting.numplaylistsongs, GameSetup::gData.userSetting.playlist);
    largestunused();
}

void Nfs2::CleanUpGameModule(void)
{
    FUNCTION_MARKER(0x800A4354, "NFS4.EXE");
    Render::KillTrackRender();
    MPause::KillMPause();
    for (sint16 k = 0; k < GameSetup::gData.numCars; k = (sint16)((uint16)k + 1))
    {
        Cars::gList[k]->stats.carType = Cars::gList[k]->carInfo->carType;
        Cars::gList[k]->stats.carClass = Cars::gList[k]->carInfo->carClass;
        Cars::gList[k]->stats.carNameIndex = Cars::gList[k]->carInfo->carNameIndex;
        Cars::gList[k]->stats.carFlags = Cars::gList[k]->carFlags;
        Car::tObj *carObj = Cars::gList[k];
        const char *source = reinterpret_cast<const char *>(&carObj->stats);
        char *destination = reinterpret_cast<char *>(&Cars::gNewCarStatsList[k]);
        for (sint32 offset = 0; offset != 160; offset += 16)
        {
            uint32 words[4];
            memcpy(words, source + offset, 16);
            memcpy(destination + offset, words, 16);
        }
    }
    Replay::StoringReplay();
    AudioCmn::DeInit();
    if (Speech::fgSpeech != 0)
    {
        delete Speech::fgSpeech;
        Speech::fgSpeech = 0;
    }
    CopSpeak::CleanUp();
    Clock::SystemCleanUp();
    GameSetup::CleanUp();
    Sim::CleanUp();
    Hrz::KillHorizon();
    BWorld::DeInit();
    Camera::Kill();
    Weather::DeInit();
    Audio::DeInitDriver();
}

void NFS4::LoadingIcon(void)
{
    FUNCTION_MARKER(0x800A45B8, "NFS4.EXE");
    char fname[80];
    sprintf(fname, "%sldic.psh", Paths_Paths[37]);
    char *ldfile = static_cast<char *>(loadfileadr(fname, 0));
    sint32 lang = (uint8)frontEnd.language;
    if (lang > 5)
        lang = 0;
    shapetbl *shp = static_cast<shapetbl *>(shapepointer(ldfile, lang));
    PSX_RECT r = { 0, 0, 1024, 512 };
    ClearImage(&r, 0, 0, 0);
    DrawSync(0);
    initlinkmode(0, 100, 0);
    Draw::DirectSetEnvironment(0, 0, 512, 240, 1, 1, 1, 0, 0, 0);
    settrans(0);
    movfxya(shp, 482 - shp->width, 210);
    DrawSync(0);
    purgememadr(ldfile);
    initlinkmode(0, 1, 1);
}

void NFS3::CheckForFileOperations(void)
{
    FUNCTION_MARKER(0x800A46DC, "NFS4.EXE");
    const char *p = reinterpret_cast<const char *>(gFileDevice.operations);
    const char *end = reinterpret_cast<const char *>(gFileDevice.files);
    for (; reinterpret_cast<uintptr_t>(p) < reinterpret_cast<uintptr_t>(end); p += 4)
    {
        uint32 value;
        memcpy(&value, p, sizeof(value));
        if (value != 0)
            abort();
    }
}

sint32 Nfs4Main(void)
{
    FUNCTION_MARKER(0x800A4730, "NFS4.EXE");
    sint16 oldReplayMode = 0;
    Platform::DebuggerPollHost();
    Nfs2::SystemNLibStartUp();
    Audio::InitDriver(0, 0);
    Audio::DeInitDriver();
    SetVideoMode(0);
    frontEnd.language = (char)0xFF;
    LoadFrontendOverlay();
    ComingIntoTheFrontEndTheVeryFirstTime = 1;
    carManager.Initialize();
    carManager.LoadDescription();
    FECheat::ActivateBonus(cheat_Roadster);
    carManager.SetClassViewable(cct_Roadster, true);
    carManager.SetClassViewable(cct_PonyCar, true);
    carManager.SetClassViewable(cct_SaloonCar, true);
    carManager.SetClassViewable(cct_SportsCar, true);
    carManager.SetClassViewable(cct_SuperCar, true);
    carManager.SetClassViewable(cct_GTRCar, true);
    tournamentManager.Initialize();
    missionManager.Initialize();
    trackManager.Initialize();
    tournamentManager.LoadDescription();
    trackManager.LoadDescription();
    Front::InitGraphics();
    Front::Menu(tFront::kFront_InitialLoad);
    for (;;)
    {
        NFS3::CheckForFileOperations();
        sint32 *setup = MinFront::ParseOptions();
        if (gUseFrontend != 0)
            setup = Front::BuildStream(setup);
        trackManager.ReleaseDescription();
        missionManager.ReleaseDescription();
        tournamentManager.ReleaseDescription();
        carManager.ReleaseDescription();
        initlinkmode(0, 1, 1);
        NFS3::CheckForFileOperations();
        Nfs2::GameModuleStartUp(setup);
        NFS3::CheckForFileOperations();
        while (simVar.restartGame != 0)
        {
            simVar.restartGame = 0;
            Nfs2::ResetGame();
            Sim::MainGameLoop();
            if (GameSetup::gData.replayMode < 2 || GameSetup::gData.instantReplay != 0)
            {
                if (simVar.restartGame == 0 || GameSetup::gData.instantReplay != 0)
                {
                    GameSetup::gData.replayMode = 2;
                    if (GameSetup::gData.instantReplay == 0)
                    {
                        Replay::ReplayInterface.statsScreen = 1;
                        sint32 opponents = Stats::GetNumOpponents();
                        if (opponents > 1 && ((GameSetup::gData.raceType != 1 && GameSetup::gData.raceType != 5) || ((Cars::gHumanRaceCarList[0]->carFlags & 0x200U) == 0 && (Cars::gNumHumanRaceCars != 2 || (Cars::gHumanRaceCarList[1]->carFlags & 0x200U) == 0))))
                        {
                            static char win[] = "win*";
                            static char lose[] = "lose*";
                            AudioMus::PlaySong(Cars::gHumanRaceCarList[0]->stats.finalPosition == 1 ? win : lose);
                        }
                    }
                    else
                        Replay::ReplayInterface.statsScreen = 0;
                    Replay::ReplayMode = 2;
                    simVar.restartGame = 1;
                    GameSetup::gData.instantReplay = 0;
                }
            }
            else
                Replay::ReplayInterface.statsScreen = 0;
        }
        Nfs2::CleanUpGameModule();
        NFS4::LoadingIcon();
        LoadOverlay();
        NFS3::CheckForFileOperations();
        Front::InitGraphics();
        GameSetup::gData.replayMode = oldReplayMode;
        Front::GetInGameVars();
        carManager.LoadDescription();
        tournamentManager.LoadDescription();
        trackManager.LoadDescription();
        Front::Menu(quitType == 1 ? tFront::kFront_QuitToPostGame : tFront::kFront_QuitToGameSetup);
        oldReplayMode = (sint16)GameSetup::gData.replayMode;
        NFS3::CheckForFileOperations();
    }
}
