#include "xport.h"
#include "eaclib/psx/internal.h"

uint32 timerflag;
sint32 timerevent;
uint32 reentryflag;

void inittimer(sint32 frequency)
{
    FUNCTION_MARKER(0x800F41F0, "NFS4.EXE");
    if (frequency == 0)
        frequency = 100;
    EnterCriticalSectionPSX();
    if (timerflag == 0)
    {
        for (sint32 i = 0; i < 8; ++i)
            tmrsub[i] = 0;
        timerevent = OpenEvent(0xF2000002, 2, 0x1000, tmrint);
        EnableEvent(timerevent);
        timerflag = 1;
        addexit(restoretimer);
    }
    timerhz = frequency;
    reentryflag = 0;
    SetRCnt(0xF2000002, (uint16)(0x409980 / frequency), 0x1000);
    StartRCnt(0xF2000002);
    ExitCriticalSection();
    resettick();
    addexit(restoretimer);
}

void restoretimer(void)
{
    FUNCTION_MARKER(0x800F4304, "NFS4.EXE");
    DisableEvent(timerevent);
    timerflag = 0;
}

sint32 tmrint(void)
{
    FUNCTION_MARKER(0x800F4328, "NFS4.EXE");
    ticks = (sint32)((uint32)ticks + 1u);
    sint32 previousLibTicks = libticks;
    g_currentthread = 1;
    libticks = previousLibTicks + 1;
    for (sint32 i = 0; i < 8; ++i)
    {
        GameTimerCallback callback = tmrsub[i];
        if (callback != 0)
            callback();
    }
    g_currentthread = 0;
    return 0;
}
