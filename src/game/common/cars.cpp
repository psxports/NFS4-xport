#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "frontend/common/fetools.h"
#include "frontend/common/front.h"
#include "game/common/aiinit.h"
#include "game/common/aiphysic.h"
#include "game/common/aispeeds.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/bworld.h"
#include "game/common/bworldsm.h"
#include "game/common/cameraobj.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/collide.h"
#include "game/common/control.h"
#include "game/common/fastrand.h"
#include "game/common/genericpmx.h"
#include "game/common/gmesetup.h"
#include "game/common/newton.h"
#include "game/common/nfs3.h"
#include "game/common/object.h"
#include "game/common/paths.h"
#include "game/common/physics.h"
#include "game/common/r3dcar.h"
#include "game/common/replay.h"
#include "game/common/scene.h"
#include "game/common/schedule.h"
#include "game/common/sim.h"
#include "game/common/stats.h"
#include "game/common/textsys.h"
#include "game/common/trgsfx.h"
#include "game/common/udff.h"
#include "game/psx/draww.h"
#include "game/psx/force.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/random.h"

sint32 accidentSlice;
sint32 frontLimit = 32768;
sint32 rearLimit = 65536;
BO::tNewtonObj InfiniteMassNewton[5];
sint32 Cars::gNumCars;
sint32 Cars::gNumRaceCars;
sint32 Cars::gNumAICars;
sint32 Cars::gNumHumanRaceCars;
sint32 Cars::gNumAIRaceCars;
sint32 Cars::gNumTrafficCars;
sint32 Cars::gNumCopCars;
sint32 Cars::gNumLifeBasisCars;
sint32 Cars::topSpeedCap[22] = { 4107141, 3932160, 4653056, 4587520, 4660264, 4631429, 4805754, 4514775, 4543610, 5097390, 5388369, 5417861, 5796003, 6087639, 5825495, 6552944, 7274496, 7274496, 7274496, 7274496, 7274496, 7274496 };
sint32 Cars::kSkidMarkSurface[16] = { 0, 1, 2, 2, 1, 2, 0, 1, 0, 2, 1, 1, 1, 2, 0, 2 };
Car::tObj *Cars::gList[9];
Car::tObj *Cars::gRaceCarList[9];
Car::tObj *Cars::gAICarList[9];
Car::tObj *Cars::gHumanRaceCarList[9];
Car::tObj *Cars::gAIRaceCarList[9];
Car::tObj *Cars::gTrafficCarList[9];
Car::tObj *Cars::gCopCarList[9];
Car::tObj *Cars::gLifeBasisCarList[9];
Car::tObj *Cars::gSortedList[9];
Car::tObj *Cars::gTotalSortedList[9];
Car::tStats Cars::gNewCarStatsList[9];
sint32 Cars::kSmokingSurface[16] = { 0, 0, 655360, 0, 0, 1310720, 0, 0, 0, 655360, 0, 0, 1310720, 1310720, 0, 655360 };
sint32 Cars::kConvertFromRoadToSfxType[16] = { 0, 1, 7, 8, 1, 6, 0, 1, 0, 9, 1, 1, 1, 6, 0, 9 };
sint32 Cars::kAudioRoadSurfaceInterface[16] = { 0, 0, 3, 12, 0, 3, 0, 0, 11, 11, 0, 0, 3, 3, 3, 11 };
sint32 Cars::kSFXWallSurfaceInterface[8] = { 0, 4, 4, 6, 4, 8, 0, 6 };
sint32 Cars::kAudioWallSurfaceInterface[8] = { 4, 4, 4, 13, 4, 14, 13, 13 };
sint32 Cars::kAudioCollisoinTypeInterface[7] = { 1, 2, 2, 2, 1, 1, 1 };

static void QDUpdateVelForCar(Car::tObj *carObj)
{
    Newton::QDUpdateVel(&carObj->N);
}

static coorddef TireEffectPoint(const BO::tNewtonObj &body, const coorddef &position, sint32 wheel, sint32 offset)
{
    const bool front = wheel < 2;
    const sint32 lateral = (sint32)((uint32)(front ? body.wheelFrontX : body.wheelBackX) + (uint32)offset) / 256;
    const sint32 longitudinal = (front ? body.wheelFrontZ : body.wheelBackZ) / 256;
    coorddef point;
    for (sint32 axis = 0; axis < 3; ++axis)
    {
        const sint32 x = (sint32)((uint32)lateral * (uint32)body.orientMat.m[axis]) / 256;
        const sint32 z = (sint32)((uint32)longitudinal * (uint32)body.orientMat.m[axis + 6]) / 256;
        const sint32 origin = axis == 0 ? position.x : (axis == 1 ? position.y : position.z);
        const sint32 value = (sint32)((uint32)origin + ((wheel & 1) != 0 ? (uint32)x : 0U - (uint32)x) +
            (front ? (uint32)z : 0U - (uint32)z));
        if (axis == 0)
            point.x = value;
        else if (axis == 1)
            point.y = value;
        else
            point.z = value;
    }
    return point;
}

void Cars::DoGravityEffectsOnAcc(Car::tObj *carObj, sint32 arcade)
{
    FUNCTION_MARKER(0x80085D84, "NFS4.EXE");

    (void)arcade;
    if ((carObj->carFlags & 0x10) != 0)
        return;
    coorddef gravity_ch;
    if (fixedmult(0x10000, carObj->N.orientMat.m[4]) < 0xb5c2)
    {
        gravity_ch.x = fixedmult(-0xa0000, carObj->N.orientMat.m[1]);
        gravity_ch.y = fixedmult(-0xa0000, carObj->N.orientMat.m[4]);
        gravity_ch.z = fixedmult(-0xa0000, carObj->N.orientMat.m[7]);
        carObj->linearAcc_ch.x = (sint32)((uint32)carObj->linearAcc_ch.x + (uint32)gravity_ch.x);
        carObj->linearAcc_ch.y = (sint32)((uint32)carObj->linearAcc_ch.y + (uint32)gravity_ch.y);
        carObj->linearAcc_ch.z = (sint32)((uint32)carObj->linearAcc_ch.z + (uint32)gravity_ch.z);
        return;
    }
    if ((uint8)carObj->control.brakeLevel >= 64)
        return;
    gravity_ch.z = fixedmult(-0xa0000, carObj->N.orientMat.m[7]);
    gravity_ch.z = fixedmult(gravity_ch.z, carObj->N.gravityMult);
    sint32 magnitude = gravity_ch.z < 0 ? (sint32)(0U - (uint32)gravity_ch.z) : gravity_ch.z;
    if (magnitude < 0xccd)
        return;
    sint32 acceleration = carObj->linearAcc_ch.z;
    sint32 shift = ((gravity_ch.z > 0) == (acceleration > 0)) ? 1 : 3;
    carObj->linearAcc_ch.z = (sint32)((uint32)acceleration + (uint32)(gravity_ch.z >> shift));
}

void Cars::GetDashData(Car::tObj *carObj, sint32 *rpm, sint32 *gear, sint32 *speed)
{
    FUNCTION_MARKER(0x80085EC4, "NFS4.EXE");

    *rpm = carObj->flywheelRpm;
    *gear = (uint8)carObj->control.gear;
    *speed = carObj->linearVel_ch.z;
}

void Cars::QDUpdateVelGlue(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80085EE8, "NFS4.EXE");

    sint32 velocityX = carObj->N.linearVel.x / 256;
    sint32 glue = carObj->glue / 256;
    sint32 velocityZ = carObj->N.linearVel.z / 256;
    uint32 productX = (uint32)velocityX * (uint32)glue;
    carObj->N.position.y = (sint32)((uint32)carObj->N.position.y + (uint32)(carObj->N.linearVel.y >> 6));
    carObj->N.position.x = (sint32)((uint32)carObj->N.position.x + (uint32)((sint32)productX >> 6));
    uint32 productZ = (uint32)velocityZ * (uint32)glue;
    carObj->N.position.z = (sint32)((uint32)carObj->N.position.z + (uint32)((sint32)productZ >> 6));
}

void Cars::InitDashData(Car::tObj *carObj, sint32 *redline, sint32 *topspeed)
{
    FUNCTION_MARKER(0x80085F64, "NFS4.EXE");

    *redline = carObj->specs->redline;
    *topspeed = carObj->specs->maxSpeed;
}

void Cars::SetAudioCalls(Car::tObj *carObj, sint32 type, sint32 chan, sint32 s1, sint32 s2, sint32 force, sint32 pan)
{
    FUNCTION_MARKER(0x80085F8C, "NFS4.EXE");

    carObj->audio[carObj->audioCount].type = type;
    carObj->audio[carObj->audioCount].channel = chan;
    carObj->audio[carObj->audioCount].surface1 = s1;
    carObj->audio[carObj->audioCount].surface2 = s2;
    carObj->audio[carObj->audioCount].force = force;
    carObj->audio[carObj->audioCount].panangle = pan;
    carObj->audioCount = (sint32)((uint32)carObj->audioCount + 1);
}

void Cars::ResetCarCounters(void)
{
    FUNCTION_MARKER(0x80086054, "NFS4.EXE");

    gNumCars = 0;
    gNumRaceCars = 0;
    gNumAICars = 0;
    gNumHumanRaceCars = 0;
    gNumAIRaceCars = 0;
    gNumTrafficCars = 0;
    gNumCopCars = 0;
    gNumLifeBasisCars = 0;
}

void Cars::InitStats(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8008607C, "NFS4.EXE");

    Car::tStats *stats = &carObj->stats;
    stats->sliceTotal = 0;
    stats->sliceTime = 0;
    stats->slice = 0;
    stats->lastSlice = 0;
    stats->lap = 0;
    stats->lapTime = 0x200;
    for (sint32 lapLoop = 0; lapLoop < 4; ++lapLoop)
    {
        stats->time[lapLoop] = 0;
        stats->topSpeed[lapLoop] = 0;
    }
    stats->position = 0;
    stats->fatalCrashes = 0;
    stats->finishType = 0;
    stats->checkpointUpdate = 0;
    stats->checkpointDifference = 0;
    stats->checkpointDisplay = 0;
    stats->numWarnings = 0;
    stats->numFines = 0;
    stats->numArrests = 0;
}

void Cars::InitializeCarTablesFlagsAndCounters(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800860E4, "NFS4.EXE");
    sint32 carClass = carObj->carInfo->carClass;
    sint32 carIndex = gNumCars;
    gList[carIndex] = carObj;
    gSortedList[carIndex] = carObj;
    gTotalSortedList[carIndex] = carObj;
    carObj->carIndex = carIndex;
    carObj->carFlags = 0;
    carObj->humanIndex = -1;
    gNumCars = (sint32)((uint32)carIndex + 1);
    strcpy(carObj->carName, GameSetup::gCarNames[carObj->carInfo->carType]);
    sint32 carType = carObj->carInfo->carType;
    if (carType < 29)
        strcpy(carObj->carNameLocalized, TextSys::Word((sint32)((uint32)carType + 0x4E)));
    else
        sprintf(carObj->carNameLocalized, "");
    if ((carClass & 1) != 0) {
        if (PlayerNameExist(gNumHumanRaceCars)) {
            char *name = PlayerNameMixedCase(gNumHumanRaceCars);
            sprintf(carObj->carInfo->driver, name);
        } else if ((uint8)frontEnd.gameMode == 1) {
            char *format = TextSys::Word(0x2F);
            sprintf(carObj->carInfo->driver, format, (sint32)((uint32)gNumHumanRaceCars + 1));
        } else {
            char *name = TextSys::Word(0x2E);
            sprintf(carObj->carInfo->driver, name);
        }
        strcpy(carObj->carInfo->license, frontEnd.licensePlate[gNumHumanRaceCars]);
        sint32 humanIndex = gNumHumanRaceCars;
        carObj->humanIndex = humanIndex;
        gHumanRaceCarList[humanIndex] = carObj;
        gNumHumanRaceCars = (sint32)((uint32)humanIndex + 1);
        carObj->carFlags |= 4;
        sint32 raceIndex = gNumRaceCars;
        gRaceCarList[raceIndex] = carObj;
        gNumRaceCars = (sint32)((uint32)raceIndex + 1);
        carObj->carFlags |= 1;
        sint32 lifeIndex = gNumLifeBasisCars;
        gLifeBasisCarList[lifeIndex] = carObj;
        sint32 flags = carObj->carFlags;
        gNumLifeBasisCars = (sint32)((uint32)lifeIndex + 1);
        carObj->carFlags = flags | 0x100;
        if ((carClass & 0x40) != 0)
            carObj->carFlags = flags | 0x300;
    }
    if ((carClass & 2) != 0) {
        sint32 personality = GameSetup::gData.carInfo[carObj->carIndex].Personality;
        strcpy(carObj->carInfo->driver, GameSetup::gPersonalityNames[personality]);
        if ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) &&
            ((gHumanRaceCarList[0]->carFlags & 0x200) != 0 ||
                (gNumHumanRaceCars == 2 && (gHumanRaceCarList[1]->carFlags & 0x200) != 0))) {
            char *license = TextSys::Word(0x32);
            strcpy(carObj->carInfo->license, license);
        } else {
            strcpy(carObj->carInfo->license, GameSetup::gPersonalityNames[personality]);
        }
        s_lower(carObj->carInfo->license);
        sint32 aiIndex = gNumAICars;
        sint32 opponentIndex = gNumAIRaceCars;
        gAICarList[aiIndex] = carObj;
        gNumAICars = (sint32)((uint32)aiIndex + 1);
        sint32 raceIndex = gNumRaceCars;
        carObj->carFlags |= 2;
        gAIRaceCarList[opponentIndex] = carObj;
        gNumAIRaceCars = (sint32)((uint32)opponentIndex + 1);
        carObj->carFlags |= 8;
        gRaceCarList[raceIndex] = carObj;
        carObj->carFlags |= 1;
        gNumRaceCars = (sint32)((uint32)raceIndex + 1);
        if (GameSetup::gData.cops != 0 &&
            ((GameSetup::gData.raceType != 1 && GameSetup::gData.raceType != 5) ||
                ((gHumanRaceCarList[0]->carFlags & 0x200) == 0 &&
                    (gNumHumanRaceCars != 2 || (gHumanRaceCarList[1]->carFlags & 0x200) == 0)))) {
            sint32 lifeIndex = gNumLifeBasisCars;
            gLifeBasisCarList[lifeIndex] = carObj;
            gNumLifeBasisCars = (sint32)((uint32)lifeIndex + 1);
            carObj->carFlags |= 0x100;
        }
    }
    if ((carClass & 4) != 0) {
        sint32 aiIndex = gNumAICars;
        gAICarList[aiIndex] = carObj;
        gNumAICars = (sint32)((uint32)aiIndex + 1);
        carObj->carFlags |= 2;
        sint32 trafficIndex = gNumTrafficCars;
        gTrafficCarList[trafficIndex] = carObj;
        gNumTrafficCars = (sint32)((uint32)trafficIndex + 1);
        carObj->carFlags |= 0x10;
    }
    if ((carClass & 0x18) != 0) {
        sint32 aiIndex = gNumAICars;
        gAICarList[aiIndex] = carObj;
        gNumAICars = (sint32)((uint32)aiIndex + 1);
        carObj->carFlags |= 2;
        sint32 copIndex = gNumCopCars;
        gCopCarList[copIndex] = carObj;
        sint32 flags = carObj->carFlags;
        gNumCopCars = (sint32)((uint32)copIndex + 1);
        carObj->carFlags = flags | 0x20;
        if ((carClass & 0x10) != 0)
            carObj->carFlags = flags | 0x60;
    }
}

void Cars::ResetVariablesAfterACollision(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800865FC, "NFS4.EXE");

    carObj->N.angularVel.x = 0;
    carObj->N.angularVel.y = 0;
    carObj->N.angularVel.z = 0;
    carObj->angularVel_ch.x = 0;
    carObj->angularVel_ch.y = 0;
    carObj->angularVel_ch.z = 0;
    carObj->linearAcc_ch.x = 0;
    carObj->linearAcc_ch.y = 0;
    carObj->linearAcc_ch.z = 0;
    carObj->N.flightTime = 0;
    carObj->N.collision.collided = 0;
    carObj->N.collision.lastCollision = 0;
    carObj->N.collision.impulse = 0;
    carObj->N.collision.otherObj = 0;
    carObj->N.collision.disableCollisionTimer = 0;
    carObj->N.groundVel = 0;
    carObj->N.speedXZ = 0;
    carObj->collision.smoking = 0;
    carObj->N.roadGravityModifier = 0;
    Physics::ResetCar(carObj);
}

void Cars::ResetCollidedCars(Car::tObj *carObj, sint32 forceReset, sint32 forceParkAtSide)
{
    FUNCTION_MARKER(0x80086664, "NFS4.EXE");
    (void)forceParkAtSide;
    coorddef offset = { 0, 0, 0 };
    sint32 blowout = carObj->blowout;
    sint32 direction = carObj->desiredDirection;
    carObj->collision.smoking = 0;
    if (blowout != 0) {
        carObj->collision.smoking = 0;
        return;
    }
    if (forceReset == 0) {
        uint32 alignment = (uint32)(carObj->N.roadMatrix.m[3] / 256) *
            (uint32)(carObj->N.orientMat.m[3] / 256);
        alignment += (uint32)(carObj->N.roadMatrix.m[4] / 256) *
            (uint32)(carObj->N.orientMat.m[4] / 256);
        alignment += (uint32)(carObj->N.roadMatrix.m[5] / 256) *
            (uint32)(carObj->N.orientMat.m[5] / 256);
        sint32 y = (sint32)alignment;
        if (y > 0xC000 && carObj->N.angularVel.x < 65536 && carObj->N.angularVel.z < 65536 &&
            carObj->N.driveSurfaceType != 14 && carObj->N.driveSurfaceType != 0 &&
            carObj->N.simRoadInfo.simQuad != 0 && carObj->N.flightTime < 6 && carObj->N.objAltitude < 65536) {
            carObj->collision.smoking = 0;
            return;
        }
    }
    sint32 addToSlice = (sint32)((uint32)direction << 2);
    sint32 newSlice = (sint32)((uint32)carObj->N.simRoadInfo.slice + (uint32)addToSlice);
    if (addToSlice < 0) {
        if (newSlice < 0)
            newSlice = (sint32)((uint32)newSlice + (uint32)gNumSlices);
    } else if (newSlice >= gNumSlices) {
        newSlice = (sint32)((uint32)newSlice - (uint32)gNumSlices);
    }
    if (forceReset == 2)
        newSlice = carObj->N.simRoadInfo.slice;
    sint32 accident = accidentSlice;
    if (accident > 0) {
        sint32 end = (sint32)((uint32)accident + 5);
        sint32 numSlices = gNumSlices;
        if (end >= numSlices) {
            end = (sint32)((uint32)accident - ((uint32)numSlices - 5));
            if (newSlice < end)
                accident = accidentSlice;
        }
        if (newSlice < end) {
            sint32 start = (sint32)((uint32)accident - 5);
            if (start < 0)
                start = (sint32)((uint32)accident + (uint32)gNumSlices - 5);
            if (start < newSlice) {
                addToSlice = (sint32)((uint32)direction * 5);
                newSlice = (sint32)((uint32)newSlice + (uint32)addToSlice);
                if (addToSlice < 0) {
                    if (newSlice < 0)
                        newSlice = (sint32)((uint32)newSlice + (uint32)gNumSlices);
                } else if (newSlice >= gNumSlices) {
                    newSlice = (sint32)((uint32)newSlice - (uint32)gNumSlices);
                }
            }
        }
    }
    sint32 side = carObj->carIndex % 2 != 0;
    if (direction == -1)
        side = 1 - side;
    if (AITune::driveSide == -1)
        side = 1 - side;
    Trk::NewSlice *road = &BWorldSm::slices[newSlice];
    if (side != 0) {
        sint32 width = road->avgPavedWidthLf * 0x8000 * (road->laneCount >> 4);
        uint32 margin = (uint32)(carObj->N.dimension.x / 256) * 384;
        offset.x = (sint32)(margin - (uint32)width);
    } else {
        sint32 width = road->avgPavedWidthRt * 0x8000 * (road->laneCount & 15);
        uint32 margin = (uint32)(carObj->N.dimension.x / 256) * 384;
        offset.x = (sint32)((uint32)width - margin);
    }
    Newton::SetInitialSlicePositionOrientationEtc(&carObj->N, newSlice, &offset, direction);
    sint32 resetCounter = 0;
    while (carObj->N.driveSurfaceType == 0 || carObj->N.driveSurfaceType == 14 || carObj->N.simRoadInfo.simQuad == 0) {
        ++resetCounter;
        Newton::SetInitialSlicePositionOrientationEtc(&carObj->N, newSlice, &offset, direction);
        if (resetCounter >= 41) {
            offset.x = 0;
            Newton::SetInitialSlicePositionOrientationEtc(&carObj->N, newSlice, &offset, direction);
            break;
        }
    }
    carObj->N.linearVel.z = 0;
    carObj->N.linearVel.y = 0;
    carObj->N.linearVel.x = 0;
    ResetVariablesAfterACollision(carObj);
}

void Cars::SetCarUpForHiRezSim(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80086AC4, "NFS4.EXE");
    uint8 simOptz = carObj->N.simOptz;
    carObj->collision.smoking = 0;
    if (simOptz < 2) {
        BWorldSm::FindClosestQuadRez(&carObj->N.position, &carObj->N.simRoadInfo, 1);
        Trk::NewSimQuad *quad = carObj->N.simRoadInfo.simQuad;
        if (quad != 0 && (quad->surface & 15) == 0)
            ResetCollidedCars(carObj, 1, 0);
    }
}

void Cars::DoExtraCarCollisionProcessing(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80086B38, "NFS4.EXE");
    if ((carObj->carFlags & 0x400) != 0)
        return;
    sint32 blowout = carObj->blowout;
    if (blowout != 0) {
        sint32 speedZ = carObj->linearVel_ch.z;
        if (speedZ < 0)
            speedZ = (sint32)(0 - (uint32)speedZ);
        if (speedZ < 0x1999) {
            sint32 speedX = carObj->linearVel_ch.x;
            if (speedX < 0)
                speedX = (sint32)(0 - (uint32)speedX);
            if (speedX < 0x1999)
                carObj->blowout = (sint32)((uint32)blowout + 1);
        }
        if (carObj->blowout > 320) {
            carObj->blowout = 0;
            ResetCollidedCars(carObj, 1, 0);
        }
    }
    if (carObj->blowout == 0 && carObj->pullOver == 0 && (uint8)carObj->control.abort != 0 &&
        carObj->stats.finishType == 0 && simGlobal.gameTicks >= 833 && carObj->stats.fatalCrashes == 0) {
        if (carObj->N.speedXZ < 0x1B9999) {
            ResetCollidedCars(carObj, 2, 0);
            carObj->stats.fatalCrashes = 160;
            sint32 player = carObj == gHumanRaceCarList[1];
            Camera::gInfo[player].relpos.x = (sint32)(0 - ((uint32)carObj->N.orientMat.m[6] << 1));
            Camera::gInfo[player].relpos.y = (sint32)(0 - ((uint32)carObj->N.orientMat.m[7] << 1));
            Camera::gInfo[player].relpos.z = (sint32)(0 - ((uint32)carObj->N.orientMat.m[8] << 1));
        }
    }
    sint32 fatalCrashes = carObj->stats.fatalCrashes;
    if (fatalCrashes > 0)
        carObj->stats.fatalCrashes = fatalCrashes - 1;
    if (carObj->collision.smoking != 0) {
        TrgSfx::CrashCar(&carObj->N.position);
        if (carObj->N.flightTime == 0)
            carObj->collision.smoking = 0;
    }
    if (carObj->audioDamageScrape != 0 && carObj->N.simOptz == 0) {
        sint32 wheel = carObj->render.currentRoll < 0 ? 3 : 2;
        sint32 roadSurfaceWheel = carObj->wheel[wheel].roadSurfaceType & 15;
        if (kSkidMarkSurface[roadSurfaceWheel] == 1 && (random() & 3) == 0) {
            coorddef sideX;
            sideX.x = (sint32)((uint32)(carObj->N.wheelBackX / 256) * (uint32)carObj->N.orientMat.m[0]) / 256;
            sideX.y = (sint32)((uint32)(carObj->N.wheelBackX / 256) * (uint32)carObj->N.orientMat.m[1]) / 256;
            sideX.z = (sint32)((uint32)(carObj->N.wheelBackX / 256) * (uint32)carObj->N.orientMat.m[2]) / 256;
            coorddef position = carObj->N.position;
            position.y = carObj->N.groundElevation;
            if ((wheel & 1) == 0) {
                sideX.x = (sint32)(0 - (uint32)sideX.x);
                sideX.y = (sint32)(0 - (uint32)sideX.y);
                sideX.z = (sint32)(0 - (uint32)sideX.z);
            }
            coorddef point;
            point.x = (sint32)((uint32)position.x + (uint32)sideX.x);
            point.y = (sint32)((uint32)position.y + (uint32)sideX.y);
            point.z = (sint32)((uint32)position.z + (uint32)sideX.z);
            TrgSfx::AddCarSfx(carObj->N.objID, &point, 4, &carObj->N.linearVel);
        }
        carObj->audioDamageScrape = 0;
    }
    if (carObj->N.collision.impulse != 0) {
        bool debris = false;
        sint32 collisionType = (sint32)((uint32)carObj->N.collision.sfxType & 0xEFFF0000);
        if (carObj->carInfo->carType < 28) {
            if (((uint16)carObj->render.damageParts & 1) == 0 && carObj->N.damage[7] > 0x1E0000) {
                carObj->render.damageParts = (sint16)((uint16)carObj->render.damageParts | 1);
                debris = true;
            }
            if (((uint16)carObj->render.damageParts & 2) == 0 && carObj->N.damage[3] > 0x1E0000) {
                carObj->render.damageParts = (sint16)((uint16)carObj->render.damageParts | 2);
                debris = true;
            }
            if (((uint16)carObj->render.damageParts & 4) == 0 && carObj->N.damage[8] > 0x30000) {
                carObj->render.damageParts = (sint16)((uint16)carObj->render.damageParts | 4);
                debris = true;
            }
            if (((uint16)carObj->render.damageParts & 8) == 0 && carObj->N.damage[0] > 0x1E0000) {
                carObj->render.damageParts = (sint16)((uint16)carObj->render.damageParts | 8);
                debris = true;
            }
            if (((uint16)carObj->render.damageParts & 16) == 0 && carObj->N.damage[2] > 0x1E0000) {
                carObj->render.damageParts = (sint16)((uint16)carObj->render.damageParts | 16);
                debris = true;
            }
            if (((uint16)carObj->render.damageParts & 32) == 0 && carObj->N.damage[6] > 0x1E0000) {
                carObj->render.damageParts = (sint16)((uint16)carObj->render.damageParts | 32);
                debris = true;
            }
            if (((uint16)carObj->render.damageParts & 64) == 0 && carObj->N.damage[4] > 0x1E0000) {
                carObj->render.damageParts = (sint16)((uint16)carObj->render.damageParts | 64);
                debris = true;
            }
        }
        if (carObj->N.collision.otherObj == 0 && carObj->N.collision.impulse > 0x4B0000) {
            if ((carObj->carFlags & 4) != 0)
                Physics::FixEngineRpm(carObj);
            TrgSfx::CrashCar(&carObj->N.position);
            sint32 resetTimer = carObj->collision.resetTimer;
            carObj->collision.smoking = 1;
            carObj->frontSkid = 0;
            carObj->rearSkid = 0;
            if (resetTimer <= 0)
                carObj->collision.resetTimer = 320;
        }
        if (collisionType == 0x40000) {
            coorddef impactPoint = carObj->N.collision.collisionPoint;
            impactPoint.y = (sint32)((uint32)impactPoint.y + 0x8000);
            sint32 speed = carObj->N.speedXZ;
            if (speed > 0x180000) {
                if (carObj->N.objAltitude < 0x9999) {
                    sint32 surface = (uint8)carObj->N.collision.sfxType;
                    TrgSfx::AddCarSfx(carObj->N.objID, &impactPoint, kSFXWallSurfaceInterface[surface], &carObj->N.linearVel);
                    surface = (uint8)carObj->N.collision.sfxType;
                    if (kSFXWallSurfaceInterface[surface] == 4)
                        SetAudioCalls(carObj, 1, -1, 1, 15, 0, 0);
                } else {
                    TrgSfx::AddCarSfx(carObj->N.objID, &impactPoint, 6, &carObj->N.linearVel);
                }
            } else if (speed > 0xF0000 || carObj->N.collision.impulse > 0xF0000 || debris) {
                TrgSfx::AddCarSfx(carObj->N.objID, &impactPoint, 1, &carObj->N.linearVel);
            }
        }
        if (carObj->N.collision.impulse > 0xA0000) {
            sint32 surface1 = kAudioCollisoinTypeInterface[collisionType >> 16];
            sint32 surface2;
            if (collisionType == 0x40000) {
                surface2 = kAudioWallSurfaceInterface[(uint8)carObj->N.collision.sfxType];
            } else if (collisionType <= 0x30000) {
                surface2 = kAudioRoadSurfaceInterface[(uint8)carObj->N.collision.sfxType];
                if (collisionType == 0x30000) {
                    if (carObj->N.orientationToGround.y < 0xE666)
                        surface1 = 1;
                    TrgSfx::CrashCar(&carObj->N.collision.collisionPoint);
                }
            } else {
                surface2 = (uint8)carObj->N.collision.sfxType;
                if (collisionType == 0x50000) {
                    coorddef impactPoint = carObj->N.collision.collisionPoint;
                    impactPoint.y = (sint32)((uint32)impactPoint.y + 0x8000);
                    TrgSfx::AddCarSfx(carObj->N.objID, &impactPoint, 4, &carObj->N.linearVel);
                }
            }
            sint32 impulse = carObj->N.collision.impulse;
            SetAudioCalls(carObj, 1, -1, surface1, surface2, impulse / 8, 0);
        }
        carObj->N.collision.impulse = 0;
        carObj->N.collision.otherObj = 0;
        carObj->N.collision.sfxType = 0;
    }
    sint32 resetTimer = carObj->collision.resetTimer;
    if (resetTimer > 0 && carObj->pullOver == 0) {
        carObj->collision.resetTimer = resetTimer - 1;
        sint32 verticalSpeed = carObj->N.linearVel.y;
        if (verticalSpeed < 0)
            verticalSpeed = (sint32)(0 - (uint32)verticalSpeed);
        sint32 totalSpeed = (sint32)((uint32)carObj->N.speedXZ + (uint32)verticalSpeed);
        if (totalSpeed < 0x10000 || carObj->N.driveSurfaceType == 0 || carObj->N.driveSurfaceType == 14)
            carObj->collision.resetTimer = (sint32)((uint32)carObj->collision.resetTimer - 8);
        if (carObj->collision.resetTimer <= 0) {
            ResetCollidedCars(carObj, 0, 0);
            carObj->collision.resetTimer = 0;
        }
    }
    uint32 alignment = (uint32)(carObj->N.roadMatrix.m[3] / 256) * (uint32)(carObj->N.orientMat.m[3] / 256);
    alignment += (uint32)(carObj->N.roadMatrix.m[4] / 256) * (uint32)(carObj->N.orientMat.m[4] / 256);
    alignment += (uint32)(carObj->N.roadMatrix.m[5] / 256) * (uint32)(carObj->N.orientMat.m[5] / 256);
    sint32 y = (sint32)alignment;
    if (carObj->collision.resetTimer <= 0 && (y < 0x1999 || carObj->N.driveSurfaceType == 0 || carObj->N.driveSurfaceType == 14))
        carObj->collision.resetTimer = 320;
}

void Cars::AddCarSfx(Car::tObj *carObj, sint32 wheel, coorddef *skidpt, sint32 roadSurface, sint32 surfaceType, sint32 drawSkid)
{
    FUNCTION_MARKER(0x8008748C, "NFS4.EXE");
    uint32 road = (uint32)roadSurface & 15;
    sint32 smoking = kSmokingSurface[road];
    uint32 stateBit = (uint32)1 << ((uint32)wheel & 31);
    sint32 objID = carObj->N.objID;
    sint32 sfxType = kConvertFromRoadToSfxType[road];
    if (surfaceType == 3)
        smoking = 0;
    sint16 currentCarType = carObj->render.currentCarType;
    bool traffic = currentCarType > 28;
    sint32 sfxDelay = gTAddCarWheelDelay;
    if (traffic)
        sfxDelay = (sint32)((uint32)sfxDelay << 1);
    if (currentCarType == 28)
        return;
    bool front = wheel < 2;
    if (drawSkid == 0 || surfaceType == 0 || traffic) {
        if ((carObj->oldSkidState & stateBit) != 0) {
            sint32 skid = front ? carObj->frontSkid : carObj->rearSkid;
            TrgSfx::AddSkidmark(objID, wheel, &carObj->oldSkidPoint[wheel], 1, skid, carObj, 0);
            carObj->oldSkidState = (sint32)((uint32)carObj->oldSkidState - stateBit);
        }
    } else if (skidpt->x != carObj->oldSkidPoint[wheel].x || skidpt->z != carObj->oldSkidPoint[wheel].z) {
        sint32 skid = front ? carObj->frontSkid : carObj->rearSkid;
        if (wheel == (front ? 0 : 2))
            skid = (sint32)((uint32)skid - (uint32)carObj->gTransferRight);
        else
            skid = (sint32)((uint32)skid + (uint32)carObj->gTransferRight);
        skid = (sint32)((uint32)skid - (front ? 0x18000 : 0x10000));
        if (skid < 0)
            skid = 0;
        else if (skid > 0x60000)
            skid = 0x60000;
        sint32 type = (sint32)((uint32)surfaceType - 1);
        TrgSfx::AddSkidmark(objID, wheel, skidpt, 0, skid, carObj, type);
        coorddef point = *skidpt;
        carObj->oldSkidPoint[wheel] = point;
        carObj->oldSkidState = (sint32)((uint32)carObj->oldSkidState | stateBit);
    }
    if (sfxType == 8) {
        TrgSfx::AddCarSplash(objID, wheel, skidpt, 8, &carObj->N.linearVel, sfxDelay, carObj->N.speedXZ);
        return;
    }
    if (!(smoking < carObj->N.speedXZ && smoking > 0)) {
        sint32 skid = front ? carObj->frontSkid : carObj->rearSkid;
        if (skid <= 0x40000)
            return;
    }
    TrgSfx::AddCarWheelSfx(objID, wheel, skidpt, sfxType, &carObj->N.linearVel, sfxDelay);
}

void Car::TireSkiddingStuff(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800878CC, "NFS4.EXE");
    BO::tNewtonObj &body = carObj->N;
    const sint32 speed = (sint16)((uint32)body.speedXZ >> 16);
    const bool wetRoad = GameSetup::gData.Weather != 0 && !BWorldSm::TunnelFlagSm(&body.simRoadInfo);
    const sint32 roadSurface = body.driveSurfaceType;
    const bool visible = body.distToPlayer < 0x3C0000 && body.objAltitude < 0x6666;
    const coorddef position = { body.position.x, body.groundElevation, body.position.z };
    sint32 originalFront = 0;
    sint32 originalRear = 0;
    sint32 skidFront = 0;
    if (visible)
    {
        if (carObj->frontSkid > 0)
        {
            originalFront = carObj->frontSkid > 0xA0000 ? 0xA0000 : carObj->frontSkid;
            sint32 velocity = carObj->linearVel_ch.z;
            sint32 magnitude = velocity < 0 ? (sint32)(0U - (uint32)velocity) : velocity;
            carObj->frontSkid = originalFront;
            skidFront = originalFront;
            if (magnitude > 0x140000)
            {
                sint32 correction = velocity / 8;
                if (correction < 0)
                    correction = -correction;
                skidFront = originalFront - correction;
            }
        }
        if (carObj->rearSkid > 0)
        {
            originalRear = carObj->rearSkid > 0xA0000 ? 0xA0000 : carObj->rearSkid;
            carObj->rearSkid = originalRear;
        }
    }

    sint32 audioSurface = Cars::kAudioRoadSurfaceInterface[roadSurface];
    if (audioSurface == 0)
    {
        if (GameSetup::gData.Weather < 1)
        {
            sint32 tireType = carObj->carInfo->TireType;
            if (tireType == 0)
                audioSurface = 0x12;
            else if (tireType == 2)
                audioSurface = 0x11;
        }
        else if (!BWorldSm::TunnelFlagSm(&body.simRoadInfo))
            audioSurface = 0x10;
    }

    sint32 front = originalFront;
    sint32 rear = originalRear;
    for (sint32 wheel = 0; wheel < 4; ++wheel)
    {
        const bool isFront = wheel < 2;
        sint32 &skid = isFront ? front : rear;
        const sint32 roadSurfaceWheel = carObj->wheel[wheel].roadSurfaceType;
        sint32 surfaceType = carObj->wheel[wheel].wheelInAir == 0 ?
            Cars::kSkidMarkSurface[roadSurfaceWheel & 15] : 0;
        if (!visible || ((skid == 0 || wetRoad) && (surfaceType < 2 || speed == 0)))
        {
            const sint32 bit = 1 << wheel;
            if ((carObj->oldSkidState & bit) != 0)
            {
                carObj->oldSkidState -= bit;
                TrgSfx::AddSkidmark(body.objID, wheel, &carObj->oldSkidPoint[wheel], 1,
                    isFront ? carObj->frontSkid : carObj->rearSkid, carObj, 0);
            }
        }
        else
        {
            coorddef point = TireEffectPoint(body, position, wheel, 0);
            if (surfaceType > 1 && (isFront ? frontLimit : rearLimit) < (isFront ? skidFront : skid))
                surfaceType = 3;
            Cars::AddCarSfx(carObj, wheel, &point, roadSurfaceWheel, surfaceType, surfaceType != 0);
        }

        if (wetRoad && surfaceType == 1 && speed != 0 && visible)
        {
            coorddef point = TireEffectPoint(body, position, wheel, 0);
            TrgSfx::AddCarSplash(body.objID, wheel, &point, 10, &body.linearVel, 0, body.speedXZ);
            if (skid < 1)
                skid = 1;
        }
        if ((roadSurfaceWheel & 0x20) != 0 && gLeafPixmap != 0 && body.speedXZ > 0xA0000 &&
            TrgSfx::AddCarExtraCheck(body.objID, wheel))
        {
            const sint32 rndOffset = ((sint32)(random() & 7) - 4) * 0x3333;
            coorddef point = TireEffectPoint(body, position, wheel, rndOffset);
            TrgSfx::AddCarExtraSfx(body.objID, wheel, &point, 13, &body.linearVel, body.speedXZ,
                point.y, carObj->render.light);
        }
    }

    if (front == 0)
    {
        if ((carObj->oldAudioSkidState & 1) != 0)
        {
            Cars::SetAudioCalls(carObj, 3, 0x12, 2, audioSurface, 0, 0);
            carObj->oldAudioSkidState -= 1;
        }
    }
    else if (originalFront > 0 || (carObj->carFlags & 4) != 0)
    {
        Cars::SetAudioCalls(carObj, 2, 0x12, 2, audioSurface, front, 0);
        carObj->oldAudioSkidState |= 1;
    }
    if (rear == 0)
    {
        if ((carObj->oldAudioSkidState & 2) != 0)
        {
            Cars::SetAudioCalls(carObj, 5, 0x14, 2, audioSurface, 0, 0);
            carObj->oldAudioSkidState -= 2;
        }
    }
    else if (originalRear > 0 || (carObj->carFlags & 4) != 0)
    {
        Cars::SetAudioCalls(carObj, 4, 0x14, 2, audioSurface, rear / 2, 0);
        carObj->oldAudioSkidState |= 2;
    }
}

void Cars::FindTotalSlice(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80089760, "NFS4.EXE");
    if (carObj->unlap > 0) {
        carObj->N.totalSlice = 0;
        return;
    }
    sint32 numSlices = gNumSlices;
    uint32 lapSlices = (uint32)carObj->lap * (uint32)numSlices;
    uint32 roadSlice = (uint16)carObj->N.simRoadInfo.slice;
    if (GameSetup::gData.reverseTrack != 0)
        roadSlice = (uint32)numSlices - roadSlice - 1;
    carObj->N.totalSlice = (uint16)(lapSlices + roadSlice);
}

void Car::DoSkiddingStuff(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800897CC, "NFS4.EXE");
    if (carObj->N.orientationToGround.y < 0x3334) {
        sint32 audioSurface = Cars::kAudioRoadSurfaceInterface[carObj->N.driveSurfaceType];
        if (carObj->N.objAltitude < 0x3333 && carObj->N.speedXZ > 0x20000) {
            Cars::SetAudioCalls(carObj, 4, 20, 1, audioSurface, 0xA0000, 0);
            carObj->oldAudioSkidState |= 4;
        } else {
            Cars::SetAudioCalls(carObj, 5, 20, 1, 0, 0, 0);
            carObj->oldAudioSkidState = (sint32)((uint32)carObj->oldAudioSkidState - 4);
        }
    } else {
        if ((carObj->oldAudioSkidState & 4) != 0) {
            Cars::SetAudioCalls(carObj, 5, 20, 1, 0, 0, 0);
            carObj->oldAudioSkidState = (sint32)((uint32)carObj->oldAudioSkidState - 4);
        }
        TireSkiddingStuff(carObj);
    }
}

void Car::DoPostCollisionStuff(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800898DC, "NFS4.EXE");
    sint32 impulse = carObj->N.collision.impulse;
    carObj->audioCount = 0;
    if (impulse != 0) {
        carObj->N.collision.lastImpulse = impulse;
        sint32 lastTime = simGlobal.gameTicks;
        BO::tNewtonObj *other = carObj->N.collision.otherObj;
        carObj->N.collision.lastTime = lastTime;
        carObj->N.collision.lastOtherObj = other;
    }
    if (carObj->N.simOptz != 0 || carObj->N.active == 0) {
        Car::tSpecs *specs = carObj->specs;
        carObj->render.bodyPitch = 0;
        carObj->render.bodyRoll = 0;
        carObj->render.currentHeight = (sint32)(0 - (uint32)specs->rideOffset);
        Cars::DoExtraCarCollisionProcessing(carObj);
        return;
    }
    AIPhysic::ProcessCollision(carObj);
    Cars::DoExtraCarCollisionProcessing(carObj);
    DoSkiddingStuff(carObj);
    bool groundedHuman = (carObj->carFlags & 4) != 0 && carObj->N.flightTime == 0;
    sint32 factor = groundedHuman ? 7 : 15;
    sint32 shift = groundedHuman ? 3 : 4;
    sint32 pitch = (sint32)((uint32)carObj->render.currentPitch * (uint32)factor +
        (uint32)carObj->linearAcc_ch.z);
    sint32 roll = (sint32)((uint32)carObj->render.currentRoll * (uint32)factor +
        (uint32)carObj->linearAcc_ch.x);
    carObj->render.currentPitch = pitch >> shift;
    carObj->render.currentRoll = roll >> shift;
    sint32 negativeGroundVelocity = (sint32)(0 - (uint32)carObj->N.groundVel);
    sint32 Yoffset = negativeGroundVelocity / 32;
    if (Yoffset < -0xA3D)
        Yoffset = -0xA3D;
    else if (Yoffset > 0x51E)
        Yoffset = 0x51E;
    sint32 rollArgument = (sint32)((uint32)carObj->render.currentRoll * 3);
    sint32 rollHeight = fixedmult(rollArgument / 2, carObj->specs->bodyRollFactor);
    if (rollHeight < 0)
        rollHeight = (sint32)(0 - (uint32)rollHeight);
    sint32 rollFactor = carObj->render.rollFactor;
    Car::tSpecs *specs = carObj->specs;
    sint32 currentRoll = carObj->render.currentRoll;
    sint32 rideOffset = specs->rideOffset;
    carObj->render.bodyRoll = currentRoll;
    carObj->render.currentHeight = (sint32)((uint32)Yoffset - (uint32)rollHeight - (uint32)rideOffset);
    sint32 bodyRoll = fixedmult(currentRoll, rollFactor);
    sint32 pitchFactor = carObj->render.pitchFactor;
    sint32 currentPitch = carObj->render.currentPitch;
    carObj->render.bodyRoll = bodyRoll;
    sint32 bodyPitch = (sint32)(0 - (uint32)currentPitch);
    carObj->render.bodyPitch = bodyPitch;
    bodyPitch = fixedmult(bodyPitch, pitchFactor);
    GameSetup::tCarData *carInfo = carObj->carInfo;
    carObj->render.bodyPitch = bodyPitch;
    if (carInfo->WeightTransfer == 1 || (carObj->carFlags & 4) == 0) {
        sint32 currentBodyRoll = carObj->render.bodyRoll;
        carObj->render.bodyPitch = bodyPitch / 2;
        carObj->render.bodyRoll = currentBodyRoll / 2;
    }
}

void Cars::CalculateStartingGridOffset(Car::tObj *carObj, sint32 *slice, coorddef *offset)
{
    FUNCTION_MARKER(0x80089B44, "NFS4.EXE");
    sint32 negDir = GameSetup::gData.reverseTrack != 0 ? 1 : -1;
    sint32 startingPosition = GameSetup::gData.carInfo[carObj->carIndex].StartingPos;
    sint32 initialSlice = negDir;
    if (gNumRaceCars >= 3)
        initialSlice = (sint32)((uint32)negDir + (uint32)(negDir * 10) * (uint32)startingPosition);
    if (initialSlice < 0)
        *slice = (sint32)((uint32)initialSlice + (uint32)gNumSlices);
    else if (initialSlice >= gNumSlices)
        *slice = (sint32)((uint32)initialSlice - (uint32)gNumSlices);
    else
        *slice = initialSlice;

    sint32 carOnRight = startingPosition & 1;
    if ((uint32)carObj->desiredDirection * (uint32)AITune::driveSide == 0xFFFFFFFFU)
        carOnRight = 1 - carOnRight;
    if (AITune::GetOneWay() != 0)
    {
        Trk::NewSlice *road = &BWorldSm::slices[*slice];
        sint32 leftWidth = road->avgPavedWidthLf * 32768;
        uint32 lanes = road->laneCount;
        sint32 leftLanes = lanes >> 4;
        leftWidth *= leftLanes;
        sint32 rightWidth = road->avgPavedWidthRt * 32768;
        sint32 rightLanes = lanes & 15;
        sint32 laneCount = leftLanes + rightLanes;
        sint32 totalWidth = leftWidth + rightWidth * rightLanes;
        sint32 center = totalWidth / 2 - leftWidth;
        if (laneCount == 0)
            abort();
        sint32 laneWidth = totalWidth / laneCount;
        offset->x = carOnRight != 0 ? center + laneWidth : center - laneWidth;
    }
    else if (carOnRight != 0)
        offset->x = BWorldSm::slices[*slice].avgPavedWidthRt * 16384;
    else
        offset->x = -(BWorldSm::slices[*slice].avgPavedWidthLf * 32768) / 2;
    offset->y = 0x8000;
    offset->z = 0;
}

void Cars::IniCarObjects(Car::tObj *carObj, sint32 index)
{
    FUNCTION_MARKER(0x80089D88, "NFS4.EXE");
    InitStats(carObj);
    carObj->swapCar = 0;
    carObj->swapTime = 0;
    if (index < GameSetup::gData.numCars)
    {
        sint32 type = carObj->carInfo->carType;
        sint32 carMass;
        if (type == 0x21 || type == 0x24 || type == 0x26 || type == 0x27 ||
            type == 0x2F || type == 0x30 || type == 0x31)
            carMass = 0x190000;
        else if (type == 0x22 || type == 0x25 || type == 0x29 || type == 0x2B || type == 0x2C)
            carMass = 0x110000;
        else
            carMass = carObj->specs->mass / 128;
        Newton::InitBaseNewtonObj(&carObj->N, index | 0x100, carMass, carMass,
            carObj->N.dimension.x, carObj->N.dimension.y, carObj->N.dimension.z);
    }
    sint32 startSlice;
    coorddef offset;
    CalculateStartingGridOffset(carObj, &startSlice, &offset);
    sint32 numSlices = gNumSlices;
    carObj->stats.extractSlice = numSlices / 2 < startSlice ?
        (sint32)((uint32)numSlices - (uint32)startSlice) : startSlice;
    Newton::SetInitialSlicePositionOrientationEtc(&carObj->N, startSlice, &offset,
        GameSetup::gData.reverseTrack != 0 ? -1 : 1);
    carObj->unlap = 1;
    carObj->lap = 0;
    if (GameSetup::gData.reverseTrack != 0)
        carObj->N.oldSlice = (uint16)((uint32)(uint16)gNumSlices -
            (uint32)(uint16)carObj->N.simRoadInfo.slice - 1U);
    else
        carObj->N.oldSlice = (uint16)carObj->N.simRoadInfo.slice;
    FindTotalSlice(carObj);
    carObj->RSControl = 0;
    carObj->RSSteering = 0;
    carObj->RSGasLevel = 0;
    carObj->RSBrakeLevel = 0;
    carObj->glue = 0x10000;
    carObj->aiGlue = 0x10000;
    carObj->drag = 0;
    carObj->crash = 0;
    carObj->blowout = 0;
    carObj->pullOver = 0;
    carObj->linearAcc.x = 0;
    carObj->linearAcc.y = 0;
    carObj->linearAcc.z = 0;
    carObj->angularAcc.x = 0;
    carObj->angularAcc.y = 0;
    carObj->angularAcc.z = 0;
    carObj->linearAcc_ch.x = 0;
    carObj->linearAcc_ch.y = 0;
    carObj->linearAcc_ch.z = 0;
    carObj->linearAcc_rh.x = 0;
    carObj->linearAcc_rh.y = 0;
    carObj->linearAcc_rh.z = 0;
    carObj->linearVel_ch.x = 0;
    carObj->linearVel_ch.y = 0;
    carObj->linearVel_ch.z = 0;
    carObj->angularVel_ch.x = 0;
    carObj->angularVel_ch.y = 0;
    carObj->angularVel_ch.z = 0;
    for (sint32 k = 0; k < 4; ++k)
    {
        Car::tWheel &wheel = carObj->wheel[k];
        wheel.actualHeight = 0;
        wheel.currentPos.x = 0;
        wheel.currentPos.y = 0;
        wheel.currentPos.z = 0;
        wheel.roadNormal.x = 0;
        wheel.roadNormal.y = 0x10000;
        wheel.roadNormal.z = 0;
        wheel.wheelAcc = 0;
        wheel.wheelInAir = 0;
        wheel.rebound = 0;
        wheel.impactCompression = 0;
        wheel.roadSurfaceType = 1;
    }
    carObj->control.desiredGear = 1;
    carObj->control.gear = 1;
    carObj->control.lastGear = 1;
    carObj->flywheelRpm = 0;
    carObj->wheelSpin = 0;
    carObj->frontWheelSpin = 0;
    carObj->wheelLock = 0;
    carObj->slide = 0;
    carObj->frontSkid = 0;
    carObj->rearSkid = 0;
    carObj->control.desiredGasLevel = 0;
    carObj->control.desiredBrakeLevel = 0;
    carObj->control.desiredSteering = 0;
    carObj->control.downShifting = 0;
    carObj->control.gearShiftTimer = 0;
    carObj->control.gasLevel = 0;
    carObj->control.brakeLevel = 0;
    carObj->control.steering = 0;
    carObj->control.handBrake = 0;
    carObj->control.horn = 0;
    carObj->control.lights = GameSetup::gData.Time != 0 ? 3 : 0;
    carObj->control.abort = 0;
    carObj->control.lookBehind = 0;
    carObj->control.hanno = 0;
    carObj->control.event = 0;
    carObj->control.queuedEvent = 0;
    carObj->gTransferFront = 0;
    carObj->oldSkidState = 0;
    carObj->oldAudioSkidState = 0;
    carObj->frontSkid = 0;
    carObj->rearSkid = 0;
    AIInit::RestartAICar(carObj);
    GameSetup::tCarData *carInfo = carObj->carInfo;
    carObj->audioCount = 0;
    carObj->audioDamageScrape = 0;
    carObj->collision.resetTimer = 0;
    carObj->collision.smoking = 0;
    carObj->N.flightTime = 0;
    carObj->N.collision.collided = 0;
    carObj->N.collision.lastCollision = 0;
    carObj->N.collision.impulse = 0;
    carObj->N.collision.otherObj = 0;
    carObj->N.collision.disableCollisionTimer = 0;
    carObj->N.groundVel = 0;
    carObj->N.roadGravityModifier = 0;
    carObj->N.speedXZ = 0;
    carObj->render.bodyPitch = 0;
    carObj->render.bodyRoll = 0;
    carObj->render.currentPitch = 0;
    carObj->render.currentRoll = 0;
    carObj->render.currentHeight = 0;
    if (carInfo->carType < 29)
    {
        Car::tSpecs *specs = carObj->specs;
        sint32 pitch = fixeddiv(specs->bodyPitchFactor, (sint32)((uint32)specs->suspensionStiffness * 3U));
        specs = carObj->specs;
        carObj->render.pitchFactor = pitch;
        carObj->render.rollFactor = fixeddiv(specs->bodyRollFactor, (sint32)((uint32)specs->suspensionStiffness << 1));
    }
    else
    {
        carObj->render.pitchFactor = 0;
        carObj->render.rollFactor = 0;
    }
}

void Cars::InitCar(Car::tObj *carObj, sint32 index)
{
    FUNCTION_MARKER(0x8008A174, "NFS4.EXE");
    char carFile[100];
    char specsFile[100];
    char name[20];
    char *file2 = 0;
    char *file1 = 0;
    Udff::tInfo *handle = 0;
    Udff::tInfo *handle2 = 0;
    if (index < GameSetup::gData.numCars) {
        if (AIInit::IsNonStandardCarFile(carObj->carInfo->carType))
            sprintf(carFile, "%s%s.qda", Paths_Paths[4], carObj->carName);
        else
            sprintf(carFile, "%sSTDR.qda", Paths_Paths[4]);
        file1 = static_cast<char *>(loadpackadrz(carFile, 0x10));
        handle2 = Udff::Opena(0, file1, 1);
        sint32 carType = carObj->carInfo->carType;
        if (carType < 29)
            sprintf(name, "p%s.dat", GameSetup::gCarNames[carType]);
        else
            sprintf(name, "ptram.dat");
        sprintf(specsFile, "%sdusty.viv", Paths_Paths[3]);
        file2 = static_cast<char *>(loadfileadrz(specsFile, 0x10));
        char *member = locatebig(file2, name);
        handle = Udff::Opena(0, member, 0);
        if (index < GameSetup::gData.numCars) {
            AIInit::InitAICar(carObj, handle2);
            AIInit::RestartAICar(carObj);
            carObj->specs = static_cast<Car::tSpecs *>(reservememadr("carSpecs", sizeof(Car::tSpecs), 0));
            Physics::InitCarSpecs(carObj, handle);
            if ((carObj->carFlags & 4) != 0)
                Physics::CalculateDerivedCarSpecs(carObj);
        }
    }
    carObj->funcUpdateRoadInfo = Newton::UpdateRoadInfo;
    carObj->funcReplay = Replay::DoReplay;
    carObj->funcControl = Control::Human;
    carObj->funcStats = Stats::TrackStats;
    carObj->funcHandlingPhysics = Physics::SimCar;
    carObj->funcGravityPhysics = Newton::ApplyTheLawOfGravity;
    carObj->funcQDPhysicsUpdateVel = QDUpdateVelForCar;
    carObj->funcQDPhysicsUpdateRot = Newton::QDUpdateRot64Hz;
    carObj->funcTestMeForCollisions = Collide::CheckMeForCollisions;
    carObj->funcDoPostCollisionStuff = Car::DoPostCollisionStuff;
    if ((carObj->carFlags & 2) != 0) {
        carObj->funcControl = Control::AI;
        carObj->funcHandlingPhysics = AIPhysic::Main;
        carObj->funcReplay = 0;
        carObj->funcQDPhysicsUpdateRot = Newton::QDUpdateRot32Hz;
    }
    if ((carObj->carFlags & 4) != 0 && GameSetup::gData.catchupLogic != 0 && GameSetup::gData.commMode > 0)
        carObj->funcQDPhysicsUpdateVel = QDUpdateVelGlue;
    R3DCar::Instantiate3DCar(carObj, index);
    if (index < GameSetup::gData.numCars) {
        Udff::Close(handle2);
        Udff::Close(handle);
        purgememadr(file2);
        purgememadr(file1);
    }
}

void Cars::DeInitCar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8008A484, "NFS4.EXE");
    AIInit::DeInitAICar(carObj);
    Car::tSpecs *specs = carObj->specs;
    if (specs != 0) {
        purgememadr(specs);
        carObj->specs = 0;
    }
    R3DCar::DeInstantiate3DCar(carObj);
}

void Cars::Restart(void)
{
    FUNCTION_MARKER(0x8008A4CC, "NFS4.EXE");
    sint32 count = gNumCars;
    sint32 i;
    for (i = 0; i < count; ++i)
    {
        gSortedList[i] = gList[i];
        gTotalSortedList[i] = gList[i];
    }
    for (i = 0; i < gNumCars; ++i)
        IniCarObjects(gList[i], i);
    for (i = 0; i < Object::GetNumIMassObjects(); ++i)
    {
        Object::tIMassObjInfo *info = &Object::IMassObjInst[i];
        info->lastPos.x = 0;
        info->lastPos.y = 0;
        info->lastPos.z = 0;
        info->lastTick = 0;
    }
    accidentSlice = 0;
}

void Cars::Initialize(char *mem, sint32 size)
{
    FUNCTION_MARKER(0x8008A5B4, "NFS4.EXE");
    for (sint32 i = 0; i < size; ++i)
        *mem++ = 0;
}

void Cars::StartUp(void)
{
    FUNCTION_MARKER(0x8008A5D8, "NFS4.EXE");
    ResetCarCounters();
    for (sint32 i = 0; i < GameSetup::gData.numCars; ++i)
    {
        Car::tObj *newCar = static_cast<Car::tObj *>(reservememadr("Car_tObj", 0x8DC, 0));
        Initialize(reinterpret_cast<char *>(newCar), 0x8DC);
        if (i < GameSetup::gData.numCars)
            newCar->carInfo = &GameSetup::gData.carInfo[i];
        InitializeCarTablesFlagsAndCounters(newCar);
    }
    for (sint32 i = 0; i < Object::GetNumIMassObjects(); ++i)
    {
        coorddef dim;
        Object::GetIMassObjectDimensions(i, &dim);
        Newton::InitBaseNewtonObj(&InfiniteMassNewton[i], 0x201, 0x280000, 0x1400000,
            dim.x, dim.y, (sint32)((uint32)dim.z + 0x10000U));
    }
    for (sint32 i = 0; i < GameSetup::gData.numCars; ++i)
    {
        Car::tObj *carObj = gList[i];
        InitCar(carObj, i);
        Sched::AddFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(carObj->funcUpdateRoadInfo), carObj, 6);
    }
    if (R3DCar::LicenseShapeFile)
        purgememadr(R3DCar::LicenseShapeFile);
    R3DCar::LicenseShapeFile = 0;
    for (sint32 i = 0; i < GameSetup::gData.numCars; ++i)
    {
        Car::tObj *carObj = gList[i];
        Sched::AddFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(carObj->funcControl), carObj, 21);
    }
    for (sint32 i = 0; i < GameSetup::gData.numCars; ++i)
    {
        Car::tObj *carObj = gList[i];
        Sched::AddFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(carObj->funcHandlingPhysics), carObj, 30);
    }
    for (sint32 i = 0; i < GameSetup::gData.numCars; ++i)
    {
        Car::tObj *carObj = gList[i];
        Sched::AddFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(carObj->funcGravityPhysics), carObj, 30);
    }
    for (sint32 i = 0; i < GameSetup::gData.numCars; ++i)
    {
        Car::tObj *carObj = gList[i];
        Sched::AddFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(carObj->funcTestMeForCollisions), carObj, 40);
    }
    for (sint32 i = 0; i < GameSetup::gData.numCars; ++i)
    {
        Car::tObj *carObj = gList[i];
        Sched::AddFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(carObj->funcDoPostCollisionStuff), carObj, 50);
    }
    Force::StartUp();
    for (sint32 i = 0; i < GameSetup::gData.numCars; ++i)
    {
        Car::tObj *carObj = gList[i];
        if ((carObj->carFlags & 1) != 0)
            Sched::AddFunction(simGlobal.schedule64Hz,
                reinterpret_cast<Sched::tFunctionPt>(carObj->funcStats), carObj, 25);
    }
    for (sint32 i = 0; i < GameSetup::gData.numCars; ++i)
    {
        Car::tObj *carObj = gList[i];
        Sched::AddFunction(simGlobal.schedule64Hz,
            reinterpret_cast<Sched::tFunctionPt>(carObj->funcQDPhysicsUpdateVel), carObj, 30);
    }
    for (sint32 i = 0; i < GameSetup::gData.numCars; ++i)
    {
        Car::tObj *carObj = gList[i];
        Sched::tSchedule *schedule = (carObj->carFlags & 4) != 0 ?
            simGlobal.schedule64Hz : simGlobal.schedule32Hz2;
        Sched::AddFunction(schedule,
            reinterpret_cast<Sched::tFunctionPt>(carObj->funcQDPhysicsUpdateRot), carObj, 30);
    }
}

void Cars::CleanUp(void)
{
    FUNCTION_MARKER(0x8008AA40, "NFS4.EXE");
    for (sint32 i = 0; i < gNumCars; ++i)
    {
        Sched::DeleteFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(gList[i]->funcUpdateRoadInfo), gList[i]);
        Sched::DeleteFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(gList[i]->funcControl), gList[i]);
        if ((gList[i]->carFlags & 1) != 0)
            Sched::DeleteFunction(simGlobal.schedule64Hz,
                reinterpret_cast<Sched::tFunctionPt>(gList[i]->funcStats), gList[i]);
        Sched::DeleteFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(gList[i]->funcHandlingPhysics), gList[i]);
        Sched::DeleteFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(gList[i]->funcGravityPhysics), gList[i]);
        Sched::DeleteFunction(simGlobal.schedule64Hz,
            reinterpret_cast<Sched::tFunctionPt>(gList[i]->funcQDPhysicsUpdateVel), gList[i]);
        Sched::DeleteFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(gList[i]->funcTestMeForCollisions), gList[i]);
        Sched::DeleteFunction(simGlobal.schedule32Hz,
            reinterpret_cast<Sched::tFunctionPt>(gList[i]->funcDoPostCollisionStuff), gList[i]);
        if (Force::IsForceOn(gList[i]) != 0)
            Sched::DeleteFunction(simGlobal.schedule32Hz,
                reinterpret_cast<Sched::tFunctionPt>(Force::Update), gList[i]);
        Car::tObj *carObj = gList[i];
        Sched::tSchedule *schedule = (carObj->carFlags & 4) != 0 ?
            simGlobal.schedule64Hz : simGlobal.schedule32Hz2;
        Sched::DeleteFunction(schedule,
            reinterpret_cast<Sched::tFunctionPt>(carObj->funcQDPhysicsUpdateRot), carObj);
        DeInitCar(gList[i]);
        purgememadr(gList[i]);
    }
}

void Cars::FindCurrentLap(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8008ABC8, "NFS4.EXE");
    sint32 roadSlice = carObj->N.simRoadInfo.slice;
    if (GameSetup::gData.reverseTrack != 0)
        roadSlice = (sint32)((uint32)gNumSlices - (uint32)roadSlice - 1);
    uint16 oldSlice = carObj->N.oldSlice;
    if (oldSlice == roadSlice)
        return;
    if (roadSlice >= 501 && oldSlice == 0) {
        carObj->unlap = (sint32)((uint32)carObj->unlap + 1);
    } else if ((uint32)roadSlice < 51 && carObj->N.oldSlice > 500) {
        sint32 unlap = carObj->unlap;
        if (unlap > 0)
            carObj->unlap = (sint32)((uint32)unlap - 1);
        else
            carObj->lap = (sint32)((uint32)carObj->lap + 1);
    }
    carObj->N.oldSlice = (uint16)roadSlice;
}

sint32 Cars::CalculateRoadSpan(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8008AC80, "NFS4.EXE");
    sint32 dimension[3] = { carObj->N.dimension.x, carObj->N.dimension.y, carObj->N.dimension.z };
    uint32 span = 0;
    for (sint32 row = 0; row < 3; ++row)
    {
        uint32 projection = (uint32)(carObj->N.roadMatrix.m[0] / 256) * (uint32)(carObj->N.orientMat.m[row * 3] / 256);
        projection += (uint32)(carObj->N.roadMatrix.m[1] / 256) * (uint32)(carObj->N.orientMat.m[row * 3 + 1] / 256);
        projection += (uint32)(carObj->N.roadMatrix.m[2] / 256) * (uint32)(carObj->N.orientMat.m[row * 3 + 2] / 256);
        sint32 tempSpan = (sint32)projection;
        if (tempSpan < 0)
            tempSpan = (sint32)(0U - (uint32)tempSpan);
        span += (uint32)(tempSpan / 256) * (uint32)(dimension[row] / 256);
    }
    return (sint32)span;
}

sint32 Cars::CalcVelDownRoad(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8008AEE8, "NFS4.EXE");
    uint32 temp = (uint32)(carObj->N.linearVel.x / 256) * (uint32)(carObj->N.roadMatrix.m[6] / 256);
    temp += (uint32)(carObj->N.linearVel.y / 256) * (uint32)(carObj->N.roadMatrix.m[7] / 256);
    temp += (uint32)(carObj->N.linearVel.z / 256) * (uint32)(carObj->N.roadMatrix.m[8] / 256);
    return (sint32)temp;
}

void Cars::Randomize(void)
{
    FUNCTION_MARKER(0x8008AF84, "NFS4.EXE");
    if (gNumAICars != 0)
    {
        sint32 count = (gHumanRaceCarList[0]->roadPosition & 0x300) >> 8;
        if (count != 0)
        {
            uint32 seed = randSeed;
            for (sint32 rLoop = 0; rLoop < count; ++rLoop)
            {
                randtemp = fastRandom * seed;
                fastRandom = randtemp & 0xFFFF;
            }
        }
    }
}

void Cars::ManageBureaucracy(void)
{
    FUNCTION_MARKER(0x8008AFFC, "NFS4.EXE");
    for (sint32 carLoop = 0; carLoop < gNumCars; ++carLoop)
    {
        Car::tObj *carObj = gList[carLoop];
        if (carObj->N.active == 0)
            continue;
        carObj->roadSpan = CalculateRoadSpan(carObj);
        carObj->roadPosition = Newton::CalculateRoadPosition(&carObj->N);
        AIWorld::CalculateLaneInfo(carObj);
        if (AIPhysics_UseCoolPhysics(carObj) != 0)
            carObj->currentSpeed = CalcVelDownRoad(carObj);
        if ((carObj->carFlags & 4) != 0)
        {
            sint32 currentSpeed = carObj->currentSpeed;
            carObj->speed = carObj->N.speedXZ;
            if ((uint32)currentSpeed + 0x2FFFFU < 0x5FFFFU)
            {
                sint32 facing = fixedmult(carObj->N.orientMat.m[6],
                    (sint8)BWorldSm::slices[carObj->N.simRoadInfo.slice].forward[0]);
                sint32 second = fixedmult(carObj->N.orientMat.m[7],
                    (sint8)BWorldSm::slices[carObj->N.simRoadInfo.slice].forward[1]);
                sint32 third = fixedmult(carObj->N.orientMat.m[8],
                    (sint8)BWorldSm::slices[carObj->N.simRoadInfo.slice].forward[2]);
                facing = (sint32)((uint32)facing + (uint32)second + (uint32)third);
                carObj->direction = facing > 0 ? 1 : -1;
            }
            else
                carObj->direction = currentSpeed < 0 ? -1 : 1;
        }
        if (carObj->N.simRoadInfo.sliceChanged != 0)
        {
            FindCurrentLap(carObj);
            FindTotalSlice(carObj);
        }
    }
    SortCars();
    Randomize();
    AISpeeds::MaintainLeaderBoard();
    DrawW::DoObjectAnimations();
}

void Cars::CheckForAccidentScenes(void)
{
    FUNCTION_MARKER(0x8008B1C4, "NFS4.EXE");
    if (GameSetup::gData.commMode == 1 || GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5)
        return;
    if (SceneLoaded != 0)
    {
        if (gHumanRaceCarList[0]->stats.lap == GameSetup::gData.SceneEndLap)
        {
            Object::ClearCustomObjects();
            SceneLoaded = 0;
            accidentSlice = 0;
        }
    }
    else if (gHumanRaceCarList[0]->stats.lap == GameSetup::gData.SceneStartLap)
    {
        accidentSlice = Scene::BuildCustomSceneList();
        SceneLoaded = 1;
    }
}

void Cars::SortCars(void)
{
    FUNCTION_MARKER(0x8008B28C, "NFS4.EXE");
    sint32 limit = (sint32)((uint32)gNumCars - 1U);
    sint32 swapped;
    sint32 i;
    Car::tObj *temp;
    do
    {
        swapped = 0;
        if (limit > 0)
        {
            sint32 passLimit = (sint32)((uint32)gNumCars - 1U);
            i = 0;
            do
            {
                temp = gSortedList[i];
                Car::tObj *next = gSortedList[i + 1];
                if (next->N.simRoadInfo.slice < temp->N.simRoadInfo.slice)
                {
                    swapped = 1;
                    gSortedList[i] = next;
                    gSortedList[i + 1] = temp;
                }
                ++i;
            } while (i < passLimit);
        }
    } while (swapped != 0);
    sint32 count = gNumCars;
    for (i = 0; i < count; ++i)
        gSortedList[i]->sortIndex = i;

    limit = (sint32)((uint32)gNumCars - 1U);
    do
    {
        swapped = 0;
        if (limit > 0)
        {
            sint32 passLimit = (sint32)((uint32)gNumCars - 1U);
            i = 0;
            do
            {
                if (gTotalSortedList[i + 1]->N.totalSlice < gTotalSortedList[i]->N.totalSlice)
                {
                    gTotalSortedList[i]->swapCar = gTotalSortedList[i + 1];
                    gTotalSortedList[i + 1]->swapCar = gTotalSortedList[i];
                    sint32 ticks = simGlobal.gameTicks;
                    gTotalSortedList[i + 1]->swapTime = ticks;
                    gTotalSortedList[i]->swapTime = ticks;
                    temp = gTotalSortedList[i];
                    gTotalSortedList[i] = gTotalSortedList[i + 1];
                    gTotalSortedList[i + 1] = temp;
                    swapped = 1;
                }
                ++i;
            } while (i < passLimit);
        }
    } while (swapped != 0);
}
