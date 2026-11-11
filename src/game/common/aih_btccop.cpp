#include <stddef.h>
#include "xport.h"
#include "game/common/ai.h"
#include "game/common/aidatarecord.h"
#include "game/common/aihigh.h"
#include "game/common/aiinit.h"
#include "game/common/ailife.h"
#include "game/common/aispeeds.h"
#include "game/common/aistate.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/bworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/fastrand.h"
#include "game/common/gmesetup.h"
#include "game/common/object.h"
#include "game/common/sim.h"
#include "game/common/speech.h"
#include "game/common/trgsfx.h"
#include "game/psx/draww.h"
#include "game/psx/hud.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

class AIH_BTCCop
{
public:
    static coorddef chasePositions[3][6];
};

extern char gBlockadeTypes[5];
extern sint32 AIH_BTC_Cop_1HC1HP_StageTimes[3];
sint32 AIH_BTC_Cop_1HC1HP_StageTimes[3] = { 60, 60, 60 };
sint32 AIHigh_BTC_HumanCop::lastInputRequestTick_;

coorddef AIH_BTCCop::chasePositions[3][6] =
{
    {
        {0, 0, 720896},
        {-524288, 0, 0},
        {0, 0, -983040},
        {0, 0, -983040},
        {0, 0, -983040},
        {0, 0, -983040},
    },
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
};

static sint32 BtcOffsetSlice(sint32 slice, sint32 offset)
{
    sint32 result = (sint32)((uint32)slice + (uint32)offset);
    if (offset < 0)
    {
        if (result < 0)
            result = (sint32)((uint32)result + (uint32)gNumSlices);
    }
    else if (result >= gNumSlices)
        result = (sint32)((uint32)result - (uint32)gNumSlices);
    return result;
}

AIHigh_BTC_Cop::AIHigh_BTC_Cop(Car::tObj *carObj, sint32 copIndex) : AIHigh_BasicCop(carObj, copIndex)
{
    FUNCTION_MARKER(0x8005CB6C, "NFS4.EXE");
    perpTarget_ = 0;
    chaseIndex_ = 0;
    freezeMode_ = FREEZE_NONE;
}

void AIHigh_BTC_Cop::AssignToPlayer(AIHigh_BTC_Perp *target)
{
    FUNCTION_MARKER(0x8005CBAC, "NFS4.EXE");
    AIHigh_BTC_Perp *old = perpTarget_;
    if (old)
        old->RemoveChaser(copIndex_, carObj_->carIndex, type_);
    perpTarget_ = target;
    if (target)
        target->AddChaser(copIndex_, carObj_->carIndex, type_);
}

sint32 AIHigh_BTC_Cop::GetCheckChasePosition(coorddef *pos)
{
    FUNCTION_MARKER(0x8005CC1C, "NFS4.EXE");
    sint32 newPosition = perpTarget_->CheckChaserPosition(copIndex_, carObj_->carIndex);
    sint32 changed = newPosition != chaseIndex_;
    if (changed)
        chaseIndex_ = newPosition;
    coorddef position = AIH_BTCCop::chasePositions[0][chaseIndex_];
    pos->x = position.x;
    pos->y = position.y;
    pos->z = position.z;
    return changed;
}

sint32 AIHigh_BTC_Cop::CheckForNewTarget(void)
{
    FUNCTION_MARKER(0x8005CCB8, "NFS4.EXE");
    AIHigh_BTC_Perp *newTarget = 0;
    sint32 newTargetDistance = 0x27100000;
    AIHigh_BTC_Perp *old = perpTarget_;
    for (sint32 perpLoop = 0; perpLoop < Cars::gNumRaceCars; ++perpLoop) {
        Car::tObj *testPerpCarObj = Cars::gRaceCarList[perpLoop];
        if (testPerpCarObj->N.active && (testPerpCarObj->carFlags & 0x200) == 0) {
            sint32 thisPerpCarIndex = testPerpCarObj->carIndex;
            AIHigh_BTC_Perp *thisPerpHigh = static_cast<AIHigh_BTC_Perp *>(highLevelAIObjs[thisPerpCarIndex]);
            if (thisPerpHigh->basicPerpInfo_.crime_ != CRIME_NONE) {
                sint32 copToTargetDistanceMeters = AIWorld::ApxSplineDistance(carObj_, testPerpCarObj);
                if (copToTargetDistanceMeters < 0)
                    copToTargetDistanceMeters = (sint32)(0 - (uint32)copToTargetDistanceMeters);
                if (copToTargetDistanceMeters < newTargetDistance) {
                    newTargetDistance = copToTargetDistanceMeters;
                    newTarget = thisPerpHigh;
                }
            }
        }
    }
    if (newTarget && newTarget != old) {
        AssignToPlayer(newTarget);
        return 1;
    }
    if (old && !newTarget)
        AssignToPlayer(0);
    return 0;
}

void AIHigh_BTC_Cop::StartArrest(AIHigh_BTC_Perp *arrestMe)
{
    FUNCTION_MARKER(0x8005CE10, "NFS4.EXE");
    (void)arrestMe;
    if (freezeMode_ == FREEZE_NONE) {
        Car::tObj *car = carObj_;
        freezeMode_ = FREEZE_ARREST;
        if ((car->carFlags & 0x200) != 0)
            Speech::Mobile(car)->Catch(1);
    }
}

void AIHigh_BTC_Cop::FinishArrest(AIHigh_BTC_Perp *arrestMe)
{
    FUNCTION_MARKER(0x8005CE80, "NFS4.EXE");
    (void)arrestMe;
    if (freezeMode_ == FREEZE_ARREST || freezeMode_ == FREEZE_NONE) {
        freezeMode_ = FREEZE_ARRESTDONE;
        FreezeAndEndChase();
        HudOff();
    }
}

void AIHigh_BTC_Cop::FalseArrest(AIHigh_BTC_Perp *cantArrestMe)
{
    FUNCTION_MARKER(0x8005CEE0, "NFS4.EXE");
    if (perpTarget_ == cantArrestMe)
        freezeMode_ = FREEZE_NEWPERP_DONE;
}

void AIHigh_BTC_Cop::FreezeAndEndChase(void)
{
    FUNCTION_MARKER(0x8005CEFC, "NFS4.EXE");
}

void AIHigh_BTC_Cop::HudOff(void)
{
    FUNCTION_MARKER(0x8005CF04, "NFS4.EXE");
    if (copIndex_ == 0)
        Hud::BustedOverlayOff();
}

AIHigh_BTC_HumanCop::AIHigh_BTC_HumanCop(Car::tObj *carObj, sint32 copIndex) : AIHigh_BTC_Cop(carObj, copIndex)
{
    FUNCTION_MARKER(0x8005CF34, "NFS4.EXE");
    initialMovement_ = 1;
    currentStage_ = -1;
    stageTimeMultiplier_ = 0x10000;
    needPerp_ = 0;
    initialDirection_ = 0;
    requestedDesiredSpeed_ = 0;
    stageRepeatCount_ = 0;
    Car::tObj *car = carObj_;
    timeLeft_ = AITune::BTC[GameSetup::gData.skill].baseChaseTime;
    car->unlap = 0;
    lastInputRequestTick_ = 0;
    AILife::PlaceCarAtLocation(carObj_, 0, 0, 1, 0, 0);
    if (copIndex == 0) {
        uint32 seed = randSeed;
        uint32 product = fastRandom * seed;
        randtemp = product;
        fastRandom = product & 0xFFFF;
        sint32 sample = (((product >> 8) & 0xFFFF) * 1000) >> 16;
        sint32 startDirection = sample < 500 ? 1 : -1;
        product = fastRandom * seed;
        randtemp = product;
        fastRandom = product & 0xFFFF;
        sample = (((product >> 8) & 0xFFFF) * 1000) >> 16;
        sint32 startMovement = 0;
        if (sample < 750)
            startMovement = AIHigh::CopGameType != AIHigh::COP_GAME_BTC_1HC1HP;
        FindRandomBarrierFreeArea(0, 100, 500);
        sint32 bend = AIWorld::CalcRoadBend(carObj_, 3);
        if (bend < 0)
            bend = (sint32)(0 - (uint32)bend);
        if (bend > 0x5EB8 && AIHigh::CopGameType != AIHigh::COP_GAME_BTC_1HC1HP)
            startMovement = 1;
        NewStage(carObj_->N.simRoadInfo.slice, startDirection, startMovement);
    }
}

sint32 AIHigh_BTC_HumanCop::FindRandomBarrierFreeArea(sint32 startSlice, sint32 safetyZone, sint32 randomDistance)
{
    FUNCTION_MARKER(0x8005D150, "NFS4.EXE");
    uint32 product = fastRandom * randSeed;
    uint32 sample = (product >> 8) & 0xFFFF;
    uint32 scaled = (uint32)randomDistance * (sample << 1);
    randtemp = product;
    fastRandom = product & 0xFFFF;
    sint32 newOffset = (sint32)((scaled >> 16) - (uint32)randomDistance);
    sint32 newSlice = (sint32)((uint32)startSlice + (uint32)newOffset);
    if (newOffset >= 0) {
        if (newSlice >= gNumSlices)
            newSlice = (sint32)((uint32)newSlice - (uint32)gNumSlices);
    } else if (newSlice < 0) {
        newSlice = (sint32)((uint32)newSlice + (uint32)gNumSlices);
    }
    sint32 step = safetyZone / 5;
    sint32 width = (sint32)((uint32)safetyZone << 1);
    sint32 negativeSafety = (sint32)(0 - (uint32)safetyZone);
    for (sint32 maxRuns = 0; maxRuns < 30; ++maxRuns) {
        sint32 startCheckSlice = (sint32)((uint32)newSlice - (uint32)safetyZone);
        if (negativeSafety >= 0) {
            if (startCheckSlice >= gNumSlices)
                startCheckSlice = (sint32)((uint32)startCheckSlice - (uint32)gNumSlices);
        } else if (startCheckSlice < 0) {
            startCheckSlice = (sint32)((uint32)startCheckSlice + (uint32)gNumSlices);
        }
        sint32 leftLaneFree = AIWorld::IsDriveableLaneInSliceRange(startCheckSlice, width, 1, 6);
        sint32 rightLaneFree = AIWorld::IsDriveableLaneInSliceRange(startCheckSlice, width, 1, 7);
        if (leftLaneFree && rightLaneFree)
            return newSlice;
        newSlice = (sint32)((uint32)newSlice + (uint32)step);
        if (step >= 0) {
            if (newSlice >= gNumSlices)
                newSlice = (sint32)((uint32)newSlice - (uint32)gNumSlices);
        } else if (newSlice < 0) {
            newSlice = (sint32)((uint32)newSlice + (uint32)gNumSlices);
        }
    }
    return newSlice;
}

void AIHigh_BTC_HumanCop::ReleaseAndStartChase(AIHigh_BTC_Perp *newPerp)
{
    FUNCTION_MARKER(0x8005D320, "NFS4.EXE");
    Car::tObj *car = carObj_;
    car->AIFlags |= 2;
    CheckForNewTarget();
    if (perpTarget_ == newPerp) {
        freezeMode_ = FREEZE_NEWPERP_DONE;
        chaseStartTime_ = simGlobal.gameTicks;
    }
    ResetClearTrafficToPurgatory();
}

void AIHigh_BTC_HumanCop::FreezeAndEndChase(void)
{
    FUNCTION_MARKER(0x8005D390, "NFS4.EXE");
    if (copIndex_ != 0)
        return;
    uint32 seed = randSeed;
    uint32 product = fastRandom * seed;
    randtemp = product;
    fastRandom = product & 0xFFFF;
    sint32 sample = (((product >> 8) & 0xFFFF) * 1000) >> 16;
    sint32 startDirection = sample < 500 ? 1 : -1;
    product = fastRandom * seed;
    randtemp = product;
    fastRandom = product & 0xFFFF;
    sample = (((product >> 8) & 0xFFFF) * 1000) >> 16;
    sint32 startMovement = 0;
    if (sample < 750)
        startMovement = AIHigh::CopGameType != AIHigh::COP_GAME_BTC_1HC1HP;
    if (AIDataRecord::TrackCurve->Get(carObj_->N.simRoadInfo.slice) >= 66 &&
        AIHigh::CopGameType != AIHigh::COP_GAME_BTC_1HC1HP)
        startMovement = 1;
    sint32 startSlice = FindRandomBarrierFreeArea(carObj_->N.simRoadInfo.slice, 100, 1);
    sint32 addToSlice = startDirection * 7;
    if (((uint32)currentStage_ + 1) & 1)
        addToSlice = -addToSlice;
    startSlice = (sint32)((uint32)startSlice + (uint32)addToSlice);
    if (addToSlice >= 0) {
        if (startSlice >= gNumSlices)
            startSlice = (sint32)((uint32)startSlice - (uint32)gNumSlices);
    } else if (startSlice < 0) {
        startSlice = (sint32)((uint32)startSlice + (uint32)gNumSlices);
    }
    carObj_->N.simRoadInfo.slice = (sint16)startSlice;
    NewStage(startSlice, startDirection, startMovement);
}

void AIHigh_BTC_HumanCop::CheckConditionWithCop0(void)
{
    FUNCTION_MARKER(0x8005D564, "NFS4.EXE");
    if (copIndex_ == 0)
        return;
    AIHigh_BTC_HumanCop *leadCop = static_cast<AIHigh_BTC_HumanCop *>(highLevelAIObjs[0]);
    if (currentStage_ < leadCop->currentStage_) {
        sint32 startDirection = leadCop->initialDirection_;
        sint32 startMovement = leadCop->initialMovement_;
        sint32 startSlice = leadCop->carObj_->N.simRoadInfo.slice;
        sint32 addToSlice = (sint32)((uint32)startDirection * 14);
        if ((((uint32)currentStage_ + 1) & 1) == 0)
            addToSlice = (sint32)(0 - (uint32)addToSlice);
        startSlice = (sint32)((uint32)startSlice + (uint32)addToSlice);
        if (addToSlice >= 0) {
            if (startSlice >= gNumSlices)
                startSlice = (sint32)((uint32)startSlice - (uint32)gNumSlices);
        } else if (startSlice < 0) {
            startSlice = (sint32)((uint32)startSlice + (uint32)gNumSlices);
        }
        NewStage(startSlice, startDirection, startMovement);
    }
    carObj_->desiredSpeed = leadCop->carObj_->desiredSpeed;
}

void AIHigh_BTC_HumanCop::NewStage(sint32 copSlice, sint32 direction, sint32 movement)
{
    FUNCTION_MARKER(0x8005D650, "NFS4.EXE");
    currentStage_ = (sint32)((uint32)currentStage_ + 1);
    GameSetup::gData.perpArrests = (sint32)((uint32)GameSetup::gData.perpArrests + 1);
    AIHigh::CopGameType_t mode = AIHigh::CopGameType;
    if (((uint32)mode - 2 < 2 && currentStage_ >= GameSetup::gData.numPerps) ||
        (mode == AIHigh::COP_GAME_BTC_1HC1HP && currentStage_ >= 3)) {
        simVar.endSimGame = 1;
        return;
    }
    BWorld::InitSpikeBelt();
    AICop::spikeBelt.active_ = 0;
    Object::ClearCustomObjects();
    Car::tObj *car = carObj_;
    initialDirection_ = direction;
    initialMovement_ = movement;
    car->N.simRoadInfo.slice = (sint16)copSlice;
    carObj_->direction = direction;
    AICop::gRoadBlockState = AICop::kAICop_RoadBlockState_None;
    sint32 throwAway = 0;
    sint32 newLatPos = 0;
    AIWorld::FindBarrierLessLaneAndPosition(carObj_, &throwAway, &newLatPos);
    sint32 speed = 0;
    sint32 rotation = 0;
    if (AIHigh::CopGameType != AIHigh::COP_GAME_BTC_1HC1HP) {
        if (movement) {
            speed = (sint32)((uint32)direction * 0xD5555);
        } else {
            Trk::NewSlice *slice = &BWorldSm::slices[copSlice];
            sint32 laneCount = slice->laneCount;
            sint32 rightPos = slice->avgPavedWidthRt * 0x8000 * (laneCount & 15);
            sint32 leftPos = slice->avgPavedWidthLf * 0x8000 * (laneCount >> 4);
            sint32 rightDrive = slice->rightDrive * 256 - 0x8000;
            if (rightPos < rightDrive)
                rightDrive = rightPos;
            sint32 leftDrive = slice->leftDrive * 256 - 0x8000;
            if (leftPos < leftDrive)
                leftDrive = leftPos;
            rightPos = rightDrive;
            leftPos = leftDrive;
            if (!AIWorld::IsDriveableLane(copSlice, 6 - (laneCount >> 4)))
                leftPos = (sint32)((uint32)leftPos - 0x20000);
            sint32 rightLane = BWorldSm::slices[copSlice].laneCount & 15;
            if (!AIWorld::IsDriveableLane(copSlice, rightLane + 7))
                rightPos = (sint32)((uint32)rightPos - 0x20000);
            sint32 side = AITune::driveSide;
            bool right = copIndex_ == 0 ? side == 1 : side == -1;
            if (right) {
                newLatPos = rightPos;
                rotation = (sint32)(((uint32)direction << 8) + 0x200);
            } else {
                newLatPos = (sint32)(0 - (uint32)leftPos);
                rotation = (sint32)(0x200 - ((uint32)direction << 8));
            }
        }
    }
    AILife::PlaceCarAtLocation(carObj_, copSlice, newLatPos, direction, speed, rotation);
    sint32 seconds;
    if (AIHigh::CopGameType == AIHigh::COP_GAME_BTC_1HC1HP)
        seconds = AIH_BTC_Cop_1HC1HP_StageTimes[currentStage_];
    else
        seconds = GameSetup::gData.perpInfo[currentStage_].TimeLimit;
    sint32 nextStageTime = (sint32)((uint32)seconds << 5);
    if (nextStageTime < 0)
        nextStageTime = 0;
    sint32 product = (sint32)((uint32)nextStageTime * (uint32)stageTimeMultiplier_);
    if (product < 0)
        product = (sint32)((uint32)product + 0xFFFF);
    nextStageTime = product >> 16;
    if (copIndex_ == 0)
        Hud::BTC_BonusTime((sint32)((uint32)nextStageTime << 1));
    sint32 remaining = timeLeft_;
    needPerp_ = 1;
    chaseStartTime_ = 0;
    wingmanStatus_ = WINGMAN_NONE;
    freezeMode_ = FREEZE_NEWPERP_ROVE;
    timeLeft_ = (sint32)((uint32)remaining + (uint32)nextStageTime);
    TrgSfx::RestartTrgSfx();
}

void AIHigh_BTC_HumanCop::UpdateAndCheckTimeLeft(void)
{
    FUNCTION_MARKER(0x8005D9C0, "NFS4.EXE");
    static sint32 sayLose = 1;
    if (freezeMode_ != FREEZE_ARREST && freezeMode_ != FREEZE_NEWPERP_ROVE)
        timeLeft_ = (sint32)((uint32)timeLeft_ - (uint32)AI::elapsedTime);
    if (perpTarget_ != 0)
        Hud::BTC_Update(perpTarget_->carObj_->carInfo->driver, timeLeft_, carObj_->RSControl == 0);
    else
        Hud::BTC_Update(0, 0, carObj_->RSControl == 0);
    if (timeLeft_ < 0)
    {
        if (sayLose != 0)
        {
            Speech::Mobile(carObj_)->Lose();
            sayLose = 0;
        }
        if (timeLeft_ < -160)
        {
            simVar.endSimGame = 1;
            sayLose = 1;
            HudOn(perpTarget_, 1, carObj_);
        }
    }
}

void AIHigh_BTC_HumanCop::UpdateFreezeModeAndPullOverMode(void)
{
    FUNCTION_MARKER(0x8005DAD4, "NFS4.EXE");
    if (timeLeft_ < 0) {
        Car::tObj *car = carObj_;
        sint32 direction = car->direction;
        AIInit::forceHumanHandBrake = 1;
        car->RSControl = direction;
        requestedDesiredSpeed_ = 0;
        return;
    }
    sint32 mode = freezeMode_;
    if (mode == FREEZE_ARREST) {
        carObj_->RSControl = 0;
        carObj_->pullOver = 1;
        requestedDesiredSpeed_ = 0;
        return;
    }
    if (mode == FREEZE_NEWPERP_ROVE) {
        Car::tObj *car = carObj_;
        car->AIFlags &= ~2;
        carObj_->RSControl = initialDirection_;
        carObj_->pullOver = initialMovement_ == 0;
        if (carObj_->pullOver)
            requestedDesiredSpeed_ = 0;
    } else {
        Car::tObj *car = carObj_;
        if (car->pullOver == 1 || car->RSControl)
            car->AIFlags |= 2;
        car = carObj_;
        requestedDesiredSpeed_ = 0;
        freezeMode_ = FREEZE_NONE;
        car->RSControl = 0;
        carObj_->pullOver = 0;
    }
}

void AIHigh_BTC_HumanCop::RequestWingman(void)
{
    FUNCTION_MARKER(0x8005DBF8, "NFS4.EXE");
    if ((uint32)freezeMode_ - 3 < 2)
        return;
    sint32 wingmanActivationTime = (sint32)(((uint32)GameSetup::gData.perpInfo[currentStage_].WingmanTime << 6) +
        (uint32)AITune::BTC[GameSetup::gData.skill].wingmanTime);
    Speech::Mobile(carObj_)->Backup();
    sint32 chaseStart = chaseStartTime_;
    if (chaseStart && wingmanActivationTime &&
        wingmanActivationTime < (sint32)((uint32)simGlobal.gameTicks - (uint32)chaseStart) &&
        wingmanStatus_ != WINGMAN_WINGMAN_ACTIVE) {
        wingmanStatus_ = WINGMAN_WINGMAN_REQUEST;
    } else {
        Speech::Dispatch()->Deny();
    }
}

void AIHigh_BTC_HumanCop::RequestBlockader(sint32 spikeBeltRequest)
{
    FUNCTION_MARKER(0x8005DD14, "NFS4.EXE");
    if ((uint32)freezeMode_ - 3 < 2)
        return;
    sint32 availableTime;
    if (spikeBeltRequest) {
        Speech::Mobile(carObj_)->SpikeBelt();
        availableTime = (sint32)(((uint32)GameSetup::gData.perpInfo[currentStage_].SpikeBeltTime << 6) +
            (uint32)AITune::BTC[GameSetup::gData.skill].spikeBeltTime);
    } else {
        Speech::Mobile(carObj_)->RoadBlock();
        availableTime = (sint32)(((uint32)GameSetup::gData.perpInfo[currentStage_].BlockadeCopTime << 6) +
            (uint32)AITune::BTC[GameSetup::gData.skill].blockaderTime);
    }
    sint32 chaseStart = chaseStartTime_;
    if (chaseStart && availableTime > 0 &&
        availableTime < (sint32)((uint32)simGlobal.gameTicks - (uint32)chaseStart) &&
        wingmanStatus_ != WINGMAN_BLOCKADER_ACTIVE) {
        Speech::Dispatch()->Grant();
        wingmanStatus_ = spikeBeltRequest ? WINGMAN_BLOCKADER_SPIKEBELT_REQUEST : WINGMAN_BLOCKADER_REQUEST;
    } else {
        Speech::Dispatch()->Deny();
    }
}

sint32 AIHigh_BTC_HumanCop::CheckForWingmanRequest(void)
{
    FUNCTION_MARKER(0x8005DEEC, "NFS4.EXE");
    if (wingmanStatus_ == WINGMAN_WINGMAN_REQUEST) {
        wingmanStatus_ = WINGMAN_WINGMAN_ACTIVE;
        return 1;
    }
    return 0;
}

sint32 AIHigh_BTC_HumanCop::CheckForBlockaderRequest(sint32 *spikeBeltRequest)
{
    FUNCTION_MARKER(0x8005DF10, "NFS4.EXE");
    *spikeBeltRequest = 0;
    sint32 status = wingmanStatus_;
    if ((uint32)status - 2 < 2) {
        if (status == WINGMAN_BLOCKADER_SPIKEBELT_REQUEST)
            *spikeBeltRequest = 1;
        wingmanStatus_ = WINGMAN_BLOCKADER_ACTIVE;
        return 1;
    }
    return 0;
}

void AIHigh_BTC_HumanCop::UpdateWingmanRole(Wingman_Role currentRole)
{
    FUNCTION_MARKER(0x8005DF54, "NFS4.EXE");
    switch (currentRole) {
    case ROLE_IDLE:
        wingmanStatus_ = WINGMAN_NONE;
        break;
    case ROLE_WINGMAN:
        wingmanStatus_ = WINGMAN_WINGMAN_ACTIVE;
        break;
    case ROLE_BLOCKADER:
    case ROLE_BLOCKADER_SPIKEBELT:
        wingmanStatus_ = WINGMAN_BLOCKADER_ACTIVE;
        break;
    default:
        break;
    }
}

void AIHigh_BTC_HumanCop::ClearTrafficToPurgatory(void)
{
    FUNCTION_MARKER(0x8005DFA0, "NFS4.EXE");
    sint32 trafficCount = Cars::gNumTrafficCars;
    for (sint32 trafficLoop = 0; trafficLoop < trafficCount; ++trafficLoop) {
        Car::tObj *testTrafficCarObj = Cars::gTrafficCarList[trafficLoop];
        AIHigh_Traffic *testTrafficHigh = static_cast<AIHigh_Traffic *>(highLevelAIObjs[testTrafficCarObj->carIndex]);
        testTrafficHigh->forcePurgatory_ = 1;
    }
}

void AIHigh_BTC_HumanCop::ResetClearTrafficToPurgatory(void)
{
    FUNCTION_MARKER(0x8005DFF8, "NFS4.EXE");
    sint32 trafficCount = Cars::gNumTrafficCars;
    for (sint32 trafficLoop = 0; trafficLoop < trafficCount; ++trafficLoop) {
        Car::tObj *testTrafficCarObj = Cars::gTrafficCarList[trafficLoop];
        AIHigh_Traffic *testTrafficHigh = static_cast<AIHigh_Traffic *>(highLevelAIObjs[testTrafficCarObj->carIndex]);
        testTrafficHigh->forcePurgatory_ = 0;
    }
}

void AIHigh_BTC_HumanCop::SetDesiredSpeed(void)
{
    FUNCTION_MARKER(0x8005E04C, "NFS4.EXE");
    Car::tObj *carObj = carObj_;
    if (carObj->RSControl) {
        sint32 curveSpeed = AISpeeds::CalcHumanTopSpeed(carObj);
        sint32 requestedSpeed = requestedDesiredSpeed_;
        carObj = carObj_;
        if (requestedSpeed < curveSpeed)
            curveSpeed = requestedSpeed;
        carObj->desiredSpeed = curveSpeed;
    }
}

void AIHigh_BTC_HumanCop::HighExecute(void)
{
    FUNCTION_MARKER(0x8005E0AC, "NFS4.EXE");
    if (copIndex_ == 0 && freezeMode_ == FREEZE_NEWPERP_ROVE)
        ClearTrafficToPurgatory();
    AIHigh_BTC_Perp *target = perpTarget_;
    if (target && freezeMode_ != FREEZE_NEWPERP_ROVE)
        carObj_->desiredDirection = target->carObj_->direction;
    SetDesiredSpeed();
    CheckConditionWithCop0();
    UpdateFreezeModeAndPullOverMode();
    UpdateAndCheckTimeLeft();
    CheckForNewTarget();
    if (perpTarget_) {
        coorddef notUsed;
        GetCheckChasePosition(&notUsed);
    }
    if (AIHigh::CopGameType == AIHigh::COP_GAME_BTC_1HC) {
        sint32 elapsed = (sint32)((uint32)simGlobal.gameTicks - (uint32)lastInputRequestTick_);
        if (elapsed >= 641) {
            if (carObj_->control.queuedEvent == 4) {
                RequestWingman();
                carObj_->control.queuedEvent = 0;
                lastInputRequestTick_ = simGlobal.gameTicks;
            }
            if (carObj_->control.queuedEvent == 5) {
                RequestBlockader(0);
                carObj_->control.queuedEvent = 0;
                lastInputRequestTick_ = simGlobal.gameTicks;
            }
            if (carObj_->control.queuedEvent == 6) {
                RequestBlockader(1);
                carObj_->control.queuedEvent = 0;
                lastInputRequestTick_ = simGlobal.gameTicks;
            }
        } else {
            Car::tObj *car = carObj_;
            if ((uint32)(uint8)car->control.queuedEvent - 4 < 3)
                car->control.queuedEvent = 0;
        }
    }
    state_->StateExecute();
}

void AIHigh_BTC_HumanCop::HudOn(AIHigh_BTC_Perp *arrestMe, sint32 gameOver, Car::tObj *arrestingCop)
{
    FUNCTION_MARKER(0x8005E284, "NFS4.EXE");
    Car::tObj *arrestingHumanCop = arrestingCop;
    if ((arrestingHumanCop->carFlags & 2) != 0)
        arrestingHumanCop = Cars::gHumanRaceCarList[0];
    if (copIndex_ == 0 || gameOver) {
        sint32 elapsed = (sint32)((uint32)simGlobal.gameTicks - (uint32)chaseStartTime_);
        Hud::BustedOverlayOn(elapsed, arrestMe->carObj_->carInfo->driver,
            gameOver == 0, (sint16)arrestingHumanCop->carIndex);
    }
}

AIHigh_BTC_Wingman::AIHigh_BTC_Wingman(Car::tObj *carObj, sint32 copIndex) : AIHigh_BTC_Cop(carObj, copIndex)
{
    FUNCTION_MARKER(0x8005E2F0, "NFS4.EXE");
    spikeBeltPlaced_ = 0;
    spikeBeltSlice_ = 0;
    spikeBeltInterceptReleaseTime_ = 0;
    currentRole_ = ROLE_IDLE;
    newRole_ = ROLE_IDLE;
    newHumanBoss_ = 0;
}

static void ReplaceWingmanState(AIHigh_BTC_Wingman &wingman, AIState_Base *newState, stateType_t type)
{
    delete wingman.state_;
    wingman.state_ = newState;
    wingman.stateType_ = type;
}

static sint32 WingmanAbs(sint32 value)
{
    return value < 0 ? (sint32)(0U - (uint32)value) : value;
}

void AIHigh_BTC_Wingman::HighExecute(void)
{
    FUNCTION_MARKER(0x8005E33C, "NFS4.EXE");
    CheckSpikeBelt();
    CheckForActivation();
    switch (stateType_)
    {
    case STATE_NONE:
        carObj_->AIFlags &= ~2;
        ReplaceWingmanState(*this, new AIState_NonActive(carObj_), STATE_NONACTIVE);
        break;

    case STATE_NORMAL:
        carObj_->AIFlags &= ~2;
        if (newRole_ != currentRole_ &&
            (newRole_ == ROLE_BLOCKADER || newRole_ == ROLE_BLOCKADER_SPIKEBELT))
        {
            if (!AILife::IsCarInAnyVisibleArea(carObj_))
            {
                Speech::Mobile(carObj_)->Purge();
                currentRole_ = newRole_;
                SetupBlockader(newHumanBoss_, newRole_ == ROLE_BLOCKADER_SPIKEBELT);
                ReplaceWingmanState(*this, new AIState_Idle(carObj_), STATE_IDLE);
            }
        }
        else
        {
            CheckForNewTarget();
            if (perpTarget_)
            {
                coorddef pos;
                GetCheckChasePosition(&pos);
                ReplaceWingmanState(*this, new AIState_Chase(carObj_, perpTarget_->carObj_, &pos,
                    0x200, 0x3C0000, 0x190000, 2, 0x10000), STATE_CHASE);
            }
        }
        goto checkFreeze;

    case STATE_CHASE:
    {
        carObj_->AIFlags |= 2;
        AIState_Chase *chaseState = static_cast<AIState_Chase *>(state_);
        HandleBlockadeSpeech();
        coorddef newPos;
        if (GetCheckChasePosition(&newPos))
            chaseState->SetTarget(perpTarget_->carObj_, &newPos);
        const bool leavingChase = chaseState->barrierTicks32_ > 160;
        if (leavingChase)
        {
            sint32 endSlice = chaseState->FindBarrierEndSlice();
            ReplaceWingmanState(*this, new AIState_GotoSlice(carObj_, endSlice, 0), STATE_GOTOSLICE);
        }
        if (CheckForNewTarget())
        {
            coorddef pos;
            GetCheckChasePosition(&pos);
            if (!leavingChase)
                chaseState->SetTarget(perpTarget_->carObj_, &pos);
        }
        if (!leavingChase && chaseState->inTargetRegion_ > 8 &&
            WingmanAbs(chaseState->latMetersBetween_) < 0xE0000 &&
            WingmanAbs(chaseState->longMetersBetween_) < 0xF0000)
            chaseState->SetMurderMode(1, 0x300);

        if (!perpTarget_)
        {
            AssignToPlayer(0);
            ReplaceWingmanState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
        }
        if (newRole_ != currentRole_ &&
            (newRole_ == ROLE_BLOCKADER || newRole_ == ROLE_BLOCKADER_SPIKEBELT))
        {
            carObj_->desiredDirection = (sint32)(0U - (uint32)carObj_->desiredDirection);
            AssignToPlayer(0);
            ReplaceWingmanState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
            Speech::Mobile(carObj_)->Lose();
        }
        goto checkFreeze;
    }

    case STATE_IDLE:
    {
        carObj_->AIFlags |= 2;
        CheckForNewTarget();
        if (!perpTarget_)
        {
            newRole_ = ROLE_WINGMAN;
            currentRole_ = ROLE_WINGMAN;
            ReplaceWingmanState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
            return;
        }

        coorddef newPos;
        GetCheckChasePosition(&newPos);
        if (spikeBeltPlaced_ && AICop::spikeBelt.slice_ == spikeBeltSlice_)
            AICop::spikeBelt.freshenTime_ = simGlobal.gameTicks;
        sint32 rbDistanceMeters = AIWorld::ApxSplineDistance(carObj_, perpTarget_->carObj_);
        sint32 rbAbsDistanceMeters = WingmanAbs(rbDistanceMeters);
        bool release = rbAbsDistanceMeters < 0x320000;
        if (!release && rbAbsDistanceMeters < 0x12C0000)
        {
            sint32 speed = perpTarget_->carObj_->currentSpeed;
            if (WingmanAbs(speed) > 0x471C7)
            {
                sint32 timeToRB = fixeddiv(rbDistanceMeters, speed);
                release = timeToRB > 0 && timeToRB < spikeBeltInterceptReleaseTime_;
            }
        }
        if (release)
        {
            spikeBeltPlaced_ = 0;
            newRole_ = ROLE_WINGMAN;
            currentRole_ = ROLE_WINGMAN;
            ReplaceWingmanState(*this, new AIState_Chase(carObj_, perpTarget_->carObj_, &newPos,
                0x200, 0x3C0000, 0x190000, 2, 0x10000), STATE_CHASE);
        }
        if (newRole_ != currentRole_ && newRole_ == ROLE_WINGMAN)
        {
            currentRole_ = ROLE_WINGMAN;
            ReplaceWingmanState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
        }
    }
    checkFreeze:
        if (UpdateFreezeModeAndPullOverMode())
        {
            AssignToPlayer(0);
            ReplaceWingmanState(*this, new AIState_NonActive(carObj_), STATE_NONACTIVE);
            newRole_ = ROLE_IDLE;
            currentRole_ = ROLE_IDLE;
        }
        break;

    case STATE_NONACTIVE:
        carObj_->AIFlags &= ~2;
        if (newRole_ == currentRole_)
            break;
        if (newRole_ == ROLE_WINGMAN)
        {
            currentRole_ = ROLE_WINGMAN;
            SetupWingman(newHumanBoss_);
            ReplaceWingmanState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
        }
        else if (newRole_ == ROLE_BLOCKADER || newRole_ == ROLE_BLOCKADER_SPIKEBELT)
        {
            currentRole_ = newRole_;
            SetupBlockader(newHumanBoss_, newRole_ == ROLE_BLOCKADER_SPIKEBELT);
            ReplaceWingmanState(*this, new AIState_Idle(carObj_), STATE_IDLE);
        }
        break;

    case STATE_GOTOSLICE:
    {
        AIState_GotoSlice *gotoState = static_cast<AIState_GotoSlice *>(state_);
        AssignToPlayer(0);
        if (gotoState->InTargetSliceRange(0xA0000))
            ReplaceWingmanState(*this, new AIState_Normal(carObj_), STATE_NORMAL);
        break;
    }
    default:
        break;
    }
    state_->StateExecute();
}

sint32 AIHigh_BTC_Wingman::CheckForActivation(void)
{
    FUNCTION_MARKER(0x8005EDC8, "NFS4.EXE");
    sint32 spikeBeltRequest = 0;
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
        Car::tObj *otherCarObj = Cars::gList[carLoop];
        if ((otherCarObj->carFlags & 0x200) != 0 && otherCarObj->N.active) {
            AIHigh_BTC_HumanCop *copHigh = static_cast<AIHigh_BTC_HumanCop *>(highLevelAIObjs[otherCarObj->carIndex]);
            if (copHigh->CheckForWingmanRequest()) {
                newRole_ = ROLE_WINGMAN;
                newHumanBoss_ = copHigh;
            }
            if (copHigh->CheckForBlockaderRequest(&spikeBeltRequest)) {
                newRole_ = spikeBeltRequest ? ROLE_BLOCKADER_SPIKEBELT : ROLE_BLOCKADER;
                newHumanBoss_ = copHigh;
            }
        }
    }
    if (newHumanBoss_)
        newHumanBoss_->UpdateWingmanRole(currentRole_);
    return currentRole_ != newRole_;
}

sint32 AIHigh_BTC_Wingman::UpdateFreezeModeAndPullOverMode(void)
{
    FUNCTION_MARKER(0x8005EEF8, "NFS4.EXE");
    if (freezeMode_ == FREEZE_ARREST) {
        carObj_->pullOver = 1;
        return 0;
    }
    carObj_->pullOver = 0;
    sint32 mode = freezeMode_;
    freezeMode_ = FREEZE_NONE;
    return mode == FREEZE_ARRESTDONE;
}

void AIHigh_BTC_Wingman::SetupWingman(AIHigh_BTC_HumanCop *humanCop)
{
    FUNCTION_MARKER(0x8005EF38, "NFS4.EXE");
    Car::tObj *otherCarObj = humanCop->carObj_;
    sint32 side = otherCarObj->currentSpeed < 0 ? -1 : 1;
    uint32 product = fastRandom * randSeed;
    randtemp = product;
    fastRandom = product & 0xFFFF;
    sint32 sample = (((product >> 8) & 0xFFFF) * 1000) >> 16;
    sint32 perpSide = sample < 500 ? 1 : -1;
    sint32 offset = perpSide * side * 28;
    sint32 slice = (sint32)((uint32)otherCarObj->N.simRoadInfo.slice + (uint32)offset);
    if (offset >= 0) {
        if (slice >= gNumSlices)
            slice = (sint32)((uint32)slice - (uint32)gNumSlices);
    } else if (slice < 0) {
        slice = (sint32)((uint32)slice + (uint32)gNumSlices);
    }
    AILife::PlaceCarAtLocation(carObj_, slice, 0, side, otherCarObj->currentSpeed, 0);
    Speech::Dispatch()->Ready(carObj_);
}

void AIHigh_BTC_Wingman::SetupBlockader(AIHigh_BTC_HumanCop *humanCop, sint32 spikeBeltRequest)
{
    FUNCTION_MARKER(0x8005F074, "NFS4.EXE");
    Car::tObj *copObj = humanCop->carObj_;
    sint32 initSlice;
    if (humanCop->perpTarget_ != 0)
    {
        Car::tObj *perpObj = humanCop->perpTarget_->carObj_;
        sint32 side = perpObj->currentSpeed < 0 ? -1 : 1;
        sint32 perpToHumanDistance = AIWorld::ApxSplineDistance(perpObj, copObj);
        sint32 distance = perpToHumanDistance;
        if (distance < 0)
            distance = (sint32)(0u - (uint32)distance);
        sint32 initializationDistance = distance > 0x1F40000 ? distance : 0x1F40000;
        if (initializationDistance >= 0x5DC0000)
            initializationDistance = 0x5DC0000;
        sint32 offset = (sint32)((uint32)(initializationDistance / 0x60000) * (uint32)side);
        if ((sint32)((uint32)perpToHumanDistance * (uint32)side) < 0)
            initSlice = BtcOffsetSlice(copObj->N.simRoadInfo.slice, offset);
        else
            initSlice = BtcOffsetSlice(perpObj->N.simRoadInfo.slice, offset);
        blockade_.blockadeSpeechFlags = 1;
        blockade_.target = humanCop->perpTarget_;
    }
    else
    {
        sint32 side = copObj->currentSpeed < 0 ? -1 : 1;
        initSlice = BtcOffsetSlice(copObj->N.simRoadInfo.slice, side * 83);
        blockade_.blockadeSpeechFlags = 0;
        blockade_.target = 0;
    }
    sint32 direction = copObj->direction;
    blockade_.direction = direction;
    sint32 slice = BtcOffsetSlice(initSlice, (sint32)(0u - (uint32)direction));
    uint32 product = fastRandom * randSeed;
    blockade_.slice = slice;
    AICop::gRoadBlockState = AICop::kAICop_RoadBlockState_WaitingForPerp;
    randtemp = product;
    fastRandom = product & 0xFFFFu;
    sint32 blockadeType = (uint8)gBlockadeTypes[((product >> 8) & 0xFFFFu) % 5u];
    blockade_.flags = blockadeType;
    if (blockadeType != 0)
    {
        if (stackSpeedUpEnbabledFlag != 0)
        {
            stackSpeedUpEnbabledFlag = 0;
            SetupBlockadeElements(&blockade_);
            stackSpeedUpEnbabledFlag = 1;
        }
        else
            SetupBlockadeElements(&blockade_);
    }
    AILife::ReencarnateCopByLatPosAndRotation(carObj_, initSlice, humanCop->carObj_->direction, 0, 0x100);
    if (spikeBeltRequest != 0)
    {
        sint32 distance = AIWorld::ApxSplineDistance(carObj_, copObj);
        sint32 spikeBeltSide = distance < 0 ? -1 : 1;
        spikeBeltSlice_ = BtcOffsetSlice(carObj_->N.simRoadInfo.slice, spikeBeltSide * 6);
        product = fastRandom * randSeed;
        randtemp = product;
        fastRandom = product & 0xFFFFu;
        spikeBeltInterceptReleaseTime_ = (sint32)(((((product >> 8) & 0xFFFFu) * 0x14CCDu) >> 16) + 0xD999u);
        Trk::NewSlice *roadSlice = &BWorldSm::slices[spikeBeltSlice_];
        sint32 left = fixedmult((sint32)(((uint32)(uint8)roadSlice->avgPavedWidthLf << 15) *
            ((uint8)roadSlice->laneCount >> 4)), 0xCCCC);
        roadSlice = &BWorldSm::slices[spikeBeltSlice_];
        sint32 right = fixedmult((sint32)(((uint32)(uint8)roadSlice->avgPavedWidthRt << 15) *
            ((uint8)roadSlice->laneCount & 15u)), 0xCCCC);
        AICop::spikeBelt.leftLatPos_ = (sint32)(0u - (uint32)left);
        AICop::spikeBelt.rightLatPos_ = right;
        sint32 slice = spikeBeltSlice_;
        sint32 timeNow = simGlobal.gameTicks;
        AICop::spikeBelt.active_ = 1;
        AICop::spikeBelt.slice_ = slice;
        AICop::spikeBelt.freshenTime_ = timeNow;
        BWorld::SetSpikeBelt(spikeBeltSlice_, (sint32)(0u - (uint32)left), (sint32)((uint32)left + (uint32)right));
        spikeBeltPlaced_ = 1;
    }
    Speech::Dispatch()->Ready(carObj_);
}
