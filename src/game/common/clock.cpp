#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/clock.h"
#include "game/common/input.h"
#include "game/common/r3dcar.h"

static sint32 clock_InterruptStarted;
sint32 generic128HzClock;
sint32 stopClock;
Clock::tGameClock clock_realTime;

void Clock::MasterInterruptHandler(void)
{
    FUNCTION_MARKER(0x8008B938, "NFS4.EXE");

    if (stopClock == 0)
    {
        clock_realTime.time128Hz = (sint32)((uint32)clock_realTime.time128Hz + 1);
        generic128HzClock = (sint32)((uint32)generic128HzClock + 1);
        if ((clock_realTime.time128Hz & 1) == 0)
        {
            clock_realTime.time64Hz = (sint32)((uint32)clock_realTime.time64Hz + 1);
            if ((clock_realTime.time64Hz & 1) == 0)
            {
                clock_realTime.time32Hz = (sint32)((uint32)clock_realTime.time32Hz + 1);
                if ((clock_realTime.time32Hz & 1) == 0)
                    Input::Update();
                Input::Store();
            }
        }
    }
}

void Clock::SystemStartUp(void)
{
    FUNCTION_MARKER(0x8008B9E4, "NFS4.EXE");

    stopClock = 0;
    generic128HzClock = 0;
    if (clock_InterruptStarted == 0)
    {
        clock_InterruptStarted = 1;
        addtimer(MasterInterruptHandler);
        clock_realTime.time128Hz = 0;
        clock_realTime.time64Hz = 0;
        clock_realTime.time32Hz = 0;
        Input::gTime = 0;
    }
    R3DCar::ClockLast = 0;
}

void Clock::SystemCleanUp(void)
{
    FUNCTION_MARKER(0x8008BA40, "NFS4.EXE");

    if (clock_InterruptStarted != 0)
    {
        clock_InterruptStarted = 0;
        deltimer(MasterInterruptHandler);
    }
}
