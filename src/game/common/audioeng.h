#ifndef AUDIOENG_H
#define AUDIOENG_H

#include "xport.h"
#include "game/common/car.h"

class AudioEng
{
public:
    struct tChanAttr
    {
        sint16 min;
        sint16 max;
        char *xlate;
        char patchnum;
    };

    struct tAdjustments
    {
        uint8 inCarBoost;
        uint8 inCarExhaust;
        uint8 outCarExhaust;
        uint8 fwdEngBoost;
        uint8 rwdExhBoost;
        uint8 outCarBoost;
        uint8 pitchScale;
        uint8 timbreScale;
    };

    struct tState
    {
        uint16 vol;
        uint16 dop;
        uint16 azi;
        uint16 sep;
        uint16 esp;
        uint8 exh;
        uint8 gas;
    };

    struct tVoiceAttr
    {
        char vol;
        uint16 pitch;
        uint16 azim;
        sint32 handle;
    };

    struct tEvent
    {
        char use;
        char patnum;
        char pad1;
        char pad2;
        sint16 delta;
        sint16 max;
        sint32 attackdelta;
        sint32 decaydelta;
    };

    struct tTable
    {
        char xlate[512];
    };

    struct AudioEng_t;

    struct tDef;
    static AudioEng_t *g[2];

    static void Set(sint32 player, sint32 vol, sint32 esp, sint32 gas, sint32 cam, sint32 dop, sint32 azi, sint32 dir);
    static void Update(void);
    static void LoadDef(char *filename, char *name, sint32 handle, sint32 offset, sint32 size, tDef **ed);
    static sint32 StartUp(sint32 player, char *carname);
    static void StartServer(void);
    static void StopServer(void);
    static bool TryQueueState(sint32 player, const tState &state);
    static void CleanUp(void);
};

#endif
