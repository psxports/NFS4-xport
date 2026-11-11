#ifndef STAGPAT_H
#define STAGPAT_H

#include "xport.h"

typedef union SoundTimbreValue SoundTimbreValue;
typedef struct SoundTimbreParameters SoundTimbreParameters;

union SoundTimbreValue
{
    uint32 value;
    void *pointer;
};

struct SoundTimbreParameters
{
    SoundTimbreValue values[38];
};

#ifdef __cplusplus
extern "C" {
#endif
extern uint32 snddefaultenvelope[2];
#ifdef __cplusplus
}
#endif

#endif
