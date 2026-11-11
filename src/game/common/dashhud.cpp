#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "game/common/cameraobj.h"
#include "game/common/car.h"
#include "game/common/cars.h"
#include "game/common/dashhud.h"
#include "game/common/gmesetup.h"
#include "game/common/hudpmx.h"
#include "game/common/replay.h"
#include "game/common/sim.h"
#include "game/common/stats.h"
#include "game/psx/hud.h"

DashHUD::tInfo DashHUD::gInfo;

void DashHUD::InitHUD(void)
{
    FUNCTION_MARKER(0x8009AEBC, "NFS4.EXE");
    gInfo.splitscreen = GameSetup::gData.commMode == 1;
    for (sint32 i = 0; i <= gInfo.splitscreen; ++i) {
        GameSetup::tCarData *carInfo = &GameSetup::gData.carInfo[i];
        if (carInfo->HudSpeed == 0)
            carInfo->HudSpeedMult = 0x23CA5;
        else if (carInfo->HudSpeed == 1)
            carInfo->HudSpeedMult = 0x39999;
    }
    gInfo.showhud[0] = 1;
    gInfo.showhud[1] = 1;
    if (gInfo.splitscreen != 0) {
        gInfo.showmap[0] = 0;
        gInfo.showmap[1] = 0;
    } else {
        gInfo.showmap[0] = 1;
    }
    gInfo.wrongway[0] = 0;
    gInfo.wrongway[1] = 0;
    gInfo.record = Replay::ReplayMode < 2 ? GameSetup::gData.userSetting.bestlap : 0;
    gInfo.tutor = 0;
    gInfo.warning[0] = 0;
    gInfo.warning[1] = 0;
    gInfo.maxlaps = GameSetup::gData.numLaps;
    HudPmx::InitTextures();
    Hud::Init0();
    Hud::Init();
    Hud::PositionMap();
}

void DashHUD::ResetHUD(void)
{
    FUNCTION_MARKER(0x8009B010, "NFS4.EXE");
    gInfo.record = Replay::ReplayMode < 2 ? GameSetup::gData.userSetting.bestlap : 0;
    gInfo.warning[0] = 0;
    gInfo.warning[1] = 0;
    if (Replay::ReplayMode < 2) {
        gInfo.showhud[0] = 1;
        gInfo.showhud[1] = 1;
    }
}

void DashHUD::ToggleHud(sint32 player)
{
    FUNCTION_MARKER(0x8009B074, "NFS4.EXE");
    gInfo.showhud[player] ^= 1;
}

void DashHUD::CheckWrongWay(sint32 player)
{
    FUNCTION_MARKER(0x8009B098, "NFS4.EXE");
    Car::tObj *car = reinterpret_cast<Car::tObj *>(Camera::gInfo[player].anchor);
    sint32 wrongway = 0;
    if (car->wrongway >= 64)
        wrongway = car->wrongway < 148 ? 1 : 2;
    if (wrongway != gInfo.wrongway[player])
        gInfo.wrongway[player] = wrongway;
}

void DashHUD::HUDCalc(sint32 player)
{
    FUNCTION_MARKER(0x8009B108, "NFS4.EXE");
    Car::tObj *car = Cars::gHumanRaceCarList[player];
    static sint32 resethud;
    gInfo.lap = (sint32)((uint32)car->stats.lap + 1);
    if (gInfo.lap < 1)
        gInfo.lap = 1;
    if (gInfo.lap > gInfo.maxlaps)
        gInfo.lap = gInfo.maxlaps;
    sint32 currentTime = simGlobal.gameTicks;
    if (currentTime < 512) {
        gInfo.laptime = 0;
        gInfo.flashtime = 0;
    } else {
        sint32 lap = car->stats.lap;
        sint32 elapsed = (sint32)((uint32)currentTime - (uint32)car->stats.lapTime);
        if (lap == 0 || (elapsed >= 320 && car->stats.finishType != 2)) {
            gInfo.flashtime = 0;
            gInfo.laptime = (sint32)((uint32)simGlobal.gameTicks - (uint32)car->stats.lapTime);
            if (resethud != 0)
                resethud = 0;
        } else {
            sint32 completedTime;
            if (lap >= 1 && lap <= 4) {
                completedTime = car->stats.time[lap - 1];
            } else {
                uint32 timeOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, time)) +
                    (((uint32)lap - 1) << 2);
                uint8 *carBytes = reinterpret_cast<uint8 *>(car);
                memcpy(&completedTime, carBytes + timeOffset, sizeof(completedTime));
            }
            gInfo.laptime = completedTime;
            if (GameSetup::gData.raceType != 4 && Replay::ReplayMode < 2 &&
                (gInfo.record == 0 || gInfo.laptime < gInfo.record || GameSetup::gData.checkpointType == 4)) {
                gInfo.flashtime = 1;
                gInfo.record = gInfo.laptime;
            } else {
                if (gInfo.record == gInfo.laptime)
                    gInfo.flashtime = 1;
                sint32 flashTicks = simGlobal.gameTicks;
                if ((sint32)((uint32)car->stats.sliceTime + 192) < flashTicks)
                    gInfo.flashtime = 0;
                if (gInfo.flashtime != 0 && (flashTicks & 16) != 0) {
                    if (resethud == 0)
                        resethud = 1;
                } else if (resethud != 0) {
                    resethud = 0;
                }
            }
        }
    }
    gInfo.flashlap = (sint32)((uint32)simGlobal.gameTicks - (uint32)car->stats.lapTime) < 320 &&
        gInfo.lap == gInfo.maxlaps;
    gInfo.position = Stats::GetPosition(car);
    gInfo.opponents = Stats::GetNumOpponents();
    Cars::InitDashData(car, &gInfo.redline, &gInfo.topspeed);
    Cars::GetDashData(car, &gInfo.rpm, &gInfo.gear, &gInfo.speed);
    sint32 speed = gInfo.speed;
    sint32 topSpeed = (sint16)((uint32)gInfo.topspeed >> 16);
    if (speed < 0)
        speed = (sint32)(0 - (uint32)speed);
    gInfo.speed = speed;
    gInfo.topspeed = topSpeed;
}
