#ifndef SPAT2HDR_H
#define SPAT2HDR_H

#include "xport.h"

typedef struct SoundAttributesPrefix SoundAttributesPrefix;

struct SoundAttributesPrefix
{
    sint32 unknown0;
    uint16 unknown4;
    uint8 unknown6;
    uint8 volume;
    uint8 pan;
    uint8 unknown9;
    uint8 unknown10;
    uint8 unknown11;
};

#ifdef __cplusplus
extern "C" {
#endif
extern uint32 sndseed[6];
#ifdef __cplusplus
}
#endif

#endif
