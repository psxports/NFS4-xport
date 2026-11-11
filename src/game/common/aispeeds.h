#ifndef AISPEEDS_H
#define AISPEEDS_H

#include "xport.h"
#include "game/common/car.h"

struct speedData_t
{
    uint16 endSlice;
    uint16 speedMPS;
};

class AISpeeds
{
public:
    struct Upgrade_t
    {
        sint32 accMult;
        sint32 brakeMult;
        sint32 handlingMult;
        sint32 topSpeedMult;
    };

    struct tLeaderBoard
    {
        Car::tObj *leadRacer;
        Car::tObj *leadHumanRacer;
        Car::tObj *leadAIRacer;
        Car::tObj *lastAIRacer;
    };

    struct tSlotInfo
    {
        sint32 distanceMaintainTime32;
        sint32 minDistanceMeters;
        sint32 maxDistanceMeters;
        uint32 fallBackRandomTime_TickPercent;
    };

    static tLeaderBoard leaderBoard;
    static Upgrade_t engineUpgrade;
    static Upgrade_t suspensionUpgrade;
    static Upgrade_t aeroUpgrade;
    static speedData_t *TrackSpeeds[11];
    static tSlotInfo CaravanInfo[6];
    static sint32 trackAndNightMult;
    static sint32 *WeatherMultFactors;

    static void StartUp(void);
    static void ReadTuningInfo(void);
    static void CleanUp(void);
    static sint32 SuperDuperSpeedUpTheCarsAtTheStartBecauseWeCannotActuallyHandleRenderingTheseCars(Car::tObj *carObj);
    static sint32 CalcOpponentTopSpeed(Car::tObj *carObj, sint32 *unFetteredDesiredSpeed);
    static sint32 NeedToSlowDownForCurve(Car::tObj *carObj, sint32 distanceMeters, sint32 currentSpeed, sint32 futureCurveSpeed);
    static sint32 CalcOpponentCurveSpeed(Car::tObj *carObj);
    static sint32 BTCGetGlueFactor(Car::tObj *carObj);
    static Car::tObj * GetNextAICar(Car::tObj *carObj);
    static Car::tObj * GetPrevAICar(Car::tObj *carObj);
    static sint32 GetCaravanFactor(Car::tObj *carObj);
    static sint32 GetGlueFactor(Car::tObj *carObj);
    static sint32 GetDamageFactor(Car::tObj *carObj);
    static sint32 LimitGlueMultiplier(Car::tObj *carObj, sint32 f_final);
    static sint32 CalcCopTopSpeed(Car::tObj *carObj);
    static sint32 CalcTrafficTopSpeed(Car::tObj *carObj);
    static sint32 GetLegalSpeed(sint32 slice);
    static sint32 RandomizeTrafficSpeed(Car::tObj *carObj, sint32 oldsafe);
    static void CalcDesiredSpeed(Car::tObj *carObj);
    static sint32 CalculateOncomingCarSpeed(Car::tObj *carObj);
    static void SetTrafficSpeedRandomFactor(Car::tObj *carObj);
    static void MaintainLeaderBoard(void);
    static sint32 GetScriptFactor(Car::tObj *carObj);
    static sint32 GetUpgradeAccMult(sint32 carIndex);
    static sint32 GetUpgradeBrakeMult(sint32 carIndex);
    static sint32 GetUpgradeHandlingMult(sint32 carIndex);
    static sint32 GetUpgradeTopSpeedMult(sint32 carIndex);
    static sint32 CalcHumanTopSpeed(Car::tObj *carObj);
    static sint32 CalcHumanCurveSpeed(Car::tObj *carObj);
};

#endif
