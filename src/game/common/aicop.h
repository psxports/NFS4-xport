#ifndef AICOP_H
#define AICOP_H

#include "xport.h"

class AICop
{
public:
    struct BasicPerpInfo;

    struct PerpChaseInfo;

    struct spikeBelt_t
    {
        sint32 active_;
        sint32 slice_;
        sint32 leftLatPos_;
        sint32 rightLatPos_;
        sint32 freshenTime_;
    };

    enum RoadBlockState
    {
        kAICop_RoadBlockState_None = 0,
        kAICop_RoadBlockState_WaitingForPerp = 1,
        kAICop_RoadBlockState_PerpPassed = 2,
    };

    static sint32 numArrestedHumans;
    static spikeBelt_t spikeBelt;
    static RoadBlockState gRoadBlockState;
    static void *rawTriggers;
    static sint32 skillDelay[3];

    static void StartUp(void);
    static void Restart(void);
    static void CleanUp(void);
    static sint32 NoCopsInArea(sint32 slice, sint32 sliceDistance);
};

struct copLevel_t
{
    sint32 copChasers[2];
    sint32 numBlockaders;
    sint32 copBlockaders[2];
    sint32 spikeBelt;
    sint32 copAggression[2];
    sint32 copsPerLap;
    sint32 engagementLapFraction;
    sint32 warningTicks;
    sint32 beatingTicks;
    sint32 numWarningsAdded;
};

struct copGame_t
{
    sint32 numLevels;

    copLevel_t *levels;
};

extern copGame_t copGame[6];

#endif
