#ifndef AUDIO_H
#define AUDIO_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"

class Audio
{
public:
    static void *gHeap;

    static sint32 direct3davail;

    static void InitDriver(sint32 buffersize, sint32 spusize);
    static void DeInitDriver(void);
    static void CleanUp(void);
    static void FECleanUp(void);
};

extern sint32 gMusicHandle;

#endif
