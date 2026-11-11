#ifndef STREAM_H
#define STREAM_H

#include "xport.h"
#include "eaclib/psx/memstd.h"

typedef struct NativeStreamHandle NativeStreamHandle;
typedef struct NativeStreamRequest NativeStreamRequest;
typedef struct NativeStreamRule NativeStreamRule;
typedef struct NativeStreamControl NativeStreamControl;

struct NativeStreamHandle;

struct NativeStreamRequest
{
    uint32 index;
    sint32 state;
    NativeStreamRequest *previous;
    NativeStreamRequest *next;
    sint32 memoryBacked;
    char name[64];
    const uint8 *data;
    sint32 sizeOrOffset;
    uint32 endMarker;
    uint8 *firstChunk;
};

struct NativeStreamRule
{
    uint32 mask;
    uint32 value;
    sint32 target;
};

struct NativeStreamControl
{
    uint32 magic;
    MemMutex *mutex;
    NativeStreamRequest *requests;
    sint32 requestCount;
    NativeStreamRule *rules;
    sint32 ruleCount;
    NativeStreamHandle *handles;
    sint32 handleCount;
    uint8 *bufferStart;
    uint8 *bufferEnd;
    sint32 state;
    sint32 priorityMask;
    sint32 priority;
    sint32 greedyLevel;
    sint32 greedyEnabled;
    sint32 bufferUsage;
    uint8 *cursor40;
    uint8 *cursor44;
    uint8 *cursor48;
    NativeStreamRequest *unknown4C;
    NativeStreamRequest *unknown50;
    NativeStreamRequest *unknown54;
    NativeStreamRequest *unknown58;
    uint8 unknown5C[64];
    sint32 unknown9C;
    sint32 offset;
    uint32 fileOperation;
    sint32 readLength;
};

struct NativeStreamHandle
{
    NativeStreamControl *control;
    uint32 index;
    uint32 available;
    uint32 *chunk;
};


#ifdef __cplusplus
class STREAM
{
public:
    static sint32 overhead(sint32 requests, sint32 files, sint32 filters);
    static NativeStreamHandle *create(sint32 requests, sint32 files, sint32 filters, void *buffer, sint32 size);
    static void destroy(NativeStreamHandle *handle);
    static void setpriority(NativeStreamHandle *handle, sint32 mask, sint32 priority);
    static void setgreedylevel(NativeStreamHandle *handle, sint32 level);
    static void setgreedystate(NativeStreamHandle *handle, sint32 enabled);
    static uint32 queuefile(NativeStreamHandle *handle, const char *filename, sint32 offset, uint32 endMarker);
    static uint32 queuemem(NativeStreamHandle *handle, const void *data, sint32 size, uint32 endMarker);
    static void cancelrequest(struct NativeStreamHandle *handle, uint32 key);
    static void kill(NativeStreamHandle *handle);
    static uint32 *get(NativeStreamHandle *handle);
    static void release(NativeStreamHandle *handle, void *chunk);
    static uint32 gettable(NativeStreamHandle *handle);
    static sint32 state(NativeStreamHandle *handle);
    static sint32 buffersize(NativeStreamHandle *handle);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern uint32 requestidcounter;
#ifdef __cplusplus
}
#endif

#endif
