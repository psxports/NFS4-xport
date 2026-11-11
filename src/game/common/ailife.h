#ifndef AILIFE_H
#define AILIFE_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

class AILife
{
public:
    static sint32 EvaluateLife(Car::tObj *carObj);
    static void RCSetSpeeds(Car::tObj *carObj);
    static void RCPickSliceAndDirection(Car::tObj *carObj);
    static void RCPickDesiredLatPosition(Car::tObj *carObj);
    static void PlaceCarAtLocation(Car::tObj *carObj, sint32 slice, sint32 desiredLatPos, sint32 direction, sint32 currentSpeed, sint32 rotation1024);
    static void SetInitialSlicePositionOrientationEtc(Car::tObj *carObj);
    static void PlaceCarAtLocation(Car::tObj *carObj, sint32 rotation1024);
    static void ReencarnateTraffic(Car::tObj *carObj);
    static void ReencarnateTrafficByPosition(Car::tObj *carObj, sint32 slice, sint32 travelDirection, coorddef *pos, matrixtdef *ori);
    static void ReencarnateCopBySlice(Car::tObj *carObj, sint32 slice, sint32 travelDirection, sint32 roadSide, sint32 moving);
    static void ReencarnateCopByPosition(Car::tObj *carObj, sint32 slice, sint32 travelDirection, coorddef *pos, matrixtdef *ori);
    static void ReencarnateCopByLatPosAndRotation(Car::tObj *carObj, sint32 slice, sint32 travelDirection, sint32 latPos, sint32 rotation1024);
    static sint32 IsCoordInThisLiveArea(coorddef *tPos, Car::tObj *racer);
    static Car::tObj * IsTrafficCarInAnyLiveArea(Car::tObj *traffic);
    static sint32 IsCoordInThisVisibleArea(coorddef *tPos, Car::tObj *racer);
    static Car::tObj * IsCarInAnyVisibleArea(Car::tObj *carObj);
    static Car::tObj * IsSliceInAnyVisibleArea(sint32 slice);
    static Car::tObj * IsSliceCloseToAnyCopCar(sint32 slice);
    static Car::tObj * IsPositionInAnyVisibleArea(coorddef *pos);
    static void Debug(char *format, ...);
};

#endif
