#ifndef AUDIOTRK_H
#define AUDIOTRK_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/mathnfs.h"

struct AudioElem
{
    coorddef cp;
    uint16 nextDelay;
    char patchID;
    char fadeIn;
    sint16 range;
    char minDelay;
    char randomDelay;
    char type;
    char chan;
    char minRepeat;
    char randomRepeat;
};

class AudioTrk
{
public:
    struct tAmbientChannel;

    struct tGlobals;
    static tGlobals *g;

    static void Reset(void);
    static void StartUp(void);
    static void AddCustomObject(AudioElem *se, sint32 tck, coorddef *vel, sint32 fade, Car::tObj *car, sint32 trkazi);
    static void SoundTrack(Car::tObj *car, sint32 trkazi);
    static sint32 PreLoad(void);
    static void CleanUp(void);
};

#endif
