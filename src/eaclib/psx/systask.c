#include "xport.h"
#include "eaclib/psx/internal.h"
#include "systask.h"

volatile uint32 systemTaskAddDepth;
volatile sint32 systemTaskLastTick;
volatile SystemTaskEntry systemtasksubs[16];

void addsystemtask(sint32 (*callback)(sint32 unused0, sint32 unused1), sint32 interval, sint32 delay)
{
    FUNCTION_MARKER(0x800E6AF4, "NFS4.EXE");
    uint32 skip = systemTaskAddDepth;
    systemTaskAddDepth = skip + 1u;
    sint32 selected = -1;
    for (sint32 i = 0; i < 16; ++i)
    {
        SystemTaskCallback current = systemtasksubs[i].callback;
        if (current == callback)
            selected = i;
        else if (current == 0 && selected == -1)
        {
            if (skip == 0)
                selected = i;
            else
                --skip;
        }
    }
    if (selected != -1)
    {
        sint32 now = libticks;
        volatile SystemTaskEntry *task = &systemtasksubs[selected];
        task->callback = callback;
        task->interval = interval;
        task->running = 0;
        task->nextTick = (sint32)(((uint32)(now)) + ((uint32)(delay)));
    }
    systemTaskAddDepth = systemTaskAddDepth - 1u;
}

void delsystemtask(sint32 (*callback)(sint32 unused0, sint32 unused1))
{
    FUNCTION_MARKER(0x800E6BA8, "NFS4.EXE");
    sint32 index;
    for (index = 0; index < 16; ++index)
        if (systemtasksubs[index].callback == callback)
            break;
    if (index < 16 && systemtasksubs[index].callback == callback)
        systemtasksubs[index].callback = 0;
}

uint32 systemtask(sint32 context)
{
    FUNCTION_MARKER(0x800E6C04, "NFS4.EXE");
    psx_native_poll_events();
    sint32 previous = systemTaskLastTick;
    sint32 current = libticks;
    uint32 result = 0;
    if (previous != current)
    {
        systemTaskLastTick = libticks;
        for (sint32 i = 0; i < 16; ++i)
        {
            volatile SystemTaskEntry *task = &systemtasksubs[i];
            SystemTaskCallback callback = task->callback;
            if (callback == 0)
                continue;
            sint32 now = libticks;
            sint32 due = task->nextTick;
            if (now < due || task->running != 0)
                continue;
            task->running = 1;
            sint32 lateness = (sint32)(((uint32)(libticks)) - ((uint32)(due)));
            sint32 value = callback(context, lateness);
            sint32 after = libticks;
            sint32 interval = task->interval;
            result |= ((uint32)(value));
            task->running = 0;
            task->nextTick = (sint32)(((uint32)(after)) + ((uint32)(interval)));
        }
    }
    return result;
}
