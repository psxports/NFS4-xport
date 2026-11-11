#include "xport.h"
#include "eaclib/psx/internal.h"

uint32 tickset;
uint32 tickval;

sint32 gettick(void)
{
    FUNCTION_MARKER(0x800E8220, "NFS4.EXE");
    return ticks;
}

sint32 elapsedticks(void)
{
    FUNCTION_MARKER(0x800E8230, "NFS4.EXE");
    uint32 previous = tickval;
    uint32 current = ((uint32)(gettick()));
    tickval = current;
    return (sint32)(current - previous);
}

void resettick(void)
{
    FUNCTION_MARKER(0x800E8260, "NFS4.EXE");
    ticks = 0;
    uint32 current = (uint32)ticks;
    tickval = current;
    tickset = current;
}

void timedwait(sint32 duration)
{
    FUNCTION_MARKER(0x800E8284, "NFS4.EXE");
    uint32 deadline = ((uint32)(gettick())) + ((uint32)(duration));
    while ((sint32)(((uint32)(gettick())) - deadline) < 0)
    {
        psx_root_counter_poll();
        systemtask(0);
    }
}
