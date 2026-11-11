#ifndef CLOCK_H
#define CLOCK_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"

class Clock
{
public:
    struct tGameClock
    {
        sint32 time128Hz;
        sint32 time64Hz;
        sint32 time32Hz;
    };

    static void MasterInterruptHandler(void);
    static void SystemStartUp(void);
    static void SystemCleanUp(void);
};

extern sint32 generic128HzClock;
extern sint32 stopClock;
extern Clock::tGameClock clock_realTime;

#endif
