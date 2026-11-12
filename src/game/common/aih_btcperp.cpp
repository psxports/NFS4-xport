#include <new>
#include <stddef.h>
#include "xport.h"
#include "game/common/ai.h"
#include "game/common/aihigh.h"
#include "game/common/ailife.h"
#include "game/common/aistate.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/audioclc.h"
#include "game/common/bworldsm.h"
#include "game/common/cameraobj.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/fastrand.h"
#include "game/common/gmesetup.h"
#include "game/common/object.h"
#include "game/common/r3dcar.h"
#include "game/common/sim.h"
#include "game/common/speech.h"
#include "game/common/trgsfx.h"
#include "game/psx/hud.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

extern uint8 strategyChart[5][3];

uint8 strategyChart[5][3] =
{
    {4, 4, 4},
    {0, 0, 0},
    {1, 0, 1},
    {1, 1, 1},
    {2, 2, 2}
};

sint32 AIHigh::BTC_uTurnProb1000Skills[3] = { 3, 4, 5 };

void AIHigh_BTC_Perp::ReleaseCops(void)
{
    FUNCTION_MARKER(0x8005F6B4, "NFS4.EXE");
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
        Car::tObj *otherCarObj = Cars::gList[carLoop];
        if ((otherCarObj->carFlags & 0x200) != 0 && otherCarObj->N.active != 0) {
            AIHigh_BTC_HumanCop *cop = static_cast<AIHigh_BTC_HumanCop *>(highLevelAIObjs[otherCarObj->carIndex]);
            cop->ReleaseAndStartChase(this);
        }
    }
    carObj_->forceNoSimOptz = 0;
}

void AIHigh_BTC_Perp::HandleCops(void)
{
    FUNCTION_MARKER(0x8005F778, "NFS4.EXE");
    HandlePullOver();
}

sint32 AIHigh_BTC_Perp::IsFalseArrest(void)
{
    FUNCTION_MARKER(0x8005F798, "NFS4.EXE");
    uint32 product = fastRandom * randSeed;
    randtemp = product;
    fastRandom = product & 0xFFFF;
    sint32 randNum1000 = (((product >> 8) & 0xFFFF) * 1000) >> 16;
    if ((carObj_->carFlags & 4) != 0 || randNum1000 < 980)
        return 0;
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
        Car::tObj *cop = Cars::gList[carLoop];
        if ((cop->carFlags & 0x200) != 0) {
            coorddef carCopVector;
            carCopVector.x = (cop->N.position.x - carObj_->N.position.x);
            carCopVector.y = (cop->N.position.y - carObj_->N.position.y);
            carCopVector.z = (cop->N.position.z - carObj_->N.position.z);
            sint32 first = fixedmult(carCopVector.x, carObj_->N.orientMat.m[0]);
            sint32 second = fixedmult(carCopVector.y, carObj_->N.orientMat.m[1]);
            sint32 third = fixedmult(carCopVector.z, carObj_->N.orientMat.m[2]);
            sint32 xDot = ((first + second) + third);
            first = fixedmult(carCopVector.x, carObj_->N.orientMat.m[6]);
            second = fixedmult(carCopVector.y, carObj_->N.orientMat.m[7]);
            third = fixedmult(carCopVector.z, carObj_->N.orientMat.m[8]);
            sint32 zDot = ((first + second) + third);
            if (xDot < 0)
                xDot = (-xDot);
            if (xDot > 0x30000 || zDot > 0x80000 || zDot < 0) {
                AudioClc::HonkHorn(carObj_, 2, 128, 32);
                return 1;
            }
        }
    }
    return 0;
}

sint32 AIHigh_BTC_Perp::CheckForControlsPressed(void)
{
    FUNCTION_MARKER(0x8005F9B8, "NFS4.EXE");
    sint32 pressed = 0;
    Car::tObj *car = Cars::gHumanRaceCarList[0];
    if ((car->carFlags & 0x200) != 0) {
        Car::tControl *control = &car->control;
        if (control->desiredGasLevel || control->desiredBrakeLevel || control->handBrake == 1)
            pressed = 1;
    }
    if (Cars::gNumHumanRaceCars == 2) {
        car = Cars::gHumanRaceCarList[1];
        if ((car->carFlags & 0x200) != 0) {
            Car::tControl *control = &car->control;
            if (control->desiredGasLevel || control->desiredBrakeLevel || control->handBrake == 1)
                pressed = 1;
        }
    }
    return pressed;
}

void AIHigh_BTC_Perp::HandlePullOver(void)
{
    FUNCTION_MARKER(0x8005FA58, "NFS4.EXE");
    if (pullOverMode_ != PULLOVER_NONE)
    {
        NotifyCopsOfArrest();
        beatingTicksLeft_ = (beatingTicksLeft_ - AI::elapsedTime);
        if (beatingTicksLeft_ <= 0 && hudActivated_ == 0)
        {
            if (IsFalseArrest() != 0)
            {
                lastPullOverTime_ = (simGlobal.gameTicks - 640);
                NotifyCopsOfFalseArrest();
                carObj_->pullOver = 0;
                pullOverMode_ = PULLOVER_NONE;
                Speech::Mobile(Cars::gList[0])->Lose();
            }
            else if (hudActivated_ == 0)
            {
                NotifyHumanCopsOfArrestHud();
                hudActivated_ = 1;
            }
        }
        sint32 userReadyToContinue = CheckForControlsPressed();
        if (beatingTicksLeft_ <= 0 && pullOverMode_ != PULLOVER_NONE && userReadyToContinue != 0 && hudActivated_ == 1 && (simGlobal.gameTicks - lastPullOverTime_) > 320)
        {
            lastPullOverTime_ = simGlobal.gameTicks;
            basicPerpInfo_.crime_ = CRIME_NONE;
            NotifyCopsOfArrestComplete();
            carObj_->pullOver = 0;
            pullOverMode_ = PULLOVER_NONE;
            caught_ = 1;
            hudActivated_ = 0;
        }
    }
    else if (originalActivationCop_->timeLeft_ >= 6 && CheckIfCaught() != 0)
    {
        carObj_->pullOver = 1;
        beatingTicksLeft_ = 96;
        pullOverMode_ = PULLOVER_BUST;
        lastPullOverTime_ = simGlobal.gameTicks;
    }
}

void AIHigh_BTC_Perp::NotifyCopsOfArrest(void)
{
    FUNCTION_MARKER(0x8005FC30, "NFS4.EXE");
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
        Car::tObj *otherCarObj = Cars::gList[carLoop];
        if ((otherCarObj->carFlags & 0x220) != 0 && otherCarObj->N.active != 0) {
            AIHigh_BTC_Cop *cop = static_cast<AIHigh_BTC_Cop *>(highLevelAIObjs[otherCarObj->carIndex]);
            cop->StartArrest(this);
        }
    }
}

void AIHigh_BTC_Perp::NotifyCopsOfArrestComplete(void)
{
    FUNCTION_MARKER(0x8005FCE8, "NFS4.EXE");
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
        Car::tObj *otherCarObj = Cars::gList[carLoop];
        if ((otherCarObj->carFlags & 0x220) != 0 && otherCarObj->N.active != 0) {
            AIHigh_BTC_Cop *cop = static_cast<AIHigh_BTC_Cop *>(highLevelAIObjs[otherCarObj->carIndex]);
            cop->FinishArrest(this);
        }
    }
}

void AIHigh_BTC_Perp::NotifyCopsOfFalseArrest(void)
{
    FUNCTION_MARKER(0x8005FDA0, "NFS4.EXE");
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
        Car::tObj *otherCarObj = Cars::gList[carLoop];
        if ((otherCarObj->carFlags & 0x220) != 0 && otherCarObj->N.active != 0) {
            AIHigh_BTC_Cop *cop = static_cast<AIHigh_BTC_Cop *>(highLevelAIObjs[otherCarObj->carIndex]);
            cop->FalseArrest(this);
        }
    }
}

void AIHigh_BTC_Perp::NotifyHumanCopsOfArrestHud(void)
{
    FUNCTION_MARKER(0x8005FE58, "NFS4.EXE");
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
        Car::tObj *otherCarObj = Cars::gList[carLoop];
        if ((otherCarObj->carFlags & 0x200) != 0 && otherCarObj->N.active != 0) {
            AIHigh_BTC_HumanCop *cop = static_cast<AIHigh_BTC_HumanCop *>(highLevelAIObjs[otherCarObj->carIndex]);
            cop->HudOn(this, 0, lastArrestingCop_);
        }
    }
}

void AIHigh_BTC_Perp::ClearForNewStage(AIHigh_BTC_HumanCop *chaserCop)
{
    FUNCTION_MARKER(0x8005FF10, "NFS4.EXE");
    Clear();
    Car::tObj *car = carObj_;
    basicPerpInfo_.copsAssigned_[0] = 0;
    basicPerpInfo_.copsAssigned_[1] = 0;
    basicPerpInfo_.crime_ = CRIME_SPEEDER;
    caught_ = 0;
    car->unlap = 0;
    Car::tObj *chaserCar = chaserCop->carObj_;
    car = carObj_;
    car->lap = chaserCar->lap;
    chaserCop->needPerp_ = 0;
}

AIHigh_BTC_HumanCop *AIHigh_BTC_Perp::CheckForActivation(void)
{
    FUNCTION_MARKER(0x8005FF74, "NFS4.EXE");
    sint32 count = Cars::gNumCars;
    for (sint32 carLoop = 0; carLoop < count; ++carLoop) {
        Car::tObj *humanCopCarObj = Cars::gList[carLoop];
        if ((humanCopCarObj->carFlags & 0x200) != 0 && humanCopCarObj->N.active != 0) {
            AIHigh_BTC_HumanCop *carHigh = static_cast<AIHigh_BTC_HumanCop *>( highLevelAIObjs[humanCopCarObj->carIndex]);
            if (carHigh->copIndex_ == 0 && carHigh->needPerp_ != 0) {
                Car::tObj *car = carObj_;
                if ((car->carFlags & 4) != 0)
                    return carHigh;
                sint32 carType = GameSetup::gData.perpInfo[carHigh->currentStage_].CarType;
                if (carType == car->carInfo->carType)
                    return carHigh;
            }
        }
    }
    return 0;
}

void AIHigh_BTC_HumanPerp::NewStage(AIHigh_BTC_HumanCop *chaserCop)
{
    FUNCTION_MARKER(0x8006007C, "NFS4.EXE");
    sint32 humanDirection = chaserCop->initialDirection_;
    originalActivationCop_ = chaserCop;
    ClearForNewStage(chaserCop);
    ReleaseCops();
    uint32 product = fastRandom * randSeed;
    Car::tObj *car = carObj_;
    randtemp = product;
    fastRandom = product & 0xFFFF;
    sint32 offset = ((uint32)humanDirection << 4);
    sint32 chaserSlice = chaserCop->carObj_->N.simRoadInfo.slice;
    sint32 nextSlice = (chaserSlice + offset);
    if (offset >= 0) {
        if (nextSlice >= gNumSlices)
            nextSlice = (nextSlice - gNumSlices);
    } else if (nextSlice < 0) {
        nextSlice = (nextSlice + gNumSlices);
    }
    car->N.simRoadInfo.slice = (sint16)nextSlice;
    carObj_->desiredDirection = humanDirection;
    car = carObj_;
    car->direction = car->desiredDirection;
    sint32 throwAway = 0;
    sint32 newLatPos = 0;
    AIWorld::FindBarrierLessLaneAndPosition(carObj_, &throwAway, &newLatPos);
    car = carObj_;
    AILife::PlaceCarAtLocation(carObj_, car->N.simRoadInfo.slice, newLatPos, car->direction, 0, 0);
    Speech::Speaker *speaker = Speech::Mobile(chaserCop->carObj_);
    speaker->Purge();
    speaker = Speech::Dispatch();
    speaker->Report(carObj_);
    speaker = Speech::Mobile(chaserCop->carObj_);
    speaker->Report(carObj_);
    speaker = Speech::Mobile(chaserCop->carObj_);
    speaker->Engage(carObj_);
    TrgSfx::RestartTrgSfx();
}

void AIHigh_BTC_HumanPerp::HighExecute(void)
{
    FUNCTION_MARKER(0x8006029C, "NFS4.EXE");
    if (caught_ == 1) {
        AIHigh_BTC_HumanCop *chaserCop = CheckForActivation();
        if (chaserCop) {
            NewStage(chaserCop);
            return;
        }
    }
    HandleCops();
}

AIHigh_BTC_Perp::AIHigh_BTC_Perp(Car::tObj *carObj) : AIHigh_BasicPerp(carObj)
{
    caught_ = 1;
    hudActivated_ = 0;
    originalActivationCop_ = 0;
}

AIHigh_BTC_HumanPerp::AIHigh_BTC_HumanPerp(Car::tObj *carObj) : AIHigh_BTC_Perp(carObj)
{
}

AIHigh_BTC_AIPerp::AIHigh_BTC_AIPerp(Car::tObj *carObj) : AIHigh_BTC_Perp(carObj)
{
    FUNCTION_MARKER(0x800602F4, "NFS4.EXE");
    Car::tObj *car = carObj_;
    perpMode_ = PERPMODE_CRUISE;
    creationTime_ = 0;
    madeContactTime_ = 0;
    timeUntilContact_ = 64000;
    escapeDuration_ = 0;
    originalMass_ = car->N.mass;
    sint32 massInv = car->N.massInv;
    closestCopCarObj_ = 0;
    closestCopCarDistanceMeters_ = 0;
    originalMassInv_ = massInv;
}

AIHigh_BTC_AIPerp::~AIHigh_BTC_AIPerp(void)
{
    FUNCTION_MARKER(0x8006037C, "NFS4.EXE");
    carObj_->N.mass = originalMass_;
    carObj_->N.massInv = originalMassInv_;
}

AIHigh_BTC_Perp::~AIHigh_BTC_Perp(void)
{
    FUNCTION_MARKER(0x8005B438, "NFS4.EXE");
}

void AIHigh_BTC_AIPerp::AvoidCops(void)
{
    FUNCTION_MARKER(0x800603CC, "NFS4.EXE");
    sint32 doBrake = 0;
    do {
        Car::tObj *cop = closestCopCarObj_;
        if (!cop || cop->RSControl)
            break;
        Car::tObj *car = carObj_;
        sint32 direction = cop->direction;
        if (direction != car->direction)
            break;
        sint32 distance = closestCopCarDistanceMeters_;
        sint32 absDistance = distance;
        if (absDistance < 0)
            absDistance = (-absDistance);
        if (absDistance >= 0x1F40000)
            break;
        sint32 xPosition = ((car->roadPosition - cop->roadPosition) * direction);
        sint32 zPosition = (distance * direction);
        sint32 halfWidth = cop->N.dimension.x;
        sint32 negativeHalfWidth = (-halfWidth);
        sint32 xPositionIndex = 0;
        if (xPosition >= negativeHalfWidth)
            xPositionIndex = xPosition > halfWidth ? 2 : 1;
        sint32 zPositionIndex = 0;
        if (zPosition <= 0x190000) {
            sint32 halfLength = closestCopCarObj_->N.dimension.z;
            zPositionIndex = 1;
            if (zPosition <= halfLength) {
                zPositionIndex = 4;
                if (zPosition >= -0x190000) {
                    sint32 negativeHalfLength = (-halfLength);
                    zPositionIndex = zPosition < negativeHalfLength ? 3 : 2;
                }
            }
        }
        if ((strategyChart[zPositionIndex][xPositionIndex] & 1) != 0) {
            uint32 product = fastRandom * randSeed;
            car = carObj_;
            sint32 wasPullingOver = car->pullOver;
            randtemp = product;
            fastRandom = product & 0xFFFF;
            uint32 sample = (((product >> 8) & 0xFFFF) * 1000) >> 16;
            uint32 brakeChance = (uint32)AI::elapsedTime * 7 + (uint32)wasPullingOver * 500;
            if (sample < brakeChance) {
                sint32 copSpeed = closestCopCarObj_->currentSpeed;
                if (copSpeed < 0)
                    copSpeed = (-copSpeed);
                if (copSpeed > 0x11C71C) {
                    sint32 ownSpeed = car->currentSpeed;
                    if (ownSpeed < 0)
                        ownSpeed = (-ownSpeed);
                    if (ownSpeed > 0x11C71C) {
                        doBrake = 1;
                        break;
                    }
                }
            }
        }
        if ((strategyChart[zPositionIndex][xPositionIndex] & 2) != 0) {
            uint32 product = fastRandom * randSeed;
            uint32 turnChance = AIHigh::BTC_uTurnProb1000Skills[GameSetup::gData.skill] * (uint32)AI::elapsedTime;
            randtemp = product;
            fastRandom = product & 0xFFFF;
            uint32 sample = (((product >> 8) & 0xFFFF) * 1000) >> 16;
            if (sample < turnChance) {
                sint32 copSpeed = closestCopCarObj_->currentSpeed;
                if (copSpeed < 0)
                    copSpeed = (-copSpeed);
                if (copSpeed > 0x11C71C) {
                    car = carObj_;
                    sint32 ownSpeed = car->currentSpeed;
                    if (ownSpeed < 0)
                        ownSpeed = (-ownSpeed);
                    if (ownSpeed > 0x11C71C && car->direction == car->desiredDirection)
                        car->desiredDirection = (-car->direction);
                }
            }
        }
    } while (0);
    carObj_->pullOver = doBrake;
}

void AIHigh_BTC_AIPerp::CalculateTimeTillContact(void)
{
    FUNCTION_MARKER(0x80060710, "NFS4.EXE");
    Car::tObj *cop = closestCopCarObj_;
    if (cop && (uint32)perpMode_ < 2) {
        sint32 relVel = (carObj_->currentSpeed - cop->currentSpeed);
        sint32 distance = closestCopCarDistanceMeters_;
        sint32 contactTime = 0x3E80000;
        if ((uint32)relVel + 0x7FFF > 0xFFFE) {
            sint32 quotient = fixeddiv(distance, relVel);
            contactTime = -(quotient / 1024);
        }
        timeUntilContact_ = contactTime;
        if (timeUntilContact_ >= 0)
            return;
    }
    timeUntilContact_ = 64000;
}

void AIHigh_BTC_AIPerp::FindClosestCop(void)
{
    FUNCTION_MARKER(0x800607B4, "NFS4.EXE");
    sint32 closestCopInMeters = 0x270F0000;
    sint32 closestCopInMetersAbs = 0x270F0000;
    sint32 closestCarIndex = -1;
    for (sint32 copLoop = 0; copLoop < Cars::gNumHumanRaceCars; ++copLoop) {
        Car::tObj *cop = Cars::gHumanRaceCarList[copLoop];
        if ((cop->carFlags & 0x200) != 0) {
            sint32 longMetersBetween = AIWorld::ApxSplineDistance(carObj_, cop);
            sint32 absLongMetersBetween = longMetersBetween;
            if (absLongMetersBetween < 0)
                absLongMetersBetween = (-absLongMetersBetween);
            if (absLongMetersBetween < closestCopInMetersAbs) {
                closestCopInMeters = longMetersBetween;
                closestCopInMetersAbs = absLongMetersBetween;
                closestCarIndex = Cars::gHumanRaceCarList[copLoop]->carIndex;
            }
        }
    }
    if (closestCarIndex == -1) {
        closestCopCarObj_ = 0;
    } else {
        Car::tObj *cop = Cars::gList[closestCarIndex];
        closestCopCarDistanceMeters_ = closestCopInMeters;
        closestCopCarObj_ = cop;
    }
}

void AIHigh_BTC_AIPerp::HighExecute(void)
{
    FUNCTION_MARKER(0x800608B4, "NFS4.EXE");
    if (carObj_->N.active) {
        FindClosestCop();
        CalculateTimeTillContact();
    }
    switch (stateType_) {
    case STATE_NORMAL:
        switch (perpMode_) {
        case PERPMODE_FAST:
            if (timeUntilContact_ < 320) {
                madeContactTime_ = simGlobal.gameTicks;
                if (perpMode_ != PERPMODE_MADECONTACT)
                    Speech::Mobile(originalActivationCop_->carObj_)->Report(carObj_);
                perpMode_ = PERPMODE_MADECONTACT;
                Camera::gInfo[0].forceFocus = FOCUS_COPANDAI;
                char focus = (char)carObj_->carIndex;
                Camera::gInfo[1].forceFocus = FOCUS_COPANDAI;
                Camera::gInfo[0].focusOnAICar = focus;
                Camera::gInfo[1].focusOnAICar = (char)carObj_->carIndex;
            }
            break;
        case PERPMODE_MADECONTACT:
            perpMode_ = PERPMODE_ESCAPE;
            break;
        case PERPMODE_ESCAPE: {
            sint32 elapsed = (simGlobal.gameTicks - madeContactTime_);
            if (escapeDuration_ < elapsed) {
                perpMode_ = PERPMODE_CHASEON;
                ReleaseCops();
            }
            elapsed = (simGlobal.gameTicks - madeContactTime_);
            if ((escapeDuration_ - 64) < elapsed) {
                if (Camera::gInfo[0].forceFocus != FOCUS_NORMAL) {
                    Speech::Mobile(originalActivationCop_->carObj_)->Engage(carObj_);
                    Camera::ResetRelPos(3);
                }
                Camera::gInfo[0].forceFocus = FOCUS_NORMAL;
                char focus = (char)carObj_->carIndex;
                Camera::gInfo[1].forceFocus = FOCUS_NORMAL;
                Camera::gInfo[0].focusOnAICar = focus;
                Camera::gInfo[1].focusOnAICar = (char)carObj_->carIndex;
            }
            Car::tObj *car = carObj_;
            Car::tObj *otherCarObj = originalActivationCop_->carObj_;
            if (otherCarObj->direction == car->direction) {
                sint32 distance = AIWorld::ApxSplineDistance(car, otherCarObj);
                car = carObj_;
                sint32 directedDistance = (distance * car->direction);
                if (directedDistance > 0) {
                    sint32 speed = car->currentSpeed;
                    if (speed < 0)
                        speed = (-speed);
                    sint32 requestedSpeed = fixedmult(speed, 0xCCCC);
                    originalActivationCop_->requestedDesiredSpeed_ = requestedSpeed;
                }
            }
            break;
        }
        default:
            break;
        }
        if (perpMode_ != PERPMODE_CHASEON)
            break;
        HandleCops();
        if (pullOverMode_ != PULLOVER_BUST)
            AvoidCops();
        if (!caught_)
            break;

    case STATE_NONE: {
        void *storage = AIState_Base::operator new(sizeof(AIState_NonActive));
        Car::tObj *carObj = carObj_;
        AIState_Base *newState = ::new (storage) AIState_NonActive(carObj);
        AIState_Base *oldState = state_;
        if (oldState)
            delete oldState;
        state_ = newState;
        stateType_ = STATE_NONACTIVE;
        perpMode_ = PERPMODE_CRUISE;
        break;
    }
    case STATE_NONACTIVE: {
        AIHigh_BTC_HumanCop *chaserCop = CheckForActivation();
        if (chaserCop) {
            NewStage(chaserCop);
            schedulingOff_ = 0;
        } else {
            schedulingOff_ = 1;
        }
        break;
    }
    case STATE_CRUISE:
        if (perpMode_ == PERPMODE_CRUISE && timeUntilContact_ < 320) {
            madeContactTime_ = simGlobal.gameTicks;
            if (perpMode_ != PERPMODE_MADECONTACT)
                Speech::Mobile(originalActivationCop_->carObj_)->Report(carObj_);
            perpMode_ = PERPMODE_MADECONTACT;
            Camera::gInfo[0].forceFocus = FOCUS_COPANDAI;
            char focus = (char)carObj_->carIndex;
            Camera::gInfo[1].forceFocus = FOCUS_COPANDAI;
            Camera::gInfo[0].focusOnAICar = focus;
            Camera::gInfo[1].focusOnAICar = (char)carObj_->carIndex;
        } else if (perpMode_ == PERPMODE_MADECONTACT) {
            void *storage = AIState_Base::operator new(sizeof(AIState_Normal));
            AIState_Base *newState = ::new (storage) AIState_Normal(carObj_);
            AIState_Base *oldState = state_;
            if (oldState)
                delete oldState;
            state_ = newState;
            stateType_ = STATE_NORMAL;
            perpMode_ = PERPMODE_ESCAPE;
        }
        break;
    default:
        break;
    }
    state_->StateExecute();
}

void AIHigh_BTC_AIPerp::NewStage(AIHigh_BTC_HumanCop *chaserCop)
{
    FUNCTION_MARKER(0x80060D74, "NFS4.EXE");
    sint32 stage = chaserCop->currentStage_;
    Car::tObj *humanCopCarObj = chaserCop->carObj_;
    originalActivationCop_ = chaserCop;
    ClearForNewStage(chaserCop);
    for (sint32 i = 0; i < 10; ++i)
        carObj_->N.damage[i] = 0;
    carObj_->render.headLight = 0;
    carObj_->render.brakeLight = 0;
    if (GameSetup::gData.Time) {
        carObj_->render.headLight = 0x33;
        carObj_->render.brakeLight = 2;
    }
    carObj_->render.signalLight[0] = 0;
    carObj_->render.signalLight[1] = 0;
    carObj_->render.damageParts = 0;
    carObj_->forceNoSimOptz = 1;
    Camera::gInfo[0].forceFocus = FOCUS_AI;
    char focus = (char)carObj_->carIndex;
    Camera::gInfo[1].forceFocus = FOCUS_AI;
    Camera::gInfo[0].focusOnAICar = focus;
    Camera::gInfo[1].focusOnAICar = (char)carObj_->carIndex;
    Object::ClearCustomObjects();
    uint32 product = fastRandom * randSeed;
    sint32 humanDirection = chaserCop->initialDirection_;
    AICop::gRoadBlockState = AICop::kAICop_RoadBlockState_None;
    randtemp = product;
    fastRandom = product & 0xFFFF;
    sint32 randPlacement = (((product >> 8) & 0xFFFF) * 1000) >> 16;
    sint32 humanMovement = chaserCop->initialMovement_;
    sint32 placementSide;
    sint32 placementDirection;
    sint32 placementSpeed;
    sint32 placementDistance;
    if (randPlacement < 333) {
        placementSide = -1;
        placementDirection = 0;
        placementSpeed = 1;
        humanCopCarObj->desiredSpeed = 0xD5555;
        chaserCop->requestedDesiredSpeed_ = 0xD5555;
        if (humanMovement) {
            escapeDuration_ = 640;
            placementDistance = 225;
        } else {
            escapeDuration_ = 384;
            placementDistance = 400;
        }
    } else {
        placementSide = 1;
        humanCopCarObj->desiredSpeed = 0x2C71C7;
        chaserCop->requestedDesiredSpeed_ = 0x2C71C7;
        if (humanMovement) {
            placementDirection = 1;
            placementSpeed = 0;
            placementDistance = 40;
            escapeDuration_ = 480;
        } else {
            placementDirection = 0;
            placementSpeed = 1;
            placementDistance = 400;
            escapeDuration_ = 384;
        }
    }
    sint32 offset = ((((placementDistance / 6)) * placementSide) * humanDirection);
    Car::tObj *car = carObj_;
    sint32 nextSlice = (humanCopCarObj->N.simRoadInfo.slice + offset);
    if (offset >= 0) {
        if (nextSlice >= gNumSlices)
            nextSlice = (nextSlice - gNumSlices);
    } else if (nextSlice < 0) {
        nextSlice = (nextSlice + gNumSlices);
    }
    car->N.simRoadInfo.slice = (sint16)nextSlice;
    sint32 direction = (placementSide * humanDirection);
    if (!placementDirection)
        direction = (-direction);
    carObj_->desiredDirection = direction;
    car = carObj_;
    car->direction = car->desiredDirection;
    sint32 newLatPos = 0;
    sint32 throwAway = 0;
    AIWorld::FindBarrierLessLaneAndPosition(carObj_, &throwAway, &newLatPos);
    car = carObj_;
    sint32 speed = placementSpeed ? 0x1F1C71 : 0x11C71C;
    AILife::PlaceCarAtLocation(carObj_, car->N.simRoadInfo.slice, newLatPos, car->direction, speed, 0);
    Camera::Update();
    GameSetup::tPerpData *stageInfo = &GameSetup::gData.perpInfo[stage];
    sint32 modifier = fixedmult(stageInfo->GlueFactor, AITune::BTC[GameSetup::gData.skill].glueMult);
    carObj_->btcGlueModifier = modifier;
    modifier = fixedmult(stageInfo->SpeedFactor, AITune::BTC[GameSetup::gData.skill].speedMult);
    carObj_->speedFactor = modifier;
    sint32 mass = fixedmult(originalMass_, stageInfo->WeightFactor);
    mass = fixedmult(mass, AITune::BTC[GameSetup::gData.skill].weightMult);
    carObj_->N.mass = mass;
    sint32 inverseMass = fixeddiv(0x10000, carObj_->N.mass);
    carObj_->N.massInv = inverseMass;
    AIPerson::SetPersonality(carObj_, stageInfo->Personality);
    R3DCar::ChangeTrafficColor(carObj_, stageInfo->Colour);
    carObj_->carInfo->SpeechColour = stageInfo->SpeechColour;
    carObj_->carInfo->HudColour = stageInfo->HudColour;
    Hud::InitMap();
    creationTime_ = simGlobal.gameTicks;
    AIState_Base *newState;
    if (placementSpeed) {
        void *storage = AIState_Base::operator new(sizeof(AIState_Normal));
        newState = ::new (storage) AIState_Normal(carObj_);
        AIState_Base *oldState = state_;
        if (oldState)
            delete oldState;
        state_ = newState;
        stateType_ = STATE_NORMAL;
        perpMode_ = PERPMODE_FAST;
    } else {
        void *storage = AIState_Base::operator new(sizeof(AIState_Cruise));
        newState = ::new (storage) AIState_Cruise(carObj_, CRUISE_ATFACTOR, 0x8000);
        AIState_Base *oldState = state_;
        if (oldState)
            delete oldState;
        state_ = newState;
        stateType_ = STATE_CRUISE;
        perpMode_ = PERPMODE_CRUISE;
    }
    Speech::Mobile(humanCopCarObj)->Purge();
    Speech::Dispatch()->Report(carObj_);
    TrgSfx::RestartTrgSfx();
}
