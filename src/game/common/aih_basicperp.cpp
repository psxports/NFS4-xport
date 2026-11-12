#include <stddef.h>
#include "xport.h"
#include "game/common/aihigh.h"
#include "game/common/aispeeds.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/collide.h"
#include "game/common/gmesetup.h"
#include "game/common/sim.h"
#include "game/common/speech.h"
#include "lib/psx/fixdmult.h"

sint32 AIHigh_BasicPerp::PlayerCaughtSpeed[3] = { 1019448, 1165084, 1310720 };
sint32 AIHigh_BasicPerp::PlayerCaughtDeltaY[3] = { 327680, 425984, 524288 };
sint32 AIHigh_BasicPerp::MinDeltaSpeed[3] = { 582542, 728177, 873813 };
sint32 AIHigh_BasicPerp::CopCaughtSpeed[3] = { 728177, 932067, 1165084 };
sint32 AIHigh_BasicPerp::CaughtDistance[3] = { 1966080, 2949120, 3932160 };

void AIHigh_BasicPerp::CheckForCrimes(void)
{
    FUNCTION_MARKER(0x8005B500, "NFS4.EXE");
    crimeType originalCrime = basicPerpInfo_.crime_;
    crimeType crime = originalCrime;
    sint32 legal = AISpeeds::GetLegalSpeed(carObj_->N.simRoadInfo.slice);
    sint32 ticks = simGlobal.gameTicks;
    if ((ticks - lastPullOverTime_) < 0x280)
        return;

    Car::tObj *car = carObj_;
    if (car->stats.finishType >= 2)
        return;
    if ((ticks - car->N.collision.lastTime) < 15) {
        BO::tNewtonObj *other = car->N.collision.lastOtherObj;
        if (other != 0 && CollisionCopFlag(other)) {
            if (car->N.collision.lastImpulse > 0x140000)
                crime = CRIME_SMASHCOP;
            else if (crime == CRIME_NONE)
                crime = CRIME_BUMPCOP;
        }
    }
    if (originalCrime != CRIME_NONE && crime != CRIME_SMASHCOP)
        return;

    sint32 speed = carObj_->currentSpeed;
    sint32 absSpeed = speed < 0 ? -speed : speed;
    if (legal < 0)
        legal = (-legal);
    if (legal < absSpeed && crime == CRIME_NONE)
        crime = CRIME_SPEEDER;

    bool wrongSide;
    if (AITune::oneWay != 0) {
        speed = carObj_->currentSpeed;
        sint32 signedDirection = GameSetup::gData.reverseTrack != 0 ? -speed : speed;
        wrongSide = signedDirection < 0;
    } else {
        car = carObj_;
        speed = car->currentSpeed;
        sint32 signedDirection = (speed * AITune::driveSide);
        wrongSide = signedDirection < 0 ? car->laneIndex >= 7 : car->laneIndex < 7;
    }
    if (wrongSide) {
        absSpeed = speed < 0 ? -speed : speed;
        if (absSpeed > 0x40000 && crime == CRIME_NONE)
            crime = CRIME_WRONGSIDE;
    }

    if (simGlobal.gameTicks < 0x200)
        crime = CRIME_NONE;
    if (crime != CRIME_NONE) {
        sint32 carCount = Cars::gNumCars;
        for (sint32 carLoop = 0; carLoop < carCount; ++carLoop) {
            car = Cars::gList[carLoop];
            if ((car->carFlags & 0x220) != 0 && car->N.active != 0) {
                basicPerpInfo_.crime_ = crime;
                return;
            }
        }
    }
}

sint32 AIHigh_BasicPerp::CheckIfCaught(void)
{
    FUNCTION_MARKER(0x8005B78C, "NFS4.EXE");
    Car::tObj *car = carObj_;
    sint32 skill = GameSetup::gData.skill;
    sint32 absSpeed;
    if ((car->carFlags & 2) != 0) {
        absSpeed = car->currentSpeed;
        if (absSpeed < 0)
            absSpeed = (-absSpeed);
    } else {
        absSpeed = car->N.speedXZ;
    }
    if (basicPerpInfo_.crime_ == CRIME_NONE || PlayerCaughtSpeed[skill] < absSpeed)
        return 0;
    sint32 elapsed = (simGlobal.gameTicks - lastPullOverTime_);
    if (elapsed < 960)
        return 0;
    car = carObj_;
    if (car->stats.finishType >= 2)
        return 0;
    sint32 perpUpright = car->N.orientMat.m[4] > 0x9999;
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
        Car::tObj *cop = Cars::gList[carLoop];
        sint32 diffSpeed = (carObj_->currentSpeed - cop->currentSpeed);
        bool btc = (GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) && ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0 || (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0));
        sint32 validCar = (cop->carFlags & (btc ? 0x200 : 0x20)) != 0;
        if (!validCar)
            continue;
        sint32 flags = cop->AIFlags;
        if ((flags & 2) == 0 || cop->N.flightTime != 0 || cop->N.active == 0 || (flags & 4) != 0)
            continue;
        if (cop->N.orientMat.m[4] <= 0x9999 || diffSpeed >= MinDeltaSpeed[skill])
            continue;
        sint32 copSpeed = cop->currentSpeed;
        if (copSpeed < 0)
            copSpeed = (-copSpeed);
        if (copSpeed >= CopCaughtSpeed[skill])
            continue;
        sint32 copY = cop->N.position.y;
        sint32 carY = carObj_->N.position.y;
        sint32 deltaY = (copY - carY);
        if (deltaY <= 0)
            deltaY = (carY - copY);
        if (deltaY >= PlayerCaughtDeltaY[skill])
            continue;
        sint32 distanceAbsMeters = AIWorld::ApxSplineDistance(carObj_, cop);
        if (distanceAbsMeters < 0)
            distanceAbsMeters = (-distanceAbsMeters);
        car = carObj_;
        uint8 laneCount = BWorldSm::slices[car->N.simRoadInfo.slice].laneCount;
        sint32 lane = car->laneIndex;
        sint32 barrierInWay = 0;
        if (lane >= 7 - (laneCount >> 4) && lane <= (laneCount & 15) + 6) {
            sint32 copSlice = cop->N.simRoadInfo.slice;
            uint8 copLaneCount = BWorldSm::slices[copSlice].laneCount;
            sint32 copLane = cop->laneIndex;
            if (copLane >= 7 - (copLaneCount >> 4) && copLane <= (copLaneCount & 15) + 6) {
                if (AIWorld::CheckForBarrierBetweenLanes(copSlice, copLane, lane))
                    barrierInWay = AIWorld::CheckForBarrierBetweenLanes(carObj_->N.simRoadInfo.slice, cop->laneIndex, carObj_->laneIndex) != 0;
            }
        }
        if (distanceAbsMeters >= CaughtDistance[skill] || barrierInWay)
            continue;
        sint32 xDot = 0;
        sint32 zDot = 0x10000;
        btc = (GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) && ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0 || (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0));
        if (btc) {
            coorddef carCopVector;
            carCopVector.x = (cop->N.position.x - carObj_->N.position.x);
            carCopVector.y = (cop->N.position.y - carObj_->N.position.y);
            carCopVector.z = (cop->N.position.z - carObj_->N.position.z);
            sint32 x = fixedmult(carCopVector.x, carObj_->N.orientMat.m[0]);
            sint32 y = fixedmult(carCopVector.y, carObj_->N.orientMat.m[1]);
            sint32 z = fixedmult(carCopVector.z, carObj_->N.orientMat.m[2]);
            xDot = ((x + y) + z);
            x = fixedmult(carCopVector.x, carObj_->N.orientMat.m[6]);
            y = fixedmult(carCopVector.y, carObj_->N.orientMat.m[7]);
            z = fixedmult(carCopVector.z, carObj_->N.orientMat.m[8]);
            zDot = ((x + y) + z);
        }
        if (perpUpright && (cop->speed > 0x471C6 || carObj_->speed > 0x471C6)) {
            sint32 absXDot = xDot;
            if (absXDot < 0)
                absXDot = (-absXDot);
            if (absXDot > 0x8FFFF || zDot < 0 || zDot > 0x8FFFF)
                continue;
        }
        if (!lastArrestingCop_)
            lastArrestingCop_ = cop;
        return 1;
    }
    return 0;
}

void AIHigh_BasicPerp::RemoveCloseCops(void)
{
    FUNCTION_MARKER(0x8005BD7C, "NFS4.EXE");
    for (sint32 copLoop = 0; copLoop < Cars::gNumCopCars; ++copLoop) {
        Car::tObj *cop = Cars::gCopCarList[copLoop];
        if ((cop->AIFlags & 4) != 0)
            continue;
        sint32 distance = AIWorld::ApxSplineDistance(cop, carObj_);
        if (distance < 0)
            distance = (-distance);
        if (distance > 0x95FFFF)
            continue;
        AIHigh_BasicCop *thisCop = static_cast<AIHigh_BasicCop *>(highLevelAIObjs[cop->carIndex]);
        sint32 direction = GameSetup::gData.reverseTrack ? 1 : -1;
        cop->direction = direction;
        cop->desiredDirection = direction;
        cop->driveDirection = 1;
        thisCop->driveAway_ = distance <= 0x1DFFFF ? AIHigh::DRIVEAWAY_RESET : AIHigh::DRIVEAWAY_NORESET;
        Speech::Mobile(thisCop->carObj_)->Purge();
    }
}

void AIHigh_BasicPerp::RemoveChaser(sint32 copIndex, sint32 carIndex, copType type)
{
    FUNCTION_MARKER(0x8005BECC, "NFS4.EXE");
    sint32 assigned = basicPerpInfo_.copsAssigned_[type];
    sint32 pos = copVSPositionList_[copIndex];
    basicPerpInfo_.copsAssigned_[type] = (assigned - 1);
    positionVSCopList_[pos].copIndex = -1;
    positionVSCopList_[pos].carIndex = -1;
}

sint32 AIHigh_BasicPerp::AddChaser(sint32 copIndex, sint32 carIndex, copType type)
{
    FUNCTION_MARKER(0x8005BF08, "NFS4.EXE");
    basicPerpInfo_.copsAssigned_[type] = (basicPerpInfo_.copsAssigned_[type] + 1);
    positionVSCopList_[5].copIndex = copIndex;
    positionVSCopList_[5].carIndex = carIndex;
    copVSPositionList_[copIndex] = 5;
    return CheckChaserPosition(copIndex, carIndex);
}

sint32 AIHigh_BasicPerp::CheckChaserPosition(sint32 copIndex, sint32 carIndex)
{
    FUNCTION_MARKER(0x8005BF5C, "NFS4.EXE");
    sint32 thisCopSlice = highLevelAIObjs[carIndex]->carObj_->N.simRoadInfo.slice;
    sint32 pos = copVSPositionList_[copIndex];
    while (pos > 0) {
        sint32 nextCopIndex = positionVSCopList_[pos - 1].copIndex;
        sint32 nextCarIndex = positionVSCopList_[pos - 1].carIndex;
        if (nextCopIndex != -1) {
            if (nextCarIndex != -1) {
                sint32 distance = AIWorld::ApxSplineDistance(highLevelAIObjs[nextCarIndex]->carObj_, thisCopSlice);
                sint32 directedDistance = (distance * carObj_->direction);
                if (directedDistance >= -0xC0000)
                    return pos;
            }
            copVSPositionList_[nextCopIndex] = pos;
        }
        copVSPositionList_[copIndex] = pos - 1;
        positionVSCopList_[pos].copIndex = positionVSCopList_[pos - 1].copIndex;
        positionVSCopList_[pos].carIndex = positionVSCopList_[pos - 1].carIndex;
        positionVSCopList_[pos - 1].copIndex = copIndex;
        positionVSCopList_[pos - 1].carIndex = carIndex;
        --pos;
    }
    return pos;
}

void AIHigh_BasicPerp::Clear(void)
{
    FUNCTION_MARKER(0x8005C0B8, "NFS4.EXE");
    for (sint32 loop = 0; loop < 6; ++loop) {
        positionVSCopList_[loop].copIndex = -1;
        positionVSCopList_[loop].carIndex = -1;
    }
    pullOverMode_ = PULLOVER_NONE;
    beatingTicksLeft_ = 0;
    lastPullOverTime_ = -0x280;
    lastArrestingCop_ = 0;
}

AIHigh_BasicPerp::AIHigh_BasicPerp(Car::tObj *carObj) : AIHigh_Base(carObj)
{
    FUNCTION_MARKER(0x8005C0F4, "NFS4.EXE");
    basicPerpInfo_.copsAssigned_[0] = 0;
    basicPerpInfo_.copsAssigned_[1] = 0;
    basicPerpInfo_.crime_ = CRIME_NONE;
    Clear();
}

AIHigh_BasicPerp::~AIHigh_BasicPerp(void)
{
    FUNCTION_MARKER(0x8005C13C, "NFS4.EXE");
}
