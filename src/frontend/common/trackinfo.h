#ifndef TRACKINFO_H
#define TRACKINFO_H

#include "xport.h"

struct tTrackInfo
{
    char fTrackNumber;
    uint8 fDirection;
    uint8 fMirrored;
    uint8 fTimeOfDay;
    uint8 fWeather;
    uint8 fRandom;
    uint8 fSituations;
    uint8 fPad;
    sint32 fPrize[6];
    uint32 fDifficulty;
    uint8 fReserved[4];
};

#endif
