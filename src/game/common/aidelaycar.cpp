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
    deltaPosition_.x = (basisCar_->N.position.x - targetCar_->N.position.x);
    deltaPosition_.y = (basisCar_->N.position.y - targetCar_->N.position.y);
    deltaPosition_.z = (basisCar_->N.position.z - targetCar_->N.position.z);
    position_ = targetCar_->N.position;
    deltaRoadPosition_ = (basisCar_->roadPosition - targetCar_->roadPosition);
    roadPosition_ = targetCar_->roadPosition;
    laneIndex_ = targetCar_->laneIndex;
    currentSpeed_ = targetCar_->currentSpeed;
}

void AIDelayCar::Update(void)
{
    FUNCTION_MARKER(0x8005AB44, "NFS4.EXE");
    sint32 distance = AIWorld::ApxSplineDistance(targetCar_, basisCar_);
    sint32 change = fixedmult(((distance - deltaMeters_)), delayFactor_);
    deltaMeters_ = (deltaMeters_ + change);
    sint32 sliceOffset = deltaMeters_ / 0x60000;
    sint32 slice = (basisCar_->N.simRoadInfo.slice + sliceOffset);
    if (sliceOffset >= 0)
    {
        if (slice >= gNumSlices)
            slice = (slice - gNumSlices);
    }
    else if (slice < 0)
        slice = (slice + gNumSlices);
    slice_ = slice;
    coorddef currentDeltaPosition;
    currentDeltaPosition.x = (targetCar_->N.position.x - basisCar_->N.position.x);
    currentDeltaPosition.y = (targetCar_->N.position.y - basisCar_->N.position.y);
    currentDeltaPosition.z = (targetCar_->N.position.z - basisCar_->N.position.z);
    coorddef changeDeltaPosition;
    changeDeltaPosition.x = (currentDeltaPosition.x - deltaPosition_.x);
    changeDeltaPosition.y = (currentDeltaPosition.y - deltaPosition_.y);
    changeDeltaPosition.z = (currentDeltaPosition.z - deltaPosition_.z);
    change = fixedmult(changeDeltaPosition.x, delayFactor_);
    deltaPosition_.x = (deltaPosition_.x + change);
    change = fixedmult(changeDeltaPosition.y, delayFactor_);
    deltaPosition_.y = (deltaPosition_.y + change);
    change = fixedmult(changeDeltaPosition.z, delayFactor_);
    deltaPosition_.z = (deltaPosition_.z + change);
    position_.x = (basisCar_->N.position.x + deltaPosition_.x);
    position_.y = (basisCar_->N.position.y + deltaPosition_.y);
    position_.z = (basisCar_->N.position.z + deltaPosition_.z);
    sint32 currentDeltaRoadPosition = (targetCar_->roadPosition - basisCar_->roadPosition);
    change = fixedmult(((currentDeltaRoadPosition - deltaRoadPosition_)), delayFactor_);
    deltaRoadPosition_ = (deltaRoadPosition_ + change);
    roadPosition_ = (basisCar_->roadPosition + deltaRoadPosition_);
    laneIndex_ = AIWorld::LaneIndex(slice_, roadPosition_);
    change = fixedmult(((targetCar_->currentSpeed - currentSpeed_)), delayFactor_);
    currentSpeed_ = (currentSpeed_ + change);
}
