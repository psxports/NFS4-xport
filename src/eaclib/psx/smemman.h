#ifndef SMEMMAN_H
#define SMEMMAN_H

#include "xport.h"

typedef struct SoundMemoryAllocation SoundMemoryAllocation;
typedef struct SoundMemoryState SoundMemoryState;

struct SoundMemoryAllocation
{
    uint16 offsetWords;
    uint16 lengthWords;
};

struct SoundMemoryState
{
    uint8 *memory;
    uint16 allocationCount;
    uint16 capacityWords;
    sint32 highWaterWords;
    SoundMemoryAllocation allocations[128];
};

#ifdef __cplusplus
extern "C" {
#endif
extern SoundMemoryState sndmm;
#ifdef __cplusplus
}
#endif

#endif
