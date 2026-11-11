#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/aicop.h"
#include "game/common/aihigh.h"
#include "game/common/aiinit.h"
#include "game/common/audioclc.h"
#include "game/common/audiocmn.h"
#include "game/common/audiomus.h"
#include "game/common/camera.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/clock.h"
#include "game/common/collide.h"
#include "game/common/dashhud.h"
#include "game/common/fastrand.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/mpause.h"
#include "game/common/r3dcar.h"
#include "game/common/render.h"
#include "game/common/replay.h"
#include "game/common/schedule.h"
#include "game/common/sim.h"
#include "game/common/simqueue.h"
#include "game/common/stats.h"
#include "game/psx/cario.h"
#include "game/psx/device.h"
#include "game/psx/draww.h"
#include "game/psx/force.h"
#include "game/psx/hud.h"
#include "game/psx/loading.h"

sint32 unPauseDelay;
sint32 skipRender;
sint32 quitType;
sint32 InBetween;
Sim::tSimGlobalVar simGlobal;
Sim::tSimSystemVar simVar;
sint32 counter[4] = { 320, 384, 448, 512 };

static void UpdateCameraForSchedule(void *)
{
    Camera::Update();
}

void Sim::StartUp(void)
{
    FUNCTION_MARKER(0x800B6574, "NFS4.EXE");
    static char schedule32Name[] = "Sc32-1";
    static char schedule32SecondName[] = "Sc32-2";
    static char schedule64Name[] = "Sc64";

    simVar.restartGame = 1;
    simVar.endSimGame = 0;
    simVar.pauseSim = 0;
    simVar.quickPauseSim = 0;
    simVar.goalClockTicks = 0;
    simVar.currentClockTicks = 0;
    simVar.keyRelease = 0;
    simGlobal.gameStarted = 0;
    simGlobal.gameTicks = 0;
    simGlobal.schedule32Hz = Sched::CreateNewSchedule(schedule32Name, 70);
    simGlobal.schedule32Hz2 = Sched::CreateNewSchedule(schedule32SecondName, 11);
    simGlobal.schedule64Hz = Sched::CreateNewSchedule(schedule64Name, 26);
    FastRandom::StartUp(GameSetup::gData.randSeed);
    AICop::StartUp();
    AIInit::StartUp1();
    R3DCar::StartUp();
    CarIO::StartUp();
    Cars::StartUp();
    R3DCar::PostStartUp();
    AIHigh::StartUp();
    Loading::UpdateLoadingScreen(8);
    Loading::UpdateLoadingScreen(9);
    AIInit::StartUp2();
    Loading::UpdateLoadingScreen(10);
    AudioCmn::Init();
    Sched::AddFunction(simGlobal.schedule64Hz, UpdateCameraForSchedule, 0, 100);
}

void Sim::Restart(void)
{
    FUNCTION_MARKER(0x800B6688, "NFS4.EXE");
    quitType = 1;
    simVar.endSimGame = 0;
    simVar.pauseSim = 0;
    simVar.quickPauseSim = 0;
    simVar.goalClockTicks = 0;
    simVar.currentClockTicks = 0;
    simGlobal.gameStarted = 0;
    countdown = 0;
    simGlobal.gameTicks = 0;
    GameSetup::gData.perpArrests = 0;
    FastRandom::StartUp(GameSetup::gData.randSeed);
    AICop::Restart();
    AIInit::Reset1();
    R3DCar::Restart();
    AIHigh::CleanUp();
    Cars::Restart();
    AIHigh::Restart2();
    AIInit::Reset2();
    Hud::Reset();
    Render::RestartTrackRender();
    SimQueue::Reset();
    simGlobal.time32Hz = Input::gTime;
}

void Sim::CleanUp(void)
{
    FUNCTION_MARKER(0x800B6754, "NFS4.EXE");
    Force::Disable();
    SimQueue::CleanUp();
    Sched::DeleteFunction(simGlobal.schedule64Hz, UpdateCameraForSchedule, 0);
    AIInit::CleanUp2();
    AIHigh::CleanUp();
    Cars::CleanUp();
    R3DCar::CleanUp();
    CarIO::CleanUp();
    AIInit::CleanUp1();
    AICop::CleanUp();
    FastRandom::CleanUp();
    Sched::CleanUpSchedule(simGlobal.schedule32Hz);
    Sched::CleanUpSchedule(simGlobal.schedule32Hz2);
    Sched::CleanUpSchedule(simGlobal.schedule64Hz);
}

void Sim::FadeInSFX(void)
{
    FUNCTION_MARKER(0x800B67F8, "NFS4.EXE");
    if (simGlobal.gameTicks == 0) {
        sint32 level = gMasterSFXLevel;
        gMasterSFXLevel = 0;
        GameSetup::gData.userSetting.sfxLevel = level;
        return;
    }
    if ((uint32)simGlobal.gameTicks - 17U < 64U) {
        sint32 level = GameSetup::gData.userSetting.sfxLevel;
        if (Replay::ReplayInterface.statsScreen != 0)
            level >>= 2;
        uint32 product = (uint32)level * (uint32)(simGlobal.gameTicks - 16);
        gMasterSFXLevel = (sint32)product >> 6;
    }
}

void Sim::ProcessSimSchedules(void)
{
    FUNCTION_MARKER(0x800B6878, "NFS4.EXE");
    if ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) &&
        ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0 ||
         (Cars::gNumHumanRaceCars == 2 &&
          (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0))) {
        FadeInSFX();
        simGlobal.gameStarted = 1;
    } else if (simGlobal.gameStarted == 0) {
        sint32 i = (uint8)countdown - 1;
        if (i < 0)
            i = 0;
        for (; i < 4; ++i)
            AudioCmn::GetAsyncSfx(2, i + 35, false);
        if (counter[(uint8)countdown] <= simGlobal.gameTicks) {
            if (simGlobal.gameTicks >= 512)
                simGlobal.gameStarted = 1;
            AudioCmn::PlaySound(-4, (uint8)countdown + 35, 0, 127, 64);
            countdown = (char)((uint8)countdown + 1);
        }
        FadeInSFX();
    }
    if ((simGlobal.gameTicks & 1) == 0) {
        systemtask(0);

        stackSpeedUpEnbabledFlag = 1;
        Stats::ClearPosition();
        Collide::ClearCollisionRegistry();
        Sched::Execute(simGlobal.schedule32Hz);
        Cars::ManageBureaucracy();
        stackSpeedUpEnbabledFlag = 0;
        Cars::CheckForAccidentScenes();
    }
    stackSpeedUpEnbabledFlag = 1;
    Sched::Execute(simGlobal.schedule64Hz);
    stackSpeedUpEnbabledFlag = 0;
    if ((simGlobal.gameTicks & 1) != 0) {
        stackSpeedUpEnbabledFlag = 1;
        AIHigh::Execute();
        stackSpeedUpEnbabledFlag = 0;
        stackSpeedUpEnbabledFlag = 1;
        Sched::Execute(simGlobal.schedule32Hz2);
        Stats::TrackEndGame();
        Stats::DoPlayerGlue();
        stackSpeedUpEnbabledFlag = 0;
        AudioClc::SoundCars();
    }
    simGlobal.gameTicks = (sint32)((uint32)simGlobal.gameTicks + 1U);
}

void Sim::ProcessPause(void)
{
    FUNCTION_MARKER(0x800B6B9C, "NFS4.EXE");
    if (unPauseDelay > clock_realTime.time32Hz)
        return;

    sint32 r = MPause::Logic();
    if (r == 1) {
        if (simVar.quickPauseSim == 0)
            AudioCmn::UnPause();
        gSimQueue_BlockSelf = 0;
        simVar.pauseSim = 0;
        Force::UnPause();
    } else if (r == 2) {
        AudioCmn::UnPauseAndRestart();
        simVar.pauseSim = 1;
        simVar.endSimGame = 1;
        simVar.restartGame = 1;
    } else {
        if ((uint32)r - 4U >= 2U)
            return;
        if (r == 5)
            GameSetup::gData.pinkSlipsForfeit = Device::gPausePortIndex != 0;
        Hud::BTC_QuitOut();
        AudioCmn::UnPauseAndRestart();
        simVar.pauseSim = 0;
        simVar.endSimGame = 1;
        simVar.restartGame = 0;
        Stats::ExtrapolateOpponentTimes(1);
        quitType = 1;
    }
    MPause::EndPauseMenu();
}

void Sim::CheckForPause(sint32 checkInput)
{
    FUNCTION_MARKER(0x800B6CA0, "NFS4.EXE");
    if (Replay::ReplayMode >= 2 || simVar.restartGame != 0 || simGlobal.gameTicks <= 32)
        return;
    if ((checkInput != 0 && Input::Interface(6, 1) != 0) || Device::gForcePause != 0) {
        unPauseDelay = (sint32)((uint32)clock_realTime.time32Hz + 4U);
        AudioCmn::Pause();
        gSimQueue_BlockSelf = 1;
        simVar.pauseSim = 1;
        Force::Pause();
    }
}

void Sim::MainGameLoop(void)
{
    FUNCTION_MARKER(0x800B6D4C, "NFS4.EXE");
    quitType = 1;
    sint32 lastGoalTick = 0;
    sint32 lastRealTick = (sint32)((uint32)clock_realTime.time64Hz + 10000U);
    simGlobal.time32Hz = Input::gTime;
    for (;;) {
        if (simVar.endSimGame != 0) {
            if (Replay::ReplayMode < 2)
                GameSetup::gData.finalPerpArrests = GameSetup::gData.perpArrests;
            return;
        }
        systemtask(0);
        sint32 realTick = clock_realTime.time64Hz;
        simVar.goalClockTicks = (sint32)((uint32)gSimQueue_Ticker << 1);
        sint32 diffReal = (sint32)((uint32)realTick - (uint32)lastRealTick);
        sint32 diffGoal = (sint32)((uint32)simVar.goalClockTicks - (uint32)lastGoalTick);
        if (diffGoal < diffReal)
            simVar.goalClockTicks = (sint32)((uint32)simVar.goalClockTicks + 1U);
        lastGoalTick = simVar.goalClockTicks;
        lastRealTick = realTick;
        if ((sint32)((uint32)Input::gTime - (uint32)simGlobal.time32Hz) > 16)
            simGlobal.time32Hz = Input::gTime;
        while (simGlobal.time32Hz <= Input::gTime && simVar.endSimGame == 0) {
            if (Replay::ReplayMode == 2 && simVar.pauseSim == 0) {
                Replay::GetInterfaceKey();
                if (Replay::ReplayInterface.depressed == 4) {
                    Replay::ReplayInterface.depressed = 0;
                    simVar.endSimGame = Replay::ReplayInterface.end;
                    if (GameSetup::gData.instantReplay != 0) {
                        AudioCmn::UnPauseAndRestart();
                    } else if (Replay::ReplayInterface.end != 0 && simVar.quickPauseSim != 0) {
                        simVar.quickPauseSim = 0;
                        AudioCmn::UnPauseAndQuit();
                    } else if (simVar.quickPauseSim != Replay::ReplayInterface.pause) {
                        simVar.quickPauseSim = Replay::ReplayInterface.pause;
                        if (Replay::ReplayInterface.pause == 0) {
                            AudioCmn::UnPause();
                        } else {
                            Camera::Update();
                            AudioCmn::Pause();
                        }
                    }
                }
            }
            if (simVar.endSimGame == 0) {
                if (simVar.pauseSim != 0) {
                    ProcessPause();
                    simVar.keyRelease = 0x18;
                } else {
                    if (Input::Interface(0x15, 1) != 0) {
                        AudioMus::SwitchSong();
                        Hud::ActivateCDPlayer = 1;
                    }
                    if (simVar.keyRelease != 0) {
                        if (Input::Interface(simVar.keyRelease, 0) == 0)
                            simVar.keyRelease = 0;
                    } else {
                        for (sint32 i = 0; i <= (sint32)(GameSetup::gData.commMode == 1); ++i) {
                            if (Input::Interface(i == 0 ? 0x1A : 0x1B, 1) != 0 &&
                                Replay::ReplayMode < 2)
                                Camera::NextMode(i);
                            if (Input::Interface(i == 0 ? 0x1C : 0x1D, 1) != 0)
                                DashHUD::ToggleHud(i);
                            Input::gLookBehind[i] = Input::Interface(i + 0x16, 0);
                            CheckForPause(1);
                        }
                    }
                }
            }
            simGlobal.time32Hz = (sint32)((uint32)simGlobal.time32Hz + 1U);
        }
        skipRender = 0;
        if (simVar.pauseSim == 0 && simVar.quickPauseSim == 0 && simVar.endSimGame == 0) {
            if (simVar.goalClockTicks < simVar.currentClockTicks) {
                skipRender = 1;
            } else {
                do {
                    if ((simVar.currentClockTicks & 1) == 0)
                        SimQueue::SetCurrentInput(simVar.currentClockTicks >> 1);
                    InBetween = 0;
                    if (Replay::ReplayMode == 2 && Replay::ReplayInterface.speed != 2 &&
                        (Replay::ReplayInterface.speed != 1 || (simVar.currentClockTicks & 1) != 0) &&
                        (Replay::ReplayInterface.speed != 0 || (simVar.currentClockTicks & 3) != 0)) {
                        if (Replay::ReplayInterface.speed == 3) {
                            ProcessSimSchedules();
                            ProcessSimSchedules();
                        } else {
                            InBetween = 1;
                            Camera::Update();
                        }
                    } else {
                        ProcessSimSchedules();
                    }
                    simVar.currentClockTicks = (sint32)((uint32)simVar.currentClockTicks + 1U);
                } while (simVar.currentClockTicks <= simVar.goalClockTicks);
            }
        } else {
            Camera::Update();
        }
        if (skipRender == 0)
            Render_Render(simVar.pauseSim);
        if (Input::MainExitKey() != 0 || (Replay::ReplayMode == 3 && simGlobal.gameTicks > 64))
            simVar.endSimGame = 1;
    }
}
