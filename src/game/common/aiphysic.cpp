#include <new>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/aidatarecord.h"
#include "game/common/aiphysic.h"
#include "game/common/aispeeds.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/fastrand.h"
#include "game/common/newton.h"
#include "game/common/schedule.h"
#include "game/common/sim.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

AIPhysic::Config_t AIPhysicConfig;
sint32 AIPhysic::iTime;
sint32 AIPhysic::time;
sint32 AIPhysic::elapsedTime;

void AIPhysic::StopCar(Car::tObj *carObj, sint32 velScale, sint32 rotScale)
{
    FUNCTION_MARKER(0x80069238, "NFS4.EXE");
    carObj->angularAcc.z = 0;
    carObj->angularAcc.y = 0;
    carObj->angularAcc.x = 0;
    carObj->linearAcc_ch.z = 0;
    carObj->linearAcc_ch.y = 0;
    carObj->linearAcc_ch.x = 0;
    carObj->linearAcc_rh.z = 0;
    carObj->linearAcc_rh.y = 0;
    carObj->linearAcc_rh.x = 0;
    sint32 scale = velScale / 256;
    sint32 x = (sint32)((uint32)scale * (uint32)carObj->N.linearVel.x);
    sint32 y = (sint32)((uint32)scale * (uint32)carObj->N.linearVel.y);
    carObj->N.linearVel.x = x / 256;
    carObj->N.linearVel.y = y / 256;
    sint32 z = (sint32)((uint32)scale * (uint32)carObj->N.linearVel.z);
    sint32 yaw = carObj->N.angularVel.y;
    carObj->N.linearVel.z = z / 256;
    carObj->N.angularVel.y = (sint32)((uint32)(yaw / 256) * (uint32)(rotScale / 256));
}

void AIPhysic::RevEngine(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800692FC, "NFS4.EXE");
    sint32 redLine = carObj->redLine / 65536;
    sint32 increase = (sint32)((uint32)(carObj->carIndex % 2 + 1) * (uint32)elapsedTime * 140U);
    if ((carObj->flywheelRpm & 1) != 0)
    {
        increase = (sint32)(0U - (uint32)(increase >> 1));
        if ((increase & 1) != 0)
            increase = (sint32)((uint32)increase - 1U);
    }
    carObj->flywheelRpm = (sint32)((uint32)carObj->flywheelRpm + (uint32)increase);
    if (carObj->flywheelRpm > redLine)
        carObj->flywheelRpm = (redLine / 2) | 1;
    if (carObj->flywheelRpm <= 500)
        carObj->flywheelRpm = 500;
}

void AIPhysic::HandleShifting(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800693CC, "NFS4.EXE");
    sint32 desiredRPM = 3000;
    if (carObj->aiShiftTimer > 0)
        carObj->aiShiftTimer = (sint32)((uint32)carObj->aiShiftTimer - (uint32)elapsedTime);
    else
    {
        Gear_t gear = CalculateGear(carObj);
        sint32 speed = carObj->currentSpeed;
        if (speed < 0)
            speed = (sint32)(0U - (uint32)speed);
        sint32 bottomSpeed = GearBottomSpeed(carObj, gear);
        sint32 speedDiff = (sint32)((uint32)speed - (uint32)bottomSpeed);
        sint32 baseRPM = (sint32)gear >= 3 ? 0xfa00000 : 0;
        sint32 invGearSpeedRange = GearInvSpeedRange(carObj, gear);
        sint32 fraction = (sint32)((uint32)(speedDiff / 256) * (uint32)(invGearSpeedRange / 256));
        sint32 rpmRange = (sint32)((uint32)carObj->redLine - (uint32)baseRPM);
        sint32 scaled = (sint32)((uint32)(fraction / 256) * (uint32)(rpmRange / 256));
        desiredRPM = (sint32)((uint32)baseRPM + (uint32)scaled) / 65536;
        carObj->control.gear = (char)gear;
    }
    sint32 rpm = carObj->flywheelRpm;
    if (rpm < desiredRPM)
    {
        sint32 difference = (sint32)((uint32)desiredRPM - (uint32)rpm);
        carObj->flywheelRpm = (sint32)((uint32)rpm + (uint32)(difference / 8));
    }
    else if (rpm > desiredRPM)
    {
        sint32 difference = (sint32)((uint32)rpm - (uint32)desiredRPM);
        carObj->flywheelRpm = (sint32)((uint32)rpm - (uint32)(difference / 8));
    }
}

Gear_t AIPhysic::CalculateGear(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80069534, "NFS4.EXE");
    sint32 speed = carObj->currentSpeed;
    Gear_t gear = (Gear_t)(uint8)carObj->control.gear;
    if (speed < 0)
        speed = (sint32)(0U - (uint32)speed);
    if (speed < 0x20000)
        return GEAR_NEUTRAL;
    if (carObj->driveDirection == -1)
        return GEAR_REVERSE;
    for (;;)
    {
        sint32 topSpeed = GearTopSpeed(carObj, gear);
        sint32 hi = topSpeed < speed;
        if (!hi && speed >= GearBottomSpeed(carObj, gear))
            return gear;
        if (hi)
        {
            Gear_t nextGear = (Gear_t)((uint32)gear + 1U);
            if (GearTopSpeed(carObj, nextGear) == 0)
                return gear;
            carObj->aiShiftTimer = carObj->aiShiftDuration;
            gear = nextGear;
        }
        else
        {
            gear = (Gear_t)((uint32)gear - 1U);
            carObj->aiShiftTimer = carObj->aiShiftDuration;
        }
    }
}

sint32 AIPhysic::GearInvSpeedRange(Car::tObj *carObj, Gear_t gear)
{
    FUNCTION_MARKER(0x80069638, "NFS4.EXE");
    if ((sint32)gear < 2)
        gear = GEAR_FIRST;
    sint32 convertedGear = (sint32)((uint32)gear - 2U);
    return carObj->invTopSpeeds[convertedGear];
}

sint32 AIPhysic::GearTopSpeed(Car::tObj *carObj, Gear_t gear)
{
    FUNCTION_MARKER(0x80069660, "NFS4.EXE");
    if ((sint32)gear < 2)
        gear = GEAR_FIRST;
    sint32 convertedGear = (sint32)((uint32)gear - 2U);
    return carObj->topSpeeds[convertedGear];
}

sint32 AIPhysic::GearBottomSpeed(Car::tObj *carObj, Gear_t gear)
{
    FUNCTION_MARKER(0x80069688, "NFS4.EXE");
    if ((sint32)gear < 3)
        return 0;
    return carObj->topSpeeds[(sint32)((uint32)gear - 3U)];
}

sint32 AIPhysic::CalcAcceleration(Car::tObj *carObj, sint32 speed)
{
    FUNCTION_MARKER(0x800696B0, "NFS4.EXE");
    sint32 absSpeed = speed;
    if (absSpeed < 0)
        absSpeed = (sint32)(0U - (uint32)absSpeed);
    if (carObj->aiShiftTimer > 0)
        return 0;
    sint32 carType = carObj->carInfo->carType;
    if (carType < 22)
    {
        sint32 normalTopCap = Cars::topSpeedCap[carType];
        sint32 upgradeTopCap = fixedmult(normalTopCap, carObj->topSpeedUpgradeMult);
        sint32 gluedTopCap = fixedmult(upgradeTopCap, carObj->aiGlue);
        if (gluedTopCap < absSpeed)
            return 0;
        if (upgradeTopCap < absSpeed)
            return 0;
        if (normalTopCap < absSpeed)
            absSpeed = (sint32)((uint32)normalTopCap - 0x238e3U);
    }
    sint32 accelEntry = absSpeed / 65536;
    if (accelEntry >= 112)
        accelEntry = 111;
    if (accelEntry < 0)
        accelEntry = 0;
    sint32 acceleration = carObj->accelerationRecord->Get(accelEntry);
    if (carObj->accNitrous != 65536)
        acceleration = (sint32)((uint32)(acceleration / 256) * (uint32)(carObj->accNitrous / 256));
    acceleration = (sint32)((uint32)(acceleration / 256) * (uint32)(carObj->accUpgradeMult / 256));
    if (GameSetup::gData.Weather > 0)
        acceleration = (sint32)((uint32)(acceleration / 256) * 230U);
    if ((carObj->carFlags & 0x20) != 0)
        acceleration = (sint32)((uint32)(acceleration / 256) * (uint32)(carObj->copAccMult / 256));
    if ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) &&
        ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0 ||
         (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0)) &&
        (carObj->carFlags & 8) != 0)
        acceleration = (sint32)((uint32)(acceleration / 256) * (uint32)(AITune::BTCPerpAccMults[GameSetup::gData.skill] / 256));
    acceleration = ModifyAccelerationAccordingToScript(carObj, acceleration);
    sint32 speedUpAcc = AISpeeds::SuperDuperSpeedUpTheCarsAtTheStartBecauseWeCannotActuallyHandleRenderingTheseCars(carObj);
    if (speedUpAcc != 65536)
        acceleration = fixedmult(acceleration, speedUpAcc);
    return acceleration;
}

sint32 AIPhysic::ModifyAccelerationAccordingToScript(Car::tObj *carObj, sint32 acc)
{
    FUNCTION_MARKER(0x80069990, "NFS4.EXE");
    if (AIScript::DoReAction(&carObj->script, AIScript::AISCRIPT_AIREACTION_SPEED_BURST) != -1)
        acc = (sint32)((uint32)(acc / 256) * 384U);
    return acc;
}

sint32 AIPhysic::CalcDeceleration(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800699E8, "NFS4.EXE");
    sint32 glue = carObj->aiGlue;
    sint32 d = carObj->brakeInfo->deceleration_;
    if (glue > 65536)
        d = (sint32)((uint32)(d / 256) * (uint32)(glue / 256));
    return d;
}

void AIPhysic::CheckDesiredDirection(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80069A34, "NFS4.EXE");
    sint32 turnAroundSpeed = 0x8e38e;
    if (carObj->speed < turnAroundSpeed)
        carObj->direction = carObj->desiredDirection;
}

void AIPhysic::HandleSignalling(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80069A7C, "NFS4.EXE");
    if ((carObj->carFlags & 0x10) == 0)
        return;
    sint32 lPos = carObj->roadPosition;
    sint32 lDes = carObj->desiredLatPos;
    if (lDes < (sint32)((uint32)lPos - 0x40000U))
    {
        uint16 light = (uint16)carObj->render.signalLight[0];
        carObj->render.signalLight[1] = 0;
        carObj->render.signalLight[0] = (sint16)(light | 0x80U);
        return;
    }
    if ((sint32)((uint32)lPos + 0x40000U) < lDes)
    {
        uint16 light = (uint16)carObj->render.signalLight[1];
        carObj->render.signalLight[0] = 0;
        carObj->render.signalLight[1] = (sint16)(light | 0x80U);
        return;
    }
    carObj->render.signalLight[0] = 0;
    carObj->render.signalLight[1] = 0;
}

void AIPhysic::HandleWipeoutTimer(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80069AF0, "NFS4.EXE");
    if ((carObj->carFlags & 8) == 0)
        return;
    sint32 ticks = simGlobal.gameTicks;
    if (carObj->wipeOutStartTick >= ticks)
        return;
    uint32 product = fastRandom * randSeed;
    AIPerson::AIPerson_t *personality = carObj->personality;
    uint32 nextTick = (uint32)ticks + (uint32)personality->minimumBetweenWipeoutTicks;
    randtemp = product;
    fastRandom = product & 0xffffU;
    nextTick += (product >> 8) & 0xffffU & (uint32)personality->randomBetweenWipeoutTicks;
    carObj->wipeOutStartTick = (sint32)nextTick;
}

void AIPhysic::Main(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80069B84, "NFS4.EXE");
    if (Sched::ExecuteCheck(1, 1, carObj->N.distToPlayer, carObj->N.objID,
        &time, &elapsedTime, &iTime, carObj->forceNoSimOptz) == 0)
        return;
    if ((carObj->carFlags & 2) != 0)
        AIWorld::CalcSpeed(carObj);
    CheckDesiredDirection(carObj);
    CheckForGripReduction(carObj);
    HandleSignalling(carObj);
    carObj->lateralVelocity = AIWorld::CalcLateralVelocity(carObj);
    char useCoolPhysics = (char)AIPhysics_UseCoolPhysics(carObj);
    CalculateRampedDesiredLatPos(carObj, useCoolPhysics ? kRampType_NoInterpolate : kRampType_Interpolate);
    if ((carObj->AIFlags & 4) != 0 || carObj->N.active == 0)
        return;
    if (simGlobal.gameStarted != 1)
    {
        RevEngine(carObj);
        return;
    }
    HandleShifting(carObj);
    if (carObj->N.flightTime != 0)
    {
        if ((carObj->carFlags & 0x400) != 0)
            StopCar(carObj, 0xfae1, 0xfae1);
        return;
    }
    if (carObj->speed < 0x40000)
    {
        carObj->linearVel_ch.z = 0;
        carObj->rearSkid = 0;
        carObj->frontSkid = 0;
    }
    if (carObj->N.flightTime == 0 && carObj->N.orientMat.m[4] < 0x4ccc)
    {
        StopCar(carObj, 0xf0a3, 0xfae1);
        return;
    }
    if (carObj->blowout != 0)
    {
        StopCar(carObj, 0xfd70, 0xfae1);
        return;
    }
    if (carObj->pullOver != 0)
    {
        if (carObj->speed < 0x238e3)
        {
            StopCar(carObj, 0xe666, 0xe666);
            return;
        }
        if (carObj->speed < 0x8e38e)
        {
            StopCar(carObj, 0xf5c2, 0xfae1);
            return;
        }
    }
    if ((carObj->carFlags & 1) != 0 && carObj->stats.finishType == 2)
    {
        sint32 speed = carObj->currentSpeed;
        if (speed < 0)
            speed = (sint32)(0U - (uint32)speed);
        if (speed < 0x20000)
        {
            StopCar(carObj, 0xf0a3, 0xfae1);
            return;
        }
    }
    if (carObj->desiredSpeed == 0)
    {
        StopCar(carObj, 0xf0a3, 0xfae1);
        return;
    }
    if (useCoolPhysics)
        CoolPhysics(carObj);
    else
        SimplePhysics(carObj);
    CheckForBadPosition(carObj);
    HandleWipeoutTimer(carObj);
}

sint32 AIPhysics_UseCoolPhysics(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80069EEC, "NFS4.EXE");
    sint32 flags = carObj->carFlags;
    if ((flags & 0x800) != 0)
        return 1;
    uint32 simOptz = carObj->N.simOptz;
    if (simOptz == 0 || ((flags & 0x20) != 0 && simOptz < 2))
        return 1;
    return 0;
}

void AIPhysic::CheckForBadPosition(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80069E14, "NFS4.EXE");
    sint32 badSpeed = carObj->N.linearVel.z > 0x730000 || carObj->N.linearVel.x > 0x730000 ||
        (carObj->N.speedXZ > 0x730000 && carObj->N.simOptz == 0);
    Trk::NewSlice *slice = &BWorldSm::slices[carObj->N.simRoadInfo.slice];
    sint32 badRoadPos = carObj->roadPosition < -slice->leftDrive * 256 - 0x230000 ||
        carObj->roadPosition > slice->rightDrive * 256 + 0x230000;
    if (badSpeed || badRoadPos)
        Cars::ResetCollidedCars(carObj, 1, 0);
}

void AIPhysic::SimplePhysics(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80069F30, "NFS4.EXE");
    if ((carObj->AIFlags & 0x10) != 0)
    {
        carObj->rampDesiredLatPos = carObj->roadPosition;
        carObj->AIFlags &= ~0x10;
    }
    if ((carObj->carFlags & 8) != 0 && carObj->wipeOutStartTick < simGlobal.gameTicks &&
        AIDataRecord::TrackCurve->Get(carObj->N.simRoadInfo.slice) > 30 && carObj->speed > 0x1638e3)
        carObj->wipeOutEndTick = (sint32)((uint32)simGlobal.gameTicks + 0x180U);
    SimplePhysics_LongVel(carObj);
    SimplePhysics_LatVel(carObj);
    coorddef forward = { carObj->N.orientMat.m[6], carObj->N.orientMat.m[7], carObj->N.orientMat.m[8] };
    coorddef right = { carObj->N.roadMatrix.m[0], carObj->N.roadMatrix.m[1], carObj->N.roadMatrix.m[2] };
    right.x = fixedmult(carObj->laneChangeSpeed, right.x);
    right.y = fixedmult(carObj->laneChangeSpeed, right.y);
    right.z = fixedmult(carObj->laneChangeSpeed, right.z);
    forward.x = fixedmult((sint32)((uint32)carObj->currentSpeed * (uint32)carObj->direction), forward.x);
    forward.y = fixedmult((sint32)((uint32)carObj->currentSpeed * (uint32)carObj->direction), forward.y);
    forward.z = fixedmult((sint32)((uint32)carObj->currentSpeed * (uint32)carObj->direction), forward.z);
    sint32 lateralSpeed = (sint32)((uint32)carObj->laneChangeSpeed * (uint32)carObj->direction);
    carObj->N.linearVel.x = (sint32)((uint32)forward.x + (uint32)right.x);
    sint32 speed = carObj->speed;
    carObj->N.linearVel.y = (sint32)((uint32)forward.y + (uint32)right.y);
    carObj->linearVel_ch.z = speed;
    carObj->N.linearVel.z = (sint32)((uint32)forward.z + (uint32)right.z);
    sint32 lateralAbs = lateralSpeed;
    if (lateralAbs < 0)
        lateralAbs = (sint32)(0U - (uint32)lateralAbs);
    carObj->linearVel_ch.x = lateralSpeed;
    carObj->slide = (lateralAbs / 256) * 25 < lateralAbs ? 0x8000 : 0;
    speed = carObj->speed;
    carObj->N.angularVel.x = 0;
    carObj->N.angularVel.y = 0;
    carObj->N.angularVel.z = 0;
    if (speed > 0x30000)
        Newton::CopyRoadMatrixToOrientMat(&carObj->N, carObj->direction == -1);
    speed = carObj->currentSpeed;
    if (speed < 0)
        speed = (sint32)(0U - (uint32)speed);
    sint32 sliceLookAhead = speed / 0x60000;
    if (sliceLookAhead >= 0 && sliceLookAhead < AIPhysicConfig.min_lookahead / 6 && speed >= 0)
        sliceLookAhead = AIPhysicConfig.min_lookahead / 6;
    if (AIPhysicConfig.max_lookahead / 6 < sliceLookAhead)
        sliceLookAhead = AIPhysicConfig.max_lookahead / 6;
    sint32 offset = (sint32)((uint32)sliceLookAhead * (uint32)carObj->direction);
    sliceLookAhead = (sint32)((uint32)carObj->N.simRoadInfo.slice + (uint32)offset);
    if (offset < 0)
    {
        if (sliceLookAhead < 0)
            sliceLookAhead = (sint32)((uint32)sliceLookAhead + (uint32)gNumSlices);
    }
    else if (sliceLookAhead >= gNumSlices)
        sliceLookAhead = (sint32)((uint32)sliceLookAhead - (uint32)gNumSlices);
    carObj->lookAheadSlice = sliceLookAhead;
}

void AIPhysic::SimplePhysics_LongVel(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006A29C, "NFS4.EXE");
    sint32 acceleration = CalcAcceleration(carObj, carObj->currentSpeed);
    sint32 deceleration = CalcDeceleration(carObj);
    if (carObj->pullOver != 0)
        carObj->desiredSpeed = 0;
    if (simGlobal.gameTicks < carObj->wipeOutEndTick)
        carObj->desiredSpeed /= 4;
    bool backwards = carObj->direction == -1;
    bool braking = backwards ?
        (carObj->currentSpeed < carObj->desiredSpeed || carObj->desiredDirection != -1) :
        (carObj->desiredSpeed < carObj->currentSpeed || carObj->direction != carObj->desiredDirection);
    if (braking)
    {
        sint32 brakeLevel = (sint32)((uint32)(uint8)carObj->control.brakeLevel + ((uint32)elapsedTime << 2));
        carObj->linearAcc_ch.z = deceleration;
        carObj->control.brakeLevel = brakeLevel < 255 ? (char)brakeLevel : (char)255;
        if ((uint8)carObj->control.brakeLevel > 3)
            carObj->control.desiredBrakeLevel = 1;
        sint32 delta = (sint32)((uint32)(deceleration / 256) * (uint32)(iTime / 256));
        sint32 speed = backwards ?
            (sint32)((uint32)carObj->currentSpeed + (uint32)delta) :
            (sint32)((uint32)carObj->currentSpeed - (uint32)delta);
        carObj->currentSpeed = speed;
        if (carObj->direction != carObj->desiredDirection)
            return;
        sint32 desiredSpeed = carObj->desiredSpeed;
        if (backwards)
            carObj->currentSpeed = speed <= desiredSpeed ? speed : desiredSpeed;
        else
            carObj->currentSpeed = desiredSpeed < speed ? speed : desiredSpeed;
        return;
    }
    sint32 delta = (sint32)((uint32)(acceleration / 256) * (uint32)(iTime / 256));
    uint32 brakeLevel = (uint8)carObj->control.brakeLevel;
    bool reducingBrake = elapsedTime < (sint32)brakeLevel;
    carObj->currentSpeed = backwards ?
        (sint32)((uint32)carObj->currentSpeed - (uint32)delta) :
        (sint32)((uint32)carObj->currentSpeed + (uint32)delta);
    carObj->control.brakeLevel = reducingBrake ? (char)(brakeLevel - (uint32)elapsedTime) : 0;
    if (carObj->control.brakeLevel == 0)
        carObj->control.desiredBrakeLevel = 0;
    sint32 speed = carObj->currentSpeed;
    sint32 desiredSpeed = carObj->desiredSpeed;
    carObj->linearAcc_ch.z = acceleration;
    if ((backwards && speed < desiredSpeed) || (!backwards && desiredSpeed < speed))
        carObj->currentSpeed = desiredSpeed;
}

void AIPhysic::SimplePhysics_LatVel(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006A5F4, "NFS4.EXE");
    sint32 off = 0;
    if (carObj->speed > 0x30000)
        off = (sint32)((uint32)carObj->rampDesiredLatPos - (uint32)carObj->roadPosition);
    if (carObj->speed > 0x190000)
    {
        coorddef right = { carObj->N.roadMatrix.m[0], carObj->N.roadMatrix.m[1], carObj->N.roadMatrix.m[2] };
        right.x = fixedmult(off, right.x);
        right.y = fixedmult(off, right.y);
        right.z = fixedmult(off, right.z);
        carObj->N.position.x = (sint32)((uint32)carObj->N.position.x + (uint32)right.x);
        carObj->N.position.y = (sint32)((uint32)carObj->N.position.y + (uint32)right.y);
        sint32 positionZ = carObj->N.position.z;
        carObj->laneChangeSpeed = 0;
        carObj->N.position.z = (sint32)((uint32)positionZ + (uint32)right.z);
    }
    else
    {
        sint32 carSpeed = carObj->currentSpeed;
        if (carSpeed < 0)
            carSpeed = (sint32)(0U - (uint32)carSpeed);
        sint32 negativeSpeed = (sint32)(0U - (uint32)carSpeed);
        carObj->laneChangeSpeed = off;
        if (off < negativeSpeed)
            carObj->laneChangeSpeed = negativeSpeed;
        else if (carSpeed < off)
            carObj->laneChangeSpeed = carSpeed;
    }
}

void AIPhysic::CoolPhysics(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006A718, "NFS4.EXE");
    carObj->AIFlags |= 0x10;
    HandleDirection(carObj);
    sint32 signedSpeed = (sint32)((uint32)carObj->direction * (uint32)carObj->currentSpeed);
    if (signedSpeed < 0 && carObj->driveDirection != -1)
        carObj->rampDesiredLatPos = 0;
    if (HitWallCheck(carObj) != 0)
        ChangeDirection(carObj, 0x40);
    Preperation(carObj);
    if (CheckIfOutOfControl(carObj) != 0)
        OutOfControlPhysics(carObj);
    else
        InControlPhysics(carObj);
    FinishUp(carObj);
}

void AIPhysic::HandleDirection(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006A7CC, "NFS4.EXE");
    if (carObj->driveDirection != -1)
        return;
    sint32 rampDesiredLatPos = carObj->rampDesiredLatPos;
    sint32 roadPosition = carObj->roadPosition;
    if ((roadPosition < (sint32)((uint32)rampDesiredLatPos - 0xa0000U) && carObj->lateralVelocity > 0) ||
        (roadPosition > (sint32)((uint32)rampDesiredLatPos + 0xa0000U) && carObj->lateralVelocity < 0))
        carObj->driveDirectionTimer = (sint32)((uint32)simGlobal.gameTicks - 0x18U);
    sint32 ticksSinceChange = (sint32)((uint32)simGlobal.gameTicks - (uint32)carObj->driveDirectionTimer);
    if (carObj->driveDirectionReverseTime < ticksSinceChange)
    {
        carObj->driveDirection = 1;
        carObj->driveDirectionTimer = simGlobal.gameTicks;
    }
    if (carObj->desiredSpeed > 0x140000)
        carObj->desiredSpeed = 0x140000;
    if ((sint32)(0U - (uint32)carObj->desiredSpeed) > 0x140000)
        carObj->desiredSpeed = -0x140000;
    carObj->rampDesiredLatPos = 0;
}

void AIPhysic::Preperation(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006A8B4, "NFS4.EXE");
    sint32 term0 = fixedmult(carObj->N.linearVel.x, carObj->N.orientMat.m[0]);
    sint32 term1 = fixedmult(carObj->N.linearVel.y, carObj->N.orientMat.m[1]);
    sint32 term2 = fixedmult(carObj->N.linearVel.z, carObj->N.orientMat.m[2]);
    carObj->linearVel_ch.x = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->N.linearVel.x, carObj->N.orientMat.m[3]);
    term1 = fixedmult(carObj->N.linearVel.y, carObj->N.orientMat.m[4]);
    term2 = fixedmult(carObj->N.linearVel.z, carObj->N.orientMat.m[5]);
    carObj->linearVel_ch.y = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->N.linearVel.x, carObj->N.orientMat.m[6]);
    term1 = fixedmult(carObj->N.linearVel.y, carObj->N.orientMat.m[7]);
    term2 = fixedmult(carObj->N.linearVel.z, carObj->N.orientMat.m[8]);
    carObj->linearVel_ch.z = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->N.angularVel.x, carObj->N.orientMat.m[0]);
    term1 = fixedmult(carObj->N.angularVel.y, carObj->N.orientMat.m[1]);
    term2 = fixedmult(carObj->N.angularVel.z, carObj->N.orientMat.m[2]);
    carObj->angularVel_ch.x = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->N.angularVel.x, carObj->N.orientMat.m[3]);
    term1 = fixedmult(carObj->N.angularVel.y, carObj->N.orientMat.m[4]);
    term2 = fixedmult(carObj->N.angularVel.z, carObj->N.orientMat.m[5]);
    carObj->angularVel_ch.y = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->N.angularVel.x, carObj->N.orientMat.m[6]);
    term1 = fixedmult(carObj->N.angularVel.y, carObj->N.orientMat.m[7]);
    term2 = fixedmult(carObj->N.angularVel.z, carObj->N.orientMat.m[8]);
    carObj->angularVel_ch.z = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    sint32 lateralAbs = carObj->linearVel_ch.x;
    if (lateralAbs < 0)
        lateralAbs = (sint32)(0U - (uint32)lateralAbs);
    carObj->slide = (lateralAbs / 256) * 25 < lateralAbs ? 0x8000 : 0;
    coorddef forward = { carObj->N.orientMat.m[6], carObj->N.orientMat.m[7], carObj->N.orientMat.m[8] };
    if (carObj->driveDirection != 1)
    {
        forward.x = (sint32)(0U - (uint32)forward.x);
        forward.z = (sint32)(0U - (uint32)forward.z);
    }
    sint32 aCar = intatan(forward.x, forward.z);
    carObj->aCar = aCar;
    GetDesiredVector(carObj);
    sint32 aDesired = intatan(carObj->desiredVector.x, carObj->desiredVector.z);
    sint32 aCarWRTDesired = (sint32)((uint32)aCar - (uint32)aDesired);
    if (aCarWRTDesired < -512)
        carObj->aCarWRTDesired = (sint32)((uint32)aCarWRTDesired + 1024U);
    else
    {
        carObj->aCarWRTDesired = aCarWRTDesired;
        if (aCarWRTDesired >= 513)
            carObj->aCarWRTDesired = (sint32)((uint32)aCarWRTDesired - 1024U);
    }
    carObj->aDesired = aDesired;
}

sint32 AIPhysic::CalculateRoadPosition(coorddef *pos, sint32 slice)
{
    FUNCTION_MARKER(0x8006AB1C, "NFS4.EXE");
    Trk::NewSlice *roadSlice = &BWorldSm::slices[slice];
    coorddef centerBack = { roadSlice->center[0], roadSlice->center[1], roadSlice->center[2] };
    coorddef carRelative = {
        (sint32)((uint32)pos->x - (uint32)centerBack.x),
        (sint32)((uint32)pos->y - (uint32)centerBack.y),
        (sint32)((uint32)pos->z - (uint32)centerBack.z)
    };
    coorddef right = {
        (sint8)roadSlice->right[0] * 512,
        (sint8)roadSlice->right[1] * 512,
        (sint8)roadSlice->right[2] * 512
    };
    return (sint32)((uint32)(right.x / 256) * (uint32)(carRelative.x / 256) +
        (uint32)(right.y / 256) * (uint32)(carRelative.y / 256) +
        (uint32)(right.z / 256) * (uint32)(carRelative.z / 256));
}

void AIPhysic::GetDesiredVector(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006AC3C, "NFS4.EXE");
    sint32 thisSlice = carObj->N.simRoadInfo.slice;
    sint32 numReductions = 0;
    if (carObj->targetPos.x != 0 || carObj->targetPos.y != 0 || carObj->targetPos.z != 0)
    {
        TargetedGetDesiredVector(carObj);
        return;
    }
    sint32 speed = carObj->currentSpeed;
    if (speed < 0)
        speed = (sint32)(0U - (uint32)speed);
    sint32 scaledSpeed = fixedmult(speed, 0xb333);
    sint32 sliceLookAhead = (sint32)((uint32)scaledSpeed + 0x30000U) / 0x60000;
    if (sliceLookAhead >= 0 && sliceLookAhead < AIPhysicConfig.min_lookahead / 6 && speed >= 0)
        sliceLookAhead = AIPhysicConfig.min_lookahead / 6;
    if (AIPhysicConfig.max_lookahead / 6 < sliceLookAhead)
        sliceLookAhead = AIPhysicConfig.max_lookahead / 6;
    sint32 offset = (sint32)((uint32)sliceLookAhead * (uint32)carObj->driveDirection * (uint32)carObj->direction);
    sint32 dirCorrectedSliceLookAhead = (sint32)((uint32)thisSlice + (uint32)offset);
    if (offset < 0)
    {
        if (dirCorrectedSliceLookAhead < 0)
            dirCorrectedSliceLookAhead = (sint32)((uint32)dirCorrectedSliceLookAhead + (uint32)gNumSlices);
    }
    else if (dirCorrectedSliceLookAhead >= gNumSlices)
        dirCorrectedSliceLookAhead = (sint32)((uint32)dirCorrectedSliceLookAhead - (uint32)gNumSlices);
    carObj->lookAheadSlice = dirCorrectedSliceLookAhead;
    sint32 futureBend = AIWorld::CalcRoadBend(carObj, offset);
    Trk::NewSlice *slice = &BWorldSm::slices[carObj->lookAheadSlice];
    sint32 roadWidth = slice->avgPavedWidthLf * 32768 * (slice->laneCount >> 4) +
        slice->avgPavedWidthRt * 32768 * (slice->laneCount & 15);
    if (futureBend < 0)
        futureBend = (sint32)(0U - (uint32)futureBend);
    if (roadWidth < 0x120000)
    {
        if (futureBend > 0xf333)
            sliceLookAhead = (sint32)((uint32)sliceLookAhead * 0xca3dU) / 65536;
        if (futureBend > 0xe666)
            sliceLookAhead = (sint32)((uint32)sliceLookAhead * 0xd70aU) / 65536;
        else if (futureBend > 0xcccc)
            sliceLookAhead = (sint32)((uint32)sliceLookAhead * 0xe666U) / 65536;
        else if (futureBend > 0xc000)
            sliceLookAhead = (sint32)((uint32)sliceLookAhead * 0xf333U) / 65536;
    }
    coorddef fPoint;
    sint32 goodVector;
    do
    {
        offset = (sint32)((uint32)sliceLookAhead * (uint32)carObj->driveDirection * (uint32)carObj->direction);
        dirCorrectedSliceLookAhead = (sint32)((uint32)thisSlice + (uint32)offset);
        if (offset < 0)
        {
            if (dirCorrectedSliceLookAhead < 0)
                dirCorrectedSliceLookAhead = (sint32)((uint32)dirCorrectedSliceLookAhead + (uint32)gNumSlices);
        }
        else if (dirCorrectedSliceLookAhead >= gNumSlices)
            dirCorrectedSliceLookAhead = (sint32)((uint32)dirCorrectedSliceLookAhead - (uint32)gNumSlices);
        carObj->lookAheadSlice = dirCorrectedSliceLookAhead;
        slice = &BWorldSm::slices[carObj->lookAheadSlice];
        coorddef fCPoint = { slice->center[0], slice->center[1], slice->center[2] };
        coorddef right = {
            (sint8)slice->right[0] * 512,
            (sint8)slice->right[1] * 512,
            (sint8)slice->right[2] * 512
        };
        fPoint.x = fixedmult(carObj->rampDesiredLatPos, right.x);
        fPoint.y = fixedmult(carObj->rampDesiredLatPos, right.y);
        fPoint.z = fixedmult(carObj->rampDesiredLatPos, right.z);
        fPoint.x = (sint32)((uint32)fPoint.x + (uint32)fCPoint.x);
        fPoint.z = (sint32)((uint32)fPoint.z + (uint32)fCPoint.z);
        fPoint.y = (sint32)((uint32)fPoint.y + (uint32)fCPoint.y);
        sint32 futureRoadPosition = CalculateRoadPosition(&fPoint, thisSlice);
        slice = &BWorldSm::slices[thisSlice];
        sint32 rightEdge = slice->avgPavedWidthRt * 32768 * (slice->laneCount & 15);
        sint32 leftEdge = -(slice->avgPavedWidthLf * 32768 * (slice->laneCount >> 4));
        if ((rightEdge < futureRoadPosition && carObj->roadPosition < futureRoadPosition) ||
            (futureRoadPosition < leftEdge && futureRoadPosition < carObj->roadPosition))
        {
            sint32 checkSide = futureRoadPosition < carObj->roadPosition ? -1 : 1;
            sint32 position = (sint32)((uint32)carObj->roadPosition + (uint32)checkSide * 0x80000U);
            sint32 lane = AIWorld::LaneIndex(thisSlice, position);
            goodVector = 0;
            if (AIWorld::IsDriveableLane(thisSlice, lane) != 0)
            {
                position = (sint32)((uint32)carObj->roadPosition + (uint32)checkSide * 0x40000U);
                lane = AIWorld::LaneIndex(thisSlice, position);
                goodVector = AIWorld::IsDriveableLane(thisSlice, lane) != 0;
            }
        }
        else
            goodVector = 1;
        ++numReductions;
        sliceLookAhead = (sint32)((uint32)sliceLookAhead * 0xccccU) / 65536;
        if (sliceLookAhead < 4)
            sliceLookAhead = 4;
    } while (numReductions < 5 && !goodVector);
    carObj->desiredVector.x = (sint32)((uint32)fPoint.x - (uint32)carObj->N.position.x);
    carObj->desiredVector.y = (sint32)((uint32)fPoint.y - (uint32)carObj->N.position.y);
    carObj->desiredVector.z = (sint32)((uint32)fPoint.z - (uint32)carObj->N.position.z);
}

void AIPhysic::TargetedGetDesiredVector(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006B224, "NFS4.EXE");
    carObj->desiredVector.x = (sint32)((uint32)carObj->targetPos.x - (uint32)carObj->N.position.x);
    carObj->desiredVector.y = (sint32)((uint32)carObj->targetPos.y - (uint32)carObj->N.position.y);
    carObj->desiredVector.z = (sint32)((uint32)carObj->targetPos.z - (uint32)carObj->N.position.z);
    carObj->rampDesiredLatPos = carObj->targetLatPos;
}

sint32 AIPhysic::CheckIfOutOfControl(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006B260, "NFS4.EXE");
    sint32 futureBend = AIWorld::CalcRoadBend(carObj, (sint32)((uint32)carObj->direction << 3));
    if (futureBend < 0)
        futureBend = (sint32)(0U - (uint32)futureBend);
    if (simGlobal.gameTicks < carObj->wipeOutEndTick)
    {
        sint32 speed = carObj->currentSpeed;
        if (speed < 0)
            speed = (sint32)(0U - (uint32)speed);
        return speed <= 0x2ffff;
    }
    sint32 angle = carObj->aCarWRTDesired;
    if (angle < 0)
        angle = (sint32)(0U - (uint32)angle);
    if (angle >= 0x74 || carObj->driveDirection == -1)
        return 1;
    sint32 speed = carObj->currentSpeed;
    if (speed < 0)
        speed = (sint32)(0U - (uint32)speed);
    if (speed < (sint32)((uint32)AIPhysicConfig.OOCModel.vel_limit_range - 0x30000U))
        return 1;
    Trk::NewSlice *slice = &BWorldSm::slices[carObj->N.simRoadInfo.slice];
    sint32 leftEdge = -(slice->avgPavedWidthLf * 32768 * (slice->laneCount >> 4));
    if (carObj->roadPosition < leftEdge)
        return 1;
    sint32 rightEdge = slice->avgPavedWidthRt * 32768 * (slice->laneCount & 15);
    if (rightEdge < carObj->roadPosition || futureBend > 0xcccc)
        return 1;
    sint32 lateralAbs = carObj->linearVel_ch.x;
    if (lateralAbs < 0)
        lateralAbs = (sint32)(0U - (uint32)lateralAbs);
    sint32 limit = (sint32)((uint32)(carObj->speed / 256) * 102U);
    if (lateralAbs <= limit || speed > 0xeffff)
        return 0;
    return 1;
}

void AIPhysic::OutOfControlPhysics(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006B400, "NFS4.EXE");
    carObj->drag = 0;
    sint32 dir = AIWorld::CalcFutureLateralVel(carObj,
        (sint32)((uint32)carObj->direction * (uint32)AIPhysicConfig.latvelcalc_lookahead));
    sint32 signedSpeed = (sint32)((uint32)carObj->direction * (uint32)carObj->currentSpeed);
    sint32 uTurn = signedSpeed < 0 && carObj->driveDirection != -1;
    carObj->wipeOutEndTick = simGlobal.gameTicks;
    sint32 angleFactor = fixedmult(0x80, AIPhysicConfig.OOCModel.dangle_to_dav);
    sint32 desiredAngVel = (sint32)((0U - (uint32)carObj->aCarWRTDesired) * (uint32)angleFactor);
    sint32 maxAngVel = AIPhysicConfig.OOCModel.max_dav;
    if (desiredAngVel >= maxAngVel)
        desiredAngVel = maxAngVel;
    sint32 minAngVel = (sint32)(0U - (uint32)maxAngVel);
    if (desiredAngVel <= minAngVel)
        desiredAngVel = minAngVel;
    sint32 desiredLatVel = (sint32)((uint32)(desiredAngVel / 256) * 0xa00U);
    sint32 maxLatVel = AIPhysicConfig.OOCModel.max_dlvel;
    if (desiredLatVel >= maxLatVel)
        desiredLatVel = maxLatVel;
    sint32 minLatVel = (sint32)(0U - (uint32)maxLatVel);
    if (desiredLatVel <= minLatVel)
        desiredLatVel = minLatVel;
    sint32 speed = carObj->speed;
    if (desiredLatVel >= speed)
        desiredLatVel = speed;
    sint32 negativeSpeed = (sint32)(0U - (uint32)speed);
    if (desiredLatVel <= negativeSpeed)
        desiredLatVel = negativeSpeed;
    sint32 velocityRange = AIPhysicConfig.OOCModel.vel_limit_range;
    if ((sint32)(0U - (uint32)velocityRange) < carObj->currentSpeed && carObj->currentSpeed < velocityRange)
    {
        sint32 latVelLimit = (sint32)((uint32)(speed / 256) * (uint32)(AIPhysicConfig.OOCModel.lat_vel_limit_factor / 256));
        if (latVelLimit < 0)
            latVelLimit = (sint32)(0U - (uint32)latVelLimit);
        if (desiredLatVel >= latVelLimit)
            desiredLatVel = latVelLimit;
        sint32 negativeLimit = (sint32)(0U - (uint32)latVelLimit);
        if (desiredLatVel <= negativeLimit)
            desiredLatVel = negativeLimit;
        maxAngVel = (sint32)((uint32)(speed / 256) * (uint32)(AIPhysicConfig.OOCModel.ang_vel_limit_factor / 256));
        if (maxAngVel < 0)
            maxAngVel = (sint32)(0U - (uint32)maxAngVel);
        if (desiredAngVel >= maxAngVel)
            desiredAngVel = maxAngVel;
        minAngVel = (sint32)(0U - (uint32)maxAngVel);
        if (desiredAngVel <= minAngVel)
            desiredAngVel = minAngVel;
    }
    if (carObj->donutMode == DONUTMODE_DONUT)
        desiredAngVel = (sint32)((uint32)desiredAngVel * 5U);
    sint32 angularDifference = (sint32)((uint32)carObj->angularVel_ch.y - (uint32)desiredAngVel);
    sint32 currentAngAcc = (sint32)(0U - (uint32)(angularDifference / 256) * (uint32)(AIPhysicConfig.OOCModel.dav_to_aa / 256));
    speed = carObj->currentSpeed;
    if (speed < 0)
        speed = (sint32)(0U - (uint32)speed);
    if (speed > 0x120000)
        currentAngAcc /= 2;
    if (currentAngAcc >= carObj->max_aa)
        currentAngAcc = carObj->max_aa;
    sint32 minAngAcc = (sint32)(0U - (uint32)carObj->max_aa);
    if (currentAngAcc <= minAngAcc)
        currentAngAcc = minAngAcc;
    sint32 currentLatVel = (sint32)((uint32)carObj->linearVel_ch.x - (uint32)desiredLatVel);
    sint32 currentLatAcc = (sint32)(0U - (uint32)(currentLatVel / 256) * (uint32)(AIPhysicConfig.OOCModel.dlvel_to_clacc / 256));
    if (currentLatAcc >= carObj->max_clacc)
        currentLatAcc = carObj->max_clacc;
    sint32 minLatAcc = (sint32)(0U - (uint32)carObj->max_clacc);
    if (currentLatAcc <= minLatAcc)
        currentLatAcc = minLatAcc;
    sint32 currentVel = carObj->linearVel_ch.z;
    sint32 targetVel;
    if (carObj->driveDirection == -1 && carObj->targetPos.x == 0 && carObj->targetPos.y == 0 && carObj->targetPos.z == 0)
        targetVel = (sint32)(0U - (uint32)AIPhysicConfig.OOCModel.vel_limit_range);
    else
        targetVel = (sint32)((uint32)carObj->desiredSpeed * (uint32)carObj->direction * (uint32)carObj->driveDirection);
    bool accelerate = carObj->pullOver == 0 &&
        (targetVel < 0 ? (targetVel <= currentVel && currentVel <= 0x20000) :
                         (-0x20000 <= currentVel && currentVel <= targetVel)) &&
        carObj->desiredDirection == carObj->direction;
    sint32 currentLongAcc = 0;
    if (accelerate)
    {
        sint32 acceleration = CalcAcceleration(carObj, carObj->linearVel_ch.z);
        currentLongAcc = (sint32)((uint32)acceleration * (uint32)carObj->driveDirection);
        uint32 brakeLevel = (uint8)carObj->control.brakeLevel;
        carObj->control.brakeLevel = elapsedTime < (sint32)brakeLevel ? (char)(brakeLevel - (uint32)elapsedTime) : 0;
        if (carObj->control.brakeLevel == 0)
            carObj->control.desiredBrakeLevel = 0;
    }
    else
    {
        currentVel = carObj->linearVel_ch.z;
        if (currentVel < 0)
            currentVel = (sint32)(0U - (uint32)currentVel);
        if (currentVel > 0)
        {
            sint32 deceleration = CalcDeceleration(carObj);
            currentLongAcc = (sint32)(0U - (uint32)deceleration);
            sint32 brakeLevel = (sint32)((uint32)(uint8)carObj->control.brakeLevel + ((uint32)elapsedTime << 2));
            carObj->control.brakeLevel = brakeLevel < 255 ? (char)brakeLevel : (char)255;
            if ((uint8)carObj->control.brakeLevel > 3)
                carObj->control.desiredBrakeLevel = 1;
            if (carObj->linearVel_ch.z < 0)
                currentLongAcc = (sint32)(0U - (uint32)currentLongAcc);
        }
    }
    if (uTurn)
    {
        if ((uint32)carObj->speed - 0x140001U < 0x1dffffU)
        {
            currentAngAcc /= 4;
            currentLongAcc /= 4;
        }
        else
        {
            currentAngAcc = fixedmult(currentAngAcc, 0xc000);
            currentLongAcc = (sint32)((uint32)(currentLongAcc / 2) + (uint32)(currentLongAcc / 4));
        }
        currentLatAcc = (sint32)((uint32)(currentLatAcc / 4) + (uint32)(currentLatAcc / 8));
    }
    if (dir < 0)
        dir = (sint32)(0U - (uint32)dir);
    sint32 skid = (sint32)((uint32)dir - (uint32)AIPhysicConfig.skid_value);
    signedSpeed = (sint32)((uint32)carObj->direction * (uint32)carObj->currentSpeed);
    if ((signedSpeed < 0 && carObj->speed > 0x38e38) || carObj->donutMode != DONUTMODE_NONE)
    {
        carObj->frontSkid = 0xa0000;
        carObj->rearSkid = 0xa0000;
    }
    else if (skid <= 0)
    {
        carObj->rearSkid = 0;
        carObj->frontSkid = 0;
    }
    else
    {
        carObj->frontSkid = skid;
        carObj->rearSkid = skid;
    }
    carObj->linearAcc_ch.x = currentLatAcc;
    carObj->linearAcc_ch.y = 0;
    carObj->linearAcc_ch.z = currentLongAcc;
    carObj->linearAcc_rh.x = 0;
    carObj->linearAcc_rh.y = 0;
    carObj->linearAcc_rh.z = 0;
    carObj->angularAcc.x = 0;
    carObj->angularAcc.y = currentAngAcc;
    carObj->angularAcc.z = 0;
}

sint32 AIPhysic::GetRearEndDamageFactor(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006BA70, "NFS4.EXE");
    sint32 totalDamage = (sint32)((uint32)carObj->N.damage[4] + (uint32)carObj->N.damage[5] +
        (uint32)carObj->N.damage[6] + (uint32)carObj->N.damage[9]);
    totalDamage = fixedmult(totalDamage, 0x147);
    return totalDamage > 65536 ? 65536 : totalDamage;
}

void AIPhysic::InControlPhysics(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006BAC8, "NFS4.EXE");
    static char copCollisionReactionTime[4] = { 140, 100, 80, 60 };
    static sint32 copCollisionGripLoss[4] = { 19660, 32768, 32768, 65536 };
    static sint32 copCollisionSlowDown[4] = { 589824, 540672, 458752, 376832 };
    sint32 dir = carObj->direction;
    carObj->drag = 0;
    coorddef unitDesiredVector = carObj->desiredVector;
    Math::QDNormalizeVector(&unitDesiredVector);
    sint32 justBumped = 0;
    coorddef unitDesiredVectorRight = { unitDesiredVector.z, unitDesiredVector.y,
        (sint32)(0U - (uint32)unitDesiredVector.x) };
    sint32 skid = 0;
    sint32 termX = fixedmult(carObj->N.linearVel.x, unitDesiredVectorRight.x);
    sint32 termY = fixedmult(carObj->N.linearVel.y, unitDesiredVectorRight.y);
    sint32 termZ = fixedmult(carObj->N.linearVel.z, unitDesiredVectorRight.z);
    sint32 currentLatVel = (sint32)(((uint32)termX + (uint32)termY + (uint32)termZ) * (uint32)carObj->direction);
    sint32 maxAngularAcceleration = carObj->max_aa;
    sint32 deltaLatPos = (sint32)((uint32)carObj->roadPosition - (uint32)carObj->rampDesiredLatPos);
    sint32 gripMultiplier = fixedmult(carObj->linearVel_ch.z, 0x62);
    gripMultiplier = fixedmult((sint32)((uint32)gripMultiplier + 65536U), carObj->gripFactor);
    if ((carObj->carFlags & 8) != 0 && carObj->wipeOutStartTick < simGlobal.gameTicks)
    {
        sint32 vely = carObj->N.angularVel.y;
        if (vely < 0)
            vely = (sint32)(0U - (uint32)vely);
        if (vely > 1000 && carObj->speed > 0x1638e3)
            carObj->wipeOutEndTick = (sint32)((uint32)simGlobal.gameTicks + 0x180U);
    }
    sint32 lastCollisionTickDiff = (sint32)((uint32)simGlobal.gameTicks - (uint32)carObj->N.collision.lastTime);
    sint32 wipeoutReaction = AIScript::DoReAction(&carObj->script, AIScript::AISCRIPT_AIREACTION_WIPE_OUT);
    if (wipeoutReaction != -1 || simGlobal.gameTicks < carObj->wipeOutEndTick)
    {
        maxAngularAcceleration = 0;
        sint32 ticksSinceCollision = (sint32)((uint32)simGlobal.gameTicks - (uint32)carObj->N.collision.lastTime);
        if (ticksSinceCollision < 64)
            carObj->wipeOutEndTick = simGlobal.gameTicks;
        sint32 vely = carObj->N.angularVel.y;
        if (vely < 0)
            vely = (sint32)(0U - (uint32)vely);
        vely = (sint32)((uint32)vely + 0x1caU);
        if (vely > 0x3333)
            vely = 0x3333;
        gripMultiplier = 0;
        if (carObj->N.angularVel.y <= 0)
            carObj->N.angularVel.y = (sint32)(0U - (uint32)vely);
        else
            carObj->N.angularVel.y = vely;
        skid = 0xa0000;
    }
    sint32 copCollisionFirmness = GameSetup::gData.skill;
    if (lastCollisionTickDiff < 16)
        carObj->wipeOutStartTick = (sint32)((uint32)carObj->wipeOutStartTick - 20U);
    if (lastCollisionTickDiff < (uint8)copCollisionReactionTime[copCollisionFirmness])
    {
        Car::tObj *lastCar = (Car::tObj *)carObj->N.collision.lastOtherObj;
        if (lastCar != 0 && (lastCar->carFlags & 0x200) != 0)
        {
            maxAngularAcceleration = 0;
            justBumped = 1;
            gripMultiplier = fixedmult(gripMultiplier, copCollisionGripLoss[copCollisionFirmness]);
        }
    }
    sint32 maxLateralAcceleration = (sint32)((uint32)(carObj->max_clacc / 256) * (uint32)(gripMultiplier / 256));
    sint32 dlvel_to_clacc = (sint32)((uint32)(AIPhysicConfig.ICModel.dlvel_to_clacc / 256) * (uint32)(gripMultiplier / 256));
    if (carObj->aiGlue > 65536)
    {
        sint32 glue = carObj->aiGlue / 256;
        maxLateralAcceleration = (sint32)((uint32)glue * (uint32)(maxLateralAcceleration / 256));
        dlvel_to_clacc = (sint32)((uint32)glue * (uint32)(dlvel_to_clacc / 256));
    }
    sint32 angleWRTdesired = carObj->aCarWRTDesired;
    sint32 desiredSpeed = carObj->desiredSpeed;
    if (simGlobal.gameTicks < carObj->AIFishtailEndTick)
    {
        sint32 fishtailtick = (sint32)((uint32)carObj->AIFishtailEndTick - (uint32)simGlobal.gameTicks);
        sint32 signAngle = ((uint32)(simGlobal.gameTicks / 40) & 1U) != 0 ? -1 : 1;
        sint32 rampTick = (sint32)(150U - (uint32)fishtailtick);
        sint32 angle;
        if (rampTick < 50)
            angle = (sint32)((uint32)signAngle * 30U * (uint32)rampTick) / 50;
        else
            angle = (sint32)((uint32)signAngle * (uint32)(fishtailtick / 7 + 10));
        angleWRTdesired = (sint32)((uint32)angleWRTdesired + (uint32)angle);
        if (fishtailtick > 50)
            skid = (sint32)((uint32)fishtailtick * 0x140000U) / 50;
    }
    else
    {
        sint32 angleAbs = angleWRTdesired;
        if (angleAbs < 0)
            angleAbs = (sint32)(0U - (uint32)angleAbs);
        if (carObj->personality->fishtailAngle < angleAbs &&
            (sint32)((uint32)angleWRTdesired * (uint32)carObj->angularAcc.y) > 0)
        {
            sint32 scaledAngle = (sint32)((uint32)angleAbs << 16);
            sint32 damage = GetRearEndDamageFactor(carObj);
            sint32 threshold = (sint32)((65536U - (uint32)damage) * 20U +
                (((uint32)carObj->personality->fishtailAngle - 20U) << 16));
            if (threshold < scaledAngle)
                carObj->AIFishtailEndTick = (sint32)((uint32)simGlobal.gameTicks + 150U);
        }
    }
    sint32 angleFactor = fixedmult(0x80, AIPhysicConfig.ICModel.dangle_to_dav);
    sint32 desiredAngVel = (sint32)((0U - (uint32)angleWRTdesired) * (uint32)angleFactor);
    sint32 maxAngVel = AIPhysicConfig.ICModel.max_dav;
    if (desiredAngVel >= maxAngVel)
        desiredAngVel = maxAngVel;
    sint32 minAngVel = (sint32)(0U - (uint32)maxAngVel);
    if (desiredAngVel <= minAngVel)
        desiredAngVel = minAngVel;
    sint32 desiredLatVel = (sint32)(0U - (uint32)(deltaLatPos / 256) * (uint32)(AIPhysicConfig.ICModel.dlpos_to_dlvel / 256));
    sint32 maxLatVel = AIPhysicConfig.ICModel.max_dlvel;
    if (desiredLatVel >= maxLatVel)
        desiredLatVel = maxLatVel;
    sint32 minLatVel = (sint32)(0U - (uint32)maxLatVel);
    if (desiredLatVel <= minLatVel)
        desiredLatVel = minLatVel;
    sint32 speed = carObj->speed;
    if (desiredLatVel >= speed)
        desiredLatVel = speed;
    sint32 negativeSpeed = (sint32)(0U - (uint32)speed);
    if (desiredLatVel <= negativeSpeed)
        desiredLatVel = negativeSpeed;
    sint32 velocityRange = AIPhysicConfig.ICModel.vel_limit_range;
    if ((sint32)(0U - (uint32)velocityRange) < carObj->currentSpeed && carObj->currentSpeed < velocityRange)
    {
        maxLatVel = (sint32)((uint32)(speed / 256) * (uint32)(AIPhysicConfig.ICModel.lat_vel_limit_factor / 256));
        if (maxLatVel < 0)
            maxLatVel = (sint32)(0U - (uint32)maxLatVel);
        if (desiredLatVel >= maxLatVel)
            desiredLatVel = maxLatVel;
        maxAngVel = (sint32)((uint32)(speed / 256) * (uint32)(AIPhysicConfig.ICModel.ang_vel_limit_factor / 256));
        if (maxAngVel < 0)
            maxAngVel = (sint32)(0U - (uint32)maxAngVel);
        if (desiredAngVel >= maxAngVel)
            desiredAngVel = maxAngVel;
        minAngVel = (sint32)(0U - (uint32)maxAngVel);
        if (desiredAngVel <= minAngVel)
            desiredAngVel = minAngVel;
    }

    (void)desiredLatVel;
    sint32 currentAngAcc = fixedmult((sint32)((uint32)carObj->angularVel_ch.y - (uint32)desiredAngVel), AIPhysicConfig.ICModel.dav_to_aa);
    currentAngAcc = (sint32)(0U - (uint32)currentAngAcc);
    if (currentAngAcc >= maxAngularAcceleration)
        currentAngAcc = maxAngularAcceleration;
    sint32 minAngAcc = (sint32)(0U - (uint32)maxAngularAcceleration);
    if (currentAngAcc <= minAngAcc)
        currentAngAcc = minAngAcc;
    sint32 currentLatAcc = fixedmult(currentLatVel, dlvel_to_clacc);
    currentLatAcc = (sint32)((0U - (uint32)dir) * (uint32)currentLatAcc);
    if (currentLatAcc >= maxLateralAcceleration)
        currentLatAcc = maxLateralAcceleration;
    sint32 minLatAcc = (sint32)(0U - (uint32)maxLateralAcceleration);
    if (currentLatAcc <= minLatAcc)
        currentLatAcc = minLatAcc;
    sint32 currentLongAcc;
    if ((sint32)((uint32)carObj->currentSpeed * (uint32)dir) < (sint32)((uint32)desiredSpeed * (uint32)dir) &&
        carObj->pullOver == 0 && carObj->desiredDirection == carObj->direction)
    {
        currentLongAcc = CalcAcceleration(carObj, carObj->currentSpeed);
        uint32 brakeLevel = (uint8)carObj->control.brakeLevel;
        carObj->control.brakeLevel = elapsedTime < (sint32)brakeLevel ? (char)(brakeLevel - (uint32)elapsedTime) : 0;
        if (carObj->control.brakeLevel == 0)
            carObj->control.desiredBrakeLevel = 0;
    }
    else
    {
        sint32 deceleration = CalcDeceleration(carObj);
        currentLongAcc = (sint32)(0U - (uint32)deceleration);
        sint32 brakeLevel = (sint32)((uint32)(uint8)carObj->control.brakeLevel + ((uint32)elapsedTime << 2));
        carObj->control.brakeLevel = brakeLevel < 255 ? (char)brakeLevel : (char)255;
        if ((uint8)carObj->control.brakeLevel > 3)
            carObj->control.desiredBrakeLevel = 1;
        if (carObj->linearVel_ch.z < 0)
            currentLongAcc = (sint32)(0U - (uint32)currentLongAcc);
    }
    if (justBumped)
        currentLongAcc = (sint32)(0U - (uint32)copCollisionSlowDown[copCollisionFirmness]);
    if (skid == 0)
    {
        if (currentLatVel < 0)
            currentLatVel = (sint32)(0U - (uint32)currentLatVel);
        skid = (sint32)((uint32)currentLatVel - (uint32)AIPhysicConfig.skid_value);
    }
    if (skid <= 0)
    {
        carObj->rearSkid = 0;
        carObj->frontSkid = 0;
    }
    else
    {
        carObj->rearSkid = skid;
        carObj->frontSkid = skid;
    }
    carObj->linearAcc_ch.x = currentLatAcc;
    carObj->linearAcc_ch.y = 0;
    carObj->linearAcc_ch.z = currentLongAcc;
    carObj->linearAcc_rh.x = 0;
    carObj->linearAcc_rh.y = 0;
    carObj->linearAcc_rh.z = 0;
    carObj->angularAcc.x = 0;
    carObj->angularAcc.y = currentAngAcc;
    carObj->angularAcc.z = 0;
}

void AIPhysic::FinishUp(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006C37C, "NFS4.EXE");
    carObj->linearAcc_ch.z = (sint32)((uint32)carObj->linearAcc_ch.z + (uint32)carObj->linearAcc_rh.z);
    Cars::DoGravityEffectsOnAcc(carObj, 0);
    carObj->linearAcc_ch.z = (sint32)((uint32)carObj->linearAcc_ch.z - (uint32)carObj->linearAcc_rh.z);
    matrixtdef transOrientMat;
    matrixtdef transRoadMat;
    transpose(&carObj->N.orientMat, &transOrientMat);
    transpose(&carObj->N.roadMatrix, &transRoadMat);
    sint32 term0 = fixedmult(carObj->linearAcc_ch.x, transOrientMat.m[0]);
    sint32 term1 = fixedmult(carObj->linearAcc_ch.y, transOrientMat.m[1]);
    sint32 term2 = fixedmult(carObj->linearAcc_ch.z, transOrientMat.m[2]);
    carObj->linearAcc.x = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->linearAcc_ch.x, transOrientMat.m[3]);
    term1 = fixedmult(carObj->linearAcc_ch.y, transOrientMat.m[4]);
    term2 = fixedmult(carObj->linearAcc_ch.z, transOrientMat.m[5]);
    carObj->linearAcc.y = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->linearAcc_ch.x, transOrientMat.m[6]);
    term1 = fixedmult(carObj->linearAcc_ch.y, transOrientMat.m[7]);
    term2 = fixedmult(carObj->linearAcc_ch.z, transOrientMat.m[8]);
    carObj->linearAcc.z = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->linearAcc_rh.x, transRoadMat.m[0]);
    term1 = fixedmult(carObj->linearAcc_rh.y, transRoadMat.m[1]);
    term2 = fixedmult(carObj->linearAcc_rh.z, transRoadMat.m[2]);
    carObj->linearAcc.x = (sint32)((uint32)carObj->linearAcc.x + (uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->linearAcc_rh.x, transRoadMat.m[3]);
    term1 = fixedmult(carObj->linearAcc_rh.y, transRoadMat.m[4]);
    term2 = fixedmult(carObj->linearAcc_rh.z, transRoadMat.m[5]);
    carObj->linearAcc.y = (sint32)((uint32)carObj->linearAcc.y + (uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->linearAcc_rh.x, transRoadMat.m[6]);
    term1 = fixedmult(carObj->linearAcc_rh.y, transRoadMat.m[7]);
    term2 = fixedmult(carObj->linearAcc_rh.z, transRoadMat.m[8]);
    carObj->linearAcc.z = (sint32)((uint32)carObj->linearAcc.z + (uint32)term0 + (uint32)term1 + (uint32)term2);
    coorddef angAcc;
    term0 = fixedmult(carObj->angularAcc.x, transOrientMat.m[0]);
    term1 = fixedmult(carObj->angularAcc.y, transOrientMat.m[1]);
    term2 = fixedmult(carObj->angularAcc.z, transOrientMat.m[2]);
    angAcc.x = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->angularAcc.x, transOrientMat.m[3]);
    term1 = fixedmult(carObj->angularAcc.y, transOrientMat.m[4]);
    term2 = fixedmult(carObj->angularAcc.z, transOrientMat.m[5]);
    angAcc.y = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    term0 = fixedmult(carObj->angularAcc.x, transOrientMat.m[6]);
    term1 = fixedmult(carObj->angularAcc.y, transOrientMat.m[7]);
    term2 = fixedmult(carObj->angularAcc.z, transOrientMat.m[8]);
    angAcc.z = (sint32)((uint32)term0 + (uint32)term1 + (uint32)term2);
    carObj->angularAcc = angAcc;
    sint32 deltaX = fixedmult(carObj->linearAcc.x, iTime);
    carObj->N.linearVel.x = (sint32)((uint32)carObj->N.linearVel.x + (uint32)deltaX);
    sint32 deltaY = fixedmult(carObj->linearAcc.y, iTime);
    carObj->N.linearVel.y = (sint32)((uint32)carObj->N.linearVel.y + (uint32)deltaY);
    sint32 deltaZ = fixedmult(carObj->linearAcc.z, iTime);
    carObj->N.linearVel.z = (sint32)((uint32)carObj->N.linearVel.z + (uint32)deltaZ);
    for (sint32 tickLoop = 0; carObj->drag != 0 && tickLoop < elapsedTime; ++tickLoop)
    {
        carObj->N.linearVel.x = fixedmult(carObj->N.linearVel.x, carObj->drag);
        carObj->N.linearVel.y = fixedmult(carObj->N.linearVel.y, carObj->drag);
        carObj->N.linearVel.z = fixedmult(carObj->N.linearVel.z, carObj->drag);
    }
    sint32 angularTime = iTime / 256;
    carObj->N.angularVel.x = (sint32)((uint32)carObj->N.angularVel.x +
        (uint32)(carObj->angularAcc.x / 256) * (uint32)angularTime);
    carObj->N.angularVel.y = (sint32)((uint32)carObj->N.angularVel.y +
        (uint32)(carObj->angularAcc.y / 256) * (uint32)angularTime);
    carObj->N.angularVel.z = (sint32)((uint32)carObj->N.angularVel.z +
        (uint32)(carObj->angularAcc.z / 256) * (uint32)angularTime);
}

void AIPhysic::CalculateRampedDesiredLatPos(Car::tObj *carObj, eRampType rampType)
{
    FUNCTION_MARKER(0x8006C768, "NFS4.EXE");
    if (rampType == kRampType_NoInterpolate)
    {
        carObj->rampDesiredLatPos = carObj->desiredLatPos;
        return;
    }
    AIWorld::CalculateDeltaRoadYaw(carObj);
    sint32 rampSpeed = (sint32)((uint32)elapsedTime * 0x3333U);
    sint32 roadPosition = carObj->roadPosition;
    if (carObj->desiredLatPos < roadPosition &&
        roadPosition < (sint32)((uint32)carObj->rampDesiredLatPos - 65536U))
        carObj->rampDesiredLatPos = roadPosition;
    else if ((sint32)((uint32)carObj->rampDesiredLatPos + 65536U) < roadPosition &&
             roadPosition < carObj->desiredLatPos)
        carObj->rampDesiredLatPos = roadPosition;
    sint32 rampPosition = carObj->rampDesiredLatPos;
    sint32 desiredLatPos = carObj->desiredLatPos;
    if (rampPosition < desiredLatPos)
    {
        rampPosition = (sint32)((uint32)rampPosition + (uint32)rampSpeed);
        carObj->rampDesiredLatPos = rampPosition;
        if (desiredLatPos < rampPosition)
            carObj->rampDesiredLatPos = desiredLatPos;
    }
    else if (desiredLatPos < rampPosition)
    {
        rampPosition = (sint32)((uint32)rampPosition - (uint32)rampSpeed);
        desiredLatPos = carObj->desiredLatPos;
        carObj->rampDesiredLatPos = rampPosition;
        if (rampPosition < desiredLatPos)
            carObj->rampDesiredLatPos = desiredLatPos;
    }
}

sint32 AIPhysic::HitWallCheck(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006C870, "NFS4.EXE");
    uint32 lanes = BWorldSm::slices[carObj->N.simRoadInfo.slice].laneCount;
    sint32 onRoad = carObj->laneIndex >= 7 - (sint32)(lanes >> 4) &&
        carObj->laneIndex <= (sint32)(lanes & 15U) + 6;
    if (onRoad)
        return 0;
    if (carObj->driveDirection == -1)
        carObj->timeOffRoad = (sint32)((uint32)carObj->timeOffRoad + (uint32)elapsedTime);
    else
        carObj->timeOffRoad = 0;
    if (carObj->timeOffRoad < 9)
        return carObj->N.roadMatrix.m[4] <= 0xd999;
    return 1;
}

void AIPhysic::ProcessBarrierCollision(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006C920, "NFS4.EXE");
    if ((carObj->carFlags & 4) != 0)
        return;
    sint32 speed = carObj->currentSpeed;
    if (speed < 0)
        speed = (sint32)(0U - (uint32)speed);
    if (speed < 0xa0000)
        ChangeDirection(carObj, 0x60);
}

void AIPhysic::ProcessCollision(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006C974, "NFS4.EXE");
    if (carObj->N.collision.impulse <= 0xd999 || carObj->N.collision.otherObj == 0)
        return;
    sint32 speed = carObj->currentSpeed;
    if (speed < 0)
        speed = (sint32)(0U - (uint32)speed);
    if (speed < 0xa0000)
    {
        sint32 reverseTime = (carObj->carFlags & 0x10) != 0 ? 0xa0 : 0x60;
        ChangeDirection(carObj, reverseTime);
    }
}

void AIPhysic::ChangeDirection(Car::tObj *carObj, sint32 time)
{
    FUNCTION_MARKER(0x8006C9F0, "NFS4.EXE");
    sint32 direction = carObj->driveDirection;
    if (direction != -1 && direction != 1)
        return;
    sint32 ticksSinceChange = (sint32)((uint32)simGlobal.gameTicks - (uint32)carObj->driveDirectionTimer);
    if (ticksSinceChange <= carObj->driveDirectionReverseTime / 2)
        return;
    if (direction == 1)
    {
        sint32 speed = carObj->currentSpeed;
        if (speed < 0)
            speed = (sint32)(0U - (uint32)speed);
        if (speed > 0x13ffff)
            return;
    }
    carObj->driveDirection = direction == -1 ? 1 : -1;
    sint32 gameTicks = simGlobal.gameTicks;
    carObj->driveDirectionReverseTime = time;
    carObj->driveDirectionTimer = gameTicks;
}

void AIPhysic::CheckForGripReduction(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006CAA8, "NFS4.EXE");
    if (carObj->gripFactor < 65536)
    {
        sint32 recovery = (sint32)((uint32)elapsedTime * (uint32)carObj->personality->gripLossRecoveryPerTick);
        sint32 lastTime = carObj->N.collision.lastTime;
        sint32 gripFactor = (sint32)((uint32)carObj->gripFactor + (uint32)recovery);
        carObj->gripFactor = gripFactor;
        sint32 ticksSinceCollision = (sint32)((uint32)simGlobal.gameTicks - (uint32)lastTime);
        if (ticksSinceCollision < 64 && carObj->N.collision.lastOtherObj == 0)
            carObj->gripFactor = 65536;
        else
        {
            sint32 minimum = carObj->personality->gripLossMinFactor;
            sint32 range = (sint32)(65536U - (uint32)minimum);
            sint32 threshold = (sint32)((uint32)minimum + (uint32)(range / 4));
            if (threshold < gripFactor)
                carObj->gripFactor = 65536;
        }
        return;
    }
    if (carObj->N.simOptz != 0 || (carObj->carFlags & 0x28) == 0)
        return;
    sint32 bend = AIWorld::CalcRoadBend(carObj, 1);
    if (bend > 0)
        bend = AIWorld::CalcRoadBend(carObj, 1);
    else
        bend = (sint32)(0U - (uint32)AIWorld::CalcRoadBend(carObj, 1));
    if (bend < 0x7d1)
        return;
    uint32 product = fastRandom * randSeed;
    AIPerson::AIPerson_t *personality = carObj->personality;
    sint32 perTickProb = (sint32)((uint32)elapsedTime * (uint32)personality->gripLossProbPerSecond);
    randtemp = product;
    fastRandom = product & 0xffffU;
    sint32 randVal = (sint32)((product >> 8) & 0xffffU);
    if (randVal < perTickProb / 32)
        carObj->gripFactor = personality->gripLossMinFactor;
}

void AIPhysic::StartUp(void)
{
    FUNCTION_MARKER(0x8006CC3C, "NFS4.EXE");
}

void AIPhysic::CleanUp(void)
{
    FUNCTION_MARKER(0x8006CC44, "NFS4.EXE");
}

void AIPhysic::Reset(void)
{
    FUNCTION_MARKER(0x8006CC4C, "NFS4.EXE");
}

void AIPhysic::ResetCar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006CC54, "NFS4.EXE");
    carObj->pullOver = 0;
    carObj->blowout = 0;
}

void AIPhysic::InitCar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006CC60, "NFS4.EXE");
    sint32 flags = carObj->carFlags;
    if ((flags & 2) == 0)
        return;
    sint32 d = (flags & 0x28) != 0 ? 0xb0000 : 0xc0000;
    d = (sint32)((uint32)(carObj->personality->brakeMultiplier / 256) * (uint32)(d / 256));
    if ((flags & 8) != 0)
    {
        sint32 brakeMult = AISpeeds::GetUpgradeBrakeMult(carObj->carIndex);
        d = (sint32)((uint32)(d / 256) * (uint32)(brakeMult / 256));
    }
    void *memory = reservememadr(0, sizeof(BrakeInfo), 0);
    BrakeInfo *info = new (memory) BrakeInfo;
    sint32 deceleration = d;
    info->deceleration_ = deceleration;
    sint32 invDeceleration = fixeddiv(65536, deceleration);
    for (sint32 brakeTableLoop = 0; brakeTableLoop < 128; ++brakeTableLoop)
    {
        sint32 speed = brakeTableLoop * 65536;
        sint32 distance = fixedmult(speed, invDeceleration);
        distance = fixedmult(distance, speed);
        sint32 brakeDistanceMeters = distance / 2;
        sint32 sIndex = brakeTableLoop;
        info->brakeTable_[sIndex] = (uint8)(brakeDistanceMeters / 131072);
    }
    carObj->brakeInfo = info;
}

void AIPhysic::DeInitCar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006CDD4, "NFS4.EXE");
    if ((carObj->carFlags & 2) != 0 && carObj->brakeInfo != 0)
    {
        purgememadr(carObj->brakeInfo);
        carObj->brakeInfo = 0;
    }
}
