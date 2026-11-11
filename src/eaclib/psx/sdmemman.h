#ifndef SDMEMMAN_H
#define SDMEMMAN_H

#include "xport.h"

typedef struct SoundSpuAllocation SoundSpuAllocation;
typedef struct SoundSpuMemoryState SoundSpuMemoryState;

struct SoundSpuAllocation
{
    uint16 offsetUnits;
    uint16 lengthUnits;
};

struct SoundSpuMemoryState
{
    uint16 allocationCount;
    uint16 firstUnit;
    uint16 endUnit;
    uint16 limitUnit;
    SoundSpuAllocation allocations[128];
};

#ifdef __cplusplus
extern "C" {
#endif
extern SoundSpuMemoryState soundSpuMemory;
#ifdef __cplusplus
}
#endif

#endif
