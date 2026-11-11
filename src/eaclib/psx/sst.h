#ifndef SST_H
#define SST_H

#include "xport.h"
#include "eaclib/psx/spktplay.h"
#include "eaclib/psx/splysdef.h"
#include "eaclib/psx/stream.h"

typedef struct SoundStreamRequestPrefix SoundStreamRequestPrefix;
typedef struct SoundStreamPrefix SoundStreamPrefix;
typedef struct SoundStreamState SoundStreamState;
typedef struct SNDSTREAMSTATUS SNDSTREAMSTATUS;
typedef struct SNDREQUESTSTATUS SNDREQUESTSTATUS;

struct SoundStreamRequestPrefix
{
    uint32 streamRequest;
    uint32 id;
    sint32 firstPacket;
    sint32 lastPacket;
    uint32 dataRate;
    uint32 consumed;
    uint32 total;
    uint32 queued;
    uint32 queuedChunks;
    uint32 chunks;
    sint16 hold;
    uint8 unknown2A[2];
};

struct SoundStreamPrefix
{
    SoundStreamRequestPrefix *requests;
    NativeStreamHandle *streamHandle;
    sint32 requestHandle;
    sint32 packetHandle;
    uint32 unknown16;
    uint8 state;
    uint8 streamCount;
    uint8 requestCount;
    uint8 parseIndex;
    uint8 sharedStream;
    uint8 unknown19[3];
    SNDSAMPLEFORMAT format;
    SNDSAMPLEFORMAT nextFormat;
    uint8 attributes[20];
    uint8 nextAttributes[20];
    SNDPLAYOPTS options;
};

struct SoundStreamState
{
    SoundStreamPrefix *stream;
    sint8 packetStreams[1];
};

#ifdef __cplusplus
extern "C" {
#endif
extern SoundStreamState sndss;
#ifdef __cplusplus
}
#endif

struct SNDSTREAMSTATUS;
struct SNDREQUESTSTATUS;

#ifdef __cplusplus
class SNDSTRM
{
public:
    static sint32 setpriority(sint32 handle, sint32 mask, sint32 priority);
    static sint32 getvol(sint32 handle);
    static sint32 status(sint32 handle, SNDSTREAMSTATUS *result);
    static sint32 requeststatus(sint32 handle, SNDREQUESTSTATUS *result);
    static sint32 create(SNDPLAYOPTS *options, sint32 streams, sint32 chunks, void *buffer, sint32 size);
    static sint32 destroy(sint32 handle);
    static sint32 queuefile(sint32 handle, sint32 delay, const char *filename, sint32 offset);
    static sint32 purge(sint32 handle);
    static sint32 setgreedylevel(sint32 handle, sint32 level);
    static sint32 setgreedystate(sint32 handle, sint32 enabled);
    static sint32 autovol(sint32 handle, sint32 ticks, sint32 volume);
    static sint32 overheadtap(sint32 streams, sint32 chunks);
    static sint32 overhead(sint32 streams, sint32 chunks);
    static bool IsPaused(void);
    static sint32 SetPaused(sint32 handle, bool paused);
    static sint32 vol(sint32 handle, sint32 volume);
};
#endif

#endif
