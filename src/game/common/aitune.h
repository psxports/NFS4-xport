#ifndef AITUNE_H
#define AITUNE_H

#include "xport.h"
#include "game/common/car.h"

struct accelscale_t
{
    sint32 scale;
};

class AITune
{
public:
    static accelscale_t accelerationScale[22];

    static sint32 oneWay;
    static sint32 driveSide;
    static sint32 gRoughLapTime;
    static sint32 MaxTraffic[2][4];
    static sint32 LifeTimer[12];
    static sint32 BTCPerpAccMults[3];

    struct BTC_t
    {
        sint32 glueMult;
        sint32 speedMult;
        sint32 weightMult;
        sint32 baseChaseTime;
        sint32 wingmanTime;
        sint32 blockaderTime;
        sint32 spikeBeltTime;
    };

    struct tTrackInfo
    {
        sint32 oneWay;
        sint32 driveSide;
        sint32 roughLapTimeSeconds;
    };

    static BTC_t BTC[3];
    static tTrackInfo trackInfo[12];

    static sint32 GetOneWay(void);
    static sint32 GetDriveSide(void);
    static sint32 GetRoughLapTimes(void);
    static void StartUp1(void);
    static void StartUp2(void);
    static void CleanUp1(void);
    static void CleanUp2(void);
};

struct copTuning_t
{
    sint32 regularCopAccMultiplier;
    sint32 superCopAccMultiplier;
    sint32 regularCopTopSpeedCap;
    sint32 superCopTopSpeedCap;
};

extern copTuning_t copTuningInfo[6];

#endif
