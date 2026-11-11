#ifndef SPLYSDEF_H
#define SPLYSDEF_H

#include "xport.h"

typedef struct SNDPLAYOPTS SNDPLAYOPTS;

struct SNDPLAYOPTS
{
    sint32 patnum;
    char bhandle;
    char keynum;
    char velocity;
    char pan;
    char vol;
    char bend;
    char fxlevel0;
    char use3dpos;
    uint16 pitchmult;
    uint16 timemult;
    uint16 azimuth;
    sint16 elevation;
};

#endif
