#ifndef WEATHER_H
#define WEATHER_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/matrixtdef.h"
#include "game/common/renderview.h"
#include "game/psx/trackspec.h"

class Weather
{
public:
    struct tSplatInfo;

    struct tSys;

    enum tState
    {
        kSnow = 0,
        kRain = 1,
    };

    static sint32 gLastTimeProcessed;
    static sint32 gDensityGoalState;
    static sint32 gIntensityGoalState;
    static sint32 gDensityChangeFactor;
    static sint32 gIntensityChangeFactor;
    static sint32 gDensityTimerGoal;
    static sint32 gIntensityTimerGoal;
    static sint32 gSnowTrack;
    static sint32 gTrackIntensityLimit;
    static sint32 gDensityTbl[4];
    static sint32 gIntensityTbl[4];
    static sint32 gTrackIntensityLimitTbl[16];
    static char gRandomVelocityVectors[12][3];
    static char * gWasDrawn;
    static tSys gSys;
    static CWeatherSpec * gTrackSpec;
    static tSplatInfo * gSplatInfoServer[2];
    static tSplatInfo * gSplatInfo;
    static char * gDrawnServer[2];
    static SVECTOR * gPos;
    static DVECTOR * gPrevPos;
    static tState gType;
    static SVECTOR * gPServer[2];
    static DVECTOR * gPrevPServer[2];
    static sint32 gLastProcessTime[2];

    static sint32 GetNumParticles(sint32 player);
    static void SetMatrix(matrixtdef *m);
    static void SetIdentMatrix(void);
    static void InitSnow(void);
    static void InitRain(void);
    static void InitSplats(void);
    static sint32 GetNewState(void);
    static void ChangeDensityState(void);
    static void ChangeIntensityState(void);
    static void ChangeDensityBasedOnTime(void);
    static void ChangeIntensityBasedOnTime(void);
    static void InitStateControls(void);
    static void Restart(void);
    static void Init(void);
    static void DeInit(void);
    static void TransformVertex(matrixtdef *m, sint32 n, SVECTOR *s);
    static sint16 CheckAndResetParticles(SVECTOR *pt);
    static void QuickReOrthogonalize(matrixtdef *rot, matrixtdef *rotNew);
    static void ProcessParticles(DRender::tView *Vi, sint32 num, SVECTOR *wpt, char *wd);
    static void CreateSnow(SVECTOR *pt);
    static void CreateRain(SVECTOR *pt0, DVECTOR *pt1, char *wd);
    static void CreateSplat(tSplatInfo *splat);
    static void DoSplats(sint32 num, tSplatInfo *splats);
    static void DoWeather(DRender::tView *Vi);
    static void BuildWeather(DRender::tView *Vi);
};

#endif
