#ifndef SSTSTAT_H
#define SSTSTAT_H

#include "xport.h"

typedef struct SNDSTREAMSTATUS SNDSTREAMSTATUS;

struct SNDSTREAMSTATUS
{
    sint32 outstandingrequests;
    sint32 currentrequest;
    sint32 timebuffered;
};

#endif
