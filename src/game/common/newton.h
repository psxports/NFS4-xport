#ifndef NEWTON_H
#define NEWTON_H

#include "xport.h"
#include "game/common/bo.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/matrixtdef.h"

class Newton
{
public:
    static void AddDamageZone(BO::tNewtonObj *newtonObj, sint32 impulse, sint32 zone, sint32 type);
    static void FindClosestQuad(BO::tNewtonObj *n);
    static sint32 CalculateSliceYaw(sint32 slice);
    static void UpdateRoadGeometry(BO::tNewtonObj *n);
    static sint32 FindGroundElevationGeneral(coorddef *point, coorddef *normal, coorddef *pointOnQuad);
    static sint32 FindGroundElevationRough(coorddef *point, coorddef *normal, coorddef *pointOnQuad);
    static sint32 FindGroundElevationAndNormalFast(BO::tNewtonObj *newtonObj, coorddef *normal);
    static sint32 FindGroundElevationAndNormal(BO::tNewtonObj *newtonObj, coorddef *normal);
    static void LimitCarsToDrivableDist(BO::tNewtonObj *newtonObj);
    static sint32 CalcPerpenHeightOfLowestPointFromGround(BO::tNewtonObj *newtonObj, coorddef *normal, coorddef *samplePoint);
    static sint32 CalcPerpenHeightOfCenterPointFromGround(BO::tNewtonObj *newtonObj, coorddef *normal, coorddef *samplePoint);
    static void CalcDistToClosestPlayerCar(BO::tNewtonObj *n);
    static void UpdateRoadInfo(BO::tNewtonObj *n);
    static void CopyRoadMatrixToOrientMat(BO::tNewtonObj *n, sint32 backwards);
    static void CopyRoadMatrixToShadowMat(BO::tNewtonObj *n, sint32 backwards);
    static void SetInitialSlicePositionOrientationEtc(BO::tNewtonObj *n, sint32 slice, coorddef *offset, sint32 direction);
    static void InitBaseNewtonObj(BO::tNewtonObj *newtonObj, sint32 index, sint32 mass, sint32 moInertia, sint32 dimX, sint32 dimY, sint32 dimZ);
    static void QDUpdateVel(BO::tNewtonObj *newtonObj);
    static sint32 OptzRotxform(matrixtdef *m, sint32 ax, sint32 ay, sint32 az, sint32 *reOrthoNeeded, sint32 reOrthoLimit, sint32 *cumulatedRot);
    static void QDUpdateRot64Hz(BO::tNewtonObj *newtonObj);
    static void QDUpdateRot32Hz(BO::tNewtonObj *newtonObj);
    static void CalculateGroundShadowMatrix(BO::tNewtonObj *newtonObj, coorddef *normal, sint32 orientToGround);
    static void CalcRealShadowCoordinates(Car::tObj *carObj, sint32 currentTick);
    static void CheckForSpikeBelts(BO::tNewtonObj *newtonObj);
    static void DoPostBarrierCollisionHandling(BO::tNewtonObj *newtonObj, coorddef normal);
    static void GenerateVector(sint32 type, coorddef *vector, BWorldSm::Pos *testSimRoadInfo);
    static void TestForUndrivableSurfaces(BO::tNewtonObj *newtonObj);
    static void LimitAngularVelocity(BO::tNewtonObj *newtonObj);
    static void ApplyTheLawOfGravity(BO::tNewtonObj *newtonObj);
    static sint32 CalculateRoadPositionFromSliceAndPosition(sint32 slice, coorddef *position, matrixtdef *matrix);
    static sint32 CalculateRoadPosition(BO::tNewtonObj *newtonObj);
};

#endif
