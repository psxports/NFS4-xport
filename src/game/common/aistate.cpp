#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/ai.h"
#include "game/common/aispeeds.h"
#include "game/common/aistate.h"
#include "game/common/aitriger.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/audioclc.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/fastrand.h"
#include "game/common/gmesetup.h"
#include "game/common/newton.h"
#include "game/common/sim.h"
#include "lib/psx/fixdmult.h"

sint32 AIState_Purgatory::numTrafficCarsInPurgatory;

void AIState::StartUp(void)
{
    FUNCTION_MARKER(0x8006F9A4, "NFS4.EXE");

    AIState_Purgatory::StartUp();
}

void AIState::Restart(void)
{
    FUNCTION_MARKER(0x8006F9C4, "NFS4.EXE");
    AIState_Purgatory::StartUp();
}

void AIState::CleanUp(void)
{
    FUNCTION_MARKER(0x8006F9E4, "NFS4.EXE");
}

void AIState_Base::StateExecute(void)
{
    FUNCTION_MARKER(0x8006F9EC, "NFS4.EXE");
    Car::tObj *carObj = carObj_;
    if (carObj->N.active != 0 && (carObj->carFlags & 4) == 0 && carObj->N.deadTimer == 0)
        AIScript::ProcessActionsAndReactions(&carObj->script, AI::elapsedTime);
    Execute();
}

AIState_Base::AIState_Base(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006FA74, "NFS4.EXE");
    carObj_ = carObj;
}

void AIState_Normal::Execute(void)
{
    FUNCTION_MARKER(0x8006FA8C, "NFS4.EXE");
    AISpeeds::CalcDesiredSpeed(carObj_);
    AI::GenericBeginCycle(carObj_);
    AI::GenericCycle(carObj_);
    AI::GenericEndCycle(carObj_);
}

AIState_Normal::AIState_Normal(Car::tObj *carObj) : AIState_Base(carObj)
{
    FUNCTION_MARKER(0x8006FADC, "NFS4.EXE");
    carObj_->targetPos.z = 0;
    carObj_->targetPos.y = 0;
    carObj_->targetPos.x = 0;
    carObj_->targetLatPos = 0;
    carObj_->N.active = 1;
}

AIState_Idle::AIState_Idle(Car::tObj *carObj) : AIState_Base(carObj)
{
    idleInPlaceFlag_ = 1;
}

void AIState_Idle::Execute(void)
{
    FUNCTION_MARKER(0x8006FB34, "NFS4.EXE");
    if (idleInPlaceFlag_ != 0)
    {
        carObj_->desiredSpeed = 0;
        carObj_->desiredLatPos = carObj_->roadPosition;
        return;
    }
    if (carObj_->roadPosition - (uint32)roadPosition_ + 0xffffU <= 0x1fffeU)
        carObj_->desiredSpeed = 0;
    else
    {
        AISpeeds::CalcDesiredSpeed(carObj_);
        sint32 off = carObj_->desiredSpeed;
        carObj_->desiredSpeed = off / 4;
    }
    carObj_->desiredLatPos = roadPosition_;
}

void AIState_Idle::SetIdlePosition(sint32 roadPosition)
{
    FUNCTION_MARKER(0x8006FBF0, "NFS4.EXE");
    roadPosition_ = roadPosition;
    idleInPlaceFlag_ = 0;
}

AIState_Chase::AIState_Chase(Car::tObj *carObj, Car::tObj *targetCar, coorddef *relPosition, sint32 nitrousTicks, sint32 nitrousMinForeDistance, sint32 nitrousMinAftDistance, sint32 aggressionLevel, sint32 delayTime) : AIState_Base(carObj), delayCar_(carObj, targetCar, delayTime), targetCar_(targetCar)
{
    FUNCTION_MARKER(0x8006FBFC, "NFS4.EXE");
    nitrousTicks_ = nitrousTicks;
    SetTarget(targetCar, relPosition);
    carObj_->accNitrous = 65536;
    carObj_->speedNitrous = 65536;
    nitrousMinForeDistance_ = nitrousMinForeDistance;
    nitrousMinAftDistance_ = nitrousMinAftDistance;
    aggressionLevel_ = aggressionLevel;
    slowDownEndTime_ = 0;
    noTurnAroundEndTime_ = 0;
    barrierTicks32_ = 0;
}

AIState_Chase::~AIState_Chase(void)
{
    FUNCTION_MARKER(0x8006FD04, "NFS4.EXE");
    carObj_->targetPos.z = 0;
    carObj_->targetPos.y = 0;
    carObj_->targetPos.x = 0;
    carObj_->targetLatPos = 0;
    carObj_->preferredLateralPosition = 0;
    carObj_->preferredLateralPositionPower = 0;
    carObj_->accNitrous = 65536;
    carObj_->speedNitrous = 65536;
}

AIState_Offroad::~AIState_Offroad(void)
{
    FUNCTION_MARKER(0x800710AC, "NFS4.EXE");
    carObj_->targetPos.z = 0;
    carObj_->targetPos.y = 0;
    carObj_->targetPos.x = 0;
    carObj_->targetLatPos = 0;
    carObj_->carFlags &= ~0x800;
}

void AIState_Chase::SetTarget(Car::tObj *targetCar, coorddef *relPosition)
{
    FUNCTION_MARKER(0x8006FD8C, "NFS4.EXE");
    if (targetCar_ != targetCar)
        delayCar_.SetNewTargetCar(targetCar);
    targetCar_ = targetCar;
    relPosition_ = *relPosition;
    longTargetRegion_ = 0;
    latTargetRegion_ = 0;
    sint32 width = targetCar_->N.dimension.x;
    if (relPosition_.x < -width)
        latTargetRegion_ = -1;
    else if (width < relPosition_.x)
        latTargetRegion_ = 1;
    sint32 length = targetCar_->N.dimension.z;
    if (relPosition_.z < -length)
        longTargetRegion_ = -1;
    else if (length < relPosition_.z)
        longTargetRegion_ = 1;
    murderMode_ = 0;
    murderEndTime_ = 0;
    inTargetRegion_ = 0;
    SetUp();
}

void AIState_Chase::SetMurderMode(sint32 murderMode, sint32 murderTicks)
{
    FUNCTION_MARKER(0x8006FE6C, "NFS4.EXE");
    if (murderMode_ == 0)
    {
        murderMode_ = murderMode;
        murderEndTime_ = (simGlobal.gameTicks + murderTicks);
    }
}

void AIState_Chase::SetUp(void)
{
    FUNCTION_MARKER(0x8006FE98, "NFS4.EXE");
    delayCar_.Update();
    carDir_ = carObj_->currentSpeed < 0 ? -1 : 1;
    targetDir_ = delayCar_.currentSpeed_ < 0 ? -1 : 1;
    latMetersBetween_ = (carObj_->roadPosition - delayCar_.roadPosition_);
    coorddef targetCarPosition = delayCar_.position_;
    longMetersBetween_ = AIWorld::SplineDistance(carObj_, delayCar_.slice_, &targetCarPosition);
    carObj_->targetPos.z = 0;
    carObj_->targetPos.y = 0;
    carObj_->targetPos.x = 0;
    carObj_->targetLatPos = 0;
    if (murderMode_ != 0 && murderEndTime_ <= simGlobal.gameTicks)
    {
        murderMode_ = 0;
        murderEndTime_ = 0;
    }
    if (carObj_->accNitrous != 65536)
        nitrousTicks_ = (nitrousTicks_ - AI::elapsedTime);
    carObj_->accNitrous = 65536;
    carObj_->speedNitrous = 65536;
    carObj_->preferredLateralPosition = 0;
    carObj_->preferredLateralPositionPower = 0;
}

void AIState_Chase::DoSlowNitrous(void)
{
    FUNCTION_MARKER(0x8006FFF4, "NFS4.EXE");
    carObj_->accNitrous = 0x8000;
    carObj_->speedNitrous = 0xc000;
}

void AIState_Chase::DoNitrous(sint32 checkForHumans)
{
    FUNCTION_MARKER(0x80070010, "NFS4.EXE");
    if (nitrousTicks_ <= 0 || simGlobal.gameTicks < slowDownEndTime_)
        return;
    carObj_->accNitrous = 0x30000;
    carObj_->speedNitrous = 0x28000;
    for (sint32 humanLoop = 0; checkForHumans != 0 && humanLoop < Cars::gNumHumanRaceCars; ++humanLoop)
    {
        sint32 distanceMeters = AIWorld::ApxSplineDistance(carObj_, Cars::gHumanRaceCarList[humanLoop]);
        distanceMeters = (distanceMeters * carObj_->direction);
        if (distanceMeters > 0 && distanceMeters < nitrousMinForeDistance_)
        {
            carObj_->accNitrous = 65536;
            carObj_->speedNitrous = 65536;
        }
        if (distanceMeters < 0 && -nitrousMinAftDistance_ < distanceMeters)
        {
            carObj_->accNitrous = 65536;
            carObj_->speedNitrous = 65536;
        }
    }
}

void AIState_Chase::Execute(void)
{
    FUNCTION_MARKER(0x8007015C, "NFS4.EXE");
    SetUp();
    sint32 velocityToHitInTime = fixedmult(longMetersBetween_, 0x666);
    sint32 targetSpeed = targetCar_->currentSpeed;
    if (velocityToHitInTime < 0)
        velocityToHitInTime = (-velocityToHitInTime);
    sint32 deltaVelocity = (carObj_->currentSpeed - targetSpeed);
    if (deltaVelocity < 0)
        deltaVelocity = (-deltaVelocity);
    sint32 far = 0;
    if (deltaVelocity < velocityToHitInTime)
    {
        sint32 distance = longMetersBetween_;
        if (distance < 0)
            distance = (-distance);
        far = distance > 0x8c0000;
    }
    if (targetSpeed < 0)
        targetSpeed = (-targetSpeed);
    sint32 targetCarVerySlow = targetSpeed <= 0x6aaa9;
    if (far)
        FarTargeting();
    else if ((carDir_ * targetDir_) == 1 && !targetCarVerySlow)
        CloseTargeting();
    else if ((carDir_ * longMetersBetween_) >= 0 && !targetCarVerySlow)
        FarTargeting();
    else
    {
        sint32 intercept = 0;
        if (deltaVelocity < 0x1aaaab && !far)
        {
            sint32 distance = longMetersBetween_;
            if (distance < 0)
                distance = (-distance);
            intercept = distance <= 0x500000;
        }
        ApproachTargeting(intercept);
    }
    if (simGlobal.gameTicks < slowDownEndTime_)
        DoSlowNitrous();
}

void AIState_Chase::FarTargeting(void)
{
    FUNCTION_MARKER(0x800702D0, "NFS4.EXE");
    inTargetRegion_ = 0;
    if (noTurnAroundEndTime_ < simGlobal.gameTicks)
        carObj_->desiredDirection = longMetersBetween_ <= 0 ? 1 : -1;
    DoNitrous(1);
    AISpeeds::CalcDesiredSpeed(carObj_);
    AI::GenericBeginCycle(carObj_);
    CheckForBarriersAndTargetAroundThem();
    AI::GenericCycle(carObj_);
    AI::GenericEndCycle(carObj_);
}

sint32 AIState_Chase::CalculateCloseTargettingAheadSlowDownFactor(void)
{
    FUNCTION_MARKER(0x8007037C, "NFS4.EXE");
    sint32 absLongMetersBetween = longMetersBetween_;
    if (absLongMetersBetween < 0)
        absLongMetersBetween = (-absLongMetersBetween);
    sint32 slowDown = 0x9999;
    if (absLongMetersBetween < 0x1e0000)
        slowDown = 0xf333;
    else if (absLongMetersBetween < 0x320000 || absLongMetersBetween < 0x640000)
        slowDown = 0xcccc;
    else if (absLongMetersBetween < 0x960000)
        slowDown = 0xc000;
    else if (absLongMetersBetween < 0xc80000)
        slowDown = 0xb333;
    return slowDown;
}

void AIState_Chase::CloseTargeting(void)
{
    FUNCTION_MARKER(0x80070408, "NFS4.EXE");
    sint32 latPos = 0;
    sint32 longPos = 0;
    sint32 dir = carObj_->direction;
    DoNitrous(1);
    sint32 bigLongPos = 0;
    sint32 latTarget;
    sint32 longTarget;
    if (murderMode_ == 0)
    {
        latTarget = latTargetRegion_;
        longTarget = longTargetRegion_;
    }
    else
    {
        longTarget = 0;
        latTarget = 0;
        DoNitrous(0);
    }
    sint32 position = (latMetersBetween_ * dir);
    sint32 dimension = targetCar_->N.dimension.x;
    if (position < -dimension)
        latPos = -1;
    else if (dimension < position)
        latPos = 1;
    position = (longMetersBetween_ * dir);
    dimension = targetCar_->N.dimension.z;
    if (position < (0x20000 - dimension))
        longPos = -1;
    else if ((dimension + 0x20000) < position)
        longPos = 1;
    position = (longMetersBetween_ * dir);
    dimension = (targetCar_->N.dimension.z + 0x20000);
    if (position < -dimension)
        bigLongPos = -1;
    else if (dimension < position)
        bigLongPos = 1;
    if (latPos == latTarget && longPos == longTarget)
        inTargetRegion_ = (inTargetRegion_ + AI::elapsedTime);
    else
        inTargetRegion_ = 0;
    sint32 forceLongAction = 0;
    sint32 forceLatAction = 0;
    sint32 doLatAction = 1;
    if (bigLongPos * longTarget == -1)
        forceLatAction = latTarget == 0 ? -1 : latTarget;
    else if (latPos * latTarget == -1 && bigLongPos == 0)
    {
        forceLongAction = -1;
        doLatAction = 0;
    }
    else if (latTarget == 0 && bigLongPos == 0 && murderMode_ == 0)
        doLatAction = 0;
    else if (longTarget == 1 && bigLongPos == 1 && latPos == 0 && longMetersBetween_ < 0x140000)
        forceLongAction = -2;
    sint32 desiredSpeed = delayCar_.currentSpeed_;
    if (desiredSpeed <= 0)
        desiredSpeed = (-desiredSpeed);
    if (longPos < longTarget || forceLongAction == 1)
        desiredSpeed = fixedmult(desiredSpeed, 0x16666);
    else if (longTarget < longPos || forceLongAction == -1 || (longMetersBetween_ > 0x140000 && (longMetersBetween_ > 0x9ffff || latPos != 0 || latTarget == 0)))
    {
        sint32 slowDown = CalculateCloseTargettingAheadSlowDownFactor();
        desiredSpeed = fixedmult(desiredSpeed, slowDown);
    }
    else if (forceLongAction == -2)
    {
        sint32 superSlowDown = aggressionLevel_ == 0 ? 0xca3d : (aggressionLevel_ == 1 ? 0xbae1 : 0xa666);
        desiredSpeed = fixedmult(desiredSpeed, superSlowDown);
    }
    AISpeeds::CalcDesiredSpeed(carObj_);
    sint32 speed = carObj_->desiredSpeed;
    if (speed < 0)
        speed = (-speed);
    carObj_->desiredSpeed = speed;
    speed = carObj_->desiredSpeed;
    if (desiredSpeed < speed)
        speed = desiredSpeed;
    if (speed < 0x50000)
        speed = 0x50000;
    carObj_->desiredSpeed = (speed * dir);
    AI::GenericBeginCycle(carObj_);
    if (doLatAction)
    {
        sint32 targettingStrength = 0xf0000;
        sint32 targetLanePosition;
        if (murderMode_ == 0)
        {
            sint32 offset = forceLatAction == 0 ? relPosition_.x : (targetCar_->N.dimension.x + 0x60000);
            targetLanePosition = (delayCar_.roadPosition_ + (offset * dir));
            Trk::NewSlice *slice = &BWorldSm::slices[carObj_->N.simRoadInfo.slice];
            sint32 leftEdge = -(slice->avgPavedWidthLf * 32768 * (slice->laneCount >> 4));
            if (targetLanePosition <= leftEdge)
                targetLanePosition = leftEdge;
            sint32 rightEdge = slice->avgPavedWidthRt * 32768 * (slice->laneCount & 15);
            if (targetLanePosition >= rightEdge)
                targetLanePosition = rightEdge;
        }
        else
        {
            targettingStrength = 0x1e0000;
            targetLanePosition = delayCar_.roadPosition_;
        }
        carObj_->preferredLateralPosition = targetLanePosition;
        carObj_->preferredLateralPositionPower = targettingStrength;
    }
    CheckForBarriersAndTargetAroundThem();
    AI::GenericCycle(carObj_);
    AI::GenericEndCycle(carObj_);
}

void AIState_Chase::ApproachTargeting(sint32 intercept)
{
    FUNCTION_MARKER(0x800708B8, "NFS4.EXE");
    sint32 xDistance = (carObj_->N.position.x - targetCar_->N.position.x);
    if (xDistance < 0)
        xDistance = (-xDistance);
    sint32 zDistance = (carObj_->N.position.z - targetCar_->N.position.z);
    if (zDistance < 0)
        zDistance = (-zDistance);
    sint32 distance = zDistance < xDistance ? (xDistance + (zDistance >> 2)) : (zDistance + (xDistance >> 2));
    inTargetRegion_ = 0;
    if (noTurnAroundEndTime_ < simGlobal.gameTicks)
        carObj_->desiredDirection = longMetersBetween_ <= 0 ? 1 : -1;
    AISpeeds::CalcDesiredSpeed(carObj_);
    sint32 minSpeed = 0x190000;
    if (aggressionLevel_ == 0 || aggressionLevel_ == 1)
    {
        if (distance > 0x960000)
            minSpeed = 0x3c0000;
        else if (distance > 0x640000)
            minSpeed = 0x320000;
        else if (distance > 0x320000)
            minSpeed = 0x280000;
        else if (distance > 0x190000)
            minSpeed = 0x230000;
        else if (distance > 0xa0000)
            minSpeed = aggressionLevel_ == 0 ? 0x140000 : 0xa0000;
        else
            minSpeed = distance > 0x60000 ? 0x30000 : 0;
    }
    else if (aggressionLevel_ == 2)
    {
        if (distance > 0x960000)
            minSpeed = 0x500000;
        else if (distance > 0x640000)
            minSpeed = 0x460000;
        else if (distance > 0x320000)
            minSpeed = 0x320000;
        else if (distance > 0x190000)
            minSpeed = 0x280000;
        else if (distance > 0xa0000)
            minSpeed = 0xe0000;
        else
            minSpeed = distance > 0x60000 ? 0x60000 : 0;
    }
    sint32 desiredSpeed;
    if (carObj_->direction == 1)
    {
        desiredSpeed = minSpeed;
        if (carObj_->desiredSpeed < desiredSpeed)
            desiredSpeed = carObj_->desiredSpeed;
    }
    else
    {
        desiredSpeed = (-minSpeed);
        if (desiredSpeed < carObj_->desiredSpeed)
            desiredSpeed = carObj_->desiredSpeed;
    }
    carObj_->desiredSpeed = desiredSpeed;
    AI::GenericBeginCycle(carObj_);
    CheckForBarriersAndTargetAroundThem();
    AI::GenericCycle(carObj_);
    AI::GenericEndCycle(carObj_);
    if (intercept != 0)
    {
        carObj_->targetPos = targetCar_->N.position;
        carObj_->targetLatPos = targetCar_->roadPosition;
        carObj_->preferredLateralPosition = targetCar_->roadPosition;
        carObj_->preferredLateralPositionPower = 0xf0000;
    }
}

void AIState_Chase::CheckForBarriersAndTargetAroundThem(void)
{
    FUNCTION_MARKER(0x80070BC4, "NFS4.EXE");
    sint32 distance = longMetersBetween_;
    if (distance < 0)
        distance = (-distance);
    if (distance > 0x1f40000)
        return;
    sint32 mySlice = carObj_->N.simRoadInfo.slice;
    sint32 myLane = carObj_->laneIndex;
    sint32 targetSlice = targetCar_->N.simRoadInfo.slice;
    sint32 targetLane = targetCar_->laneIndex;
    uint32 myLanes = BWorldSm::slices[mySlice].laneCount;
    if (myLane < 6 - (sint32)(myLanes >> 4) || myLane > (sint32)(myLanes & 15U) + 7)
        return;
    uint32 targetLanes = BWorldSm::slices[targetSlice].laneCount;
    if (targetLane < 6 - (sint32)(targetLanes >> 4) || targetLane > (sint32)(targetLanes & 15U) + 7)
        return;
    sint32 barrierBesideTarget = AIWorld::CheckForBarrierBetweenLanes(targetSlice, targetLane, myLane);
    sint32 barrierBesideMe = AIWorld::CheckForBarrierBetweenLanes(mySlice, targetLane, myLane);
    if (barrierBesideTarget == 0)
        barrierTicks32_ = 0;
    else if (barrierBesideMe == 0)
    {
        barrierTicks32_ = 0;
        AI::TargetLane(carObj_, targetLane);
        AI::TargetLane(carObj_, targetLane);
        AI::TargetLane(carObj_, targetLane);
    }
    else
        barrierTicks32_ = (barrierTicks32_ + AI::elapsedTime);
}

sint32 AIState_Chase::FindBarrierEndSlice(void)
{
    FUNCTION_MARKER(0x80070D14, "NFS4.EXE");
    sint32 myLane = carObj_->laneIndex;
    sint32 targetLane = targetCar_->laneIndex;
    sint32 mySlice = carObj_->N.simRoadInfo.slice;
    sint32 leftLane = myLane;
    sint32 rightLane = targetLane;
    if (targetLane < myLane)
    {
        leftLane = targetLane;
        rightLane = myLane;
    }
    sint32 barrierLane = leftLane;
    while (barrierLane < rightLane && AIWorld::CheckForBarrierBetweenLanes(mySlice, barrierLane, barrierLane) == 0)
        barrierLane = (barrierLane + 1);
    sint32 forwardBarrierEndSlice = mySlice + 0x53;
    sint32 forwardBarrierDistance = 0x53;
    sint32 backwardsBarrierEndSlice = mySlice - 0x53;
    sint32 backwardsBarrierDistance = 0x53;
    if (forwardBarrierEndSlice >= gNumSlices)
        forwardBarrierEndSlice = (forwardBarrierEndSlice - gNumSlices);
    if (backwardsBarrierEndSlice < 0)
        backwardsBarrierEndSlice = (backwardsBarrierEndSlice + gNumSlices);
    sint32 currentBarrierLane = barrierLane;
    for (sint32 sliceLoop = 0; sliceLoop < 0x53; sliceLoop += 4)
    {
        sint32 sliceCheck = mySlice + sliceLoop;
        if (sliceCheck >= gNumSlices)
            sliceCheck = (sliceCheck - gNumSlices);
        sint32 laneLeft = (currentBarrierLane - 1);
        sint32 laneRight = (currentBarrierLane + 1);
        sint32 temp = AIWorld::CheckForBarrierBetweenLanes(sliceCheck, laneRight, laneLeft);
        sint32 leftBarrier = temp & 4;
        sint32 hereBarrier = temp & 2;
        sint32 rightBarrier = temp & 1;
        if (hereBarrier != 0)
            continue;
        if (leftBarrier != 0)
        {
            uint32 lanes = BWorldSm::slices[sliceCheck].laneCount;
            if (laneLeft >= 6 - (sint32)(lanes >> 4) && laneLeft <= (sint32)(lanes & 15U) + 7)
            {
                currentBarrierLane = laneLeft;
                continue;
            }
        }
        if (rightBarrier != 0)
        {
            uint32 lanes = BWorldSm::slices[sliceCheck].laneCount;
            if (laneRight >= 6 - (sint32)(lanes >> 4) && laneRight <= (sint32)(lanes & 15U) + 7)
            {
                currentBarrierLane = laneRight;
                continue;
            }
        }
        forwardBarrierEndSlice = sliceCheck;
        forwardBarrierDistance = sliceLoop;
        break;
    }
    currentBarrierLane = barrierLane;
    for (sint32 sliceLoop = 0; sliceLoop < 0x53; sliceLoop += 4)
    {
        sint32 sliceCheck = mySlice - sliceLoop;
        if (sliceLoop == 0)
        {
            if (sliceCheck >= gNumSlices)
                sliceCheck = (sliceCheck - gNumSlices);
        }
        else if (sliceCheck < 0)
            sliceCheck = (sliceCheck + gNumSlices);
        sint32 laneLeft = (currentBarrierLane - 1);
        sint32 laneRight = (currentBarrierLane + 1);
        sint32 temp = AIWorld::CheckForBarrierBetweenLanes(sliceCheck, laneRight, laneLeft);
        sint32 leftBarrier = temp & 4;
        sint32 hereBarrier = temp & 2;
        sint32 rightBarrier = temp & 1;
        if (hereBarrier != 0)
            continue;
        if (leftBarrier != 0)
        {
            uint32 lanes = BWorldSm::slices[sliceCheck].laneCount;
            if (laneLeft >= 6 - (sint32)(lanes >> 4) && laneLeft <= (sint32)(lanes & 15U) + 7)
            {
                currentBarrierLane = laneLeft;
                continue;
            }
        }
        if (rightBarrier != 0)
        {
            uint32 lanes = BWorldSm::slices[sliceCheck].laneCount;
            if (laneRight >= 6 - (sint32)(lanes >> 4) && laneRight <= (sint32)(lanes & 15U) + 7)
            {
                currentBarrierLane = laneRight;
                continue;
            }
        }
        backwardsBarrierEndSlice = sliceCheck;
        backwardsBarrierDistance = sliceLoop;
        break;
    }
    if (forwardBarrierDistance < backwardsBarrierDistance)
    {
        sint32 result = forwardBarrierEndSlice + 6;
        if (result >= gNumSlices)
            result = (forwardBarrierEndSlice - (gNumSlices - 6));
        return result;
    }
    sint32 result = backwardsBarrierEndSlice - 6;
    if (result < 0)
        result = ((backwardsBarrierEndSlice + gNumSlices) - 6);
    return result;
}

AIState_Offroad::AIState_Offroad(Car::tObj *carObj, sint32 startSlice, coorddef *position, matrixtdef *orientation, sint32 maxSpeedKPH, sint32 releaseTime, sint32 endSlice) : AIState_Base(carObj)
{
    FUNCTION_MARKER(0x80071120, "NFS4.EXE");
    letGo_ = 0;
    startSlice_ = startSlice;
    startPosition_ = *position;
    startOrientation_ = *orientation;
    startHeading_.x = startOrientation_.m[6];
    startHeading_.y = startOrientation_.m[7];
    startHeading_.z = startOrientation_.m[8];
    maxSpeedMPS_ = (maxSpeedKPH * 0x4700);
    targetSlice_ = endSlice;
    releaseTime_ = releaseTime;
    Trk::NewSlice *slice = &BWorldSm::slices[endSlice];
    targetPosition_.x = slice->center[0];
    targetPosition_.y = slice->center[1];
    targetPosition_.z = slice->center[2];
    carObj_->carFlags |= 0x800;
}

void AIState_Offroad::UnleashIfInRange(Car::tObj *car)
{
    FUNCTION_MARKER(0x80071268, "NFS4.EXE");
    sint32 distanceAbsMeters = AIWorld::SplineDistance(carObj_, car);
    if (distanceAbsMeters < 0)
        distanceAbsMeters = (-distanceAbsMeters);
    sint32 releaseDistanceMeters = fixedmult(car->N.speedXZ, releaseTime_);
    if (releaseDistanceMeters < 0x140000)
        releaseDistanceMeters = 0x140000;
    if (distanceAbsMeters < releaseDistanceMeters)
        letGo_ = 1;
}

void AIState_Offroad::Execute(void)
{
    FUNCTION_MARKER(0x800712E0, "NFS4.EXE");
    if (letGo_ == 0)
    {
        coorddef zero = { 0, 0, 0 };
        carObj_->N.linearVel = zero;
        carObj_->N.angularVel = zero;
        carObj_->desiredSpeed = 0;
        carObj_->N.position = startPosition_;
        carObj_->N.orientMat = startOrientation_;
        carObj_->targetPos.z = 0;
        carObj_->targetPos.y = 0;
        carObj_->targetPos.x = 0;
    }
    else
    {
        longMetersBetween_ = AIWorld::ApxSplineDistance(carObj_, targetSlice_);
        carObj_->desiredSpeed = maxSpeedMPS_;
        if (longMetersBetween_ <= 0)
            carObj_->desiredDirection = 1;
        else
        {
            carObj_->desiredDirection = -1;
            carObj_->desiredSpeed = (-carObj_->desiredSpeed);
        }
        AI::GenericBeginCycle(carObj_);
        AI::GenericCycle(carObj_);
        AI::GenericEndCycle(carObj_);
        carObj_->targetPos = targetPosition_;
    }
    carObj_->targetLatPos = 0;
}

AIState_NonActive::AIState_NonActive(Car::tObj *carObj) : AIState_Base(carObj)
{
    coorddef trafficOffset = { 0, 0, 0 };
    trafficOffset.y = (carObj->carIndex * 0xa0000);
    Newton::SetInitialSlicePositionOrientationEtc(&carObj_->N, 0, &trafficOffset, 1);
    carObj_->N.active = 0;
}

AIState_Purgatory::AIState_Purgatory(Car::tObj *carObj) : AIState_NonActive(carObj)
{
    FUNCTION_MARKER(0x8007148C, "NFS4.EXE");
    uint32 random = fastRandom * randSeed;
    carObj_->basisCar = Cars::gList[0];
    sint32 lifeTimer = AITune::LifeTimer[Cars::gNumTrafficCars];
    randtemp = random;
    carObj_->physicsModelTimer = (sint32)((lifeTimer * ((random >> 8) & 0xffffU)) >> 16) + 1;
    fastRandom = random & 0xffff;
    if (strcmp(carObj_->carName, "SBUS") == 0 || strcmp(carObj_->carName, "TBUS") == 0)
        carObj_->physicsModelTimer = (carObj_->physicsModelTimer * 5);
    carObj_->AIFlags |= 4;
    if ((carObj_->carFlags & 0x10) != 0)
        numTrafficCarsInPurgatory = (numTrafficCarsInPurgatory + 1);
}

AIState_Purgatory::~AIState_Purgatory(void)
{
    FUNCTION_MARKER(0x8007163C, "NFS4.EXE");
    carObj_->collision.resetTimer = 0;
    carObj_->N.collision.disableCollisionTimer = 0;
    carObj_->AIFlags &= ~4;
    if ((carObj_->carFlags & 0x10) != 0)
        numTrafficCarsInPurgatory = (numTrafficCarsInPurgatory - 1);
    sint32 search = Cars::gNumCars - 1;
    carObj_->direction = 1;
    carObj_->desiredDirection = 1;
    for (; search >= 0; --search)
    {
        Car::tObj *test = Cars::gSortedList[search];
        if ((test->carFlags & 0x100) != 0)
        {
            carObj_->basisCar = test;
            break;
        }
    }
}

AIState_Donuts::~AIState_Donuts(void)
{
    FUNCTION_MARKER(0x80071E0C, "NFS4.EXE");
    carObj_->targetPos.z = 0;
    carObj_->targetPos.y = 0;
    carObj_->targetPos.x = 0;
    carObj_->targetLatPos = 0;
    carObj_->donutMode = DONUTMODE_NONE;
}

void *AIState_Base::operator new(size_t size)
{
    return reservememadr(0, (sint32)size, 0);
}

void AIState_Base::operator delete(void *memory)
{
    purgememadr(memory);
}

sint32 AIState_Purgatory::TestForRelease(void)
{
    FUNCTION_MARKER(0x8007175C, "NFS4.EXE");
    if (carObj_->physicsModelTimer <= 0)
    {
        sint32 trafficInWorld = (Cars::gNumTrafficCars - numTrafficCarsInPurgatory);
        sint32 commMode = GameSetup::gData.commMode == 1 ? 1 : 0;
        if (trafficInWorld < AITune::MaxTraffic[commMode][GameSetup::gData.trafficDensity])
            return 1;
        carObj_->physicsModelTimer = 1;
    }
    return 0;
}

void AIState_Purgatory::Execute(void)
{
    FUNCTION_MARKER(0x800717D8, "NFS4.EXE");
    carObj_->collision.resetTimer = 0;
    carObj_->N.collision.disableCollisionTimer = 0;
    if ((carObj_->carFlags & 0x20) == 0 && simGlobal.gameTicks >= 960)
        carObj_->physicsModelTimer = (carObj_->physicsModelTimer - AI::elapsedTime);
}

void AIState_Purgatory::StartUp(void)
{
    FUNCTION_MARKER(0x80071840, "NFS4.EXE");
    numTrafficCarsInPurgatory = 0;
}

AIState_RovingTraffic::AIState_RovingTraffic(Car::tObj *carObj, trigger::trigger_t *trigger) : AIState_Base(carObj)
{
    FUNCTION_MARKER(0x8007184C, "NFS4.EXE");
    path_ = trigger->trafficPath.path;
    numPathPoints_ = trigger->trafficPath.numPoints;
    pathIndex_ = 0;
    waitTick_ = 0;
    carObj_->carFlags |= 0x800;
}

AIState_Base::~AIState_Base(void)
{
    FUNCTION_MARKER(0x8005B4CC, "NFS4.EXE");
}

void AIState_RovingTraffic::CheckIfCarIsNearbyAndStop(Car::tObj *otherCarObj, sint32 &status)
{
    FUNCTION_MARKER(0x800718B4, "NFS4.EXE");
    if (carObj_ == otherCarObj || otherCarObj->N.active == 0)
    {
        status = 2;
        return;
    }
    sint32 firstDistance = AIWorld::SplineDistance(carObj_, otherCarObj);
    sint32 distance = AIWorld::SplineDistance(carObj_, otherCarObj);
    if (firstDistance <= 0)
        distance = (-distance);
    if (distance > 0xc0000)
    {
        status = 0;
        return;
    }
    sint32 roadDifference = (carObj_->roadPosition - otherCarObj->roadPosition);
    if (roadDifference <= 0)
        roadDifference = (otherCarObj->roadPosition - carObj_->roadPosition);
    if (roadDifference > 0x9ffff)
    {
        status = 2;
        return;
    }
    coorddef posDiff;
    posDiff.x = (otherCarObj->N.position.x - carObj_->N.position.x);
    posDiff.y = (otherCarObj->N.position.y - carObj_->N.position.y);
    posDiff.z = (otherCarObj->N.position.z - carObj_->N.position.z);
    sint32 forwardX = fixedmult(carObj_->N.orientMat.m[6], posDiff.x);
    sint32 forwardY = fixedmult(carObj_->N.orientMat.m[7], posDiff.y);
    sint32 forwardZ = fixedmult(carObj_->N.orientMat.m[8], posDiff.z);
    sint32 dot = ((forwardX + forwardY) + forwardZ);
    if (dot > 0)
    {
        AudioClc::HonkHorn(carObj_, 4, 16, 8);
        carObj_->desiredSpeed = 0;
        status = 1;
    }
}

void AIState_RovingTraffic::Execute(void)
{
    FUNCTION_MARKER(0x80071A68, "NFS4.EXE");
    carObj_->targetPos = path_[pathIndex_].position;
    carObj_->desiredSpeed = (path_[pathIndex_].targetSpeed * 0x7247);
    Trk::NewSlice *slice = &BWorldSm::slices[carObj_->N.simRoadInfo.slice];
    coorddef centerBack;
    centerBack.x = slice->center[0];
    centerBack.y = slice->center[1];
    centerBack.z = slice->center[2];
    coorddef carRelativeForLatPos;
    carRelativeForLatPos.x = (carObj_->targetPos.x - centerBack.x);
    carRelativeForLatPos.y = (carObj_->targetPos.y - centerBack.y);
    carRelativeForLatPos.z = (carObj_->targetPos.z - centerBack.z);
    carObj_->targetLatPos = ( ((((carObj_->N.roadMatrix.m[0] / 256)) * (carRelativeForLatPos.x / 256)) + (((carObj_->N.roadMatrix.m[1] / 256)) * (carRelativeForLatPos.y / 256))) + (((carObj_->N.roadMatrix.m[2] / 256)) * (carRelativeForLatPos.z / 256)));
    coorddef carRelativeForDistance;
    carRelativeForDistance.x = (carObj_->targetPos.x - carObj_->N.position.x) >> 12;
    carRelativeForDistance.y = (carObj_->targetPos.y - carObj_->N.position.y);
    carRelativeForDistance.z = (carObj_->targetPos.z - carObj_->N.position.z) >> 12;
    sint32 distanceSquared = ( (carRelativeForDistance.x * carRelativeForDistance.x) + (carRelativeForDistance.z * carRelativeForDistance.z));
    if (distanceSquared < 10000)
    {
        if (waitTick_ == 0)
            waitTick_ = (simGlobal.gameTicks + path_[pathIndex_].waitTime);
        if (simGlobal.gameTicks < waitTick_)
            carObj_->desiredSpeed = 0;
        else
        {
            waitTick_ = 0;
            pathIndex_ = (pathIndex_ + 1);
        }
    }
    sint32 status = 2;
    sint32 search = carObj_->sortIndex + 1;
    while (search < Cars::gNumCars && status == 2)
    {
        Car::tObj *otherCarObj = Cars::gSortedList[search];
        ++search;
        CheckIfCarIsNearbyAndStop(otherCarObj, status);
    }
    if (status != 1)
    {
        status = 2;
        search = carObj_->sortIndex - 1;
        while (search >= 0 && status == 2)
        {
            Car::tObj *otherCarObj = Cars::gSortedList[search];
            --search;
            CheckIfCarIsNearbyAndStop(otherCarObj, status);
        }
    }
}

void AIState_Donuts::Execute(void)
{
    FUNCTION_MARKER(0x80071E70, "NFS4.EXE");
    sint32 slice = carObj_->N.simRoadInfo.slice;
    sint32 forwardDot = ( ((((carObj_->N.orientMat.m[6] / 256)) * (carObj_->N.roadMatrix.m[6] / 256)) + (((carObj_->N.orientMat.m[7] / 256)) * (carObj_->N.roadMatrix.m[7] / 256))) + (((carObj_->N.orientMat.m[8] / 256)) * (carObj_->N.roadMatrix.m[8] / 256)));
    carObj_->direction = forwardDot > 0 ? 1 : -1;
    carObj_->desiredDirection = carObj_->direction;
    coorddef targetPos;
    sint32 latPos;
    if (donutMode_ == DONUTMODE_GOCENTER || donutMode_ == DONUTMODE_BURNOUT)
    {
        sint32 forwardSlice = slice + 3;
        if (forwardDot < 0)
        {
            forwardSlice = slice - 3;
            if (forwardSlice < 0)
                forwardSlice = ((slice + gNumSlices) - 3);
        }
        else if (forwardSlice >= gNumSlices)
            forwardSlice = ((slice - gNumSlices) + 3);
        Trk::NewSlice *targetSlice = &BWorldSm::slices[forwardSlice];
        targetPos.x = targetSlice->center[0];
        targetPos.y = targetSlice->center[1];
        targetPos.z = targetSlice->center[2];
        sint32 dCarToCenter = carObj_->roadPosition;
        if (dCarToCenter < 0)
            dCarToCenter = (-dCarToCenter);
        latPos = 0;
        if (donutMode_ == DONUTMODE_GOCENTER && dCarToCenter < 0x28000)
            donutMode_ = DONUTMODE_DONUT;
    }
    else
    {
        donutLookForward_ = (donutLookForward_ - 0x10000);
        if (donutLookForward_ < 0x8000)
        {
            randtemp = fastRandom * randSeed;
            fastRandom = randtemp & 0xffff;
            donutLookForward_ = (((((randtemp >> 8) & 0xffffU) * 5U) >> 14) * 0x10000U + 0x140000U);
        }
        coorddef right;
        right.x = carObj_->N.orientMat.m[0];
        right.y = carObj_->N.orientMat.m[1];
        right.z = carObj_->N.orientMat.m[2];
        coorddef forward;
        forward.x = carObj_->N.orientMat.m[6];
        forward.y = carObj_->N.orientMat.m[7];
        forward.z = carObj_->N.orientMat.m[8];
        right.x = fixedmult(0x60000, right.x);
        right.y = fixedmult(0x60000, right.y);
        right.z = fixedmult(0x60000, right.z);
        forward.x = fixedmult(donutLookForward_, forward.x);
        forward.y = fixedmult(donutLookForward_, forward.y);
        forward.z = fixedmult(donutLookForward_, forward.z);
        targetPos.x = ((carObj_->N.position.x + right.x) + forward.x);
        targetPos.y = ((carObj_->N.position.y + right.y) + forward.y);
        targetPos.z = ((carObj_->N.position.z + right.z) + forward.z);
        latPos = Newton::CalculateRoadPositionFromSliceAndPosition(carObj_->N.simRoadInfo.slice, &targetPos, &carObj_->N.roadMatrix);
        Trk::NewSlice *roadSlice = &BWorldSm::slices[slice];
        sint32 roadPosition = carObj_->roadPosition;
        if (roadPosition < 0)
        {
            sint32 leftEdge = -(((roadSlice->avgPavedWidthLf) * 0x8000) * ((roadSlice->laneCount >> 4)));
            if ((roadPosition - 0x20000) < leftEdge)
                donutMode_ = DONUTMODE_GOCENTER;
        }
        else
        {
            sint32 rightEdge = (((uint8)roadSlice->avgPavedWidthRt) * 0x8000U * (roadSlice->laneCount & 15U));
            if (rightEdge < (roadPosition + 0x20000))
                donutMode_ = DONUTMODE_GOCENTER;
        }
    }
    carObj_->donutMode = donutMode_;
    AI::GenericBeginCycle(carObj_);
    AI::GenericCycle(carObj_);
    AI::GenericEndCycle(carObj_);
    carObj_->targetPos = targetPos;
    carObj_->targetLatPos = latPos;
    carObj_->desiredSpeed = 0x471c7;
}

AIState_GotoSlice::AIState_GotoSlice(Car::tObj *carObj, sint32 targetSlice, sint32 stopWhenArrivedAtSlice) : AIState_Normal(carObj)
{
    FUNCTION_MARKER(0x8007236C, "NFS4.EXE");
    targetSlice_ = targetSlice;
    stopWhenArrivedAtSlice_ = stopWhenArrivedAtSlice;
}

void AIState_GotoSlice::Execute(void)
{
    FUNCTION_MARKER(0x800723C0, "NFS4.EXE");
    sint32 longMetersBetween = AIWorld::ApxSplineDistance(targetSlice_, carObj_);
    carObj_->desiredDirection = longMetersBetween < 0 ? -1 : 1;
    AIState_Normal::Execute();
    if (stopWhenArrivedAtSlice_ != 0)
    {
        sint32 distMeters = longMetersBetween;
        if (distMeters < 0)
            distMeters = (-distMeters);
        sint32 cap = 0xc80000;
        if (distMeters < 0xc0000)
            cap = 0x40000;
        else if (distMeters < 0x320000)
            cap = 0x140000;
        else if (distMeters < 0x960000)
            cap = 0x280000;
        else if (distMeters < 0x1900000)
            cap = 0x500000;
        if (carObj_->desiredSpeed < 0)
        {
            cap = -cap;
            if (cap < carObj_->desiredSpeed)
                cap = carObj_->desiredSpeed;
        }
        else if (carObj_->desiredSpeed < cap)
            cap = carObj_->desiredSpeed;
        carObj_->desiredSpeed = cap;
    }
}

sint32 AIState_GotoSlice::InTargetSliceRange(sint32 rangeMeters)
{
    FUNCTION_MARKER(0x800724D8, "NFS4.EXE");
    sint32 distanceMeters = AIWorld::ApxSplineDistance(carObj_, targetSlice_);
    if (distanceMeters < 0)
        distanceMeters = (-distanceMeters);
    return distanceMeters < rangeMeters;
}

AIState_Cruise::AIState_Cruise(Car::tObj *carObj, cruiseMode_t cruiseMode, sint32 speedOrFactor) : AIState_Normal(carObj)
{
    FUNCTION_MARKER(0x8007251C, "NFS4.EXE");
    cruiseMode_ = cruiseMode;
    if (cruiseMode == CRUISE_ATSETSPEED)
        cruiseSpeed_ = speedOrFactor;
    else if (cruiseMode == CRUISE_ATFACTOR)
        cruiseFactor_ = speedOrFactor;
}

void AIState_Cruise::Execute(void)
{
    FUNCTION_MARKER(0x80072588, "NFS4.EXE");
    if (cruiseMode_ == CRUISE_ATFACTOR)
    {
        AISpeeds::CalcDesiredSpeed(carObj_);
        carObj_->desiredSpeed = fixedmult(carObj_->desiredSpeed, cruiseFactor_);
    }
    else if (cruiseMode_ == CRUISE_ATSETSPEED)
        carObj_->desiredSpeed = (cruiseSpeed_ * carObj_->direction);
    else if (cruiseMode_ == CRUISE_ATTRAFFICSPEED)
        AISpeeds::CalcTrafficTopSpeed(carObj_);
    AI::GenericBeginCycle(carObj_);
    AI::GenericCycle(carObj_);
    AI::GenericEndCycle(carObj_);
}

sint32 AIState_Base::TestForRelease(void)
{
    FUNCTION_MARKER(0x8005B4C4, "NFS4.EXE");
    return 0;
}

AIState_None::AIState_None(Car::tObj *carObj) : AIState_Base(carObj)
{
}

AIState_None::~AIState_None(void)
{
    FUNCTION_MARKER(0x8005B490, "NFS4.EXE");
}

AIState_Normal::~AIState_Normal(void)
{
    FUNCTION_MARKER(0x800727FC, "NFS4.EXE");
}

AIState_Idle::~AIState_Idle(void)
{
    FUNCTION_MARKER(0x800727A4, "NFS4.EXE");
    carObj_->carFlags &= ~0x400;
}

AIState_NonActive::~AIState_NonActive(void)
{
    FUNCTION_MARKER(0x8005F62C, "NFS4.EXE");
    carObj_->N.active = 1;
}

AIState_RovingTraffic::~AIState_RovingTraffic(void)
{
    FUNCTION_MARKER(0x800726DC, "NFS4.EXE");
    carObj_->targetPos.z = 0;
    carObj_->targetPos.y = 0;
    carObj_->targetPos.x = 0;
    carObj_->targetLatPos = 0;
    carObj_->carFlags &= ~0x800;
}

AIState_GotoSlice::~AIState_GotoSlice(void)
{
    FUNCTION_MARKER(0x80072690, "NFS4.EXE");
}

AIState_Cruise::~AIState_Cruise(void)
{
    FUNCTION_MARKER(0x8007265C, "NFS4.EXE");
}

void AIState_None::Execute(void)
{
    FUNCTION_MARKER(0x8005B488, "NFS4.EXE");
}

void AIState_NonActive::Execute(void)
{
    FUNCTION_MARKER(0x8005F624, "NFS4.EXE");
}

sint32 AIState_RovingTraffic::TestForRelease(void)
{
    FUNCTION_MARKER(0x800726C4, "NFS4.EXE");
    return pathIndex_ >= numPathPoints_;
}
