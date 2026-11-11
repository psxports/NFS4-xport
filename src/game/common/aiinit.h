#ifndef AIINIT_H
#define AIINIT_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/udff.h"

class AIInit
{
public:
    static sint32 useSpreadForce;
    static sint32 forceHumanHandBrake;

    static void StartUp1(void);
    static void StartUp2(void);
    static void Reset1(void);
    static void Reset2(void);
    static void CleanUp1(void);
    static void CleanUp2(void);
    static void LoadConfigs(void);
    static void LoadPhysicsConfig(Udff::tInfo *handle);
    static void ClearAICar(Car::tObj *carObj);
    static void RestartAICar(Car::tObj *carObj);
    static void InitAICar(Car::tObj *carObj, Udff::tInfo *handle);
    static void DeInitAICar(Car::tObj *carObj);
    static void InitAICar2(Car::tObj *carObj);
    static void DeInitAICar2(Car::tObj *carObj);
    static sint32 IsNonStandardCarFile(sint32 index);
};

#endif
