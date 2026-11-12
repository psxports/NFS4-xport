#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/copspeak.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/sim.h"
#include "game/common/stats.h"

Stats::tPosition Stats::racePosition[6];

void Stats::DoPlayerGlue(void)
{
    FUNCTION_MARKER(0x800B7F58, "NFS4.EXE");
    sint32 humanLeader = 99;
    for (sint32 i = 0; i < Cars::gNumRaceCars; ++i) {
        sint32 position = GetPosition(Cars::gRaceCarList[i]);
        Cars::gRaceCarList[i]->stats.position = position;
        if (racePosition[i].isHuman != 0 && humanLeader == 99)
            humanLeader = i;
    }
    if (GameSetup::gData.catchupLogic == 0 || GameSetup::gData.commMode != 1)
        return;
    Car::tObj *first = Cars::gHumanRaceCarList[0];
    if (first->stats.finishType == 2 || Cars::gHumanRaceCarList[1]->stats.finishType == 2) {
        first->glue = 0x10000;
        Cars::gHumanRaceCarList[1]->glue = 0x10000;
        return;
    }
    for (sint32 i = 0; i < Cars::gNumHumanRaceCars; ++i) {
        XASSERT(humanLeader >= 0 && humanLeader < 6);
        Car::tObj *car = Cars::gHumanRaceCarList[i];
        sint32 dist = (racePosition[humanLeader].slice - car->stats.sliceTotal);
        if (dist < 5)
            car->glue = 0x10000;
        else if (dist < 10)
            car->glue = 0x10666;
        else if (dist < 30)
            car->glue = 0x10CCC;
        else if (dist < 60)
            car->glue = 0x11333;
        else
            car->glue = 0x11999;
    }
}

void Stats::ClearPosition(void)
{
    FUNCTION_MARKER(0x800B8114, "NFS4.EXE");
    for (sint32 i = 0; i < 6; ++i) {
        racePosition[i].car = -1;
        racePosition[i].slice = -99999;
        racePosition[i].sliceTime = 0;
        racePosition[i].isHuman = 0;
    }
}

sint32 Stats::GetPosition(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800B8154, "NFS4.EXE");
    sint32 numCars = Cars::gNumRaceCars;
    sint32 carindex = carObj->carIndex;
    for (sint32 i = 0; i < numCars; ++i) {
        if (racePosition[i].car == carindex)
            return i + 1;
    }
    return 0;
}

sint32 Stats::GetNumOpponents(void)
{
    FUNCTION_MARKER(0x800B81A8, "NFS4.EXE");
    sint32 count = Cars::gNumRaceCars;
    sint32 numCars = 0;
    for (sint32 i = 0; i < count; ++i) {
        if (racePosition[i].car != -1)
            ++numCars;
    }
    return numCars;
}

void Stats::TrackStats(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800B81FC, "NFS4.EXE");
    sint32 currentTime = simGlobal.gameTicks;
    if (((uint32)currentTime & 1) != 0)
        return;
    sint32 lap = carObj->stats.lap;
    sint32 trackSlices = gNumSlices;
    uint8 *carBytes = reinterpret_cast<uint8 *>(carObj);
    if (lap < 4) {
        uint32 speedOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, topSpeed)) + ((uint32)lap << 2);
        sint32 previousSpeed;
        memcpy(&previousSpeed, carBytes + speedOffset, sizeof(previousSpeed));
        if (previousSpeed < carObj->linearVel_ch.z) {
            if ((carObj->carFlags & 8) == 0 || carObj->carInfo->carType >= 19) {
                sint32 speed = carObj->linearVel_ch.z;
                speedOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, topSpeed)) + ((uint32)carObj->stats.lap << 2);
                memcpy(carBytes + speedOffset, &speed, sizeof(speed));
            } else {
                sint32 draw = randPSX();
                sint32 limit = (Cars::topSpeedCap[carObj->carInfo->carType] - (draw * 3));
                sint32 speed = carObj->linearVel_ch.z;
                if (limit < speed) {
                    draw = randPSX();
                    speed = (Cars::topSpeedCap[carObj->carInfo->carType] - (draw * 3));
                }
                memcpy(carBytes + speedOffset, &speed, sizeof(speed));
            }
        }
    }
    lap = carObj->stats.lap;
    if (lap != carObj->lap && carObj->stats.finishType == 0) {
        sint32 completedTime;
        if ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) && ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0 || (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0))) {
            completedTime = 99999;
        } else {
            completedTime = (currentTime - carObj->stats.lapTime);
            lap = carObj->stats.lap;
        }

        uint32 timeOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, time)) + ((uint32)lap << 2);
        memcpy(carBytes + timeOffset, &completedTime, sizeof(completedTime));
        sint32 newLap = carObj->lap;
        carObj->stats.lapTime = currentTime;
        carObj->stats.lap = newLap;
        if (newLap == GameSetup::gData.numLaps && ((GameSetup::gData.raceType != 1 && GameSetup::gData.raceType != 5) || ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) == 0 && (Cars::gNumHumanRaceCars != 2 || (Cars::gHumanRaceCarList[1]->carFlags & 0x200) == 0)))) {
            carObj->stats.sliceTime = currentTime;
            carObj->stats.finishType = 2;
            carObj->stats.lapTime = (carObj->stats.lapTime - 512);
            carObj->stats.sliceTotal = ((carObj->stats.lap + 1) * trackSlices);
        }
    }
    sint32 roadSlice;
    if (GameSetup::gData.reverseTrack != 0)
        roadSlice = ((trackSlices - carObj->N.simRoadInfo.slice) - 1);
    else
        roadSlice = carObj->N.simRoadInfo.slice;
    if (carObj->stats.slice != roadSlice) {
        carObj->stats.slice = roadSlice;
        if (carObj->stats.finishType == 0) {
            carObj->stats.sliceTime = currentTime;
            carObj->stats.sliceTotal = (((carObj->stats.lap - carObj->unlap) * trackSlices) + carObj->stats.slice);
        }
    }
    sint32 numCars = Cars::gNumRaceCars;
    for (sint32 i = 0; i < numCars; ++i) {
        if (racePosition[i].slice < carObj->stats.sliceTotal || (carObj->stats.sliceTotal == racePosition[i].slice && carObj->stats.sliceTime < racePosition[i].sliceTime)) {
            for (sint32 j = numCars - 2; j >= i; --j) {
                sint32 r1 = racePosition[j].car;
                sint32 r2 = racePosition[j].slice;
                sint32 r3 = racePosition[j].sliceTime;
                sint32 r4 = racePosition[j].isHuman;
                racePosition[j + 1].car = r1;
                racePosition[j + 1].slice = r2;
                racePosition[j + 1].sliceTime = r3;
                racePosition[j + 1].isHuman = r4;
            }
            sint32 r1 = carObj->carIndex;
            sint32 r2 = carObj->stats.sliceTotal;
            sint32 flags = carObj->carFlags;
            sint32 r3 = carObj->stats.sliceTime;
            sint32 r4 = flags & 4;
            racePosition[i].car = r1;
            racePosition[i].slice = r2;
            racePosition[i].sliceTime = r3;
            racePosition[i].isHuman = r4;
            return;
        }
    }
}

void Stats::ExtrapolateOpponentTimes(sint32 type)
{
    FUNCTION_MARKER(0x800B8604, "NFS4.EXE");
    sint32 quick_finish = Input::Interface(3, 0) != 0;
    for (sint32 i = 0; i < Cars::gNumHumanRaceCars; ++i) {
        Car::tObj *carObj = Cars::gHumanRaceCarList[i];
        if (carObj->stats.finishType != 2) {
            sint32 sliceTotal = carObj->stats.sliceTotal;
            if (sliceTotal <= 0)
                sliceTotal = 1;
            sint32 startingTime = (carObj->stats.sliceTime - 512);
            if (sliceTotal < 100)
                startingTime = sliceTotal * 13;
            uint32 distance = gNumSlices * (uint32)GameSetup::gData.numLaps + carObj->stats.extractSlice;
            sint32 extrapolatedTime = (sint32)(startingTime * distance) / sliceTotal;
            carObj->stats.lapTime = quick_finish ? extrapolatedTime : (sint32)((uint32)extrapolatedTime << 1);
            if (GameSetup::gData.raceType != 2 || GameSetup::gData.localCar == i)
                Cars::gHumanRaceCarList[i]->stats.finishType = quick_finish ? 2 : 1;
        }
        if (type == 1) {
            if (GameSetup::gData.raceType == 2) {
                carObj = Cars::gHumanRaceCarList[i];
                if (carObj->stats.finishType != 2 && GameSetup::gData.localCar == i)
                    carObj->stats.finishType = quick_finish ? 2 : 1;
            } else if (quick_finish) {
                Cars::gHumanRaceCarList[i]->stats.finishType = 2;
            } else if (Cars::gHumanRaceCarList[i]->stats.finishType != 2) {
                Cars::gHumanRaceCarList[i]->stats.finishType = 1;
            }
            sint32 position = GetPosition(Cars::gHumanRaceCarList[i]);
            Cars::gHumanRaceCarList[i]->stats.finalPosition = position;
            Cars::gHumanRaceCarList[i]->stats.finalTotalTime = Cars::gHumanRaceCarList[i]->stats.lapTime;
            for (sint32 m = 0; m < GameSetup::gData.numLaps; ++m) {
                uint8 *carBytes = reinterpret_cast<uint8 *>(Cars::gHumanRaceCarList[i]);
                uint32 timeOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, time)) + ((uint32)m << 2);
                uint32 finalOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, finalLapTime)) + ((uint32)m << 2);
                sint32 lapTime;
                memcpy(&lapTime, carBytes + timeOffset, sizeof(lapTime));
                memcpy(carBytes + finalOffset, &lapTime, sizeof(lapTime));
            }
        }
        sint32 position = GetPosition(Cars::gHumanRaceCarList[i]);
        Cars::gHumanRaceCarList[i]->stats.finalPosition = position;
        position = GetPosition(Cars::gHumanRaceCarList[i]);
        Cars::gHumanRaceCarList[i]->stats.finalPosition = position;
        Cars::gHumanRaceCarList[i]->stats.finalTotalTime = Cars::gHumanRaceCarList[i]->stats.lapTime;
        Cars::gHumanRaceCarList[i]->stats.finalFinishType = Cars::gHumanRaceCarList[i]->stats.finishType;
        Cars::gHumanRaceCarList[i]->stats.finalBestLap = Cars::gHumanRaceCarList[i]->stats.time[0];
        Cars::gHumanRaceCarList[i]->stats.finalNumWarnings = Cars::gHumanRaceCarList[i]->stats.numWarnings;
        Cars::gHumanRaceCarList[i]->stats.finalNumFines = Cars::gHumanRaceCarList[i]->stats.numFines;
        Cars::gHumanRaceCarList[i]->stats.finalNumArrests = Cars::gHumanRaceCarList[i]->stats.numArrests;
        Cars::gHumanRaceCarList[i]->stats.finalDamage = 0;
        for (sint32 j = 0; j < 10; ++j) {
            carObj = Cars::gHumanRaceCarList[i];
            carObj->stats.finalDamage = (carObj->stats.finalDamage + carObj->N.damage[j]);
        }
        for (sint32 j = 0; j < GameSetup::gData.numLaps; ++j) {
            uint8 *carBytes = reinterpret_cast<uint8 *>(Cars::gHumanRaceCarList[i]);
            uint32 timeOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, time)) + ((uint32)j << 2);
            uint32 finalOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, finalLapTime)) + ((uint32)j << 2);
            sint32 lapTime;
            memcpy(&lapTime, carBytes + timeOffset, sizeof(lapTime));
            memcpy(carBytes + finalOffset, &lapTime, sizeof(lapTime));
            carObj = Cars::gHumanRaceCarList[i];
            memcpy(&lapTime, reinterpret_cast<uint8 *>(carObj) + finalOffset, sizeof(lapTime));
            if (lapTime < carObj->stats.finalBestLap && lapTime != 0)
                carObj->stats.finalBestLap = lapTime;
        }
    }
    for (sint32 i = 0; i < Cars::gNumAIRaceCars; ++i) {
        Car::tObj *carObj = Cars::gAIRaceCarList[i];
        if (carObj->stats.finishType != 2) {
            sint32 sliceTotal = carObj->stats.sliceTotal;
            if (sliceTotal <= 0)
                sliceTotal = 1;
            sint32 startingTime = (carObj->stats.sliceTime - 512);
            if (sliceTotal < 100)
                startingTime = sliceTotal * 13;
            uint32 distance = gNumSlices * (uint32)GameSetup::gData.numLaps + carObj->stats.extractSlice;
            sint32 extrapolatedTime = (sint32)(startingTime * distance) / sliceTotal;
            if (quick_finish || Cars::gHumanRaceCarList[0]->stats.finishType == 2) {
                Cars::gAIRaceCarList[i]->stats.lapTime = extrapolatedTime;
            } else {
                sint32 draw = randPSX();
                sint32 delay = (GameSetup::gData.numLaps * draw) / 128;
                Cars::gAIRaceCarList[i]->stats.lapTime = (extrapolatedTime + delay);
            }
            Cars::gAIRaceCarList[i]->stats.finishType = 2;
        }
        sint32 position = GetPosition(Cars::gAIRaceCarList[i]);
        Cars::gAIRaceCarList[i]->stats.finalPosition = position;
        Cars::gAIRaceCarList[i]->stats.finalTotalTime = Cars::gAIRaceCarList[i]->stats.lapTime;
        Cars::gAIRaceCarList[i]->stats.finalFinishType = Cars::gAIRaceCarList[i]->stats.finishType;
        Cars::gAIRaceCarList[i]->stats.finalNumArrests = Cars::gAIRaceCarList[i]->stats.numArrests;
        Cars::gAIRaceCarList[i]->stats.finalBestLap = 99999;
        for (sint32 j = 0; j < GameSetup::gData.numLaps; ++j) {
            carObj = Cars::gAIRaceCarList[i];
            uint32 timeOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, time)) + ((uint32)j << 2);
            sint32 lapTime;
            memcpy(&lapTime, reinterpret_cast<uint8 *>(carObj) + timeOffset, sizeof(lapTime));
            if (lapTime < carObj->stats.finalBestLap && lapTime > 0)
                carObj->stats.finalBestLap = lapTime;
        }
        sint32 draw = randPSX();
        carObj = Cars::gAIRaceCarList[i];
        sint32 numLaps = GameSetup::gData.numLaps;
        sint32 totalTime = carObj->stats.finalTotalTime;
        if (numLaps == 0 || (numLaps == -1 && totalTime == INT_MIN))
            abort();
        sint32 averageLap = (((totalTime / numLaps)) - (draw / 48));
        if (averageLap < carObj->stats.finalBestLap)
            carObj->stats.finalBestLap = averageLap;
        for (sint32 j = 0; j < GameSetup::gData.numLaps; ++j) {
            uint8 *carBytes = reinterpret_cast<uint8 *>(Cars::gAIRaceCarList[i]);
            uint32 timeOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, time)) + ((uint32)j << 2);
            uint32 finalOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, finalLapTime)) + ((uint32)j << 2);
            sint32 lapTime;
            memcpy(&lapTime, carBytes + timeOffset, sizeof(lapTime));
            memcpy(carBytes + finalOffset, &lapTime, sizeof(lapTime));
            carObj = Cars::gAIRaceCarList[i];
            memcpy(&lapTime, reinterpret_cast<uint8 *>(carObj) + finalOffset, sizeof(lapTime));
            if (lapTime < carObj->stats.finalBestLap && lapTime != 0)
                carObj->stats.finalBestLap = lapTime;
        }
    }
    if (!quick_finish) {
        sint32 numCars = Cars::gNumRaceCars;
        for (sint32 x = 0; x < numCars; ++x) {
            sint32 position = 1;
            sint32 compareCount = Cars::gNumRaceCars;
            for (sint32 y = 0; y < compareCount; ++y) {
                if (x != y) {
                    sint32 otherTime = Cars::gRaceCarList[y]->stats.finalTotalTime;
                    sint32 ownTime = Cars::gRaceCarList[x]->stats.finalTotalTime;
                    if (ownTime > otherTime || (ownTime == otherTime && y < x))
                        ++position;
                }
            }
            Cars::gRaceCarList[x]->stats.finalPosition = position;
        }
    }
}

void Stats::TrackEndGame(void)
{
    FUNCTION_MARKER(0x800B8DB8, "NFS4.EXE");
    sint32 Stats_PlayersFinishedRace = 0;
    if (Cars::gNumRaceCars >= 2) {
        sint32 ticks = simGlobal.gameTicks;
        if ((ticks / 64) * 64 == (ticks - 1)) {
            sint32 trackSlices = (GameSetup::gData.numLaps * gNumSlices);
            for (sint32 i = 0; i < Cars::gNumHumanRaceCars; ++i) {
                sint32 PlayerSlice = Cars::gHumanRaceCarList[i]->stats.sliceTotal;
                if (trackSlices < PlayerSlice)
                    PlayerSlice = trackSlices;
                sint32 PlayerPosition = GetPosition(Cars::gHumanRaceCarList[i]);
                sint32 DesiredSlice = 0;
                sint32 DesiredSpeed = 0;
                sint32 DesiredComparison;
                if (PlayerPosition == 1)
                    DesiredComparison = 2;
                else if (GameSetup::gData.checkpointType == 1)
                    DesiredComparison = 1;
                else
                    DesiredComparison = (PlayerPosition - 1);
                for (sint32 j = 0; j < Cars::gNumRaceCars; ++j) {
                    if (GetPosition(Cars::gRaceCarList[j]) == DesiredComparison) {
                        DesiredSlice = Cars::gRaceCarList[j]->stats.sliceTotal;
                        if (trackSlices < DesiredSlice)
                            DesiredSlice = trackSlices;
                        if (PlayerPosition == 1) {
                            sint32 speed = Cars::gRaceCarList[j]->linearVel_ch.z;
                            if (speed < 0)
                                speed = (-speed);
                            DesiredSpeed = speed >> 16;
                        } else {
                            DesiredSpeed = Cars::gHumanRaceCarList[i]->linearVel_ch.z >> 16;
                        }
                        break;
                    }
                }
                Cars::gHumanRaceCarList[i]->stats.checkpointUpdate = (DesiredSlice - PlayerSlice);
                if (DesiredSpeed < 16 || Cars::gHumanRaceCarList[i]->stats.finishType == 2) {
                    Cars::gHumanRaceCarList[i]->stats.checkpointDifference = 0;
                    Cars::gHumanRaceCarList[i]->stats.checkpointDisplay = 0;
                } else {
                    Car::tObj *carObj = Cars::gHumanRaceCarList[i];
                    sint32 difference = (carObj->stats.checkpointUpdate * 384);
                    carObj->stats.checkpointDifference = difference / DesiredSpeed;
                    Cars::gHumanRaceCarList[i]->stats.checkpointDisplay = 1;
                }
            }
        }
    }
    if (simVar.endSimGame == 0) {
        sint32 numHumanCars = Cars::gNumHumanRaceCars;
        if (numHumanCars > 0) {
            sint32 queuePlay = CopSpeak::gQueuePlay;
            sint32 queueHead = CopSpeak::gQueueHead;
            sint32 speechHandle = CopSpeak::gSpchHandle;
            for (sint32 i = 0; i < numHumanCars; ++i) {
                Car::tObj *carObj = Cars::gHumanRaceCarList[i];
                if (carObj->stats.finishType >= 2) {
                    sint32 sliceTime = carObj->stats.sliceTime;
                    sint32 ticks = simGlobal.gameTicks;
                    if ((sliceTime + 320) < ticks && ((queuePlay == queueHead && speechHandle == -1) || (sliceTime + 640) < ticks)) {
                        ++Stats_PlayersFinishedRace;
                    }
                }
            }
        }
        if (Stats_PlayersFinishedRace == Cars::gNumHumanRaceCars) {
            simVar.endSimGame = 1;
            ExtrapolateOpponentTimes(2);
        }
    }
}
