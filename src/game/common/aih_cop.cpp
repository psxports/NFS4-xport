#include <stddef.h>
#include "xport.h"
#include "game/common/ai.h"
#include "game/common/aicop.h"
#include "game/common/aihigh.h"
#include "game/common/ailife.h"
#include "game/common/aiphysic.h"
#include "game/common/aistate.h"
#include "game/common/aitriger.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/bworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/fastrand.h"
#include "game/common/sim.h"
#include "game/common/speech.h"
#include "game/psx/draww.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

class AIH_Cop
{
public:
    static coorddef chasePositions[3][6];
};

extern sint32 NitroDistanceMeters[2][2];

sint32 NitroDistanceMeters[2][2] =
{
    {3932160, 1638400},
    {3932160, 1638400}
};

sint32 AICop::skillDelay[3] = { 3276, 6553, 65536 };

coorddef AIH_Cop::chasePositions[3][6] =
{
    {
        {0, 0, 524288},
        {-393216, 0, 524288},
        {393216, 0, 0},
        {0, 0, -655360},
        {0, 0, -655360},
        {0, 0, -655360},
    },
    {
        {0, 0, 327680},
        {-262144, 0, 327680},
        {262144, 0, 327680},
        {0, 0, -327680},
        {0, 0, -327680},
        {0, 0, -327680},
    },
    {
        {0, 0, 327680},
        {-262144, 0, 327680},
        {262144, 0, 327680},
        {0, 0, -327680},
        {0, 0, -327680},
        {0, 0, -327680},
    },
};

AIHigh_Cop::AggressionEntry AIHigh_Cop::AggressionData[3] =
{
    { 10, 655360, 851968, 512, 512 },
    { 8, 917504, 983040, 768, 512 },
    { 4, 1179648, 1179648, 1152, 1024 },
};

AIHigh_Cop::AIHigh_Cop(Car::tObj *carObj, sint32 copIndex) : AIHigh_BasicCop(carObj, copIndex)
{
    FUNCTION_MARKER(0x80063C00, "NFS4.EXE");
    perpTarget_ = 0;
    chaseIndex_ = 0;
    requestSpikeBeltAtSlice_ = -1;
    forcePurgatory_ = 0;
    aggressionLevel_ = 0;
}

static void InstallCopState(AIHigh_Cop &cop, AIState_Base *newState, stateType_t type)
{
    delete cop.state_;
    cop.state_ = newState;
    cop.stateType_ = type;
}

static sint32 CopAbs(sint32 value)
{
    return value < 0 ? (sint32)(0U - (uint32)value) : value;
}

static bool CopIsSettingBlockade(blockadeMode_t mode)
{
    return mode == BLOCKADEMODE_SETUP || mode == BLOCKADEMODE_SETUP_FOR_OTHER;
}

static void AimCopAway(Car::tObj *car)
{
    car->desiredDirection = GameSetup::gData.reverseTrack == 0 ? -1 : 1;
    car->driveDirection = 1;
}

static void StartCopChase(AIHigh_Cop &cop, coorddef &pos)
{
    InstallCopState(cop, new AIState_Chase(cop.carObj_, cop.perpTarget_->carObj_, &pos,
        AIHigh_Cop::AggressionData[cop.aggressionLevel_].nitrousTicks,
        NitroDistanceMeters[cop.type_][0], NitroDistanceMeters[cop.type_][1],
        cop.aggressionLevel_, AICop::skillDelay[GameSetup::gData.skill]), STATE_CHASE);
}

void AIHigh_Cop::SetTuningLevers(void)
{
    FUNCTION_MARKER(0x80063C4C, "NFS4.EXE");
    Car::tObj *car = carObj_;
    sint32 carTypeIndex = car->carInfo->carType;
    copTuning_t *tuning = &copTuningInfo[carTypeIndex - 22];
    if (type_ == COP_SUPER) {
        car->copTopSpeed = tuning->superCopTopSpeedCap;
        carObj_->copAccMult = tuning->superCopAccMultiplier;
    } else {
        car->copTopSpeed = tuning->regularCopTopSpeedCap;
        carObj_->copAccMult = tuning->regularCopAccMultiplier;
    }
}

void AIHigh_Cop::HighExecute(void)
{
    FUNCTION_MARKER(0x80063CC4, "NFS4.EXE");
    carObj_->unlap = 0;
    carObj_->lap = 0;
    CheckSpikeBelt();
    CheckForWipeOut();
    SetTuningLevers();

    switch (stateType_)
    {
    case STATE_NONE:
        AssignToPlayer(0);
        carObj_->AIFlags &= ~2;
        if ((carObj_->carFlags & 0x400) != 0)
            InstallCopState(*this, new AIState_Idle(carObj_), STATE_IDLE);
        else
            InstallCopState(*this, new AIState_Purgatory(carObj_), STATE_PURGATORY);
        return;

    case STATE_PURGATORY:
    {
        carObj_->AIFlags &= ~2;
        requestSpikeBeltAtSlice_ = -1;
        driveAway_ = AIHigh::DRIVEAWAY_NONE;
        if (blockade_.mode == BLOCKADEMODE_SETUP_FOR_OTHER)
            blockade_.mode = BLOCKADEMODE_NONE;
        if (blockade_.mode == BLOCKADEMODE_WAITING)
        {
            AssignToPlayer(static_cast<AIHigh_Player *>(blockade_.target));
            InstallCopState(*this, new AIState_Idle(carObj_), STATE_IDLE);
            AILife::ReencarnateCopByLatPosAndRotation(carObj_, blockade_.slice, blockade_.direction,
                blockade_.latPos, blockade_.rotation);
            requestSpikeBeltAtSlice_ = blockade_.requestSpikeBeltAtSlice;
            break;
        }
        if ((carObj_->AIFlags & 8) == 0 || blockade_.mode == BLOCKADEMODE_SETUP)
            break;
        trigger::trigger_t *pNewTrigger = CheckForNewTriggers();
        if (!pNewTrigger)
            break;
        trigger::trigger_t newTrigger;
        const bool forceForwardTrigger = Cars::gNumHumanRaceCars == 2 &&
            CopAbs(AIWorld::ApxSplineDistance(Cars::gHumanRaceCarList[0], Cars::gHumanRaceCarList[1])) < 0x12C0000;
        if (forceForwardTrigger)
        {
            const trigger::simple_t forward = { TRIGGER_COP_SIMPLE, pNewTrigger->any.slice, 1, 1, 1 };
            newTrigger.simple = forward;
        }
        else
        {
            switch (pNewTrigger->any.type)
            {
            case TRIGGER_COP_SIMPLE: newTrigger.simple = pNewTrigger->simple; break;
            case TRIGGER_COP_ROADBLOCK: newTrigger.roadblock = pNewTrigger->roadblock; break;
            case TRIGGER_COP_OFFROAD: newTrigger.offroad = pNewTrigger->offroad; break;
            default: newTrigger.any = pNewTrigger->any; break;
            }
        }
        triggerManagerCops->DescribeTrigger(&newTrigger);
        if (newTrigger.any.type == TRIGGER_COP_SIMPLE)
        {
            randtemp = fastRandom * randSeed;
            fastRandom = randtemp & 0xFFFFU;
            sint32 direction = GameSetup::gData.reverseTrack == 0 ? 1 : -1;
            AssignToPlayer(0);
            if (newTrigger.simple.moving == 0)
                InstallCopState(*this, new AIState_Idle(carObj_), STATE_IDLE);
            else
                InstallCopState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
            AILife::ReencarnateCopBySlice(carObj_, newTrigger.simple.slice, direction, direction,
                newTrigger.simple.moving);
        }
        else if (newTrigger.any.type == TRIGGER_COP_OFFROAD)
        {
            trigger::offroad_t &offroad = newTrigger.offroad;
            sint32 direction = GameSetup::gData.reverseTrack == 0 ? 1 : -1;
            if (offroad.dir == 0 || offroad.dir == direction)
            {
                AssignToPlayer(0);
                InstallCopState(*this, new AIState_Offroad(carObj_, offroad.slice, &offroad.position,
                    &offroad.orientation, offroad.maxSpeed, offroad.releaseTime, offroad.endSlice), STATE_OFFROAD);
                AILife::ReencarnateCopByPosition(carObj_, offroad.slice, 1, &offroad.position, &offroad.orientation);
            }
        }
        break;
    }

    case STATE_NORMAL:
        requestSpikeBeltAtSlice_ = -1;
        if (!CopIsSettingBlockade(blockade_.mode))
        {
            carObj_->AIFlags &= ~2;
            if (CheckForNewTarget())
            {
                coorddef pos;
                GetCheckChasePosition(&pos);
                StartCopChase(*this, pos);
                Speech::Mobile(carObj_)->Engage(perpTarget_->carObj_);
            }
        }
        if (forcePurgatory_ || AILife::EvaluateLife(carObj_))
        {
            forcePurgatory_ = 0;
            Speech::Mobile(carObj_)->Purge();
            AssignToPlayer(0);
            InstallCopState(*this, new AIState_Purgatory(carObj_), STATE_PURGATORY);
        }
        break;

    case STATE_CHASE:
    {
        carObj_->AIFlags |= 2;
        AIState_Chase *chaseState = static_cast<AIState_Chase *>(state_);
        const bool leavingChase = chaseState->barrierTicks32_ > 160;
        if (leavingChase)
        {
            const sint32 endSlice = chaseState->FindBarrierEndSlice();
            InstallCopState(*this, new AIState_GotoSlice(carObj_, endSlice, 0), STATE_GOTOSLICE);
            Speech::Mobile(carObj_)->Lose();
        }
        if (!leavingChase && perpTarget_)
        {
            const copLevel_t &level = *perpTarget_->perpChaseInfo_.chaseLevel_;
            if (level.copChasers[type_] == 0 && level.copBlockaders[type_] == 0)
                chaseState->nitrousTicks_ = 0;
        }
        requestSpikeBeltAtSlice_ = -1;
        if (!leavingChase && chaseState->slowDownEndTime_ <= simGlobal.gameTicks && CheckForNeedyPlayers() != -1)
            chaseState->slowDownEndTime_ = (sint32)((uint32)simGlobal.gameTicks + 0x3C0);
        HandleBlockadeSpeech();
        if (perpTarget_)
        {
            coorddef newPos;
            if (GetCheckChasePosition(&newPos) && !leavingChase && chaseState->murderMode_ == 0)
                chaseState->SetTarget(perpTarget_->carObj_, &newPos);
            if (!leavingChase)
            {
                const AggressionEntry &aggression = AggressionData[aggressionLevel_];
                if (chaseState->inTargetRegion_ > aggression.ticksInChaseRegionForMurder &&
                    CopAbs(chaseState->latMetersBetween_) < aggression.minLatMetersDistanceForMurder &&
                    CopAbs(chaseState->longMetersBetween_) < aggression.minLongMetersDistanceForMurder)
                    chaseState->SetMurderMode(1, aggression.murderTicks);
            }
            if (ShouldIPerformCutOffBlock(0x4000, perpTarget_->carObj_) && !leavingChase && chaseState->murderMode_ == 0)
            {
                coorddef zero = { 0, 0, 0 };
                chaseState->SetTarget(perpTarget_->carObj_, &zero);
                chaseState->SetMurderMode(1, 0x20);
            }
        }
        if (CheckForNewTarget())
        {
            coorddef pos;
            GetCheckChasePosition(&pos);
            if (!leavingChase)
                chaseState->SetTarget(perpTarget_->carObj_, &pos);
        }
        bool settingBlockade = CopIsSettingBlockade(blockade_.mode);
        if (settingBlockade || !perpTarget_ || perpTarget_->carObj_->stats.finishType >= 2)
        {
            if (settingBlockade)
                AimCopAway(carObj_);
            AssignToPlayer(0);
            InstallCopState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
        }
        if (!forcePurgatory_ && !AILife::EvaluateLife(carObj_))
        {
            if (driveAway_ != AIHigh::DRIVEAWAY_NONE)
                goto driveAway;
            break;
        }
        if (AILife::EvaluateLife(carObj_) && driveAway_ == AIHigh::DRIVEAWAY_NONE)
            Speech::Mobile(carObj_)->Lose();
        Speech::Mobile(carObj_)->Purge();
        forcePurgatory_ = 0;
        AssignToPlayer(0);
        InstallCopState(*this, new AIState_Purgatory(carObj_), STATE_PURGATORY);
        break;
    }

    case STATE_IDLE:
        if (CopIsSettingBlockade(blockade_.mode))
        {
            AimCopAway(carObj_);
            AssignToPlayer(0);
            InstallCopState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
        }
        if (forcePurgatory_)
        {
            forcePurgatory_ = 0;
            Speech::Mobile(carObj_)->Purge();
            AssignToPlayer(0);
            InstallCopState(*this, new AIState_Purgatory(carObj_), STATE_PURGATORY);
            break;
        }
        if (driveAway_ != AIHigh::DRIVEAWAY_NONE)
        {
        driveAway:
            AimCopAway(carObj_);
            AssignToPlayer(0);
            InstallCopState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
            if (driveAway_ == AIHigh::DRIVEAWAY_RESET)
                Cars::ResetCollidedCars(carObj_, 1, 1);
            driveAway_ = AIHigh::DRIVEAWAY_NONE;
            break;
        }
        if (blockade_.mode != BLOCKADEMODE_WAITING)
        {
            carObj_->AIFlags &= ~2;
            if (CheckForNewTarget() && !CopIsSettingBlockade(blockade_.mode))
            {
                coorddef pos;
                GetCheckChasePosition(&pos);
                StartCopChase(*this, pos);
                Speech::Mobile(carObj_)->Engage(perpTarget_->carObj_);
            }
            else if (AILife::EvaluateLife(carObj_))
            {
                AssignToPlayer(0);
                InstallCopState(*this, new AIState_Purgatory(carObj_), STATE_PURGATORY);
            }
            break;
        }
        {
            carObj_->AIFlags |= 2;
            sint32 slice = requestSpikeBeltAtSlice_;
            if (slice != -1 && !AICop::spikeBelt.active_)
            {
                const sint32 skill = GameSetup::gData.skill;
                const sint32 size = skill == 0 ? 0xB333 : (skill == 1 ? 0xCCCC : 0xE666);
                const Trk::NewSlice &road = BWorldSm::slices[slice];
                sint32 left = fixedmult(road.avgPavedWidthLf * 0x8000 * (road.laneCount >> 4), size);
                sint32 right = fixedmult(road.avgPavedWidthRt * 0x8000 * (road.laneCount & 15), size);
                AICop::spikeBelt.leftLatPos_ = -left;
                AICop::spikeBelt.rightLatPos_ = right;
                AICop::spikeBelt.active_ = 1;
                AICop::spikeBelt.slice_ = requestSpikeBeltAtSlice_;
                AICop::spikeBelt.freshenTime_ = simGlobal.gameTicks;
                BWorld::SetSpikeBelt(requestSpikeBeltAtSlice_, -left, left + right);
                slice = requestSpikeBeltAtSlice_;
            }
            AICop::gRoadBlockState = AICop::kAICop_RoadBlockState_WaitingForPerp;
            if (slice != -1 && AICop::spikeBelt.slice_ == slice)
                AICop::spikeBelt.freshenTime_ = simGlobal.gameTicks;
            if (blockade_.flags != 0)
            {
                const bool switched = stackSpeedUpEnbabledFlag != 0;
                if (switched)
                    stackSpeedUpEnbabledFlag = 0;
                SetupBlockadeElements(&blockade_);
                if (switched)
                    stackSpeedUpEnbabledFlag = 1;
            }

            AICop::PerpChaseInfo &chase = perpTarget_->perpChaseInfo_;
            const sint32 seconds = (sint32)((uint32)chase.chaseLevel_->engagementLapFraction *
                (uint32)AITune::gRoughLapTime) / 65536;
            sint32 increase = 65536 / (seconds * 32);
            chase.engagementTime_ = (sint32)((uint32)seconds << 21);
            if (GameSetup::gData.numLaps == 2)
                increase = fixedmult(increase, 0x13333);
            else if (GameSetup::gData.numLaps == 4)
                increase = fixedmult(increase, 0xA8F5);
            chase.engagementPercentIncreasePerTick_ = increase;

            coorddef newPos;
            GetCheckChasePosition(&newPos);
            sint32 rbDistanceMeters = AIWorld::ApxSplineDistance(carObj_, perpTarget_->carObj_);
            sint32 rbAbsDistanceMeters = CopAbs(rbDistanceMeters);
            bool release = rbAbsDistanceMeters < 0x320000;
            if (!release && rbAbsDistanceMeters < 0x12C0000)
            {
                sint32 speed = perpTarget_->carObj_->currentSpeed;
                if (CopAbs(speed) > 0x471C7)
                {
                    sint32 timeToRB = fixeddiv(rbDistanceMeters, speed);
                    release = timeToRB > 0 && timeToRB < blockade_.releaseTime;
                }
            }
            if (!release)
            {
                sint32 distanceMeters = AIWorld::ApxSplineDistance(perpTarget_->carObj_, blockade_.slice) / 65536;
                if ((sint32)((uint32)blockade_.initialPlayerDistanceMetersInt * (uint32)distanceMeters) <= 0)
                    break;
            }
            requestSpikeBeltAtSlice_ = -1;
            if (blockade_.chaseLevel == perpTarget_->perpChaseInfo_.chaseLevelIndex_)
                perpTarget_->perpChaseInfo_.engagementTime_ = 0;
            blockade_.mode = BLOCKADEMODE_NONE;
            StartCopChase(*this, newPos);
            if (blockade_.reverse != 0)
                AIPhysic::ChangeDirection(carObj_, 0x40);
        }
        break;

    case STATE_OFFROAD:
    {
        carObj_->extraWallCollisionAllowance = carObj_->N.simOptz == 0 ? 0 : 0x18000;
        requestSpikeBeltAtSlice_ = -1;
        for (sint32 hLoop = 0; hLoop < Cars::gNumRaceCars; ++hLoop)
        {
            Car::tObj *thisPlayerObj = Cars::gRaceCarList[hLoop];
            AIHigh_Player *thisPlayer = static_cast<AIHigh_Player *>(highLevelAIObjs[thisPlayerObj->carIndex]);
            if (thisPlayer->basicPerpInfo_.crime_ != CRIME_NONE)
                static_cast<AIState_Offroad *>(state_)->UnleashIfInRange(thisPlayerObj);
        }
        const Trk::NewSlice &road = BWorldSm::slices[carObj_->N.simRoadInfo.slice];
        sint32 left = road.avgPavedWidthLf * 0x8000 * (road.laneCount >> 4);
        sint32 right = road.avgPavedWidthRt * 0x8000 * (road.laneCount & 15);
        if (carObj_->roadPosition >= -left && carObj_->roadPosition <= right)
        {
            carObj_->extraWallCollisionAllowance = 0;
            AssignToPlayer(0);
            InstallCopState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
        }
        if (forcePurgatory_ || AILife::EvaluateLife(carObj_))
        {
            carObj_->extraWallCollisionAllowance = 0;
            forcePurgatory_ = 0;
            AssignToPlayer(0);
            InstallCopState(*this, new AIState_Purgatory(carObj_), STATE_PURGATORY);
        }
        break;
    }

    case STATE_GOTOSLICE:
    {
        AIState_GotoSlice *gotoState = static_cast<AIState_GotoSlice *>(state_);
        AssignToPlayer(0);
        if (gotoState->InTargetSliceRange(0xA0000))
            InstallCopState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
        break;
    }
    default:
        break;
    }
    state_->StateExecute();
}

sint32 AIHigh_Cop::CheckForNeedyPlayers(void)
{
    FUNCTION_MARKER(0x80065394, "NFS4.EXE");
    sint32 needy = -1;
    sint32 numHumans = Cars::gNumHumanRaceCars;
    for (sint32 hLoop = 0; hLoop < numHumans; ++hLoop) {
        Car::tObj *thisPlayerObj = Cars::gHumanRaceCarList[hLoop];
        sint32 carIndex = thisPlayerObj->carIndex;
        AIHigh_Player *thisPlayer = static_cast<AIHigh_Player *>(highLevelAIObjs[carIndex]);
        if (thisPlayer->perpChaseInfo_.copFreeTicks_ > 800) {
            sint32 speed = thisPlayerObj->currentSpeed;
            if (speed < 0)
                speed = (sint32)(0 - (uint32)speed);
            if (speed > 0x140000)
                needy = carIndex;
        }
    }
    return needy;
}

void AIHigh_Cop::CheckForWipeOut(void)
{
    FUNCTION_MARKER(0x80065430, "NFS4.EXE");
    if (stateType_ != STATE_CHASE)
        return;
    AIHigh_Player *target = perpTarget_;
    if (!target || (target->carObj_->carFlags & 8) == 0)
        return;
    if (carObj_->wipeOutEndTick > simGlobal.gameTicks)
        return;
    if (target->perpChaseInfo_.engagementTime_ / 65536 >= 2)
        return;
    uint32 product = fastRandom * randSeed;
    sint32 elapsed = AI::elapsedTime;
    sint32 numHumans = Cars::gNumHumanRaceCars;
    sint32 thisTargetLevel = perpTarget_->perpChaseInfo_.chaseLevelIndex_;
    sint32 perTickProb = (sint32)((uint32)elapsed * 89);
    sint32 randVal = (product >> 8) & 0xFFFF;
    randtemp = product;
    fastRandom = product & 0xFFFF;
    for (sint32 hLoop = 0; hLoop < numHumans; ++hLoop) {
        Car::tObj *thisPlayerObj = Cars::gHumanRaceCarList[hLoop];
        AIHigh_Player *thisPlayer = static_cast<AIHigh_Player *>(highLevelAIObjs[thisPlayerObj->carIndex]);
        if (thisTargetLevel < thisPlayer->perpChaseInfo_.chaseLevelIndex_ && randVal < perTickProb)
            carObj_->wipeOutEndTick = (uint32)simGlobal.gameTicks + 640;
    }
}

sint32 AIHigh_Cop::CheckForNewTarget(void)
{
    FUNCTION_MARKER(0x800655A8, "NFS4.EXE");
    AIHigh_Player *newTarget = 0;
    sint32 newTargetDistance = 0x27100000;
    AIHigh_Player *old = perpTarget_;
    blockadeMode_t mode = blockade_.mode;
    if (mode == BLOCKADEMODE_SETUP || mode == BLOCKADEMODE_SETUP_FOR_OTHER || mode == BLOCKADEMODE_WAITING)
        return 0;
    for (sint32 playerLoop = 0; playerLoop < Cars::gNumRaceCars; ++playerLoop) {
        sint32 thisCarIndex = Cars::gRaceCarList[playerLoop]->carIndex;
        AIHigh_Player *thisPlayer = static_cast<AIHigh_Player *>(highLevelAIObjs[thisCarIndex]);
        sint32 needs = 0;
        if (thisPlayer->basicPerpInfo_.crime_ != CRIME_NONE)
            needs = thisPlayer->perpChaseInfo_.chaseLevel_->copChasers[type_];
        sint32 got = thisPlayer->basicPerpInfo_.copsAssigned_[type_];
        if (perpTarget_ && perpTarget_ == thisPlayer)
            got = (sint32)((uint32)got - 1);
        if (got < needs) {
            sint32 copToTargetDistanceMeters = AIWorld::ApxSplineDistance(carObj_, thisPlayer->carObj_);
            if (copToTargetDistanceMeters < 0)
                copToTargetDistanceMeters = (sint32)(0 - (uint32)copToTargetDistanceMeters);
            if (copToTargetDistanceMeters < newTargetDistance) {
                newTargetDistance = copToTargetDistanceMeters;
                newTarget = thisPlayer;
            }
        }
    }
    if (!newTarget) {
        for (sint32 playerLoop = 0; playerLoop < Cars::gNumRaceCars; ++playerLoop) {
            sint32 thisCarIndex = Cars::gRaceCarList[playerLoop]->carIndex;
            AIHigh_Player *thisPlayer = static_cast<AIHigh_Player *>(highLevelAIObjs[thisCarIndex]);
            sint32 copToTargetDistanceMeters = AIWorld::ApxSplineDistance(carObj_, thisPlayer->carObj_);
            if (copToTargetDistanceMeters < 0)
                copToTargetDistanceMeters = (sint32)(0 - (uint32)copToTargetDistanceMeters);
            if (copToTargetDistanceMeters < newTargetDistance && thisPlayer->basicPerpInfo_.crime_ != CRIME_NONE) {
                newTargetDistance = copToTargetDistanceMeters;
                newTarget = thisPlayer;
            }
        }
    }
    if (newTarget && newTarget != old) {
        AssignToPlayer(newTarget);
        aggressionLevel_ = newTarget->perpChaseInfo_.chaseLevel_->copAggression[type_];
        return 1;
    }
    if (old && !newTarget)
        AssignToPlayer(0);
    return 0;
}

void AIHigh_Cop::AssignToPlayer(AIHigh_Player *target)
{
    FUNCTION_MARKER(0x80065808, "NFS4.EXE");
    if (perpTarget_)
        perpTarget_->RemoveChaser(copIndex_, carObj_->carIndex, type_);
    if (!target && blockade_.mode != BLOCKADEMODE_SETUP)
        blockade_.mode = BLOCKADEMODE_NONE;
    perpTarget_ = target;
    if (target)
        target->AddChaser(copIndex_, carObj_->carIndex, type_);
}

sint32 AIHigh_Cop::GetCheckChasePosition(coorddef *pos)
{
    FUNCTION_MARKER(0x80065894, "NFS4.EXE");
    sint32 newPosition = perpTarget_->CheckChaserPosition(copIndex_, carObj_->carIndex);
    sint32 changed = 0;
    if (newPosition != chaseIndex_) {
        changed = 1;
        chaseIndex_ = newPosition;
    }
    coorddef chasePosition = AIH_Cop::chasePositions[aggressionLevel_][chaseIndex_];
    pos->x = chasePosition.x;
    pos->y = chasePosition.y;
    pos->z = chasePosition.z;
    return changed;
}

trigger::trigger_t *AIHigh_Cop::CheckForNewTriggers(void)
{
    FUNCTION_MARKER(0x80065944, "NFS4.EXE");
    if (simGlobal.gameTicks < 1472)
        return 0;
    for (sint32 sortedLoop = Cars::gNumCars - 1; sortedLoop >= 0; --sortedLoop) {
        Car::tObj *testCar = Cars::gTotalSortedList[sortedLoop];
        if ((testCar->carFlags & 1) == 0)
            continue;
        AIHigh_Player *thisPlayer = static_cast<AIHigh_Player *>(highLevelAIObjs[testCar->carIndex]);
        copLevel_t *pLevel = thisPlayer->perpChaseInfo_.chaseLevel_;
        copType type = type_;
        sint32 fRandomChance = thisPlayer->newTriggerProb_;
        sint32 got = thisPlayer->basicPerpInfo_.copsAssigned_[type];
        sint32 needs;
        if (thisPlayer->basicPerpInfo_.crime_ == CRIME_NONE) {
            fRandomChance = (uint32)fRandomChance << 1;
            needs = 0;
            if (pLevel->copChasers[type] > 0 &&
                AICop::NoCopsInArea(thisPlayer->carObj_->N.simRoadInfo.slice, 0x1F40000))
                needs = 1;
        } else {
            needs = pLevel->copChasers[type];
        }
        if (GameSetup::gData.skill == 2)
            fRandomChance = 0x10000;
        sint32 elapsed = (sint32)((uint32)simGlobal.gameTicks - (uint32)thisPlayer->lastPullOverTime_);
        if (elapsed < 448 || got >= needs)
            continue;
        sint32 dir = testCar->currentSpeed < 0 ? -1 : 1;
        sint32 offset = dir * 31;
        sint32 thisSlice = testCar->N.simRoadInfo.slice + offset;
        if (offset < 0) {
            if (thisSlice < 0)
                thisSlice = (sint32)((uint32)thisSlice + (uint32)gNumSlices);
        } else if (thisSlice >= gNumSlices) {
            thisSlice = (sint32)((uint32)thisSlice - (uint32)gNumSlices);
        }
        sint32 startSlice = thisPlayer->lastTriggerCheckSlice_;
        thisPlayer->lastTriggerCheckSlice_ = thisSlice;
        sint32 endSlice = thisSlice;
        if (endSlice < startSlice) {
            sint32 temp = startSlice;
            startSlice = endSlice;
            endSlice = temp;
        }
        sint32 gap = (sint32)((uint32)endSlice - (uint32)startSlice);
        if (gap >= 6)
            continue;
        for (sint32 sliceLoop = startSlice; sliceLoop < endSlice; ++sliceLoop) {
            sint32 triggerHere = triggerManagerCops->CheckForTriggerAtSlice(testCar->carIndex, sliceLoop);
            if (triggerHere == -1)
                continue;
            sint32 scaledChance = (sint32)((uint32)fRandomChance * 100);
            sint32 iRandomChance = scaledChance / 65536;
            uint32 product = fastRandom * randSeed;
            randtemp = product;
            fastRandom = product & 0xFFFF;
            sint32 randomValue = (((product >> 8) & 0xFFFF) * 25) >> 14;
            if (AILife::IsSliceInAnyVisibleArea(sliceLoop))
                continue;
            if (got != 0 || randomValue < iRandomChance) {
                sint32 unused;
                return triggerManagerCops->GetTrigger(triggerHere, &unused);
            }
        }
    }
    return 0;
}
