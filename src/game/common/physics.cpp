#include <stdlib.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/aiinit.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/collide.h"
#include "game/common/fastrand.h"
#include "game/common/physics.h"
#include "game/common/sim.h"
#include "game/common/udff.h"
#include "game/psx/force.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

struct Physics::tWheelAccStruct
{
    sint32 acc;
    sint32 roadGrip;
    sint32 steeringAcc;
    sint32 steeringAngle;
    coorddef velCap;
    coorddef finalAcc;
    sint32 frontTire;
    sint32 skid;
};

static sint32 currentWallType;
sint32 roadMult;
sint32 frontMult;
sint32 rearMult;
sint32 leftMult;
sint32 rightMult;
sint32 slippery;
sint32 steeringControl;
sint32 powerControl;
sint32 gBrakeRatio;
sint32 gGasRatio;
sint32 gSteerRatio;
sint32 exceedRedline;
coorddef gravity_ch;
sint32 roadSurfaceFrictionCoeff[10] = { 72089, 68812, 65536, 62259, 58982, 55705, 52428, 49152, 45875, 32768 };
sint32 ReverseRoadSurfaceFrictionCoeff[10] = { 59637, 62259, 65536, 69009, 72744, 77070, 81920, 87162, 93716, 131072 };

char roadSurfaceIndex[3][20] = {
    { 7, 3, 3, 3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 3, 5, 3, 5, 5, 5, 5 },
    { 7, 2, 4, 4, 2, 4, 5, 2, 2, 4, 2, 3, 3, 4, 5, 4, 5, 5, 5, 5 },
    { 7, 0, 2, 2, 1, 3, 3, 1, 0, 3, 0, 1, 1, 3, 5, 5, 5, 5, 5, 5 },
};

sint32 gripLossTable[3] = { 32, 8, 8 };
sint32 gripLossTableWet[3] = { 16, 6, 4 };

void Physics::InitCarSpecs(Car::tObj *carObj, Udff::tInfo *handle)
{
    FUNCTION_MARKER(0x800A86C8, "NFS4.EXE");

    sint32 value;
    value = Udff::GetInt(handle);
    carObj->specs->mass = value;
    value = Udff::GetInt(handle);
    carObj->specs->numGears = value;
    value = Udff::GetInt(handle);
    carObj->specs->gearShiftDelay = value;
    for (sint32 i = 0; i < 8; ++i)
    {
        value = Udff::GetInt(handle);
        carObj->specs->velToRpmRatio[i] = value;
    }
    for (sint32 i = 0; i < 8; ++i)
    {
        value = Udff::GetInt(handle);
        carObj->specs->gearEfficiency[i] = value;
    }
    for (sint32 i = 0; i < 41; ++i)
    {
        value = Udff::GetInt(handle);
        carObj->specs->torqueCurve[i] = value;
    }
    value = Udff::GetInt(handle);
    carObj->specs->redline = value;
    value = Udff::GetInt(handle);
    carObj->specs->maxSpeed = value;
    value = Udff::GetInt(handle);
    carObj->specs->frontDriveRatio = value;
    value = Udff::GetInt(handle);
    carObj->specs->maxBrakeAcc = value;
    value = Udff::GetInt(handle);
    carObj->specs->frontBrakeRatio = value;
    value = Udff::GetInt(handle);
    carObj->specs->wheelBase = value;
    value = Udff::GetInt(handle);
    carObj->specs->frontGripBias = value;
    value = Udff::GetInt(handle);
    carObj->specs->maxSteeringAcc = value;
    value = Udff::GetInt(handle);
    carObj->specs->steeringRamp = value;
    value = Udff::GetInt(handle);
    carObj->specs->lateralGripMult = value;
    value = Udff::GetInt(handle);
    carObj->specs->frontAeroDownForce = value;
    value = Udff::GetInt(handle);
    carObj->specs->rearAeroDownForce = value;
    value = Udff::GetInt(handle);
    carObj->specs->gasOffFactor = value;
    value = Udff::GetInt(handle);
    carObj->specs->gTransferFactor = value;
    value = Udff::GetInt(handle);
    carObj->specs->slideMultiplier = value;
    value = Udff::GetInt(handle);
    carObj->specs->spinVelCap = value;
    value = Udff::GetInt(handle);
    carObj->specs->dampingPitchRate = value;
    value = Udff::GetInt(handle);
    carObj->specs->dampingRollRate = value;
    value = Udff::GetInt(handle);
    carObj->specs->suspensionStiffness = value;
    value = Udff::GetInt(handle);
    carObj->specs->pitchAngularVelCap = value;
    value = Udff::GetInt(handle);
    carObj->specs->rollAngularVelCap = value;
    value = Udff::GetInt(handle);
    carObj->specs->damageFactor = value;
    value = Udff::GetInt(handle);
    carObj->specs->bodyPitchFactor = value;
    value = Udff::GetInt(handle);
    carObj->specs->bodyRollFactor = value;
    value = Udff::GetInt(handle);
    carObj->specs->tireRange = value;
    value = Udff::GetInt(handle);
    carObj->specs->rideOffset = value;
    value = Udff::GetInt(handle);
    carObj->specs->dragFactor = value;
}

void Physics::CalculateDerivedCarSpecs(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800A8964, "NFS4.EXE");

    sint32 redline = carObj->specs->redline;
    if (redline == 0)
        abort();
    carObj->specs->redlineInv = 65536 / redline;
    if (carObj->carInfo->WeightTransfer == 1)
    {
        carObj->specs->steeringRamp = (sint32)((uint32)carObj->specs->steeringRamp + 1);
        carObj->specs->maxSteeringAcc = fixedmult(carObj->specs->maxSteeringAcc, 0x1147a);
    }
    if (carObj->carInfo->GroundEffects == 1)
    {
        carObj->specs->steeringRamp = (sint32)((uint32)carObj->specs->steeringRamp + 1);
        carObj->specs->frontAeroDownForce = fixedmult(carObj->specs->frontAeroDownForce, 0x13333);
        carObj->specs->rearAeroDownForce = fixedmult(carObj->specs->rearAeroDownForce, 0x13333);
        carObj->specs->mass = fixedmult(carObj->specs->mass, 0xcccc);
        carObj->specs->lateralGripMult = fixedmult(carObj->specs->lateralGripMult, 0x11999);
    }
    if (carObj->carInfo->EngineMods == 1)
    {
        carObj->specs->gearShiftDelay /= 2;
        carObj->specs->maxBrakeAcc = fixedmult(carObj->specs->maxBrakeAcc, 0x14000);
        for (sint32 i = 0; i < 41; ++i)
            carObj->specs->torqueCurve[i] = fixedmult(carObj->specs->torqueCurve[i], 0x12666);
        carObj->specs->maxSpeed = fixedmult(carObj->specs->maxSpeed, 0x11999);
    }
    for (sint32 i = 0; i < carObj->specs->numGears; ++i)
    {
        sint32 inverse = carObj->specs->velToRpmRatio[i] == 0 ? 0x28f : fixeddiv(65536, carObj->specs->velToRpmRatio[i]);
        carObj->specs->velToRpmRatioInv[i] = inverse;
        carObj->specs->gearAccCoeff[i] = fixeddiv(carObj->specs->velToRpmRatio[i], carObj->specs->mass);
        carObj->specs->gearAccCoeff[i] = fixeddiv(carObj->specs->gearAccCoeff[i], 0xa0000);
        carObj->specs->gearAccCoeff[i] = fixedmult(carObj->specs->gearAccCoeff[i], carObj->specs->gearEfficiency[i]);
        sint32 maximumVelocity = fixedmult(carObj->specs->velToRpmRatioInv[i], (sint32)((uint32)carObj->specs->redline << 16));
        carObj->specs->gearVelInv[i] = fixeddiv(65536, maximumVelocity);
    }
    sint32 rpm = fixedmult(carObj->specs->maxSpeed, carObj->specs->velToRpmRatio[carObj->specs->numGears - 1]) / 65536;
    sint32 accAtMaxSpeedInHighestGear = fixedmult(carObj->specs->torqueCurve[rpm / 256], carObj->specs->gearAccCoeff[carObj->specs->numGears - 1]);
    sint32 velocity = carObj->specs->maxSpeed / 65536;
    uint32 cube = (uint32)velocity * (uint32)velocity * (uint32)velocity;
    carObj->specs->dragCoeff = fixeddiv(accAtMaxSpeedInHighestGear, (sint32)cube);
    if (GameSetup::gData.track >= 8)
    {
        carObj->specs->gasOffFactor = (sint32)((uint32)carObj->specs->gasOffFactor + 0x2666);
        carObj->specs->frontBrakeRatio = (sint32)((uint32)carObj->specs->frontBrakeRatio - 0x2666);
        carObj->specs->frontGripBias = (sint32)((uint32)carObj->specs->frontGripBias + 0x147);
    }
    carObj->specs->alphaToAccRotInertia = fixedmult(65536, carObj->specs->wheelBase / 2);
    carObj->specs->alphaToAccRotInertia = fixedmult(carObj->specs->alphaToAccRotInertia, 0x648);
    carObj->specs->alphaToAccRotInertia = (sint32)((uint32)carObj->specs->alphaToAccRotInertia << 8);
    carObj->specs->accToAlphaRotInertia = fixeddiv(65536, carObj->specs->alphaToAccRotInertia);
    carObj->specs->lateralGripMultInv = fixeddiv(65536, carObj->specs->lateralGripMult);
}

void Physics::CheckGamedata(void)
{
    FUNCTION_MARKER(0x800A8E74, "NFS4.EXE");
}

sint32 Physics::AttenuateVelocity(Car::tObj *carObj, sint32 force, matrixtdef *roadMat)
{
    FUNCTION_MARKER(0x800A8E7C, "NFS4.EXE");

    sint32 forceScaled = force / 256;
    uint32 offsetX = (uint32)forceScaled * (uint32)(roadMat->m[0] / 256);
    uint32 offsetY = (uint32)forceScaled * (uint32)(roadMat->m[3] / 256);
    uint32 offsetZ = (uint32)forceScaled * (uint32)(roadMat->m[6] / 256);
    carObj->N.position.x = (sint32)((uint32)carObj->N.position.x - offsetX);
    carObj->N.position.z = (sint32)((uint32)carObj->N.position.z + offsetZ);
    carObj->N.position.y = (sint32)((uint32)carObj->N.position.y + offsetY);
    uint32 dotX = (uint32)(carObj->N.linearVel.x / 256) * (uint32)(roadMat->m[0] / 256);
    dotX += (uint32)(carObj->N.linearVel.y / 256) * (uint32)(roadMat->m[1] / 256);
    dotX += (uint32)(carObj->N.linearVel.z / 256) * (uint32)(roadMat->m[2] / 256);
    sint32 vx = (sint32)(0U - dotX);
    sint32 absvelbx = (vx < 0 ? (sint32)(0U - (uint32)vx) : vx) >> 1;
    coorddef vel_b;
    vel_b.x = 0;
    vel_b.y = 0;
    uint32 dotZ = (uint32)(carObj->N.linearVel.x / 256) * (uint32)(roadMat->m[6] / 256);
    dotZ += (uint32)(carObj->N.linearVel.y / 256) * (uint32)(roadMat->m[7] / 256);
    dotZ += (uint32)(carObj->N.linearVel.z / 256) * (uint32)(roadMat->m[8] / 256);
    vel_b.z = (sint32)dotZ;
    uint32 attenuation = (uint32)(absvelbx / 256) * 192U;
    if (vel_b.z > 0x50000)
    {
        vel_b.z = (sint32)((uint32)vel_b.z - attenuation);
        if (vel_b.z < 0)
            vel_b.z = 0;
    }
    else if (vel_b.z < -0x50000)
    {
        vel_b.z = (sint32)((uint32)vel_b.z + attenuation);
        if (vel_b.z > 0)
            vel_b.z = 0;
    }
    matrixtdef transposeMat;
    transpose(roadMat, &transposeMat);
    sint32 a = fixedmult(vel_b.x, transposeMat.m[0]);
    sint32 b = fixedmult(vel_b.y, transposeMat.m[1]);
    sint32 c = fixedmult(vel_b.z, transposeMat.m[2]);
    carObj->N.linearVel.x = (sint32)((uint32)a + (uint32)b + (uint32)c);
    a = fixedmult(vel_b.x, transposeMat.m[6]);
    b = fixedmult(vel_b.y, transposeMat.m[7]);
    c = fixedmult(vel_b.z, transposeMat.m[8]);
    carObj->N.linearVel.z = (sint32)((uint32)a + (uint32)b + (uint32)c);
    if (carObj->N.linearVel.y > 0)
        carObj->N.linearVel.y = 0;
    a = fixedmult(carObj->N.linearVel.x, carObj->N.orientMat.m[0]);
    b = fixedmult(carObj->N.linearVel.y, carObj->N.orientMat.m[1]);
    c = fixedmult(carObj->N.linearVel.z, carObj->N.orientMat.m[2]);
    carObj->linearVel_ch.x = (sint32)((uint32)a + (uint32)b + (uint32)c);
    a = fixedmult(carObj->N.linearVel.x, carObj->N.orientMat.m[3]);
    b = fixedmult(carObj->N.linearVel.y, carObj->N.orientMat.m[4]);
    c = fixedmult(carObj->N.linearVel.z, carObj->N.orientMat.m[5]);
    carObj->linearVel_ch.y = (sint32)((uint32)a + (uint32)b + (uint32)c);
    a = fixedmult(carObj->N.linearVel.x, carObj->N.orientMat.m[6]);
    b = fixedmult(carObj->N.linearVel.y, carObj->N.orientMat.m[7]);
    c = fixedmult(carObj->N.linearVel.z, carObj->N.orientMat.m[8]);
    sint32 absX = carObj->N.linearVel.x;
    sint32 absZ = carObj->N.linearVel.z;
    if (absX < 0)
        absX = (sint32)(0U - (uint32)absX);
    if (absZ < 0)
        absZ = (sint32)(0U - (uint32)absZ);
    carObj->linearVel_ch.z = (sint32)((uint32)a + (uint32)b + (uint32)c);
    carObj->N.speedXZ = absZ < absX ? (sint32)((uint32)absX + (uint32)(absZ >> 2)) : (sint32)((uint32)absZ + (uint32)(absX >> 2));
    return absvelbx;
}

void Physics::SetCurrentWallType(sint32 wallType)
{
    FUNCTION_MARKER(0x800A92D8, "NFS4.EXE");

    currentWallType = wallType;
}

sint32 Physics::GetTorque(Car::tObj *carObj, sint32 index)
{
    FUNCTION_MARKER(0x800A92E4, "NFS4.EXE");

    if (index < 0)
        index = 0;
    else if (index > 40)
        index = 40;
    return carObj->specs->torqueCurve[index];
}

void Physics::CorrectPostCollisionYaw(Car::tObj *carObj, sint32 impactVel, coorddef barrierVec)
{
    FUNCTION_MARKER(0x800A9320, "NFS4.EXE");

    carObj->N.collision.impulse = (sint32)((uint32)impactVel * 6U);
    sint32 wallType = currentWallType;
    carObj->N.collision.otherObj = 0;
    carObj->N.collision.sfxType = wallType | 0x40000;
    if (impactVel >= 0xf0000 && carObj->linearVel_ch.z >= 0x140000)
        return;
    sint32 z1 = fixedmult(barrierVec.x, carObj->N.shadowMat.m[6]);
    sint32 z2 = fixedmult(barrierVec.y, carObj->N.shadowMat.m[7]);
    sint32 z3 = fixedmult(barrierVec.z, carObj->N.shadowMat.m[8]);
    sint32 diffZ = (sint32)((uint32)z1 + (uint32)z2 + (uint32)z3);
    sint32 x1 = fixedmult(barrierVec.x, carObj->N.shadowMat.m[0]);
    sint32 x2 = fixedmult(barrierVec.y, carObj->N.shadowMat.m[1]);
    sint32 x3 = fixedmult(barrierVec.z, carObj->N.shadowMat.m[2]);
    sint32 diffX = (sint32)((uint32)x1 + (uint32)x2 + (uint32)x3);
    sint32 absZ = diffZ < 0 ? (sint32)(0U - (uint32)diffZ) : diffZ;
    sint32 absX = diffX < 0 ? (sint32)(0U - (uint32)diffX) : diffX;
    sint32 result = (absX < absZ ? absX : absZ) >> 1;
    if (diffZ < 0)
    {
        if (diffX < 0)
            result = (sint32)(0U - (uint32)result);
    }
    else
    {
        if (diffX > 0)
            result = (sint32)(0U - (uint32)result);
        result >>= 1;
    }
    if (carObj->linearVel_ch.z > 0xd6666 && ((result > 0 && carObj->control.steering > 0) || (result < 0 && carObj->control.steering < 0)))
        result >>= 2;
    carObj->N.angularVel.y = result;
}

sint32 Physics::DoBarrierCheck(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800A94B8, "NFS4.EXE");
    BO::tNewtonObj &obj = carObj->N;
    sint32 slice = obj.simRoadInfo.slice;
    const Trk::NewSlice &road = BWorldSm::slices[slice];
    coorddef right = {
        (sint8)road.right[0] * 512,
        (sint8)road.right[1] * 512,
        (sint8)road.right[2] * 512
    };
    coorddef predicted = {
        (sint32)((uint32)obj.position.x + (uint32)(obj.linearVel.x >> 5) - (uint32)road.center[0]),
        (sint32)((uint32)obj.position.y + (uint32)(obj.linearVel.y >> 5) - (uint32)road.center[1]),
        (sint32)((uint32)obj.position.z + (uint32)(obj.linearVel.z >> 5) - (uint32)road.center[2])
    };

    sint32 r1 = right.x / 256;
    sint32 r2 = right.y / 256;
    sint32 r3 = right.z / 256;
    sint32 x_relRoad = (sint32)((uint32)r1 * (uint32)(predicted.x / 256) +
        (uint32)r2 * (uint32)(predicted.y / 256) + (uint32)r3 * (uint32)(predicted.z / 256));
    obj.xRelRoadCenter = x_relRoad;

    const sint32 *orientation = obj.orientMat.m;
    sint32 forward = (sint32)((uint32)r1 * (uint32)(orientation[6] / 256) +
        (uint32)r2 * (uint32)(orientation[7] / 256) + (uint32)r3 * (uint32)(orientation[8] / 256));
    sint32 lengthWidth = fixedmult(obj.dimension.z, forward);
    if (lengthWidth < 0)
        lengthWidth = (sint32)(0U - (uint32)lengthWidth);
    sint32 lateral = (sint32)((uint32)r1 * (uint32)(orientation[0] / 256) +
        (uint32)r2 * (uint32)(orientation[1] / 256) + (uint32)r3 * (uint32)(orientation[2] / 256));
    sint32 halfWidth = obj.dimension.x / 256;
    sint32 sideWidth = (sint32)((uint32)halfWidth * (uint32)(lateral / 256));
    if (sideWidth < 0)
        sideWidth = (sint32)(0U - (uint32)sideWidth);
    uint32 carCollisionWidth = (uint32)lengthWidth + (uint32)sideWidth;

    sint32 diff = 0;
    sint32 collide = 0;
    sint32 left = (sint32)(carCollisionWidth - (uint32)(road.leftDrive * 256));
    if (x_relRoad < (sint32)((uint32)left - (uint32)carObj->extraWallCollisionAllowance))
    {
        collide = -1;
        diff = (sint32)((uint32)left - (uint32)x_relRoad);
    }
    sint32 rightLimit = (sint32)((uint32)(road.rightDrive * 256) - carCollisionWidth);
    if (x_relRoad > (sint32)((uint32)rightLimit + (uint32)carObj->extraWallCollisionAllowance))
    {
        collide = 1;
        diff = (sint32)((uint32)x_relRoad - (uint32)rightLimit);
    }
    if (collide == 0)
        return 0;

    currentWallType = 1;
    if (Force::IsForceOn(carObj))
        Force::HitWall(0x1E0000);

    halfWidth = obj.dimension.x / 256;
    coorddef widthVector = {
        (sint32)((uint32)halfWidth * (uint32)right.x) / 256,
        (sint32)((uint32)halfWidth * (uint32)right.y) / 256,
        (sint32)((uint32)halfWidth * (uint32)right.z) / 256
    };
    obj.collision.collisionPoint.x = (sint32)((uint32)obj.position.x + (uint32)(collide * widthVector.x));
    obj.collision.collisionPoint.y = (sint32)((uint32)obj.position.y + (uint32)(collide * widthVector.y));
    obj.collision.collisionPoint.z = (sint32)((uint32)obj.position.z + (uint32)(collide * widthVector.z));

    coorddef normal = { -collide * right.x, -collide * right.y, -collide * right.z };
    if (obj.objAltitude < 0x999A && obj.orientationToGround.y > 0xE665 && obj.flightTime == 0)
    {
        sint32 force = (sint32)((uint32)diff << 1);
        if (force < 0)
            force = (sint32)(0U - (uint32)force);
        sint32 impactVel = AttenuateVelocity(carObj, (sint32)((uint32)collide * (uint32)force), &obj.roadMatrix);
        CorrectPostCollisionYaw(carObj, impactVel, normal);
    }
    else
    {
        Collide::TestWithPlane(&obj, &normal, &obj.position);
        if (obj.collision.impulse != 0)
        {
            obj.collision.otherObj = 0;
            obj.collision.sfxType = currentWallType | 0x40000;
        }
    }
    return collide;
}

void Physics::AutoShift(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800A9A50, "NFS4.EXE");

    sint32 redline = carObj->specs->redline;
    sint32 ShiftPoint = redline - redline / 6;
    if ((uint8)carObj->control.gear < 2)
        return;
    if (carObj->RSControl != 0)
        ShiftPoint = redline - redline / 2;
    sint32 sliding = carObj->slide;
    if (sliding < 0)
        sliding = (sint32)(0U - (uint32)sliding);
    sint32 maximumVelocity = fixedmult(carObj->specs->velToRpmRatioInv[carObj->specs->numGears - 2], (sint32)((uint32)redline << 16));
    sint32 SkipLastGear = carObj->specs->maxSpeed < maximumVelocity;
    sint32 previousGear = (uint8)carObj->control.gear;
    sint32 nextGear = previousGear;
    if (nextGear < carObj->specs->numGears - (SkipLastGear + 1))
        ++nextGear;
    if (previousGear >= 3)
        --previousGear;
    sint32 velocity = carObj->N.speedXZ;
    if (carObj->linearVel_ch.z < 0)
        velocity = (sint32)(0U - (uint32)velocity);
    sint32 wheelRpm = fixedmult(velocity, carObj->specs->velToRpmRatio[(uint8)carObj->control.gear]) / 65536;
    (void)fixedmult(velocity, carObj->specs->velToRpmRatio[nextGear]);
    sint32 previousRpm = fixedmult(velocity, carObj->specs->velToRpmRatio[previousGear]) / 65536;
    if (gGasRatio <= 0x8000 && (wheelRpm < 0 ? -wheelRpm : wheelRpm) <= carObj->specs->redline)
    {
        if (previousRpm >= ShiftPoint || previousGear < 2 || sliding > 0x1998 || previousGear == (uint8)carObj->control.gear)
            return;
        carObj->control.downShifting = 1;
        carObj->control.gear = (char)previousGear;
        carObj->control.gearShiftTimer = (char)carObj->specs->gearShiftDelay;
        return;
    }
    if (wheelRpm > ShiftPoint + 500 && (uint8)carObj->control.gear < carObj->specs->numGears - (SkipLastGear + 1))
    {
        if (nextGear != (uint8)carObj->control.gear)
        {
            char gear = carObj->control.gear;
            carObj->control.downShifting = 0;
            carObj->control.gear = (char)nextGear;
            carObj->control.lastGear = gear;
            carObj->control.gearShiftTimer = (char)carObj->specs->gearShiftDelay;
        }
    }
    else if (previousRpm < ShiftPoint && previousGear != (uint8)carObj->control.gear)
    {
        char gear = carObj->control.gear;
        carObj->control.downShifting = 1;
        carObj->control.gear = (char)previousGear;
        carObj->control.lastGear = gear;
        carObj->control.gearShiftTimer = (char)carObj->specs->gearShiftDelay;
    }
}

void Physics::RampCarControlValues(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800A9D0C, "NFS4.EXE");

    if (carObj->RSControl != 0 && (uint8)carObj->control.gear < 2)
    {
        carObj->control.desiredGear = 2;
        carObj->control.gear = 2;
    }
    if (carObj->stats.finishType >= 2)
    {
        carObj->control.steering = 0;
        carObj->control.gasLevel = 0;
        carObj->control.brakeLevel = 255;
        carObj->control.downShifting = 0;
        carObj->control.hanno = 0;
        carObj->N.linearVel.x = (sint32)((uint32)carObj->N.linearVel.x * 254U) / 256;
        carObj->N.linearVel.y = (sint32)((uint32)carObj->N.linearVel.y * 254U) / 256;
        carObj->N.linearVel.z = (sint32)((uint32)carObj->N.linearVel.z * 254U) / 256;
    }
    else
    {
        char inc = carObj->carInfo->RampGas != 0 ? 36 : 48;
        sint32 diff = (uint8)carObj->control.desiredGasLevel - (uint8)carObj->control.gasLevel;
        if (diff > inc)
            diff = inc;
        else if (diff < -inc)
            diff = -inc;
        carObj->control.gasLevel = (char)((uint8)carObj->control.gasLevel + diff);
        if (carObj->carInfo->RampBrake == 0)
            carObj->control.brakeLevel = carObj->control.desiredBrakeLevel;
        else
        {
            diff = (uint8)carObj->control.desiredBrakeLevel - (uint8)carObj->control.brakeLevel;
            if (diff > 16)
                diff = 16;
            else if (diff < -16)
                diff = -16;
            carObj->control.brakeLevel = (char)((uint8)carObj->control.brakeLevel + diff);
        }
        if (carObj->control.gearShiftTimer != 0)
            carObj->control.gearShiftTimer = (char)((uint8)carObj->control.gearShiftTimer - 1);
        if (simGlobal.gameTicks > 512)
        {
            if (GameSetup::gData.carInfo[carObj->carIndex].Transmission == 1)
            {
                if (simGlobal.gameTicks < 520 && ((GameSetup::gData.raceType != 1 && GameSetup::gData.raceType != 5) || ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) == 0 && (Cars::gNumHumanRaceCars != 2 || (Cars::gHumanRaceCarList[1]->carFlags & 0x200) == 0))))
                    carObj->control.desiredGear = 2;
                else if (carObj->control.desiredGear != carObj->control.gear || carObj->stats.finishType != 0)
                    carObj->control.hanno = 0;
                else if (carObj->pullOver == 0 && carObj->blowout == 0)
                {
                    sint32 speed = carObj->N.speedXZ;
                    if (speed < 0)
                        speed = (sint32)(0U - (uint32)speed);
                    if (speed < 0x3333)
                    {
                        if ((uint8)carObj->control.desiredBrakeLevel > 128 && carObj->control.desiredGasLevel == 0 && (uint8)carObj->control.desiredGear >= 2 && carObj->control.hanno == 0)
                        {
                            carObj->control.desiredGear = 0;
                            carObj->control.hanno = 1;
                        }
                        else if ((uint8)carObj->control.desiredGasLevel > 128 && carObj->control.gear == 0 && carObj->control.hanno != 0)
                        {
                            carObj->control.desiredGear = 2;
                            carObj->control.hanno = 0;
                        }
                    }
                }
            }
            if (carObj->control.desiredGear != carObj->control.gear)
            {
                if (GameSetup::gData.carInfo[carObj->carIndex].Transmission == 1 || carObj->RSControl != 0)
                {
                    if ((uint8)carObj->control.desiredGear < 2)
                    {
                        carObj->control.downShifting = 0;
                        carObj->control.lastGear = carObj->control.gear;
                        carObj->control.gear = carObj->control.desiredGear;
                        carObj->control.gearShiftTimer = (char)carObj->specs->gearShiftDelay;
                    }
                    else if (carObj->control.desiredGear == 2 && (uint8)carObj->control.gear < 2)
                    {
                        sint32 gear = 2;
                        carObj->control.lastGear = carObj->control.gear;
                        for (sint32 i = 2; i < carObj->specs->numGears; ++i)
                        {
                            sint32 maximumVelocity = fixedmult(carObj->specs->velToRpmRatioInv[i], (sint32)((uint32)carObj->specs->redline << 16));
                            if (maximumVelocity < carObj->linearVel_ch.z)
                                gear = i;
                        }
                        carObj->control.downShifting = 0;
                        carObj->control.gear = (char)gear;
                        carObj->control.gearShiftTimer = (char)carObj->specs->gearShiftDelay;
                    }
                }
                else
                {
                    carObj->control.downShifting = (uint8)carObj->control.desiredGear < (uint8)carObj->control.gear && (uint8)carObj->control.desiredGear >= 2;
                    carObj->control.lastGear = carObj->control.gear;
                    carObj->control.gear = carObj->control.desiredGear;
                    carObj->control.gearShiftTimer = (char)carObj->specs->gearShiftDelay;
                }
            }
        }
        if (carObj->carInfo->RampSteering == 0)
            carObj->control.steering = carObj->control.desiredSteering;
        else
        {
            sint32 steering = carObj->control.steering;
            diff = (sint32)((uint32)carObj->control.desiredSteering - (uint32)steering);
            sint32 rampIn = carObj->specs->steeringRamp;
            if (diff < 0)
            {
                sint32 magnitude = (sint32)(0U - (uint32)diff);
                carObj->control.steering = (sint32)((uint32)steering - (uint32)(magnitude < rampIn ? magnitude : rampIn));
            }
            else
                carObj->control.steering = (sint32)((uint32)steering + (uint32)(diff < rampIn ? diff : rampIn));
        }
    }
    if (simGlobal.gameTicks < 512 && ((GameSetup::gData.raceType != 1 && GameSetup::gData.raceType != 5) || ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) == 0 && (Cars::gNumHumanRaceCars != 2 || (Cars::gHumanRaceCarList[1]->carFlags & 0x200) == 0))))
        carObj->control.brakeLevel = 255;
    else if (carObj->blowout != 0 || carObj->pullOver != 0)
    {
        if (carObj->control.hanno == 0)
        {
            carObj->control.gasLevel = 0;
            carObj->control.brakeLevel = 128;
        }
        else
        {
            carObj->control.gasLevel = 128;
            carObj->control.brakeLevel = 0;
        }
        carObj->control.downShifting = 0;
    }
    if (carObj->crash == 2)
        carObj->control.gasLevel = 0;
    if (AIInit::forceHumanHandBrake != 0 && carObj->RSControl != 0)
    {
        sint32 direction = (sint32)((uint32)carObj->roadPosition * (uint32)carObj->direction);
        carObj->control.steering = direction > 0 ? -124 : 124;
        carObj->control.handBrake = 1;
    }
    gGasRatio = (((uint8)carObj->control.gasLevel + 1) * 65536) / 248;
    if (gGasRatio > 65536)
        gGasRatio = 65536;
    sint32 brakeRatio = (((uint8)carObj->control.brakeLevel + 1) * 65536) / 248;
    if (brakeRatio > 65536)
        brakeRatio = 65536;
    uint32 steeringMagnitude = (uint32)carObj->control.steering;
    if (carObj->control.steering < 0)
        steeringMagnitude = 0U - steeringMagnitude;
    gSteerRatio = (sint32)(steeringMagnitude << 9);
    gBrakeRatio = brakeRatio;
    if (GameSetup::gData.carInfo[carObj->carIndex].Transmission == 1 && carObj->control.gear == 0 && carObj->control.hanno == 1)
    {
        gGasRatio = (((uint8)carObj->control.brakeLevel + 1) * 65536) / 248;
        if (gGasRatio > 65536)
            gGasRatio = 65536;
        gBrakeRatio = (((uint8)carObj->control.gasLevel + 1) * 65536) / 248;
        if (gBrakeRatio > 65536)
            gBrakeRatio = 65536;
    }
}

void Physics::FixEngineRpm(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800AA4E4, "NFS4.EXE");

    uint32 productX = (uint32)(carObj->N.linearVel.x / 256) * (uint32)(carObj->N.shadowMat.m[0] / 256);
    productX += (uint32)(carObj->N.linearVel.y / 256) * (uint32)(carObj->N.shadowMat.m[1] / 256);
    productX += (uint32)(carObj->N.linearVel.z / 256) * (uint32)(carObj->N.shadowMat.m[2] / 256);
    carObj->linearVel_ch.x = (sint32)productX;
    uint32 productZ = (uint32)(carObj->N.linearVel.x / 256) * (uint32)(carObj->N.shadowMat.m[6] / 256);
    productZ += (uint32)(carObj->N.linearVel.y / 256) * (uint32)(carObj->N.shadowMat.m[7] / 256);
    productZ += (uint32)(carObj->N.linearVel.z / 256) * (uint32)(carObj->N.shadowMat.m[8] / 256);
    carObj->linearVel_ch.z = (sint32)productZ;
    sint32 collided = carObj->N.collision.collided;
    carObj->wheelSpin = 0;
    carObj->slide = 0;
    carObj->frontSkid = 0;
    carObj->rearSkid = 0;
    if (collided == 2)
        carObj->N.collision.collided = 0;
}

void Physics::ResetCar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800AA63C, "NFS4.EXE");

    char gear = carObj->carInfo->Transmission == 1 ? 2 : 1;
    carObj->control.desiredGear = gear;
    carObj->control.gear = gear;
    carObj->linearAcc.x = 0;
    carObj->linearAcc.y = 0;
    carObj->linearAcc.z = 0;
    carObj->angularAcc.x = 0;
    carObj->angularAcc.y = 0;
    carObj->angularAcc.z = 0;
    carObj->linearVel_ch.x = 0;
    carObj->linearVel_ch.y = 0;
    carObj->linearVel_ch.z = 0;
    carObj->angularVel_ch.x = 0;
    carObj->angularVel_ch.y = 0;
    carObj->angularVel_ch.z = 0;
    carObj->wheelSpin = 0;
    carObj->slide = 0;
    carObj->frontSkid = 0;
    carObj->rearSkid = 0;
    carObj->crash = 0;
    carObj->blowout = 0;
    carObj->control.hanno = 0;
    for (sint32 i = 0; i < 4; ++i)
    {
        carObj->wheel[i].wheelInAir = 0;
        carObj->wheel[i].rebound = 0;
    }
}

void Physics::StopCar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800AA6D8, "NFS4.EXE");

    carObj->N.linearVel.x = (sint32)((uint32)carObj->N.linearVel.x * 245U) / 256;
    carObj->N.linearVel.y = (sint32)((uint32)carObj->N.linearVel.y * 245U) / 256;
    carObj->N.linearVel.z = (sint32)((uint32)carObj->N.linearVel.z * 245U) / 256;
    if (carObj->N.orientationToGround.y < 0x3333)
        carObj->N.angularVel.y = (carObj->N.angularVel.y / 256) * 250;
}

void Physics::TestForBarrierCollision(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800AA7B0, "NFS4.EXE");

    sint32 hit = DoBarrierCheck(carObj);
    if (hit != 0)
    {
        if ((carObj->carFlags & 4) != 0)
            FixEngineRpm(carObj);
        carObj->crash = hit;
    }
}

void Physics::CalculateRoadGripModifiers(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800AA808, "NFS4.EXE");

    sint32 frontWheels = 0;
    sint32 rearWheels = 0;
    sint32 leftWheels = 0;
    sint32 rightWheels = 0;
    for (sint32 i = 0; i < 4; ++i)
    {
        uint32 roadSurfaceType = carObj->wheel[i].roadSurfaceType & 15;
        uint32 tempSurface = (uint8)roadSurfaceIndex[carObj->carInfo->TireType][roadSurfaceType];
        if (slippery != 0)
            ++tempSurface;
        sint32 friction = roadSurfaceFrictionCoeff[tempSurface];
        if (i < 2)
            frontWheels = (sint32)((uint32)frontWheels + (uint32)friction);
        else
            rearWheels = (sint32)((uint32)rearWheels + (uint32)friction);
        if (i == 0 || i == 2)
            leftWheels = (sint32)((uint32)leftWheels + (uint32)friction);
        else
            rightWheels = (sint32)((uint32)rightWheels + (uint32)friction);
    }
    leftMult = leftWheels >> 1;
    frontMult = frontWheels >> 1;
    rightMult = rightWheels >> 1;
    rearMult = rearWheels >> 1;
    roadMult = (sint32)((uint32)((sint32)((uint32)frontMult + (uint32)rearMult) >> 1) + (uint32)carObj->N.roadGravityModifier);
    if (carObj->linearVel_ch.z > 0x50000)
    {
        sint32 downForce = fixedmult(carObj->linearVel_ch.z, carObj->specs->frontAeroDownForce);
        frontMult = (sint32)((uint32)frontMult + (uint32)downForce);
        downForce = fixedmult(carObj->linearVel_ch.z, carObj->specs->rearAeroDownForce);
        rearMult = (sint32)((uint32)rearMult + (uint32)downForce);
    }
}

sint32 Physics::CalculateCarAcceleration(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800AA984, "NFS4.EXE");
    const sint32 blip[8] = { 0, 0, 250, 200, 175, 150, 125, 0 };
    const sint32 bblip[8] = { 0, 0, 200, 175, 150, 125, 100, 0 };
    sint32 smokeRpm = carObj->specs->redline / 8;
    sint32 driveAcc = 0;
    uint32 product = fastRandom * randSeed;
    sint32 damageSum = (sint32)((uint32)carObj->N.damage[1] + (uint32)carObj->N.damage[5]);
    randtemp = product;
    fastRandom = product & 0xFFFF;
    bool damage = ((product >> 8) & 0xFFFF) < (uint32)(damageSum / 256);
    Car::tSpecs *specs = carObj->specs;
    exceedRedline = (sint32)((uint32)specs->redline + 500) < carObj->flywheelRpm;
    sint32 aero = 0x10000;
    if (carObj->carInfo->GroundEffects != 0)
    {
        sint32 downForce = (sint32)((uint32)carObj->specs->frontAeroDownForce + (uint32)carObj->specs->rearAeroDownForce);
        aero = (sint32)((uint32)(downForce / 2) + 0x10000);
    }
    sint32 speed = fixedmult(carObj->linearVel_ch.z, aero) / 65536;
    sint32 cube = (sint32)((uint32)speed * (uint32)speed * (uint32)speed);
    sint32 drag = fixedmult(specs->dragCoeff, cube);
    sint32 rpmAllowance = carObj->control.gear == 1 || powerControl == 0 ? 250 : 100;
    sint32 desiredRpm = (sint32)((uint32)specs->redline + (uint32)rpmAllowance);
    sint32 gasRpm = fixedmult(desiredRpm, gGasRatio);
    if (gasRpm < desiredRpm)
        desiredRpm = gasRpm;
    if (specs->redline <= carObj->flywheelRpm)
    {
        uint8 gear = carObj->control.gear;
        carObj->flywheelRpm = (sint32)((uint32)specs->redline + 50);
        carObj->revLimit = gear == 1 || powerControl == 0 ? 3 : 4;
    }
    if (carObj->revLimit > 0)
    {
        sint32 limit = (sint32)((uint32)specs->redline - (carObj->control.gear == 1 || powerControl == 0 ? 800 : 400));
        if (desiredRpm > limit)
            desiredRpm = limit;
        --carObj->revLimit;
    }
    if (carObj->control.gear == 1 || carObj->control.gearShiftTimer != 0 || powerControl == 0)
    {
        if (damage)
        {
            sint32 rpm = (sint32)((uint32)carObj->flywheelRpm - 100);
            carObj->flywheelRpm = rpm;
            carObj->flywheelRpm = rpm < 0 ? 0 : rpm;
        }
        else if (carObj->flywheelRpm < desiredRpm && carObj->control.gearShiftTimer == 0)
        {
            sint32 rpm = (sint32)((uint32)carObj->flywheelRpm + 250);
            carObj->flywheelRpm = rpm;
            carObj->flywheelRpm = rpm <= desiredRpm ? rpm : desiredRpm;
        }
        else if (carObj->control.gearShiftTimer != 0 && carObj->control.lastGear != 1)
        {
            if (carObj->control.downShifting != 0)
            {
                sint32 gear = (uint8)carObj->control.desiredGear;
                sint32 rpmRise = (uint8)carObj->control.brakeLevel < 65 ? blip[gear] : bblip[gear];
                carObj->flywheelRpm = (sint32)((uint32)carObj->flywheelRpm + (uint32)rpmRise);
                if (carObj->flywheelRpm > specs->redline)
                    carObj->flywheelRpm = specs->redline;
            }
            else
            {
                sint32 rpmDrop = (uint8)carObj->control.gear < 4 ? 100 : 200;
                sint32 rpm = (sint32)((uint32)carObj->flywheelRpm - (uint32)rpmDrop);
                carObj->flywheelRpm = rpm;
                carObj->flywheelRpm = rpm < 0 ? 0 : rpm;
            }
        }
        else if (carObj->flywheelRpm >= desiredRpm)
        {
            sint32 rpm = (sint32)((uint32)carObj->flywheelRpm - 200);
            carObj->flywheelRpm = rpm;
            carObj->flywheelRpm = desiredRpm < rpm ? rpm : desiredRpm;
        }
        carObj->frontWheelSpin = 0;
        carObj->wheelSpin = 0;
    }
    else
    {
        if (GameSetup::gData.carInfo[carObj->carIndex].Transmission == 1 || carObj->RSControl != 0)
            AutoShift(carObj);
        sint32 gear = carObj->control.gearShiftTimer != 0 && carObj->control.downShifting == 0
                 ? (uint8)carObj->control.lastGear : (uint8)carObj->control.gear;
        sint32 wheelRpm = fixedmult(carObj->linearVel_ch.z, specs->velToRpmRatio[gear]) / 65536;
        if (exceedRedline == 0 && carObj->revLimit < 1)
        {
            sint32 torque = GetTorque(carObj, carObj->flywheelRpm / 256);
            driveAcc = fixedmult(torque, specs->gearAccCoeff[(uint8)carObj->control.gear]);
        }
        else
        {
            sint32 torque = specs->torqueCurve[specs->redline / 256];
            driveAcc = (sint32)((uint32)fixedmult(torque, specs->gearAccCoeff[(uint8)carObj->control.gear]) << 1);
        }
        sint32 diffDesiredRpm = (sint32)((uint32)desiredRpm - (uint32)wheelRpm);
        sint32 magnitude = diffDesiredRpm < 0 ? (sint32)(0 - (uint32)diffDesiredRpm) : diffDesiredRpm;
        if (magnitude < 125 && desiredRpm < (sint32)((uint32)specs->redline - 300))
            diffDesiredRpm = 0;
        sint32 diffFlywheelRpm = (sint32)((uint32)carObj->flywheelRpm - (uint32)wheelRpm);
        bool spinning = diffFlywheelRpm >= 251 && carObj->control.gearShiftTimer == 0
                        && (uint8)carObj->control.gear < 5;
        if (!spinning)
            spinning = (uint8)carObj->control.gear >= 2 && carObj->linearVel_ch.z <= -0x199A && gGasRatio > 0x8000;
        if (!spinning)
            spinning = carObj->control.gear == 0 && carObj->linearVel_ch.z >= 0x199A && gGasRatio > 0x8000;
        if (!spinning)
            spinning = carObj->wheelSpin == 1;
        spinning = spinning && carObj->revLimit == 0;
        if (spinning)
        {
            sint32 rpmDrop = 0;
            if (desiredRpm < 2000 || (uint8)carObj->control.desiredGasLevel < 64 || damage
                || (carObj->carInfo->carType == 0x13 && (uint8)carObj->control.gear > 2))
                rpmDrop = 200;
            else if (carObj->control.gear == 2 || carObj->control.gear == 0)
                rpmDrop = 10;
            else if ((uint8)carObj->control.gear > 2)
                rpmDrop = 50;
            if (smokeRpm < diffFlywheelRpm)
                carObj->wheelSpin = 2;
            if (diffFlywheelRpm <= rpmDrop)
                rpmDrop = diffFlywheelRpm;
            carObj->flywheelRpm = (sint32)((uint32)carObj->flywheelRpm - (uint32)rpmDrop);
        }
        else if (diffDesiredRpm < 0)
        {
            driveAcc = (sint32)(0 - (uint32)fixedmult(driveAcc, specs->gasOffFactor));
            sint32 slope = gravity_ch.z;
            sint32 currentGear = (uint8)carObj->control.gear;
            if ((slope > 0 && currentGear == 2 && driveAcc < 0)
                || (slope < 0 && currentGear == 0 && driveAcc > 0))
                driveAcc /= 2;
            sint32 ratio = (sint32)((uint32)specs->velToRpmRatioInv[(uint8)carObj->control.gear] << 3);
            sint32 rpmDrop = fixedmult(ratio, 0x28000000) / 65536;
            sint32 negativeDiff = (sint32)(0 - (uint32)diffFlywheelRpm);
            if (carObj->control.gear == 0)
            {
                if (rpmDrop < negativeDiff) rpmDrop = negativeDiff;
            }
            else if (negativeDiff <= rpmDrop)
                rpmDrop = negativeDiff;
            carObj->flywheelRpm = (sint32)((uint32)carObj->flywheelRpm + (uint32)rpmDrop);
            if (exceedRedline == 0 && desiredRpm > carObj->flywheelRpm)
                carObj->flywheelRpm = desiredRpm;
        }
        else if (diffDesiredRpm == 0)
        {
            carObj->flywheelRpm = wheelRpm;
            driveAcc = drag;
        }
        else
        {
            if (damage)
            {
                driveAcc = 0;
                carObj->flywheelRpm = (sint32)((uint32)carObj->flywheelRpm - 100);
            }
            else
            {
                if (diffFlywheelRpm >= 201)
                    carObj->flywheelRpm = (sint32)((uint32)carObj->flywheelRpm - 200);
                else if (diffFlywheelRpm < -200)
                    carObj->flywheelRpm = (sint32)((uint32)carObj->flywheelRpm + 200);
                else
                    carObj->flywheelRpm = wheelRpm;
                driveAcc = fixedmult(driveAcc, gGasRatio);
            }
            if (carObj->flywheelRpm > desiredRpm)
                carObj->flywheelRpm = desiredRpm;
            sint32 sliding = carObj->slide;
            if (sliding < 0) sliding = (sint32)(0 - (uint32)sliding);
            sint32 ratio = (sint32)((uint32)sliding + 0x10000);
            sint32 limit = (GameSetup::gData.sgge & 8) != 0 ? 0x30000 : 0x20000;
            if (ratio > limit) ratio = limit;
            driveAcc = (sint32)((uint32)(driveAcc / 256) * (uint32)(ratio / 256));
        }
        if (carObj->flywheelRpm < 0)
        {
            sint32 limit = (sint32)(0 - ((uint32)carObj->linearVel_ch.z << 5));
            sint32 difference = (sint32)((uint32)driveAcc - (uint32)limit);
            if (!((driveAcc > 0 && limit > 0 && difference > 0)
                  || (driveAcc < 0 && limit < 0 && difference < 0)))
            {
                driveAcc = limit;
                carObj->flywheelRpm = 0;
            }
        }
    }
    if ((uint32)carObj->carInfo->carType - 12 < 4 && slippery != 0
        && carObj->control.gear == 2 && driveAcc > 0)
        driveAcc = (sint32)(3 * (uint32)driveAcc) >> 2;
    return (sint32)((uint32)driveAcc - (uint32)drag);
}

void Physics::CalcWheelLockAcc(Car::tObj *carObj, Physics::tWheelAccStruct *wheel)
{
    FUNCTION_MARKER(0x800AB49C, "NFS4.EXE");
    sint32 roadGrip;
    if (wheel->frontTire != 0)
    {
        sint32 skid = carObj->frontSkid;
        roadGrip = wheel->roadGrip / 256;
        wheel->skid = skid;
        roadGrip = (sint32)((uint32)roadGrip * (uint32)(frontMult / 256));
    }
    else
    {
        sint32 skid = carObj->rearSkid;
        roadGrip = wheel->roadGrip / 256;
        wheel->skid = skid;
        roadGrip = (sint32)((uint32)roadGrip * (uint32)(rearMult / 256));
    }
    sint32 vx = wheel->velCap.x;
    sint32 vz = wheel->velCap.z;
    if (vx < 0) vx = (sint32)(0 - (uint32)vx);
    if (vz < 0) vz = (sint32)(0 - (uint32)vz);
    sint32 totalAcc = vz < vx ? (sint32)((uint32)vx + (uint32)(vz >> 2))
                          : (sint32)((uint32)vz + (uint32)(vx >> 2));
    sint32 divisor = (slippery != 0 ? gripLossTableWet : gripLossTable)[carObj->carInfo->TireType];
    if (divisor == 0 || (divisor == -1 && roadGrip == (-2147483647 - 1)))
        abort();
    sint32 optVar1 = (sint32)((uint32)roadGrip - (uint32)(roadGrip / divisor));
    if (roadGrip < totalAcc)
        wheel->skid = (sint32)(3 * (uint32)wheel->skid + (uint32)totalAcc - (uint32)roadGrip) >> 2;
    else
        wheel->skid = 0;
    sint32 skidLimit = carObj->carInfo->TireType == 2 ? 0x80000 : 0xA0000;
    if (wheel->skid > skidLimit)
        wheel->skid = skidLimit;
    sint32 magnitude = totalAcc < 0 ? (sint32)(0 - (uint32)totalAcc) : totalAcc;
    sint32 optVar2 = vx;
    if (magnitude > 256)
        optVar2 = fixeddiv(optVar1, totalAcc) >> 8;
    sint32 accX = (sint32)((uint32)wheel->velCap.x * (uint32)optVar2) >> 8;
    sint32 accZ = (sint32)((uint32)wheel->velCap.z * (uint32)optVar2) >> 8;
    wheel->finalAcc.x = accX;
    wheel->finalAcc.z = accZ;
}

void Physics::CalcTractionCircleAcc(Car::tObj *carObj, Physics::tWheelAccStruct *wheel)
{
    FUNCTION_MARKER(0x800AB698, "NFS4.EXE");
    sint32 ax = wheel->finalAcc.x;
    sint32 az = wheel->finalAcc.z;
    if (ax < 0) ax = (sint32)(0 - (uint32)ax);
    if (az < 0) az = (sint32)(0 - (uint32)az);
    sint32 totalAcc = az < ax ? (sint32)((uint32)ax + (uint32)(az >> 2))
                          : (sint32)((uint32)az + (uint32)(ax >> 2));
    sint32 roadGrip = wheel->roadGrip;
    if (wheel->frontTire != 0)
        wheel->skid = carObj->frontSkid;
    else
    {
        sint32 forwardAcc = wheel->finalAcc.z;
        sint32 skid = carObj->rearSkid;
        if (forwardAcc < 0) forwardAcc = (sint32)(0 - (uint32)forwardAcc);
        wheel->skid = skid;
        if ((roadGrip < forwardAcc && (uint8)carObj->control.gasLevel > 128)
            || carObj->wheelSpin == 2)
            carObj->wheelSpin = 1;
        else
            carObj->wheelSpin = 0;
    }
    sint32 gripLossDivider = (slippery != 0 ? gripLossTableWet : gripLossTable)[carObj->carInfo->TireType];
    if (roadGrip < totalAcc)
    {
        sint32 excess = (sint32)((uint32)totalAcc - (uint32)roadGrip);
        bool lowSlideRear = false;
        if (carObj->carInfo->Traction != 0 && wheel->frontTire == 0)
        {
            sint32 slide = carObj->slide;
            if (slide < 0) slide = (sint32)(0 - (uint32)slide);
            lowSlideRear = slide <= 0x2665;
        }
        sint32 ratio;
        if (lowSlideRear)
        {
            ratio = fixeddiv(roadGrip, totalAcc);
            wheel->skid = 0;
            if ((uint8)carObj->control.gear > 2)
                wheel->finalAcc.x = fixedmult(wheel->finalAcc.x, ratio);
        }
        else
        {
            if (gripLossDivider == 0 || (gripLossDivider == -1 && excess == (-2147483647 - 1)))
                abort();
            sint32 gripLoss = excess / gripLossDivider;
            if (gripLossDivider == -1 && roadGrip == (-2147483647 - 1))
                abort();
            sint32 maxGripLoss = roadGrip / gripLossDivider;
            if (gripLoss >= maxGripLoss)
                gripLoss = maxGripLoss;
            ratio = fixeddiv((sint32)((uint32)roadGrip - (uint32)gripLoss), totalAcc);
            if (carObj->carInfo->TireType == 2)
                wheel->skid = (sint32)(15 * (uint32)wheel->skid + (uint32)excess) / 16;
            else
                wheel->skid = (sint32)(3 * (uint32)wheel->skid + (uint32)excess) / 4;
            sint32 accX = fixedmult(wheel->finalAcc.x, ratio);
            wheel->finalAcc.x = accX;
        }
        wheel->finalAcc.z = fixedmult(wheel->finalAcc.z, ratio);
    }
    else
        wheel->skid = 0;
    if (wheel->frontTire == 0 && carObj->wheelSpin == 1 && carObj->carInfo->Traction == 0)
    {
        wheel->skid = totalAcc;
        if ((uint8)carObj->control.gear < 4)
            wheel->skid = (sint32)((uint32)totalAcc << 2);
    }
    sint32 tireType = carObj->carInfo->TireType;
    if (tireType == 1 || tireType == 2)
    {
        sint32 limit = tireType == 1 ? 0x80000 : 0x40000;
        if (wheel->skid > limit)
            wheel->skid = limit;
    }
    if (carObj->carInfo->Traction != 0)
        wheel->skid = (sint32)(3 * (uint32)wheel->skid) / 4;
}

void Physics::CalculateTireForces(Car::tObj *carObj, Physics::tWheelAccStruct *wheel)
{
    FUNCTION_MARKER(0x800ABA3C, "NFS4.EXE");
    sint32 roadGrip = wheel->roadGrip;
    wheel->skid = 0;
    if (wheel->steeringAngle != 0)
        Math::ResolveRotatedVector(wheel->velCap.x, wheel->velCap.z, wheel->steeringAngle,
                                  &wheel->velCap.x, &wheel->velCap.z);
    bool brakingSituation = false;
    sint32 acc = wheel->acc;
    if (acc < 0 && wheel->velCap.z < 0)
    {
        if (gGasRatio <= 0x4000 || carObj->control.gear != 0)
        {
            sint32 velocity = wheel->velCap.z;
            wheel->acc = velocity < acc ? acc : velocity;
            brakingSituation = true;
        }
    }
    else if (acc > 0 && wheel->velCap.z > 0)
    {
        if (gGasRatio <= 0x4000 || (uint8)carObj->control.gear < 2)
        {
            sint32 velocity = wheel->velCap.z;
            wheel->acc = velocity < acc ? velocity : acc;
            brakingSituation = true;
        }
    }
    acc = fixedmult(wheel->acc, carObj->specs->lateralGripMult);
    wheel->acc = acc;
    bool brakingForce = false;
    if (brakingSituation)
    {
        sint32 magnitude = acc < 0 ? (sint32)(0 - (uint32)acc) : acc;
        brakingForce = wheel->roadGrip < magnitude;
    }
    if (!brakingForce && carObj->control.handBrake != 0 && wheel->frontTire == 0)
    {
        sint32 speed = carObj->linearVel_ch.z;
        if (speed < 0) speed = (sint32)(0 - (uint32)speed);
        brakingForce = speed > 0x8000;
    }
    if (brakingForce)
    {
        bool locked = carObj->control.handBrake != 0;
        if (!locked && carObj->carInfo->ABS == 0)
        {
            sint32 speed = carObj->linearVel_ch.z;
            if (speed <= 0x190000 && (uint8)carObj->control.brakeLevel > 235)
            {
                if (speed < 0) speed = (sint32)(0 - (uint32)speed);
                locked = speed > 0x4FFFF || carObj->wheelSpin != 0;
            }
        }
        if (locked)
        {
            sint32 increment = wheel->frontTire != 0 ? 1 : 2;
            carObj->wheelLock = (sint32)((uint32)carObj->wheelLock + (uint32)increment);
            CalcWheelLockAcc(carObj, wheel);
            if (wheel->frontTire != 0)
                carObj->frontSkid = wheel->skid;
            else
                carObj->rearSkid = wheel->skid;
            return;
        }
        sint32 limit = wheel->roadGrip;
        if (limit < wheel->acc)
            wheel->acc = limit;
        else
        {
            limit = (sint32)(0 - (uint32)limit);
            if (wheel->acc < limit)
                wheel->acc = limit;
        }
        if (carObj->carInfo->ABS != 0 && carObj->linearVel_ch.z < 0x190000)
            wheel->skid = (simGlobal.gameTicks & 3) == 0 ? 0x80000 : 0;
    }
    sint32 slipAngle = 0;
    sint32 velocityZ = wheel->velCap.z;
    if (velocityZ != 0)
    {
        slipAngle = fixedatan(wheel->velCap.x, (sint32)(0 - (uint32)velocityZ) / 2);
        if (wheel->velCap.z > 0)
        {
            sint32 velocityX = wheel->velCap.x;
            if (velocityX > 0)
                slipAngle = (sint32)(0x8000 - (uint32)slipAngle);
            else if (velocityX < 0)
                slipAngle = (sint32)((uint32)-0x8000 - (uint32)slipAngle);
        }
        slipAngle = (sint32)((uint32)slipAngle << 8);
    }
    sint32 angleMagnitude = slipAngle < 0 ? (sint32)(0 - (uint32)slipAngle) : slipAngle;
    sint32 latAcc;
    bool limitLateral;
    sint32 velocityX;
    if (wheel->frontTire != 0)
    {
        if (angleMagnitude > 0x100000)
            angleMagnitude = 0x100000;
        latAcc = fixedmult(fixedmult(angleMagnitude, roadGrip), 0x1555);
        if (slipAngle < 0) latAcc = (sint32)(0 - (uint32)latAcc);
        wheel->finalAcc.x = latAcc;
        sint32 gravity = gravity_ch.x;
        velocityX = wheel->velCap.x;
        wheel->finalAcc.x = (sint32)((uint32)latAcc + (uint32)(gravity / 2));
        sint32 vx = velocityX < 0 ? (sint32)(0 - (uint32)velocityX) : velocityX;
        sint32 vz = wheel->velCap.z;
        if (vz < 0) vz = (sint32)(0 - (uint32)vz);
        limitLateral = (sint32)((uint32)vx + (uint32)vz) < 0x200000;
    }
    else
    {
        if (angleMagnitude < 0x8000) angleMagnitude = 0x8000;
        if (angleMagnitude > 0x20000) angleMagnitude = 0x20000;
        latAcc = fixedmult(angleMagnitude, roadGrip) / 2;
        if (slipAngle < 0) latAcc = (sint32)(0 - (uint32)latAcc);
        sint32 gravity = gravity_ch.x;
        velocityX = wheel->velCap.x;
        latAcc = (sint32)((uint32)latAcc + (uint32)(gravity / 2));
        limitLateral = true;
    }
    if (limitLateral)
    {
        sint32 magnitude = latAcc < 0 ? (sint32)(0 - (uint32)latAcc) : latAcc;
        if (velocityX > 0)
            wheel->finalAcc.x = magnitude < velocityX ? magnitude : velocityX;
        else
        {
            sint32 negative = (sint32)(0 - (uint32)magnitude);
            wheel->finalAcc.x = negative < velocityX ? velocityX : negative;
        }
    }
    acc = wheel->acc;
    wheel->finalAcc.y = 0;
    wheel->finalAcc.z = acc;
    CalcTractionCircleAcc(carObj, wheel);
    sint32 angle = wheel->steeringAngle;
    if (angle != 0)
        Math::ResolveRotatedVector(wheel->finalAcc.x, wheel->finalAcc.z,
                                  (sint32)(0 - (uint32)angle), &wheel->finalAcc.x, &wheel->finalAcc.z);
    if (wheel->frontTire != 0)
        carObj->frontSkid = wheel->skid;
    else
        carObj->rearSkid = wheel->skid;
}

sint32 Physics::CalculateRSControlDesiredPosition(Car::tObj *carObj, sint32 sliceAhead, sint32 lookAhead)
{
    FUNCTION_MARKER(0x800ABFA4, "NFS4.EXE");
    sint32 driveSide = (sint32)((uint32)carObj->RSControl * (uint32)AITune::driveSide);
    bool flip = false;
    if (carObj->roadPosition < 0 && driveSide > 0)
        flip = AIWorld::IsDriveableLane(carObj->N.simRoadInfo.slice,
                    (sint32)((uint32)carObj->laneIndex + 1)) == 0;
    if (!flip && carObj->roadPosition > 0 && driveSide < 0)
        flip = AIWorld::IsDriveableLane(carObj->N.simRoadInfo.slice,
                    (sint32)((uint32)carObj->laneIndex - 1)) == 0;
    if (flip)
        driveSide = (sint32)(0 - (uint32)driveSide);
    sint32 position;
    if (driveSide > 0)
    {
        sint32 desLane = 7;
        for (;; ++desLane)
        {
            sint32 driveable = AIWorld::IsDriveableLaneInSliceRange(
                carObj->N.simRoadInfo.slice, lookAhead, carObj->RSControl, desLane);
            if (driveable != 0 || desLane >= 10)
                break;
        }
        sint32 laneOffset = desLane - 7;
        sint32 width = BWorldSm::slices[sliceAhead].avgPavedWidthRt * 0x8000;
        position = laneOffset * width + (width >> 1);
        if (laneOffset > 0)
            position += 0x18000;
    }
    else
    {
        sint32 desLane = 6;
        for (;; --desLane)
        {
            sint32 driveable = AIWorld::IsDriveableLaneInSliceRange(
                carObj->N.simRoadInfo.slice, lookAhead, carObj->RSControl, desLane);
            if (driveable != 0 || desLane < 4)
                break;
        }
        sint32 laneOffset = 6 - desLane;
        sint32 width = BWorldSm::slices[sliceAhead].avgPavedWidthLf * 0x8000;
        position = laneOffset * width + (width >> 1);
        if (laneOffset > 0)
            position += 0x18000;
        position = -position;
    }
    return position;
}

void Physics::Real(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800AC164, "NFS4.EXE");
    carObj->linearAcc_ch.x = 0;
    carObj->linearAcc_ch.y = 0;
    carObj->linearAcc_ch.z = 0;
    Car::tSpecs *specs = carObj->specs;
    steeringControl = 1;
    powerControl = 1;
    slippery = GameSetup::gData.Weather != 0 && !BWorldSm::TunnelFlagSm(&carObj->N.simRoadInfo);
    if (carObj->wheel[0].wheelInAir != 0 && carObj->wheel[1].wheelInAir != 0 && carObj->N.objAltitude > 0xCCC)
        steeringControl = 0;
    if (carObj->wheel[2].wheelInAir != 0 && carObj->wheel[3].wheelInAir != 0 && carObj->N.objAltitude > 0xCCC)
        powerControl = 0;
    for (sint32 kind = 0; kind < 2; ++kind)
    {
        coorddef *input = kind == 0 ? &carObj->N.linearVel : &carObj->N.angularVel;
        coorddef *output = kind == 0 ? &carObj->linearVel_ch : &carObj->angularVel_ch;
        for (sint32 row = 0; row < 3; ++row)
        {
            sint32 x = fixedmult(input->x, carObj->N.shadowMat.m[row * 3]);
            sint32 y = fixedmult(input->y, carObj->N.shadowMat.m[row * 3 + 1]);
            sint32 z = fixedmult(input->z, carObj->N.shadowMat.m[row * 3 + 2]);
            sint32 value = (sint32)((uint32)x + (uint32)y + (uint32)z);
            if (row == 0) output->x = value;
            else if (row == 1) output->y = value;
            else output->z = value;
        }
    }
    carObj->slide = carObj->linearVel_ch.z > 0x50000
                  ? fixeddiv(carObj->linearVel_ch.x, carObj->linearVel_ch.z) : 0;
    coorddef carAccCap_ch;
    carAccCap_ch.x = (sint32)(0 - ((uint32)carObj->linearVel_ch.x << 5)) / 2;
    carAccCap_ch.y = (sint32)(0 - ((uint32)carObj->linearVel_ch.y << 5)) / 2;
    carAccCap_ch.z = (sint32)(0 - ((uint32)carObj->linearVel_ch.z << 5)) / 2;
    sint32 rotationalAccCap = (sint32)(0 - (uint32)fixedmult(
        (sint32)((uint32)carObj->N.angularVel.y << 5), specs->alphaToAccRotInertia)) / 2;
    gravity_ch.x = fixedmult(-0xA0000, carObj->N.shadowMat.m[1]);
    gravity_ch.y = fixedmult(-0xA0000, carObj->N.shadowMat.m[4]);
    gravity_ch.z = fixedmult(-0xA0000, carObj->N.shadowMat.m[7]);
    tWheelAccStruct frontWheel;
    tWheelAccStruct rearWheel;
    frontWheel.velCap.x = (sint32)((uint32)carAccCap_ch.x + (uint32)rotationalAccCap);
    frontWheel.velCap.y = carAccCap_ch.y;
    frontWheel.velCap.z = carAccCap_ch.z;
    rearWheel.velCap.x = (sint32)((uint32)carAccCap_ch.x - (uint32)rotationalAccCap);
    rearWheel.velCap.y = carAccCap_ch.y;
    rearWheel.velCap.z = carAccCap_ch.z;
    RampCarControlValues(carObj);
    sint32 gravitySteer = (sint32)((uint32)(gravity_ch.x / 256) * 0x300) / 65536;
    sint32 tempSteer = (sint32)((uint32)carObj->control.steering + (uint32)gravitySteer);
    if (tempSteer > 127) tempSteer = 127;
    else if (tempSteer < -127) tempSteer = -127;
    tempSteer = (sint32)((uint32)tempSteer * (uint32)specs->maxSteeringAcc) / 128;
    sint32 damage = (sint32)((uint32)carObj->N.damage[0] + (uint32)carObj->N.damage[1]
                       + (uint32)carObj->N.damage[2] + (uint32)carObj->N.damage[9]);
    sint32 damageMult = (sint32)(0x10000 - (uint32)(damage / 512));
    frontWheel.steeringAngle = (sint32)((uint32)(tempSteer / 256) * (uint32)(damageMult / 256));
    if (carObj->linearVel_ch.z > 0x3C0000)
        frontWheel.steeringAngle = fixeddiv(frontWheel.steeringAngle, fixedmult(carObj->linearVel_ch.z, 0x444));
    sint32 threshold = 0;
    if (carObj->linearVel_ch.z > 0x1AB333) threshold = 0xCCC;
    else if (carObj->linearVel_ch.z > 0xD6666) threshold = 0x2666;
    if (threshold != 0 && ((carObj->slide > threshold && frontWheel.steeringAngle > 0)
        || (carObj->slide < -threshold && frontWheel.steeringAngle < 0)))
    {
        sint32 sliding = carObj->slide;
        if (sliding < 0) sliding = (sint32)(0 - (uint32)sliding);
        sint32 ratio = (sint32)((uint32)sliding << 1);
        if (ratio > 0x10000) ratio = 0x10000;
        frontWheel.steeringAngle = fixedmult(frontWheel.steeringAngle, ratio);
    }
    rearWheel.steeringAngle = 0;
    frontWheel.frontTire = 1;
    rearWheel.frontTire = 0;
    sint32 driveAcc = CalculateCarAcceleration(carObj);
    frontWheel.acc = (sint32)((uint32)(driveAcc / 256) * (uint32)(specs->frontDriveRatio / 256));
    rearWheel.acc = (sint32)((uint32)driveAcc - (uint32)frontWheel.acc);
    if (steeringControl == 0 && powerControl == 0)
    {
        carObj->frontSkid = 0;
        carObj->rearSkid = 0;
        return;
    }
    carObj->crash = 0;
    sint32 brakeAcc = (sint32)((uint32)(gBrakeRatio / 256) * (uint32)(specs->maxBrakeAcc / 256));
    sint32 magnitude = carObj->linearVel_ch.z;
    if (magnitude < 0) magnitude = (sint32)(0 - (uint32)magnitude);
    sint32 cap = (sint32)((uint32)magnitude << 5);
    if (brakeAcc > cap) brakeAcc = cap;
    damage = carObj->N.damage[9];
    if (damage != 0)
    {
        damageMult = (sint32)(0x10000 - (uint32)(damage / 128));
        brakeAcc = (sint32)((uint32)(brakeAcc / 256) * (uint32)(damageMult / 256));
    }
    if (carObj->linearVel_ch.z > 0) brakeAcc = (sint32)(0 - (uint32)brakeAcc);
    sint32 frontBrake = fixedmult(brakeAcc, specs->frontBrakeRatio);
    frontWheel.acc = (sint32)((uint32)frontWheel.acc + (uint32)frontBrake);
    rearWheel.acc = (sint32)((uint32)rearWheel.acc + (uint32)brakeAcc - (uint32)frontBrake);
    sint32 roadGrip = fixedmult((sint32)(0 - (uint32)gravity_ch.y), specs->lateralGripMult);
    CalculateRoadGripModifiers(carObj);
    roadGrip = (sint32)((uint32)(roadGrip / 256) * (uint32)(roadMult / 256));
    if (roadGrip < 0) return;
    sint32 bias = slippery != 0 ? (sint32)((uint32)specs->frontGripBias + 0x28F) : specs->frontGripBias;
    sint32 frontGrip = fixedmult(roadGrip, bias);
    damage = (sint32)((uint32)carObj->N.damage[4] + (uint32)carObj->N.damage[5]
                   + (uint32)carObj->N.damage[6] + (uint32)carObj->N.damage[9]);
    sint32 transferMult = (sint32)((uint32)(damage / 512) + 0xC000);
    sint32 transfer = fixedmult(carObj->gTransferFront, transferMult);
    carObj->gTransferFront = transfer;
    if (transfer < 0)
        carObj->gTransferFront = (sint32)((uint32)(transfer / 256) * (uint32)(transferMult / 256));
    else if (transfer > 0)
        carObj->gTransferFront = (sint32)(3 * (uint32)transfer) / 4;
    sint32 frontLoad = (sint32)((uint32)frontGrip - (uint32)carObj->gTransferFront);
    frontWheel.roadGrip = (sint32)((uint32)(frontMult / 256) * (uint32)(frontLoad / 256));
    sint32 rearLoad = (sint32)((uint32)roadGrip - (uint32)frontGrip + (uint32)carObj->gTransferFront);
    rearWheel.roadGrip = (sint32)((uint32)(rearMult / 256) * (uint32)(rearLoad / 256));
    carObj->wheelLock = 0;
    frontWheel.finalAcc.x = 0;
    frontWheel.finalAcc.z = 0;
    rearWheel.finalAcc.x = 0;
    rearWheel.finalAcc.z = 0;
    if (steeringControl != 0) CalculateTireForces(carObj, &frontWheel);
    if (powerControl != 0) CalculateTireForces(carObj, &rearWheel);
    carObj->linearAcc_ch.z = (sint32)((uint32)frontWheel.finalAcc.z + (uint32)rearWheel.finalAcc.z);
    carObj->linearAcc_ch.x = (sint32)((uint32)frontWheel.finalAcc.x + (uint32)rearWheel.finalAcc.x);
    carObj->linearAcc_ch.z = fixedmult(carObj->linearAcc_ch.z, specs->lateralGripMultInv);
    if (GameSetup::gData.sgge == 0x80 && carObj->linearAcc_ch.z > 0 && carObj->control.horn != 0)
        carObj->linearAcc_ch.z = (sint32)((uint32)carObj->linearAcc_ch.z << 2);
    if (slippery == 0)
    {
        sint32 factor = carObj->carInfo->TireType == 2 && (uint8)carObj->control.gear < 4 ? 0x12666 : 0x11999;
        carObj->linearAcc_ch.z = fixedmult(carObj->linearAcc_ch.z, factor);
    }
    carObj->linearAcc_ch.y = 0;
    sint32 gravityTransfer = fixedmult(gravity_ch.z, 0x1999);
    sint32 Xcomponent = fixedmult(carObj->linearAcc_ch.x / 8, specs->gTransferFactor);
    carObj->gTransferRight = (sint32)(0 - (uint32)Xcomponent);
    carObj->gTransferFront = (sint32)((uint32)fixedmult(carObj->linearAcc_ch.z, specs->gTransferFactor) - (uint32)gravityTransfer);
    coorddef finalAngularAcc_ch;
    sint32 angular = fixedmult((sint32)((uint32)frontWheel.finalAcc.x - (uint32)rearWheel.finalAcc.x), specs->accToAlphaRotInertia);
    sint32 asymmetric = fixedmult((sint32)((uint32)frontWheel.finalAcc.z + (uint32)rearWheel.finalAcc.z),
                               (sint32)((uint32)leftMult - (uint32)rightMult));
    asymmetric = fixedmult(asymmetric, specs->accToAlphaRotInertia);
    angular = (sint32)((uint32)angular + ((uint32)asymmetric << 1));
    if ((carObj->N.angularVel.y > 0 && angular > 0) || (carObj->N.angularVel.y < 0 && angular < 0))
    {
        if (carObj->control.handBrake != 0 && (GameSetup::gData.sgge & 8) == 0)
            angular /= 2;
        else if (((carObj->control.desiredSteering < 0 && carObj->N.angularVel.y > 0x3333)
                || (carObj->control.desiredSteering > 0 && carObj->N.angularVel.y < -0x3333)) && carObj->wheelSpin == 1)
            angular = (sint32)(0 - (uint32)carObj->N.angularVel.y);
        else
            angular = fixedmult(angular, carObj->specs->spinVelCap);
    }
    finalAngularAcc_ch.x = 0;
    finalAngularAcc_ch.y = angular;
    finalAngularAcc_ch.z = 0;
    matrixtdef transposeMat;
    transpose(&carObj->N.shadowMat, &transposeMat);
    Cars::DoGravityEffectsOnAcc(carObj, 0);
    for (sint32 kind = 0; kind < 2; ++kind)
    {
        coorddef *input = kind == 0 ? &carObj->linearAcc_ch : &finalAngularAcc_ch;
        coorddef *output = kind == 0 ? &carObj->linearAcc : &carObj->angularAcc;
        for (sint32 row = 0; row < 3; ++row)
        {
            sint32 x = fixedmult(input->x, transposeMat.m[row * 3]);
            sint32 y = fixedmult(input->y, transposeMat.m[row * 3 + 1]);
            sint32 z = fixedmult(input->z, transposeMat.m[row * 3 + 2]);
            sint32 value = (sint32)((uint32)x + (uint32)y + (uint32)z);
            if (row == 0) output->x = value;
            else if (row == 1) output->y = value;
            else output->z = value;
        }
    }
    carObj->N.linearVel.x = (sint32)((uint32)carObj->N.linearVel.x + (uint32)(carObj->linearAcc.x / 32));
    carObj->N.linearVel.y = (sint32)((uint32)carObj->N.linearVel.y + (uint32)(carObj->linearAcc.y / 32));
    carObj->N.linearVel.z = (sint32)((uint32)carObj->N.linearVel.z + (uint32)(carObj->linearAcc.z / 32));
    carObj->N.angularVel.x = (sint32)((uint32)carObj->N.angularVel.x + (uint32)(carObj->angularAcc.x / 32));
    carObj->N.angularVel.y = (sint32)((uint32)carObj->N.angularVel.y + (uint32)(carObj->angularAcc.y / 64));
    carObj->N.angularVel.z = (sint32)((uint32)carObj->N.angularVel.z + (uint32)(carObj->angularAcc.z / 32));
    if (carObj->RSControl != 0)
    {
        sint32 gear = (uint8)carObj->control.gear;
        if (gear < 2) gear = 2;
        sint32 desiredSpeed = carObj->desiredSpeed <= 0x471C6 ? 0x188000 : carObj->desiredSpeed;
        sint32 desiredRpm = fixedmult(desiredSpeed, specs->velToRpmRatio[gear]) / 65536;
        gear = (uint8)carObj->control.gear;
        if (gear < 2) gear = 2;
        sint32 currentRpm = fixedmult(carObj->linearVel_ch.z, specs->velToRpmRatio[gear]) / 65536;
        sint32 numerator = (sint32)((uint32)desiredRpm << 8);
        sint32 redline = specs->redline;
        if (redline == 0 || (redline == -1 && numerator == (-2147483647 - 1))) abort();
        sint32 tempGas = numerator / redline;
        sint32 diffRpm = (sint32)((uint32)desiredRpm - (uint32)currentRpm);
        if (diffRpm != 200)
        {
            numerator = (sint32)((uint32)diffRpm << 7);
            if (desiredRpm == 0 || (desiredRpm == -1 && numerator == (-2147483647 - 1))) abort();
            tempGas = (sint32)((uint32)tempGas + (uint32)(numerator / desiredRpm));
            if (diffRpm > 200)
            {
                if (tempGas > 224) tempGas = 224;
                carObj->RSGasLevel = (char)tempGas;
            }
            else
            {
                carObj->RSGasLevel = (char)(tempGas < 0 ? 0 : tempGas);
                if (diffRpm < 0)
                {
                    numerator = (sint32)((uint32)diffRpm << 9);
                    if (numerator < 0) numerator = (sint32)(0 - (uint32)numerator);
                    redline = specs->redline;
                    if (redline == 0 || (redline == -1 && numerator == (-2147483647 - 1))) abort();
                    sint32 braking = numerator / redline;
                    if (braking > 255) braking = 255;
                    carObj->RSBrakeLevel = (char)braking;
                    if ((uint8)carObj->RSBrakeLevel > 128) carObj->RSGasLevel = 0;
                }
            }
        }
        sint32 speed = carObj->currentSpeed;
        if (speed < 0) speed = (sint32)(0 - (uint32)speed);
        sint32 factor = speed / 0x60000;
        if (factor < 3) factor = 3;
        sint32 lookAhead = (sint32)((uint32)carObj->RSControl * (uint32)factor);
        sint32 sliceAhead = (sint32)((uint32)carObj->N.simRoadInfo.slice + (uint32)lookAhead);
        if (lookAhead < 0)
        {
            if (sliceAhead < 0) sliceAhead = (sint32)((uint32)sliceAhead + (uint32)gNumSlices);
        }
        else if (sliceAhead >= gNumSlices)
            sliceAhead = (sint32)((uint32)sliceAhead - (uint32)gNumSlices);
        sint32 sliceRange = (sint32)(3 * (uint32)lookAhead);
        if (sliceRange < 0) sliceRange = (sint32)(0 - (uint32)sliceRange);
        coorddef carPos = carObj->N.position;
        coorddef dirVector;
        dirVector.x = BWorldSm::slices[sliceAhead].center[0];
        dirVector.y = BWorldSm::slices[sliceAhead].center[1];
        dirVector.z = BWorldSm::slices[sliceAhead].center[2];
        sint32 roadPosition = CalculateRSControlDesiredPosition(carObj, sliceAhead, sliceRange);
        coorddef offset;
        offset.x = fixedmult((sint8)BWorldSm::slices[sliceAhead].right[0] * 512, roadPosition);
        offset.y = fixedmult((sint8)BWorldSm::slices[sliceAhead].right[1] * 512, roadPosition);
        offset.z = fixedmult((sint8)BWorldSm::slices[sliceAhead].right[2] * 512, roadPosition);
        dirVector.x = (sint32)((uint32)dirVector.x + (uint32)offset.x - (uint32)carPos.x);
        dirVector.y = (sint32)((uint32)dirVector.y + (uint32)offset.y - (uint32)carPos.y);
        dirVector.z = (sint32)((uint32)dirVector.z + (uint32)offset.z - (uint32)carPos.z);
        Math::NormalizeVector(&dirVector);
        sint32 x = fixedmult(dirVector.x, carObj->N.orientMat.m[0]);
        sint32 y = fixedmult(dirVector.y, carObj->N.orientMat.m[1]);
        sint32 z = fixedmult(dirVector.z, carObj->N.orientMat.m[2]);
        carObj->RSSteering = (sint32)((uint32)x + (uint32)y + (uint32)z) / 256;
        if (carObj->RSSteering > 127) carObj->RSSteering = 127;
        else if (carObj->RSSteering < -127) carObj->RSSteering = -127;
    }
    if ((GameSetup::gData.sgge & 8) == 0)
    {
        sint32 slide = carObj->slide;
        if (slide < 0) slide = (sint32)(0 - (uint32)slide);
        if (carObj->control.gasLevel == 0 || slide >= 0x199A)
        {
            carObj->N.linearVel.x = fixedmult(carObj->specs->dragFactor, carObj->N.linearVel.x);
            carObj->N.linearVel.y = fixedmult(carObj->specs->dragFactor, carObj->N.linearVel.y);
            carObj->N.linearVel.z = fixedmult(carObj->specs->dragFactor, carObj->N.linearVel.z);
        }
    }
    if (carObj->N.driveSurfaceType == 3 && carObj->N.speedXZ > 0x2CA3D7)
    {
        carObj->N.linearVel.x = fixedmult(0xFEB8, carObj->N.linearVel.x);
        carObj->N.linearVel.y = fixedmult(0xFEB8, carObj->N.linearVel.y);
        carObj->N.linearVel.z = fixedmult(0xFEB8, carObj->N.linearVel.z);
        return;
    }
    if (carObj->control.gear == 1)
    {
        sint32 slope = gravity_ch.z;
        if (slope < 0) slope = (sint32)(0 - (uint32)slope);
        if (slope < 0x8000)
        {
            sint32 speed = carObj->linearVel_ch.z;
            if (speed < 0) speed = (sint32)(0 - (uint32)speed);
            sint32 damp = 0xFD70;
            if (speed > 0x13FFFF)
            {
                sint32 steering = carObj->control.steering;
                if (steering < 0) steering = (sint32)(0 - (uint32)steering);
                damp = steering > 32 ? 0xFD70 : 0xFF7C;
            }
            carObj->N.linearVel.x = fixedmult(damp, carObj->N.linearVel.x);
            carObj->N.linearVel.y = fixedmult(damp, carObj->N.linearVel.y);
            carObj->N.linearVel.z = fixedmult(damp, carObj->N.linearVel.z);
            carObj->N.angularVel.x = fixedmult(damp, carObj->N.angularVel.x);
            carObj->N.angularVel.y = fixedmult(damp, carObj->N.angularVel.y);
            carObj->N.angularVel.z = fixedmult(damp, carObj->N.angularVel.z);
            return;
        }
    }
    if (carObj->control.handBrake != 0 && carObj->N.speedXZ <= 0xFFFF)
    {
        carObj->N.linearVel.x = fixedmult(0x8000, carObj->N.linearVel.x);
        carObj->N.linearVel.y = fixedmult(0x8000, carObj->N.linearVel.y);
        carObj->N.linearVel.z = fixedmult(0x8000, carObj->N.linearVel.z);
    }
}

void Physics::SimCar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800AD544, "NFS4.EXE");
    if (carObj->N.orientationToGround.y < 0x1999)
    {
        sint32 flywheelRpm = carObj->flywheelRpm;
        carObj->wheelSpin = 0;
        carObj->slide = 0;
        carObj->frontSkid = 0;
        carObj->rearSkid = 0;
        carObj->flywheelRpm = flywheelRpm < 501 ? 0 : flywheelRpm - 500;
        if (carObj->N.objAltitude <= 0x7FFF)
            StopCar(carObj);
    }
    else
        Real(carObj);
}
