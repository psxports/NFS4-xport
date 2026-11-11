#include <new>
#include <stddef.h>
#include "xport.h"
#include "game/common/aihigh.h"
#include "game/common/ailife.h"
#include "game/common/aistate.h"
#include "game/common/aitriger.h"
#include "game/common/aiworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/fastrand.h"
#include "game/common/newton.h"
#include "game/common/scene.h"
#include "game/common/trk.h"

Car::tObj *AIHigh_Traffic::CheckForCops(sint32 *closestDistance)
{
    FUNCTION_MARKER(0x80065C8C, "NFS4.EXE");
    Car::tObj *closestCop = 0;
    *closestDistance = 0x4E200000;
    for (sint32 copLoop = 0; copLoop < Cars::gNumCopCars; ++copLoop) {
        Car::tObj *cop = Cars::gCopCarList[copLoop];
        if ((cop->AIFlags & 4) == 0) {
            sint32 sliceDistance = AIWorld::ApxSplineDistance(carObj_, cop);
            if (sliceDistance < 0)
                sliceDistance = (sint32)(0 - (uint32)sliceDistance);
            if (sliceDistance < *closestDistance) {
                closestCop = cop;
                *closestDistance = sliceDistance;
            }
        }
    }
    return closestCop;
}

AIHigh_Cop *AIHigh_Traffic::CopCheck(sint32 *blockade)
{
    FUNCTION_MARKER(0x80065D60, "NFS4.EXE");
    *blockade = 0;
    if (Cars::gNumCopCars == 0)
        return 0;
    sint32 closestDistance;
    Car::tObj *closest = CheckForCops(&closestDistance);
    if (!closest)
        return 0;
    sint32 speed = closest->currentSpeed;
    if (speed < 0)
        speed = (sint32)(0 - (uint32)speed);
    if (speed < 0x20000 && closestDistance < 0x4B0000) {
        *blockade = 1;
        AIHigh_Cop *cop = static_cast<AIHigh_Cop *>(highLevelAIObjs[closest->carIndex]);
        return cop->perpTarget_ ? cop : 0;
    }
    if (speed > 0x20000 && closestDistance < 0x4B0000)
        return static_cast<AIHigh_Cop *>(highLevelAIObjs[closest->carIndex]);
    return 0;
}

void AIHigh_Traffic::HighExecute(void)
{
    FUNCTION_MARKER(0x80065E6C, "NFS4.EXE");
    carObj_->unlap = 0;
    carObj_->lap = 0;
    switch (stateType_)
    {
    case STATE_NONE:
    {
        coorddef trafficOffset = { 0, 6553600, 0 };
        AIState_Base *newState;
        stateType_t newType;
        if ((carObj_->carFlags & 0x400) != 0)
        {
            void *storage = AIState_Base::operator new(sizeof(AIState_Idle));
            newState = ::new (storage) AIState_Idle(carObj_);
            newType = STATE_IDLE;
        }
        else
        {
            void *storage = AIState_Base::operator new(sizeof(AIState_Purgatory));
            newState = ::new (storage) AIState_Purgatory(carObj_);
            newType = STATE_PURGATORY;
        }
        AIState_Base *oldState = state_;
        if (oldState)
            delete oldState;
        state_ = newState;
        stateType_ = newType;
        Newton::SetInitialSlicePositionOrientationEtc(&carObj_->N, 0, &trafficOffset, 1);
        return;
    }
    case STATE_PURGATORY:
    {
        if (accidentData_ != 0)
        {
            void *storage = AIState_Base::operator new(sizeof(AIState_Idle));
            AIState_Base *newState = ::new (storage) AIState_Idle(carObj_);
            AIState_Base *oldState = state_;
            if (oldState)
                delete oldState;
            state_ = newState;
            stateType_ = STATE_IDLE;
            BWorldSm::Pos spos;
            spos.slice = 0;
            BWorldSm::FindClosestSlice(&accidentData_->cp, &spos);
            SceneElem *accident = accidentData_;
            AILife::ReencarnateTrafficByPosition(carObj_, spos.slice, 1, &accident->cp, &accident->orient);
            Car::tObj *car = carObj_;
            car->carFlags |= 0x400;
            accidentData_ = 0;
        }
        else if (state_->TestForRelease() != 0 && forcePurgatory_ == 0)
        {
            trigger::trigger_t *pNewTrigger = CheckForNewTriggers();
            if (pNewTrigger == 0)
            {
                void *storage = AIState_Base::operator new(sizeof(AIState_Normal));
                AIState_Base *newState = ::new (storage) AIState_Normal(carObj_);
                AIState_Base *oldState = state_;
                if (oldState)
                    delete oldState;
                state_ = newState;
                stateType_ = STATE_NORMAL;
                AILife::ReencarnateTraffic(carObj_);
            }
            else
            {
                triggerManagerTraffic->DescribeTrigger(pNewTrigger);
                if (pNewTrigger->any.type == TRIGGER_TRAFFIC_PATH)
                {
                    void *storage = AIState_Base::operator new(sizeof(AIState_RovingTraffic));
                    AIState_Base *newState = ::new (storage) AIState_RovingTraffic(carObj_, pNewTrigger);
                    AIState_Base *oldState = state_;
                    if (oldState)
                        delete oldState;
                    stateType_ = STATE_ROVING_TRAFFIC;
                    state_ = newState;
                    AILife::ReencarnateTrafficByPosition(carObj_, pNewTrigger->any.slice, 1,
                        &pNewTrigger->trafficPath.path->position, &pNewTrigger->trafficPath.orientation);
                }
            }
        }
        break;
    }
    case STATE_NORMAL:
    {
        if (AILife::EvaluateLife(carObj_) != 0 || forcePurgatory_ != 0)
            goto makePurgatory;
        sint32 blockade;
        AIHigh_Cop *cop = CopCheck(&blockade);
        if (cop == 0)
        {
            ignoreCops_ = 0;
            break;
        }
        if (ignoreCops_ != 0)
            break;
        uint32 product = fastRandom * randSeed;
        randtemp = product;
        fastRandom = product & 0xFFFF;
        sint32 cRand = (((product >> 8) & 0xFFFF) * 5) >> 15;
        if (blockade == 0 && cRand == 0)
        {
            void *storage = AIState_Base::operator new(sizeof(AIState_Idle));
            AIState_Base *newState = ::new (storage) AIState_Idle(carObj_);
            AIState_Base *oldState = state_;
            if (oldState)
                delete oldState;
            state_ = newState;
            stateType_ = STATE_IDLE;
        }
        else if (blockade != 0 || cRand < 8)
        {
            sint32 slice = carObj_->N.simRoadInfo.slice;
            void *storage = AIState_Base::operator new(sizeof(AIState_Idle));
            AIState_Idle *idleState = ::new (storage) AIState_Idle(carObj_);
            AIState_Base *oldState = state_;
            if (oldState)
                delete oldState;
            Car::tObj *car = carObj_;
            state_ = idleState;
            stateType_ = STATE_IDLE;
            Trk::NewSlice *roadSlice = BWorldSm::slices + slice;
            sint32 idlePosition;
            if (car->direction == 1)
                idlePosition = sint32(roadSlice->avgPavedWidthRt) * 32768 * (roadSlice->laneCount & 15);
            else
                idlePosition = sint32(roadSlice->avgPavedWidthLf) * 32768 * (roadSlice->laneCount >> 4);
            idleState->SetIdlePosition(idlePosition);
        }
        else
            ignoreCops_ = 1;
        break;
    }
    case STATE_IDLE:
    {
        if (AILife::EvaluateLife(carObj_) != 0)
            goto makePurgatory;
        sint32 blockade;
        if (CopCheck(&blockade) == 0 && (carObj_->carFlags & 0x400) == 0)
            goto makeNormal;
        break;
    }
    case STATE_ROVING_TRAFFIC:
        if (AILife::EvaluateLife(carObj_) != 0 || forcePurgatory_ != 0)
            goto makePurgatory;
        if (state_->TestForRelease() != 0)
            goto makeNormal;
        break;
    default:
        break;
    }
    state_->StateExecute();
    return;

makePurgatory:
    {
        void *storage = AIState_Base::operator new(sizeof(AIState_Purgatory));
        AIState_Base *newState = ::new (storage) AIState_Purgatory(carObj_);
        AIState_Base *oldState = state_;
        if (oldState)
            delete oldState;
        state_ = newState;
        stateType_ = STATE_PURGATORY;
    }
    state_->StateExecute();
    return;

makeNormal:
    {
        void *storage = AIState_Base::operator new(sizeof(AIState_Normal));
        AIState_Base *newState = ::new (storage) AIState_Normal(carObj_);
        AIState_Base *oldState = state_;
        if (oldState)
            delete oldState;
        state_ = newState;
        stateType_ = STATE_NORMAL;
    }
    state_->StateExecute();
}

AIHigh_Traffic::AIHigh_Traffic(Car::tObj *carObj) : AIHigh_Base(carObj)
{
    FUNCTION_MARKER(0x800666F8, "NFS4.EXE");
    ignoreCops_ = 0;
    forcePurgatory_ = 0;
    accidentData_ = 0;
}

trigger::trigger_t *AIHigh_Traffic::CheckForNewTriggers(void)
{
    FUNCTION_MARKER(0x80066738, "NFS4.EXE");
    sint32 sortedLoop = (sint32)((uint32)Cars::gNumCars - 1);
    for (; sortedLoop >= 0; --sortedLoop) {
        Car::tObj *testCar = Cars::gTotalSortedList[sortedLoop];
        if ((testCar->carFlags & 0x204) == 0)
            continue;
        AIHigh_Base *thisPlayer = highLevelAIObjs[testCar->carIndex];
        sint32 dir = testCar->currentSpeed < 0 ? -1 : 1;
        sint32 thisSlice = testCar->N.simRoadInfo.slice + dir * 45;
        if (dir > 0) {
            if (thisSlice >= gNumSlices)
                thisSlice = (sint32)((uint32)thisSlice - (uint32)gNumSlices);
        } else if (thisSlice < 0) {
            thisSlice = (sint32)((uint32)thisSlice + (uint32)gNumSlices);
        }
        sint32 oldSlice = thisPlayer->lastTrafficTriggerCheckSlice_;
        thisPlayer->lastTrafficTriggerCheckSlice_ = thisSlice;
        sint32 startSlice;
        sint32 endSlice;
        if (oldSlice < thisSlice) {
            startSlice = oldSlice;
            endSlice = thisSlice;
        } else {
            startSlice = thisSlice;
            endSlice = oldSlice;
        }
        sint32 span = (sint32)((uint32)endSlice - (uint32)startSlice);
        for (sint32 sliceLoop = startSlice; sliceLoop < endSlice && span < 50; ++sliceLoop) {
            sint32 triggerHere = triggerManagerTraffic->CheckForTriggerAtSlice(testCar->carIndex, sliceLoop);
            if (triggerHere != -1) {
                sint32 iRandomChance = 5000;
                uint32 product = fastRandom * randSeed;
                randtemp = product;
                fastRandom = product & 0xFFFF;
                sint32 randomValue = (((product >> 8) & 0xFFFF) * 25) >> 14;
                if (!AILife::IsSliceInAnyVisibleArea(sliceLoop) && randomValue < iRandomChance) {
                    sint32 unused;
                    return triggerManagerTraffic->GetTrigger(triggerHere, &unused);
                }
            }
        }
    }
    return 0;
}
