#ifndef SPKTPLAY_H
#define SPKTPLAY_H

#include "xport.h"

typedef struct SNDSAMPLEFORMAT SNDSAMPLEFORMAT;
typedef struct SoundPacketSubmit SoundPacketSubmit;
typedef struct SoundPacketRecord SoundPacketRecord;
typedef struct SoundPacketPlayPrefix SoundPacketPlayPrefix;
typedef struct SNDPLAYOPTS SNDPLAYOPTS;

struct SNDSAMPLEFORMAT
{
    uint16 samplerate;
    uint8 channels;
    uint8 samplerep;
};

typedef void (*SoundPacketReleaseCallback)(void *data);

typedef void (*SoundPacketNotifyCallback)(sint32 handle, uint32 frames);

struct SoundPacketSubmit
{
    uint32 unknown0;
    uint32 frames;
    uint32 unknown8;
    void *channels[5];
};

struct SoundPacketRecord
{
    sint32 id;
    uint32 frames;
    void *channels[4];
};

struct SoundPacketPlayPrefix
{
    sint32 playHandle;
    uint32 sequence;
    sint16 capacity;
    sint16 head;
    sint16 current;
    sint16 queued;
    uint32 frames;
    uint32 inFlightFrames;
    uint8 *platformBuffer;
    SoundPacketReleaseCallback release;
    SoundPacketNotifyCallback notify;
    SNDSAMPLEFORMAT format;
};

#ifdef __cplusplus
extern "C" {
#endif
extern sint32 sndpp;
extern SoundPacketPlayPrefix *sndpps[1];
#ifdef __cplusplus
}
#endif

struct SNDPLAYOPTS;

#ifdef __cplusplus
class SNDPKTPLAY
{
public:
    static sint32 overhead(sint32 chunks);
    static sint32 create(void *buffer, sint32 size, SoundPacketReleaseCallback release, SoundPacketNotifyCallback notify);
    static sint32 start(sint32 handle, SNDSAMPLEFORMAT *format, void *attributes, SNDPLAYOPTS *options);
    static sint32 submit(sint32 handle, const SoundPacketSubmit *data);
    static sint32 submitspace(sint32 handle);
    static sint32 unsafeframesoutstanding(sint32 handle);
    static sint32 framesoutstanding(sint32 handle);
    static sint32 purge(sint32 handle, sint32 first, sint32 last);
    static sint32 stop(sint32 handle);
    static sint32 destroy(sint32 handle);
};
#endif

#endif
