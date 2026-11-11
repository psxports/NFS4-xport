#include "xport.h"
#include "game/common/aistate.h"
#include "game/common/aiworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "lib/psx/fixdmult.h"

AIDelayCar::AIDelayCar(Car::tObj *basisCar, Car::tObj *targetCar, sint32 delayFactor)
{
    FUNCTION_MARKER(0x8005AA28, "NFS4.EXE");
    basisCar_ = basisCar;
    delayFactor_ = delayFactor;
    SetNewTargetCar(targetCar);
}

void AIDelayCar::SetNewTargetCar(Car::tObj *targetCar)
{
    FUNCTION_MARKER(0x8005AA5C, "NFS4.EXE");
    targetCar_ = targetCar;
    deltaMeters_ = AIWorld::ApxSplineDistance(basisCar_, targetCar);
    slice_ = targetCar_->N.simRoadInfo.slice;
    deltaPosition_.x = (sint32)((uint32)basisCar_->N.position.x - (uint32)targetCar_->N.position.x);
    deltaPosition_.y = (sint32)((uint32)basisCar_->N.position.y - (uint32)targetCar_->N.position.y);
    deltaPosition_.z = (sint32)((uint32)basisCar_->N.position.z - (uint32)targetCar_->N.position.z);
    position_ = targetCar_->N.position;
    deltaRoadPosition_ = (sint32)((uint32)basisCar_->roadPosition - (uint32)targetCar_->roadPosition);
    roadPosition_ = targetCar_->roadPosition;
    laneIndex_ = targetCar_->laneIndex;
    currentSpeed_ = targetCar_->currentSpeed;
}

void AIDelayCar::Update(void)
{
    FUNCTION_MARKER(0x8005AB44, "NFS4.EXE");
    sint32 distance = AIWorld::ApxSplineDistance(targetCar_, basisCar_);
    sint32 change = fixedmult((sint32)((uint32)distance - (uint32)deltaMeters_), delayFactor_);
    deltaMeters_ = (sint32)((uint32)deltaMeters_ + (uint32)change);
    sint32 sliceOffset = deltaMeters_ / 0x60000;
    sint32 slice = (sint32)((uint32)basisCar_->N.simRoadInfo.slice + (uint32)sliceOffset);
    if (sliceOffset >= 0)
    {
        if (slice >= gNumSlices)
            slice = (sint32)((uint32)slice - (uint32)gNumSlices);
    }
    else if (slice < 0)
        slice = (sint32)((uint32)slice + (uint32)gNumSlices);
    slice_ = slice;
    coorddef currentDeltaPosition;
    currentDeltaPosition.x = (sint32)((uint32)targetCar_->N.position.x - (uint32)basisCar_->N.position.x);
    currentDeltaPosition.y = (sint32)((uint32)targetCar_->N.position.y - (uint32)basisCar_->N.position.y);
    currentDeltaPosition.z = (sint32)((uint32)targetCar_->N.position.z - (uint32)basisCar_->N.position.z);
    coorddef changeDeltaPosition;
    changeDeltaPosition.x = (sint32)((uint32)currentDeltaPosition.x - (uint32)deltaPosition_.x);
    changeDeltaPosition.y = (sint32)((uint32)currentDeltaPosition.y - (uint32)deltaPosition_.y);
    changeDeltaPosition.z = (sint32)((uint32)currentDeltaPosition.z - (uint32)deltaPosition_.z);
    change = fixedmult(changeDeltaPosition.x, delayFactor_);
    deltaPosition_.x = (sint32)((uint32)deltaPosition_.x + (uint32)change);
    change = fixedmult(changeDeltaPosition.y, delayFactor_);
    deltaPosition_.y = (sint32)((uint32)deltaPosition_.y + (uint32)change);
    change = fixedmult(changeDeltaPosition.z, delayFactor_);
    deltaPosition_.z = (sint32)((uint32)deltaPosition_.z + (uint32)change);
    position_.x = (sint32)((uint32)basisCar_->N.position.x + (uint32)deltaPosition_.x);
    position_.y = (sint32)((uint32)basisCar_->N.position.y + (uint32)deltaPosition_.y);
    position_.z = (sint32)((uint32)basisCar_->N.position.z + (uint32)deltaPosition_.z);
    sint32 currentDeltaRoadPosition = (sint32)((uint32)targetCar_->roadPosition - (uint32)basisCar_->roadPosition);
    change = fixedmult((sint32)((uint32)currentDeltaRoadPosition - (uint32)deltaRoadPosition_), delayFactor_);
    deltaRoadPosition_ = (sint32)((uint32)deltaRoadPosition_ + (uint32)change);
    roadPosition_ = (sint32)((uint32)basisCar_->roadPosition + (uint32)deltaRoadPosition_);
    laneIndex_ = AIWorld::LaneIndex(slice_, roadPosition_);
    change = fixedmult((sint32)((uint32)targetCar_->currentSpeed - (uint32)currentSpeed_), delayFactor_);
    currentSpeed_ = (sint32)((uint32)currentSpeed_ + (uint32)change);
}
