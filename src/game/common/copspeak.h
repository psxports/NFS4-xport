#ifndef COPSPEAK_H
#define COPSPEAK_H

#include "xport.h"
#include "eaclib/psx/sststat.h"
#include "game/common/car.h"

class CopSpeak
{
public:
    struct tFileIndex
    {
        sint32 offset;
        sint32 size;
    };

    struct tBankHeader
    {
        sint32 id;
        uint8 ver;
    };

    struct tRequest
    {
        Car::tObj *car;
        sint32 buffer;
        sint32 offset;
        sint32 size;
        sint32 filehandle;
        sint32 ophandle;
        sint32 phrase;
        char player;
        char sfx;
        char bank;
        char noise;
    };

    struct tBank
    {
        sint32 FileHandle;
        sint32 FileOpen;
        tFileIndex *Index;
        sint32 Count;
    };

    static sint32 gQueueHead;
    static sint32 gQueuePlay;
    static sint32 gSpchHandle;
    static sint32 gNumTrackSfx;
    static sint32 gQueueLoad;
    static sint32 gQueueReady;
    static sint32 gWrongWay;
    static sint16 gBufferLow;
    static sint16 gBufferStart;
    static sint16 gBufferEnd;
    static sint16 gBufferHigh;
    static char gHandleCount;
    static char * gBuffer;
    static sint32 gStaticHandle[2];
    static char gStaticPatch[2];
    static tRequest gRequest;
    static tRequest gCurrent;
    static SNDSTREAMSTATUS gMusicStat;
    static tRequest gQueue[64];

    static void RadioStaticInit(void);
    static void RadioStaticActive(sint32 noise);
    static void RadioStaticSquelch(void);
    static void InitRequest(tRequest *r);
    static void SilenceCop(Car::tObj *car, sint32 playerIndex);
    static void Alloc(tRequest *r);
    static void Free(tRequest *r);
    static void ReadyNextRequest(void);
    static void Cancel(void);
    static void CleanUp(void);
    static void InitVars(void);
    static void DirectRequest(sint32 filehandle, sint32 offset, sint32 size, Car::tObj *car, char *name);
    static void GenericBankRequest(sint32 patch, Car::tObj *car);
    static void StartUp(void);
    static sint32 GetEnginePatch(sint32 type, sint32 timbre);
    static sint32 Play(tRequest *r, sint32 handle);
    static void Skip(void);
    static sint32 Request(tRequest *r);
    static sint32 BankVolume(tRequest *r);
    static void LoadNextRequest(void);
    static void PlayNextRequest(void);
    static void Flush(void);
    static void Server(void);
    static sint32 SfxQueued(void);
    static void ShowQueue(void);
};

#endif
