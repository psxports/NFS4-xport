#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "xport.h"

class Sched
{
public:
    static sint32 ExecuteTimes[7];
    static sint32 ExecuteElapsedTimes[7];
    static sint32 ExecuteiTimes[7];
    static sint32 ExecuteMasks[7];
    static char gExecuteInfo[4][20];

    typedef void (*tFunctionPt)(void *);

    struct tFunctionSchedule
    {
        sint32 priority;
        void (*function)(void *);
        void *var1;
        void *var2;
    };

    struct tSchedule
    {
        sint32 maxNumFunctions;
        sint32 numFunctions;
        tFunctionSchedule func[1];
    };

    static sint32 ExecuteCheck(sint32 staggered, sint32 module, sint32 distance, sint32 carId, sint32 *time, sint32 *elapsedTime, sint32 *iTime, sint32 forceNoSimOptz);
    static tSchedule * CreateNewSchedule(char *scheduleName, sint32 maxFunctions);
    static void CleanUpSchedule(tSchedule *schedule);
    static void AddFunction(tSchedule *schedule, void (*function)(void *), void *var1, sint32 priority);
    static void DeleteFunction(tSchedule *schedule, void (*function)(void *), void *var1);
    static void Execute(tSchedule *schedule);
};

#endif
