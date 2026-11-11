#ifndef STATS_H
#define STATS_H

#include "xport.h"
#include "game/common/car.h"

struct tRecordBuffer;

class Stats
{
public:
    static tRecordBuffer gTrackRecords[187];

    struct tPosition
    {
        sint32 car;
        sint32 slice;
        sint32 sliceTime;
        sint32 isHuman;
    };

    static tPosition racePosition[6];

    static void DoPlayerGlue(void);
    static void ClearPosition(void);
    static sint32 GetPosition(Car::tObj *carObj);
    static sint32 GetNumOpponents(void);
    static void TrackStats(Car::tObj *carObj);
    static void ExtrapolateOpponentTimes(sint32 type);
    static void TrackEndGame(void);
};

#endif
