#ifndef AIDATARECORD_H
#define AIDATARECORD_H

#include <stddef.h>

#include "xport.h"

class AIDataRecord_BestLine_t;

class AIDataRecord_TrackCurve_t;

class AIDataRecord
{
public:
    enum WhichRecord_t
    {
        NORECORD_R = 0,
        RACER_SPEED_R = 1,
        TRAFFIC_SPEED_R = 2,
        ACCELERATION_R = 3,
        REMOVED_R = 4,
        BEST_LINE_R = 5,
        TRACK_CURVE_R = 6,
        CURVESPEED_TABLE_R = 7,
        CAR_TRACKING_R = 8
    };

    enum RecordMethod_t
    {
        NORMAL_M = 0,
        RECORD_M = 1,
        TEST_M = 2
    };

    static WhichRecord_t WhichRecord;
    static RecordMethod_t RecordMethod;
    static AIDataRecord_BestLine_t *BestLine;
    static AIDataRecord_TrackCurve_t *TrackCurve;
};

class AIDataRecord_t
{
public:
    static void *operator new(size_t size);
    static void operator delete(void *memory);
    AIDataRecord_t(AIDataRecord::WhichRecord_t whichIsThis, char *preBuffer);
    virtual ~AIDataRecord_t(void);
    sint32 AddRecordToCollection(void);
    sint32 RemoveRecordFromCollection(void);
    static void StartUp1(void);
    static void StartUp2(void);
    static void CleanUp1(void);
    static void CleanUp2(void);
    virtual void Setup(void);
    sint32 Load(void);
    sint32 SaveAndPurge(void);
    sint32 numElements_;
    sint32 bSize_;
    char name_[64];
    char *dataBuffer_;
    char *preAllocatedBuffer_;
    AIDataRecord::RecordMethod_t recordMethod_;
};

class AIDataRecord_AccTable_t : public AIDataRecord_t
{
public:
    AIDataRecord_AccTable_t(char *preBuffer, sint32 scale, AIDataRecord::WhichRecord_t whichIsThis);
    virtual ~AIDataRecord_AccTable_t(void);
    sint32 Get(sint32 speed);
    virtual void Setup(void);
    sint32 scale_;
};

class AIDataRecord_BestLine_t : public AIDataRecord_t
{
public:
    AIDataRecord_BestLine_t(AIDataRecord::WhichRecord_t whichIsThis);
    virtual ~AIDataRecord_BestLine_t(void);
};

class AIDataRecord_TrackCurve_t : public AIDataRecord_t
{
public:
    AIDataRecord_TrackCurve_t(AIDataRecord::WhichRecord_t whichIsThis);
    virtual ~AIDataRecord_TrackCurve_t(void);
    sint32 Get(sint32 slice);
};

class AIDataRecord_CurveSpeedTable_t : public AIDataRecord_t
{
public:
    AIDataRecord_CurveSpeedTable_t(char *carName, AIDataRecord::WhichRecord_t whichIsThis);
    virtual ~AIDataRecord_CurveSpeedTable_t(void);
    sint32 Get(sint32 curve);
    void Upgrade(sint32 handlingUpgrade);
};

class AIDataRecord_CarTracking_t : public AIDataRecord_t
{
public:
    virtual sint32 Get(sint32 slice);
    virtual ~AIDataRecord_CarTracking_t(void);
};

#endif
