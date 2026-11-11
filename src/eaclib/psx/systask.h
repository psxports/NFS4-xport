#ifndef SYSTASK_H
#define SYSTASK_H

#include "xport.h"

typedef sint32 (*SystemTaskCallback)(sint32 context, sint32 lateness);

typedef struct SystemTaskEntry SystemTaskEntry;

struct SystemTaskEntry
{
    SystemTaskCallback callback;
    sint32 interval;
    sint32 nextTick;
    sint32 running;
};

#ifdef __cplusplus
extern "C" {
#endif
extern volatile uint32 systemTaskAddDepth;
extern volatile sint32 systemTaskLastTick;
extern volatile SystemTaskEntry systemtasksubs[16];
#ifdef __cplusplus
}
#endif

#endif
