#ifndef PHYSICS_H
#define PHYSICS_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"
#include "game/common/udff.h"

class Physics
{
public:
    struct tWheelAccStruct;

    static void InitCarSpecs(Car::tObj *carObj, Udff::tInfo *handle);
    static void CalculateDerivedCarSpecs(Car::tObj *carObj);
    static void CheckGamedata(void);
    static sint32 AttenuateVelocity(Car::tObj *carObj, sint32 force, matrixtdef *roadMat);
    static void SetCurrentWallType(sint32 wallType);
    static sint32 GetTorque(Car::tObj *carObj, sint32 index);
    static void CorrectPostCollisionYaw(Car::tObj *carObj, sint32 impactVel, coorddef barrierVec);
    static sint32 DoBarrierCheck(Car::tObj *carObj);
    static void AutoShift(Car::tObj *carObj);
    static void RampCarControlValues(Car::tObj *carObj);
    static void FixEngineRpm(Car::tObj *carObj);
    static void ResetCar(Car::tObj *carObj);
    static void StopCar(Car::tObj *carObj);
    static void TestForBarrierCollision(Car::tObj *carObj);
    static void CalculateRoadGripModifiers(Car::tObj *carObj);
    static sint32 CalculateCarAcceleration(Car::tObj *carObj);
    static void CalcWheelLockAcc(Car::tObj *carObj, tWheelAccStruct *wheel);
    static void CalcTractionCircleAcc(Car::tObj *carObj, tWheelAccStruct *wheel);
    static void CalculateTireForces(Car::tObj *carObj, tWheelAccStruct *wheel);
    static sint32 CalculateRSControlDesiredPosition(Car::tObj *carObj, sint32 sliceAhead, sint32 lookAhead);
    static void Real(Car::tObj *carObj);
    static void SimCar(Car::tObj *carObj);
};

extern sint32 gGasRatio;
extern sint32 gBrakeRatio;
extern sint32 gSteerRatio;
extern sint32 roadMult;
extern sint32 frontMult;
extern sint32 rearMult;
extern sint32 leftMult;
extern sint32 rightMult;
extern sint32 slippery;
extern sint32 roadSurfaceFrictionCoeff[10];
extern char roadSurfaceIndex[3][20];

#endif
