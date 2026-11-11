#ifndef AISTATE_H
#define AISTATE_H

#include <stddef.h>
#include "xport.h"
#include "game/common/aitriger.h"
#include "game/common/car.h"
#include "game/common/donutmode.h"
#include "game/common/mathnfs.h"

class AIState
{
public:
    static void StartUp(void);
    static void Restart(void);
    static void CleanUp(void);
};

class AIDelayCar
{
public:
    AIDelayCar(Car::tObj *basisCar, Car::tObj *targetCar, sint32 delayFactor);
    void SetNewTargetCar(Car::tObj *targetCar);
    void Update(void);
    sint32 delayFactor_;
    Car::tObj *basisCar_;
    Car::tObj *targetCar_;
    sint32 deltaMeters_;
    sint32 slice_;
    coorddef deltaPosition_;
    coorddef position_;
    sint32 deltaRoadPosition_;
    sint32 roadPosition_;
    sint32 laneIndex_;
    sint32 currentSpeed_;
};

enum cruiseMode_t
{
    CRUISE_ATSETSPEED = 0,
    CRUISE_ATFACTOR = 1,
    CRUISE_ATTRAFFICSPEED = 2
};

class AIState_Base
{
public:
    static void *operator new(size_t size);
    static void operator delete(void *memory);
    AIState_Base(Car::tObj *carObj);
    void StateExecute(void);
    virtual void Execute(void) = 0;

    virtual ~AIState_Base(void);

    virtual sint32 TestForRelease(void);

    Car::tObj *carObj_;
};

class AIState_None : public AIState_Base
{
public:
    AIState_None(Car::tObj *carObj);
    virtual ~AIState_None(void);

    virtual void Execute(void);
};

class AIState_Normal : public AIState_Base
{
public:
    AIState_Normal(Car::tObj *carObj);

    virtual ~AIState_Normal(void);

    virtual void Execute(void);
};

class AIState_Idle : public AIState_Base
{
public:
    AIState_Idle(Car::tObj *carObj);
    virtual ~AIState_Idle(void);

    virtual void Execute(void);
    void SetIdlePosition(sint32 roadPosition);
    sint32 roadPosition_;
    sint32 idleInPlaceFlag_;
};

class AIState_Chase : public AIState_Base
{
public:
    AIState_Chase(Car::tObj *carObj, Car::tObj *targetCar, coorddef *relPosition, sint32 nitrousTicks, sint32 nitrousMinForeDistance, sint32 nitrousMinAftDistance, sint32 aggressionLevel, sint32 delayTime);
    virtual ~AIState_Chase(void);
    void SetTarget(Car::tObj *targetCar, coorddef *relPosition);
    void SetMurderMode(sint32 murderMode, sint32 murderTicks);
    void SetUp(void);
    void DoSlowNitrous(void);
    void DoNitrous(sint32 checkForHumans);
    virtual void Execute(void);
    void FarTargeting(void);
    sint32 CalculateCloseTargettingAheadSlowDownFactor(void);
    void CloseTargeting(void);
    void ApproachTargeting(sint32 intercept);
    void CheckForBarriersAndTargetAroundThem(void);
    sint32 FindBarrierEndSlice(void);
    AIDelayCar delayCar_;
    sint32 noTurnAroundEndTime_;
    Car::tObj *targetCar_;
    coorddef relPosition_;
    sint32 longTargetRegion_;
    sint32 latTargetRegion_;
    sint32 targetDir_;
    sint32 carDir_;
    sint32 longMetersBetween_;
    sint32 latMetersBetween_;
    sint32 murderMode_;
    sint32 murderEndTime_;
    sint32 inTargetRegion_;
    sint32 nitrousTicks_;
    sint32 nitrousMinForeDistance_;
    sint32 nitrousMinAftDistance_;
    sint32 aggressionLevel_;
    sint32 slowDownEndTime_;
    sint32 barrierTicks32_;
};

class AIState_Offroad : public AIState_Base
{
public:
    AIState_Offroad(Car::tObj *carObj, sint32 startSlice, coorddef *position, matrixtdef *orientation, sint32 maxSpeedKPH, sint32 releaseTime, sint32 endSlice);
    virtual ~AIState_Offroad(void);
    void UnleashIfInRange(Car::tObj *car);
    virtual void Execute(void);
    sint32 startSlice_;
    coorddef startPosition_;
    matrixtdef startOrientation_;
    coorddef startHeading_;
    sint32 targetSlice_;
    coorddef targetPosition_;
    sint32 longMetersBetween_;
    sint32 letGo_;
    sint32 maxSpeedMPS_;
    sint32 releaseTime_;
};

class AIState_NonActive : public AIState_Base
{
public:

    AIState_NonActive(Car::tObj *carObj);
    virtual ~AIState_NonActive(void);

    virtual void Execute(void);
};

class AIState_Purgatory : public AIState_NonActive
{
public:

    static sint32 numTrafficCarsInPurgatory;

    AIState_Purgatory(Car::tObj *carObj);

    static void StartUp(void);
    virtual ~AIState_Purgatory(void);
    virtual sint32 TestForRelease(void);
    virtual void Execute(void);
};

class AIState_RovingTraffic : public AIState_Base
{
public:
    AIState_RovingTraffic(Car::tObj *carObj, trigger::trigger_t *trigger);

    virtual ~AIState_RovingTraffic(void);

    virtual sint32 TestForRelease(void);

    virtual void Execute(void);
    void CheckIfCarIsNearbyAndStop(Car::tObj *otherCarObj, sint32 &status);
    trigger::pathPosition_t *path_;
    sint32 numPathPoints_;
    sint32 pathIndex_;
    sint32 waitTick_;
};

class AIState_Donuts : public AIState_Base
{
public:
    virtual ~AIState_Donuts(void);
    virtual void Execute(void);
    sint32 donutLookForward_;
    donutMode_t donutMode_;
};

class AIState_GotoSlice : public AIState_Normal
{
public:
    AIState_GotoSlice(Car::tObj *carObj, sint32 targetSlice, sint32 stopWhenArrivedAtSlice);

    virtual ~AIState_GotoSlice(void);

    virtual void Execute(void);
    sint32 InTargetSliceRange(sint32 rangeMeters);
    sint32 targetSlice_;
    sint32 stopWhenArrivedAtSlice_;
};

class AIState_Cruise : public AIState_Normal
{
public:
    AIState_Cruise(Car::tObj *carObj, cruiseMode_t cruiseMode, sint32 speedOrFactor);

    virtual ~AIState_Cruise(void);

    virtual void Execute(void);
    cruiseMode_t cruiseMode_;
    sint32 cruiseSpeed_;
    sint32 cruiseFactor_;
};

#endif
