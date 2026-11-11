#ifndef AITRIGER_H
#define AITRIGER_H

#include "xport.h"
#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

class trigger
{
public:
    struct anyTrigger_t
    {
        sint32 type;
        sint32 slice;
    };

    struct simple_t
    {
        sint32 type;
        sint32 slice;
        sint32 dir;
        sint32 side;
        sint32 moving;
    };

    struct roadblock_t
    {
        sint32 type;
        sint32 slice;
        sint32 dir;
        sint32 numCars;
        sint32 spikeBelt;
    };

    struct offroad_t;

    struct trafficAccident_t;

    struct pathPosition_t;

    struct trafficPath_t;

    union trigger_t;
};

struct trigger::offroad_t
{
    sint32 type;
    sint32 slice;
    sint32 dir;
    coorddef position;
    matrixtdef orientation;
    sint32 maxSpeed;
    sint32 releaseTime;
    sint32 endSlice;
};

struct trigger::trafficAccident_t
{
    sint32 type;
    sint32 slice;
    coorddef position;
    matrixtdef orientation;
};

struct trigger::pathPosition_t
{
    coorddef position;
    sint32 targetSpeed;
    sint32 waitTime;
};

struct trigger::trafficPath_t
{
    sint32 type;
    sint32 slice;
    sint32 dir;
    matrixtdef orientation;
    sint32 maxSpeed;
    sint32 releaseTime;
    sint32 numPoints;
    trigger::pathPosition_t *path;
};

union trigger::trigger_t
{
    trigger::anyTrigger_t any;
    trigger::simple_t simple;
    trigger::roadblock_t roadblock;
    trigger::offroad_t offroad;
    trigger::trafficAccident_t trafficAccident;
    trigger::trafficPath_t trafficPath;
};

enum triggerType

{
    TRIGGER_NONE = 0,

    TRIGGER_COP_SIMPLE = 1,

    TRIGGER_COP_ROADBLOCK = 2,

    TRIGGER_COP_OFFROAD = 3,

    TRIGGER_TRAFFIC_ACCIDENT = 4,

    TRIGGER_TRAFFIC_PATH = 5,

    TRIGGER_NUM_TRIGGER_TYPES = 6
};

class AITrigger_TriggerManager

{
public:
    void Init(char *rawTriggers);

    sint32 InsertTrigger(trigger::trigger_t *trigger, bool fromFile);

    trigger::trigger_t *GetNextTrigger(sint32 car);

    trigger::trigger_t *GetPrevTrigger(sint32 car);

    sint32 CheckForTriggerAtSlice(sint32 car, sint32 slice);

    trigger::trigger_t *GetTrigger(sint32 trigger, sint32 *used);

    sint32 CheckForClosestTriggerOfType(sint32 slice, triggerType type, sint32 direction);

    void DescribeTrigger(trigger::trigger_t *trigger);

    void Sort(void);

    sint32 numTriggers_;

    sint32 invNumTriggers_;

    trigger::trigger_t *triggers_[100];

    sint32 checkTime_[100];

    sint32 lastTriggerChecked_[9];
};

extern AITrigger_TriggerManager *triggerManagerCops;
extern AITrigger_TriggerManager *triggerManagerTraffic;

#endif
