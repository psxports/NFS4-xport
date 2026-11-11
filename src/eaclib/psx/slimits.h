#ifndef SLIMITS_H
#define SLIMITS_H

#include "xport.h"

typedef struct SNDLIMITS SNDLIMITS;

struct SNDLIMITS
{
    sint32 dmabuflen;
    uint8 numdmabufs;
    uint8 numdmamsgs;
    uint8 nummicrotalkinstances;
    uint8 microtalkinstanceabort;
    sint16 numrspcmds;
    sint16 pad;
    sint32 audiostreambufsize;
    sint32 packetbufsize;
};

#ifdef __cplusplus
extern "C" {
#endif
extern SNDLIMITS sndpsxlimits;
#ifdef __cplusplus
}
#endif

#endif
