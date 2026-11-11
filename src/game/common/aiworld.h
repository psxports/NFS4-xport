#ifndef AIWORLD_H
#define AIWORLD_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

extern sint32 inverseLaneWidthTable[80];

class AIWorld
{
public:
    static sint32 ZSplineDistance(Car::tObj *carObj, Car::tObj *otherCarObj);
    static sint32 ZSplineDistance(coorddef *pos1, coorddef *pos2, matrixtdef *roadMatrix);
    static sint32 ApxSplineDistance(Car::tObj *carObj, Car::tObj *otherCarObj);
    static sint32 ApxSplineDistance(Car::tObj *carObj, sint32 location);
    static sint32 ApxSplineDistance(sint32 location, Car::tObj *carObj);
    static sint32 ApxSplineDistance(sint32 locationA, sint32 locationB);
    static sint32 SplineDistance(Car::tObj *carObj, Car::tObj *otherCarObj);
    static sint32 SplineDistance(Car::tObj *carObj, sint32 location, coorddef *position);
    static sint32 GameOdometer(Car::tObj *carObj);
    static sint32 IsDriveableLaneInSliceRange(sint32 startSlice, sint32 numSlicesToCheck, sint32 direction, sint32 laneIndex);
    static sint32 IsDriveableLane(sint32 slice, sint32 laneIndex);
    static sint32 GetProfileMask(sint32 laneIndex);
    static sint32 IsDriveableLane_UsingMask(sint32 slice, sint32 mask);
    static sint32 CheckForBarrierBetweenLanes(sint32 slice, sint32 lane0, sint32 lane1);
    static sint32 LaneIndex(sint32 slice, sint32 position);
    static void CalculateLaneInfo(Car::tObj *carObj);
    static sint32 CalculateDeltaRoadYaw(Car::tObj *carObj);
    static sint32 CalcRoadBend(Car::tObj *carObj, sint32 lookAhead);
    static sint32 CalcFutureLateralVel(Car::tObj *carObj, sint32 slicesAhead);
    static void CalcSpeed(Car::tObj *carObj);
    static sint32 CalcLateralVelocity(Car::tObj *carObj);
    static void FindBarrierLessLaneAndPosition(Car::tObj *carObj, sint32 *goodLane, sint32 *goodPosition);
};

#endif
