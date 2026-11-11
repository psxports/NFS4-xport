#ifndef AIPHYSIC_H
#define AIPHYSIC_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"

enum Gear_t
{
    GEAR_REVERSE = 0,
    GEAR_NEUTRAL = 1,
    GEAR_FIRST = 2,
    GEAR_SECOND = 3,
    GEAR_THIRD = 4,
    GEAR_FOURTH = 5,
    GEAR_FIFTH = 6,
    GEAR_SIXTH = 7,
    GEAR_SEVENTH = 8,
};

enum eRampType
{
    kRampType_Interpolate = 0,
    kRampType_NoInterpolate = 1
};

class AIPhysic
{
public:
    struct ModelConfig_t
    {
        sint32 dlpos_to_dlvel;
        sint32 max_dlvel;
        sint32 dlvel_to_clacc;
        sint32 max_clacc;
        sint32 dangle_to_dav;
        sint32 max_dav;
        sint32 dav_to_aa;
        sint32 max_aa;
        sint32 vel_limit_range;
        sint32 lat_vel_limit_factor;
        sint32 ang_vel_limit_factor;
    };

    struct Config_t
    {
        sint32 latvelcalc_lookahead;
        sint32 min_lookahead;
        sint32 max_lookahead;
        sint32 look_ahead_factor;
        sint32 skid_value;
        AIPhysic::ModelConfig_t ICModel;
        AIPhysic::ModelConfig_t OOCModel;
    };

    struct BrakeInfo;
    static sint32 iTime;
    static sint32 time;
    static sint32 elapsedTime;

    static void StopCar(Car::tObj *carObj, sint32 velScale, sint32 rotScale);
    static void RevEngine(Car::tObj *carObj);
    static void HandleShifting(Car::tObj *carObj);
    static Gear_t CalculateGear(Car::tObj *carObj);
    static sint32 GearInvSpeedRange(Car::tObj *carObj, Gear_t gear);
    static sint32 GearTopSpeed(Car::tObj *carObj, Gear_t gear);
    static sint32 GearBottomSpeed(Car::tObj *carObj, Gear_t gear);
    static sint32 CalcAcceleration(Car::tObj *carObj, sint32 speed);
    static sint32 ModifyAccelerationAccordingToScript(Car::tObj *carObj, sint32 acc);
    static sint32 CalcDeceleration(Car::tObj *carObj);
    static void CheckDesiredDirection(Car::tObj *carObj);
    static void HandleSignalling(Car::tObj *carObj);
    static void HandleWipeoutTimer(Car::tObj *carObj);
    static void Main(Car::tObj *carObj);
    static void CheckForBadPosition(Car::tObj *carObj);
    static void SimplePhysics(Car::tObj *carObj);
    static void SimplePhysics_LongVel(Car::tObj *carObj);
    static void SimplePhysics_LatVel(Car::tObj *carObj);
    static void CoolPhysics(Car::tObj *carObj);
    static void HandleDirection(Car::tObj *carObj);
    static void Preperation(Car::tObj *carObj);
    static sint32 CalculateRoadPosition(coorddef *pos, sint32 slice);
    static void GetDesiredVector(Car::tObj *carObj);
    static void TargetedGetDesiredVector(Car::tObj *carObj);
    static sint32 CheckIfOutOfControl(Car::tObj *carObj);
    static void OutOfControlPhysics(Car::tObj *carObj);
    static sint32 GetRearEndDamageFactor(Car::tObj *carObj);
    static void InControlPhysics(Car::tObj *carObj);
    static void FinishUp(Car::tObj *carObj);
    static void CalculateRampedDesiredLatPos(Car::tObj *carObj, eRampType rampType);
    static sint32 HitWallCheck(Car::tObj *carObj);
    static void ProcessBarrierCollision(Car::tObj *carObj);
    static void ProcessCollision(Car::tObj *carObj);
    static void ChangeDirection(Car::tObj *carObj, sint32 time);
    static void CheckForGripReduction(Car::tObj *carObj);
    static void StartUp(void);
    static void CleanUp(void);
    static void Reset(void);
    static void ResetCar(Car::tObj *carObj);
    static void InitCar(Car::tObj *carObj);
    static void DeInitCar(Car::tObj *carObj);
};

sint32 AIPhysics_UseCoolPhysics(Car::tObj *carObj);

extern AIPhysic::Config_t AIPhysicConfig;

#endif
