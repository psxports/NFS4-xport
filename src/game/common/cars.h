#ifndef CARS_H
#define CARS_H

#include "xport.h"
#include "game/common/bo.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"

class Cars
{
public:
    static Car::tObj *gList[9];
    static Car::tObj *gRaceCarList[9];
    static Car::tObj *gAICarList[9];
    static Car::tObj *gHumanRaceCarList[9];
    static Car::tObj *gAIRaceCarList[9];
    static Car::tObj *gTrafficCarList[9];
    static Car::tObj *gCopCarList[9];
    static Car::tObj *gLifeBasisCarList[9];
    static Car::tObj *gSortedList[9];
    static Car::tObj *gTotalSortedList[9];

    static Car::tStats gNewCarStatsList[9];

    static sint32 gNumCars;
    static sint32 gNumRaceCars;
    static sint32 gNumAICars;
    static sint32 gNumHumanRaceCars;
    static sint32 gNumAIRaceCars;
    static sint32 gNumTrafficCars;
    static sint32 gNumCopCars;
    static sint32 gNumLifeBasisCars;
    static sint32 topSpeedCap[22];
    static sint32 kSkidMarkSurface[16];
    static sint32 kSmokingSurface[16];
    static sint32 kConvertFromRoadToSfxType[16];
    static sint32 kAudioRoadSurfaceInterface[16];
    static sint32 kSFXWallSurfaceInterface[8];
    static sint32 kAudioWallSurfaceInterface[8];
    static sint32 kAudioCollisoinTypeInterface[7];

    struct tCollisionInfo
    {
        sint32 resetTimer;
        sint32 smoking;
    };

    struct tAudio
    {
        sint32 type;
        sint32 channel;
        sint32 surface1;
        sint32 surface2;
        sint32 force;
        sint32 panangle;
    };

    struct tRenderInfo;

    static void DoGravityEffectsOnAcc(Car::tObj *carObj, sint32 arcade);
    static void GetDashData(Car::tObj *carObj, sint32 *rpm, sint32 *gear, sint32 *speed);
    static void QDUpdateVelGlue(Car::tObj *carObj);
    static void InitDashData(Car::tObj *carObj, sint32 *redline, sint32 *topspeed);
    static void SetAudioCalls(Car::tObj *carObj, sint32 type, sint32 chan, sint32 s1, sint32 s2, sint32 force, sint32 pan);
    static void ResetCarCounters(void);
    static void InitStats(Car::tObj *carObj);
    static void InitializeCarTablesFlagsAndCounters(Car::tObj *carObj);
    static void ResetVariablesAfterACollision(Car::tObj *carObj);
    static void ResetCollidedCars(Car::tObj *carObj, sint32 forceReset, sint32 forceParkAtSide);
    static void SetCarUpForHiRezSim(Car::tObj *carObj);
    static void DoExtraCarCollisionProcessing(Car::tObj *carObj);
    static void AddCarSfx(Car::tObj *carObj, sint32 wheel, coorddef *skidpt, sint32 roadSurface, sint32 surfaceType, sint32 drawSkid);
    static void FindTotalSlice(Car::tObj *carObj);
    static void CalculateStartingGridOffset(Car::tObj *carObj, sint32 *slice, coorddef *offset);
    static void IniCarObjects(Car::tObj *carObj, sint32 index);
    static void InitCar(Car::tObj *carObj, sint32 index);
    static void DeInitCar(Car::tObj *carObj);
    static void Restart(void);
    static void Initialize(char *mem, sint32 size);
    static void StartUp(void);
    static void CleanUp(void);
    static void FindCurrentLap(Car::tObj *carObj);
    static sint32 CalculateRoadSpan(Car::tObj *carObj);
    static sint32 CalcVelDownRoad(Car::tObj *carObj);
    static void Randomize(void);
    static void ManageBureaucracy(void);
    static void CheckForAccidentScenes(void);
    static void SortCars(void);
};

extern BO::tNewtonObj InfiniteMassNewton[5];

#endif
