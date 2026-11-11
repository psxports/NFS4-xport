#ifndef SYSTEMMEMORY_H
#define SYSTEMMEMORY_H

#include <stddef.h>

#include "xport.h"

struct cluttbl
{
    uint32 type : 8;
    sint32 next : 24;
    sint16 width;
    sint16 height;
    sint16 centerx;
    sint16 centery;
    sint16 shapex;
    sint16 shapey;
    sint16 data[16];
};

struct NativeSystemMemoryOwner
{
    enum { allocationBytes = 0xB34F4, allocationWords = (allocationBytes + 43 + 3) / 4 };

    cluttbl fontPalette;
    uint32 allocation[allocationWords];
};

extern NativeSystemMemoryOwner gSystemMemoryOwner;
extern cluttbl &shpfontclut;

#endif
