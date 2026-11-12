#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "game/common/aitriger.h"
#include "game/common/sim.h"
#include "lib/psx/fixddiv.h"

AITrigger_TriggerManager *triggerManagerCops;
AITrigger_TriggerManager *triggerManagerTraffic;

sint32 AITrigger_Compare(const void *op1, const void *op2)
{
    FUNCTION_MARKER(0x80072E18, "NFS4.EXE");
    trigger::trigger_t *ta = *(trigger::trigger_t *const *)op1;
    trigger::trigger_t *tb = *(trigger::trigger_t *const *)op2;
    return (ta->any.slice - tb->any.slice);
}

void AITrigger_TriggerManager::Init(char *rawTriggers)
{
    FUNCTION_MARKER(0x8007286C, "NFS4.EXE");
    numTriggers_ = 0;
    invNumTriggers_ = 0;
    for (sint32 tLoop = 99; tLoop >= 0; --tLoop)
        checkTime_[tLoop] = -0xa00;
    for (sint32 car = 8; car >= 0; --car)
        lastTriggerChecked_[car] = 0;
    if (rawTriggers == 0)
        return;
    char *currentTrigger = rawTriggers + 4;
    sint32 numTriggers;
    memcpy(&numTriggers, rawTriggers, sizeof(numTriggers));
    for (sint32 tLoop = 0; tLoop < numTriggers; ++tLoop)
    {
        sint32 size = InsertTrigger((trigger::trigger_t *)currentTrigger, true);
        currentTrigger += size;
        memcpy(&numTriggers, rawTriggers, sizeof(numTriggers));
    }
    Sort();
    if (numTriggers_ != 0)
        invNumTriggers_ = fixeddiv(65536, ((uint32)numTriggers_ << 16));
}

sint32 AITrigger_TriggerManager::InsertTrigger(trigger::trigger_t *trigger, bool fromFile)
{
    FUNCTION_MARKER(0x80072948, "NFS4.EXE");
    sint32 size = 0;
    DescribeTrigger(trigger);
    if (trigger->any.type == TRIGGER_TRAFFIC_PATH)
    {
        if (fromFile)
            trigger->trafficPath.path = (::trigger::pathPosition_t *)((char *)trigger + 0x40);
        triggers_[numTriggers_] = trigger;
        size = ((trigger->trafficPath.numPoints * 0x14) + 0x40);
    }
    else
    {
        if (trigger->any.type == TRIGGER_COP_SIMPLE || trigger->any.type == TRIGGER_COP_ROADBLOCK)
            size = 0x14;
        else if (trigger->any.type == TRIGGER_COP_OFFROAD)
            size = 0x48;
        triggers_[numTriggers_] = trigger;
    }
    numTriggers_ = (numTriggers_ + 1);
    return size;
}

trigger::trigger_t *AITrigger_TriggerManager::GetNextTrigger(sint32 car)
{
    FUNCTION_MARKER(0x80072A44, "NFS4.EXE");
    if (numTriggers_ == 0)
        return 0;
    sint32 triggerNum = (lastTriggerChecked_[car] + 1);
    lastTriggerChecked_[car] = triggerNum;
    if (triggerNum == numTriggers_)
        lastTriggerChecked_[car] = 0;
    triggerNum = lastTriggerChecked_[car];
    return triggerNum < numTriggers_ ? triggers_[triggerNum] : 0;
}

trigger::trigger_t *AITrigger_TriggerManager::GetPrevTrigger(sint32 car)
{
    FUNCTION_MARKER(0x80072AB4, "NFS4.EXE");
    if (numTriggers_ == 0)
        return 0;
    sint32 triggerNum = (lastTriggerChecked_[car] - 1);
    lastTriggerChecked_[car] = triggerNum;
    if (triggerNum == -1)
        lastTriggerChecked_[car] = (numTriggers_ - 1);
    triggerNum = lastTriggerChecked_[car];
    return triggerNum < numTriggers_ ? triggers_[triggerNum] : 0;
}

sint32 AITrigger_TriggerManager::CheckForTriggerAtSlice(sint32 car, sint32 slice)
{
    FUNCTION_MARKER(0x80072B24, "NFS4.EXE");
    if (numTriggers_ == 0)
        return -1;
    sint32 *lastTrigger = &lastTriggerChecked_[car];
    if (triggers_[*lastTrigger]->any.slice < slice && numTriggers_ > 1)
    {
        do
        {
            if (*lastTrigger == (numTriggers_ - 1))
                break;
            GetNextTrigger(car);
        } while (triggers_[*lastTrigger]->any.slice < slice);
    }
    else if (slice < triggers_[*lastTrigger]->any.slice && numTriggers_ > 1)
    {
        do
        {
            if (*lastTrigger == 0)
                break;
            GetPrevTrigger(car);
        } while (slice < triggers_[*lastTrigger]->any.slice);
    }
    sint32 triggerNum = *lastTrigger;
    if (triggers_[triggerNum]->any.slice != slice)
        return -1;
    sint32 elapsed = (simGlobal.gameTicks - checkTime_[triggerNum]);
    if (elapsed <= 0xa00 || triggers_[triggerNum]->any.type == TRIGGER_COP_ROADBLOCK)
        return -1;
    return triggerNum;
}

trigger::trigger_t *AITrigger_TriggerManager::GetTrigger(sint32 trigger, sint32 *used)
{
    FUNCTION_MARKER(0x80072CD8, "NFS4.EXE");
    sint32 elapsed = (simGlobal.gameTicks - checkTime_[trigger]);
    *used = elapsed < 0xa01 ? 1 : 0;
    checkTime_[trigger] = simGlobal.gameTicks;
    return trigger < numTriggers_ ? triggers_[trigger] : 0;
}

sint32 AITrigger_TriggerManager::CheckForClosestTriggerOfType(sint32 slice, triggerType type, sint32 direction)
{
    FUNCTION_MARKER(0x80072D40, "NFS4.EXE");
    trigger::trigger_t *prevTrigger = 0;
    trigger::trigger_t *firstTrigger = 0;
    sint32 prevTriggerIndex = -1;
    sint32 firstTriggerIndex = -1;
    const sint32 numTriggers = numTriggers_;
    if (numTriggers == 0)
        return -1;
    for (sint32 tLoop = 0; tLoop < numTriggers; ++tLoop)
    {
        trigger::trigger_t *thisTrigger = triggers_[tLoop];
        if (thisTrigger->any.type == type)
        {
            if (firstTrigger == 0)
            {
                firstTrigger = thisTrigger;
                firstTriggerIndex = tLoop;
            }
            if (thisTrigger->any.slice == slice)
                return tLoop;
            if (slice < thisTrigger->any.slice)
            {
                if (direction > 0)
                    return tLoop;
                if (direction < 0 && prevTrigger != 0)
                    return prevTriggerIndex;
            }
            prevTrigger = thisTrigger;
            prevTriggerIndex = tLoop;
        }
    }
    if (prevTrigger != 0 && direction < 0)
        return prevTriggerIndex;
    if (direction > 0)
        return firstTriggerIndex;
    return -1;
}

void AITrigger_TriggerManager::DescribeTrigger(trigger::trigger_t *trigger)
{
    FUNCTION_MARKER(0x80072E10, "NFS4.EXE");
}

void AITrigger_TriggerManager::Sort(void)
{
    FUNCTION_MARKER(0x80072E30, "NFS4.EXE");
    if (numTriggers_ != 0)
        qsortPSX(triggers_, (uint32)numTriggers_, sizeof(triggers_[0]), AITrigger_Compare);
}
