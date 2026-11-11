#ifndef CALLBACK_H
#define CALLBACK_H

#include "xport.h"

typedef char *(*MemAllocFuncPtr)(sint32, char *);

typedef void (*MemFreeFuncPtr)(char *);

typedef void (*GameTimerCallback)(void);

typedef void *(*GameFileLoadCallback)(void *memory, const char *name, uint32 flags);

#endif
