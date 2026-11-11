#include <new>
#include <stddef.h>
#include "xport.h"
#include "game/common/ai.h"
#include "game/common/aihigh.h"
#include "game/common/aistate.h"
#include "game/common/aiworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/collide.h"
#include "game/common/fastrand.h"
#include "game/common/sim.h"

void AIHigh_Opponent::CheckForWipeOut(void)
{
    FUNCTION_MARKER(0x80063308, "NFS4.EXE");
    sint32 oppLevel = perpChaseInfo_.chaseLevelIndex_;
    sint32 oppFines = carObj_->stats.numFines;
    if (Cars::gNumCopCars == 0 || carObj_->wipeOutEndTick > simGlobal.gameTicks)
        return;
    if (basicPerpInfo_.crime_ == CRIME_NONE ||
        (uint32)basicPerpInfo_.copsAssigned_[0] + (uint32)basicPerpInfo_.copsAssigned_[1] == 0)
        return;
    sint32 timeNow = simGlobal.gameTicks;
    sint32 elapsed = (sint32)((uint32)timeNow - (uint32)carObj_->wipeOutEndTick);
    if (elapsed < 640)
        return;
    uint32 product = fastRandom * randSeed;
    sint32 perTickProb = (sint32)((uint32)AI::elapsedTime * 3);
    randtemp = product;
    sint32 randVal = (product >> 8) & 0xFFFF;
    fastRandom = product & 0xFFFF;
    if (randVal < perTickProb)
        carObj_->wipeOutEndTick = (uint32)timeNow + 192;
    sint32 lastLevel = (sint32)((uint32)perpChaseInfo_.copGameInfo_->numLevels - 1);
    if (perpChaseInfo_.bestChaseLevelIndex_ == lastLevel)
        return;
    sint32 numHumans = Cars::gNumHumanRaceCars;
    perTickProb = (sint32)((uint32)AI::elapsedTime * 116);
    for (sint32 hLoop = 0; hLoop < numHumans; ++hLoop) {
        Car::tObj *thisPlayerObj = Cars::gHumanRaceCarList[hLoop];
        sint32 speed = thisPlayerObj->currentSpeed;
        if (speed < 0)
            speed = (sint32)(0 - (uint32)speed);
        AIHigh_Player *thisPlayer = static_cast<AIHigh_Player *>(highLevelAIObjs[thisPlayerObj->carIndex]);
        sint32 playFines = thisPlayerObj->stats.numFines;
        sint32 fineDifference = (sint32)((uint32)playFines - (uint32)oppFines);
        if (speed > 0xD5554 &&
            ((thisPlayer->perpChaseInfo_.chaseLevelIndex_ < 2 && oppLevel >= 3) || fineDifference >= 2) &&
            randVal < perTickProb)
            carObj_->wipeOutEndTick = (uint32)simGlobal.gameTicks + 192;
    }
}

sint32 AIHigh_Opponent::DoRearEnder(void)
{
    FUNCTION_MARKER(0x800634E8, "NFS4.EXE");
    sint32 attackIndex = AIScript::DoReAction(&carObj_->script, AIScript::AISCRIPT_AIREACTION_REAR_END);
    if (attackIndex != -1) {
        Car::tObj *otherCarObj = Cars::gList[attackIndex];
        sint32 distance = AIWorld::SplineDistance(otherCarObj, carObj_);
        Car::tObj *car = carObj_;
        sint32 longDistance = (sint32)((uint32)distance * (uint32)car->direction);
        sint32 latDistance = (sint32)((uint32)car->roadPosition - (uint32)otherCarObj->roadPosition);
        if (latDistance < 0)
            latDistance = (sint32)(0 - (uint32)latDistance);
        if ((uint32)longDistance - 0x10001 <= 0x26FFFE && latDistance < longDistance * 2) {
            sint32 speed = otherCarObj->currentSpeed;
            if (speed < 0)
                speed = (sint32)(0 - (uint32)speed);
            if (speed > 0xB1C71)
                return attackIndex;
        }
    }
    Car::tObj *car = carObj_;
    if (car->N.simOptz != 0)
        return -1;
    sint32 speed = car->currentSpeed;
    if (speed < 0)
        speed = (sint32)(0 - (uint32)speed);
    if (speed <= 0x140000)
        return -1;
    for (sint32 racerLoop = 0; racerLoop < Cars::gNumHumanRaceCars; ++racerLoop) {
        Car::tObj *otherCarObj = Cars::gHumanRaceCarList[racerLoop];
        Trk::NewSlice *road = &BWorldSm::slices[otherCarObj->N.simRoadInfo.slice];
        sint32 left = -(road->avgPavedWidthLf * 0x8000 * (road->laneCount >> 4));
        sint32 right = road->avgPavedWidthRt * 0x8000 * (road->laneCount & 15);
        sint32 roadPosition = otherCarObj->roadPosition;
        if (roadPosition < left || roadPosition > right)
            continue;
        sint32 distance = AIWorld::SplineDistance(otherCarObj, carObj_);
        car = carObj_;
        sint32 longDistance = (sint32)((uint32)distance * (uint32)car->direction);
        sint32 latDistance = (sint32)((uint32)car->roadPosition - (uint32)otherCarObj->roadPosition);
        if (latDistance < 0)
            latDistance = (sint32)(0 - (uint32)latDistance);
        if ((uint32)longDistance - 0x10001 <= 0x26FFFE && latDistance < longDistance * 2) {
            uint32 mask = car->personality->rearBumpProbMask;
            uint32 phase = (uint32)simGlobal.gameTicks + (uint32)car->carIndex * 123;
            if ((phase & mask) == mask)
                return otherCarObj->carIndex;
        }
        if ((uint32)longDistance + 0x3FFFF <= 0x7FFFE) {
            car = carObj_;
            uint32 mask = car->personality->smackProbMask;
            uint32 phase = (uint32)simGlobal.gameTicks + (uint32)car->carIndex * 123;
            if ((phase & mask) == mask)
                return otherCarObj->carIndex;
        }
    }
    return -1;
}

void AIHigh_Opponent::HighExecute(void)
{
    FUNCTION_MARKER(0x800637BC, "NFS4.EXE");
    switch (stateType_) {
    case STATE_NONE: {
        void *storage = AIState_Base::operator new(sizeof(AIState_Normal));
        AIState_Base *newState = ::new (storage) AIState_Normal(carObj_);
        AIState_Base *oldState = state_;
        if (oldState)
            delete oldState;
        state_ = newState;
        stateType_ = STATE_NORMAL;
        return;
    }
    case STATE_NORMAL: {
        HandleCops();
        CheckForWipeOut();
        sint32 attackIndex = AIScript::DoReAction(&carObj_->script, AIScript::AISCRIPT_AIREACTION_ATTACK);
        if (attackIndex != -1) {
            attackMode_ = AIHigh::SCRIPT_ATTACK;
        } else {
            attackIndex = DoRearEnder();
            if (attackIndex != -1) {
                attackMode_ = AIHigh::REAR_END;
            } else {
                attackIndex = DoProvokedAttack();
                if (attackIndex != -1)
                    attackMode_ = AIHigh::PROVOKED_ATTACK;
            }
        }
        if (attackMode_ != AIHigh::NO_ATTACK) {
            coorddef pos = { 0, 0, 0 };
            sint32 aggression = (sint32)((uint32)attackMode_ - 1);
            if (aggression < 2)
                aggression = 2;
            void *storage = AIState_Base::operator new(sizeof(AIState_Chase));
            AIState_Base *newState = ::new (storage) AIState_Chase(carObj_, Cars::gList[attackIndex], &pos,
                32, 0x960000, 0x960000, aggression, 0x10000);
            AIState_Base *oldState = state_;
            if (oldState)
                delete oldState;
            state_ = newState;
            stateType_ = STATE_CHASE;
        }
        break;
    }
    case STATE_CHASE: {
        AIState_Chase *attackState = static_cast<AIState_Chase *>(state_);
        HandleCops();
        CheckForWipeOut();
        attackState->SetMurderMode(1, 15);
        switch (attackMode_) {
        case AIHigh::SCRIPT_ATTACK:
            if (AIScript::DoReAction(&carObj_->script, AIScript::AISCRIPT_AIREACTION_ATTACK) == -1)
                attackMode_ = AIHigh::NO_ATTACK;
            break;
        case AIHigh::REAR_END:
            if (DoRearEnder() == -1)
                attackMode_ = AIHigh::NO_ATTACK;
            break;
        case AIHigh::PROVOKED_ATTACK:
            attackTicksLeft_ = (sint32)((uint32)attackTicksLeft_ - (uint32)AI::elapsedTime);
            if (attackTicksLeft_ <= 0)
                attackMode_ = AIHigh::NO_ATTACK;
            break;
        default:
            break;
        }
        if (attackMode_ == AIHigh::NO_ATTACK) {
            carObj_->desiredDirection = GameSetup::gData.reverseTrack ? -1 : 1;
            void *storage = AIState_Base::operator new(sizeof(AIState_Normal));
            AIState_Base *newState = ::new (storage) AIState_Normal(carObj_);
            AIState_Base *oldState = state_;
            if (oldState)
                delete oldState;
            state_ = newState;
            stateType_ = STATE_NORMAL;
            attackMode_ = AIHigh::NO_ATTACK;
        }
        break;
    }
    default:
        break;
    }
    state_->StateExecute();
}

AIHigh_Opponent::AIHigh_Opponent(Car::tObj *carObj) : AIHigh_Player(carObj)
{
    FUNCTION_MARKER(0x80063AE8, "NFS4.EXE");
    attackMode_ = AIHigh::NO_ATTACK;
    lastHumanHitter_ = 0;
    hitCount_ = 0;
    attackTicksLeft_ = 0;
}

sint32 AIHigh_Opponent::DoProvokedAttack(void)
{
    FUNCTION_MARKER(0x80063B2C, "NFS4.EXE");
    Car::tObj *carObj = carObj_;
    sint32 elapsed = (sint32)((uint32)simGlobal.gameTicks - (uint32)carObj->N.collision.lastTime);
    if (elapsed >= 15)
        return -1;
    BO::tNewtonObj *other = carObj->N.collision.lastOtherObj;
    if (other == 0 || !CollisionHumanFlag(other))
        return -1;
    Car::tObj *hitter = reinterpret_cast<Car::tObj *>(other);
    if (lastHumanHitter_ != hitter)
    {
        lastHumanHitter_ = hitter;
        hitCount_ = 0;
    }
    carObj = carObj_;
    sint32 hits = (sint32)((uint32)hitCount_ + 1U);
    hitCount_ = hits;
    AIPerson::AIPerson_t *personality = carObj->personality;
    if (personality->attackActivationHits < hits)
    {
        sint32 attackTime = personality->attackTime;
        hitCount_ = 0;
        attackTicksLeft_ = attackTime;
        for (sint32 i = 0; i < 4; ++i)
        {
            if (other == &InfiniteMassNewton[i])
                return InfiniteMassNewton[i + 1].simRoadInfo.quadPts[0].y;
        }
        return hitter->carIndex;
    }
    return -1;
}
