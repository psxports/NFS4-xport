#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/schedule.h"
#include "game/common/sim.h"

sint32 Sched::ExecuteTimes[7] = { 0, 1, 2, 4, 8, 16, 32 };
sint32 Sched::ExecuteElapsedTimes[7] = { 32, 32, 16, 8, 4, 2, 1 };
sint32 Sched::ExecuteiTimes[7] = { 0, 65536, 32768, 16384, 8192, 4096, 2048 };
sint32 Sched::ExecuteMasks[7] = { 63, 31, 15, 7, 3, 1, 0 };

char Sched::gExecuteInfo[4][20] = {
    { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3 },
    { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
    { 6, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
    { 6, 6, 6, 6, 6, 6, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
};

sint32 Sched::ExecuteCheck(sint32 staggered, sint32 module, sint32 distance, sint32 carId, sint32 *time, sint32 *elapsedTime, sint32 *iTime, sint32 forceNoSimOptz)
{
    FUNCTION_MARKER(0x800B61BC, "NFS4.EXE");

    if (simGlobal.gameTicks < 16)
    {
        *time = ExecuteTimes[6];
        *iTime = ExecuteiTimes[6];
        *elapsedTime = ExecuteElapsedTimes[6];
        return 1;
    }
    sint32 distanceIndex = (distance / 16) / 65536;
    if (distanceIndex >= 20)
        distanceIndex = 19;
    if (forceNoSimOptz != 0)
        distanceIndex = 0;
    sint32 index = (uint8)gExecuteInfo[module][distanceIndex];
    *time = ExecuteTimes[index];
    *iTime = ExecuteiTimes[index];
    *elapsedTime = ExecuteElapsedTimes[index];
    sint32 mask = ExecuteMasks[index];
    uint32 phase = (simGlobal.gameTicks / 2) + (uint32)carId * staggered;
    return (phase & mask) == (uint32)mask;
}

Sched::tSchedule * Sched::CreateNewSchedule(char *scheduleName, sint32 maxFunctions)
{
    FUNCTION_MARKER(0x800B62F0, "NFS4.EXE");

    sint32 length = (sint32)(sizeof(tSchedule) + (uint32)maxFunctions * sizeof(tFunctionSchedule));
    tSchedule *newSchedule = (tSchedule *)reservememadr(scheduleName, length, 0);
    newSchedule->maxNumFunctions = maxFunctions;
    newSchedule->numFunctions = 0;
    for (sint32 i = 0; i < newSchedule->maxNumFunctions; ++i)
    {
        newSchedule->func[i].priority = 0x7ffe;
        newSchedule->func[i].function = 0;
    }
    return newSchedule;
}

void Sched::CleanUpSchedule(Sched::tSchedule *schedule)
{
    FUNCTION_MARKER(0x800B6360, "NFS4.EXE");

    purgememadr(schedule);
}

void Sched::AddFunction(Sched::tSchedule *schedule, void (*function)(void *), void *var1, sint32 priority)
{
    FUNCTION_MARKER(0x800B6380, "NFS4.EXE");

    sint32 n = 0;
    schedule->numFunctions = (schedule->numFunctions + 1);
    for (sint32 i = 0; i < schedule->numFunctions; ++i)
    {
        n = i;
        if (priority < schedule->func[i].priority)
        {
            for (sint32 j = schedule->numFunctions - 1; j > i; --j)
                memcpy(&schedule->func[j], &schedule->func[j - 1], sizeof(tFunctionSchedule));
            break;
        }
    }
    schedule->func[n].priority = priority;
    schedule->func[n].function = function;
    schedule->func[n].var1 = var1;
}

void Sched::DeleteFunction(Sched::tSchedule *schedule, void (*function)(void *), void *var1)
{
    FUNCTION_MARKER(0x800B6434, "NFS4.EXE");

    if (schedule->numFunctions != 0)
    {
        for (sint32 i = 0; i < schedule->numFunctions; ++i)
            if (schedule->func[i].function == function && schedule->func[i].var1 == var1)
            {
                for (sint32 j = i; j < schedule->numFunctions - 1; ++j)
                    memcpy(&schedule->func[j], &schedule->func[j + 1], sizeof(tFunctionSchedule));
                break;
            }
        schedule->func[schedule->numFunctions].priority = 0x7ffe;
        schedule->numFunctions = (schedule->numFunctions - 1);
    }
}

void Sched::Execute(Sched::tSchedule *schedule)
{
    FUNCTION_MARKER(0x800B6500, "NFS4.EXE");

    for (sint32 i = 0; i < schedule->numFunctions; ++i)
    {
        tFunctionPt function = schedule->func[i].function;
        if (function != 0)
            function(schedule->func[i].var1);
    }
}
