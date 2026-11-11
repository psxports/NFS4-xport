#ifndef NASYNC_H
#define NASYNC_H

#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/memstd.h"

typedef void (*NativeAsyncCallback)(...);

typedef void (*NativeFileCompletion)(uint32 operation, uint32 result, void *userData);

typedef sint32 (*NativeReadOperation)(sint32 fileHandle, sint32 offset, void *destination, sint32 size, sint32 priority, void *userData);

struct NativeSyncReadState
{
    sint32 priority;
    sint32 fileHandle;
    sint32 offset;
    sint32 remaining;
    sint32 transferred;
    sint32 chunkSize;
    char *cursor;
    NativeReadOperation reader;
    uint32 pending;
};

struct NativeAsyncRequest
{
    uint32 handle;
    NativeAsyncRequest *next;
    sint32 result;
    sint32 state;
    void *storage;
    NativeAsyncCallback callback;
    uint32 pending;
    sint32 fileHandle;
    sint32 offset;
    union
    {
        uint32 allocationFlags;
        sint32 remaining;
    };

    char *cursor;
};

struct NativeAsyncQueue
{
    NativeAsyncRequest *head;
    NativeAsyncRequest *tail;
};

extern "C"
{
    extern sint32 asyncChunkSize;
    extern sint32 asyncRequestCount;
    extern NativeAsyncRequest *asyncRequests;
    extern uint32 asyncGeneration;
    extern NativeAsyncQueue asyncFreeQueue;
    extern NativeAsyncQueue asyncCompletionQueue;
    extern sint32 asyncFile;
    extern sint32 asyncFileOffset;
    extern MemMutex *asyncMutex;
}

#endif
