#ifndef SSTRSTAT_H
#define SSTRSTAT_H

#include "xport.h"

typedef struct SNDREQUESTSTATUS SNDREQUESTSTATUS;

struct SNDREQUESTSTATUS
{
    sint32 state;
    sint32 currenttime;
    sint32 timetoend;
    sint32 timebuffered;
};

#endif
