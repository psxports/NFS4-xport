#ifndef NFILE_H
#define NFILE_H

#include "xport.h"
#include "eaclib/psx/nasync.h"

union NativeFileAtomicResult
{
    sint32 value;
    void *memory;
};

struct NativeFileSizeRequest
{
    const char *name;
};

struct NativeFileLoadRequest
{
    const char *name;
    void *destination;
    uint32 flags;
    uint32 abortFlags;
};

struct NativeFileSlot;

struct NativeBigArchive;

struct NativeFileOperation
{
    uint32 key;
    sint32 cancelled;
    sint32 status;
    sint32 error;
    sint32 priority;
    void *userData;
    sint32 offset;
    sint32 length;
    void *destination;
    NativeFileSlot *file;
    NativeFileCompletion callback;
    NativeFileOperation *next;
};

struct NativeFileSlot
{
    union
    {
        uint32 descriptor;
        ::FILE *stream;
        NativeBigArchive *archive;
    };

    sint32 length;
    uint32 archiveOffset;
    char name[64];
};

struct NativeBigArchive
{
    char *indexData;
    NativeFileSlot *file;
    NativeFileOperation *operation;
    NativeBigArchive *next;
};

struct NativeFileDevicePrefix
{
    sint32 operationCount;
    sint32 fileCount;
    sint32 priorityLimit;
    uint32 queuedCount;
    uint32 callbackDepth;
    NativeFileOperation *active;
    NativeFileOperation *operations;
    NativeFileSlot *files;
    NativeFileOperation *queue;
    NativeBigArchive *archives;
};

extern "C" NativeFileDevicePrefix gFileDevice;
extern "C" uint32 fileOperationGeneration;

#ifdef __cplusplus
namespace XPORT_UNPLACED
{
class FILE
{
public:
    static sint32 opensync(const char *filename, sint32 mode, sint32 priority, sint32 *handle);
    static sint32 readsync(sint32 handle, sint32 offset, void *destination, sint32 size, sint32 priority);
    static sint32 closesync(sint32 handle, sint32 priority);
    static sint32 sizesync(sint32 handle, sint32 priority);
    static sint32 addbigsync(const char *name, uint32 flags, sint32 priority, sint32 *handle);
    static sint32 delbigsync(sint32 handle, sint32 priority);
    static sint32 init(sint32 fileCount, sint32 bufferSize, sint32 operationCount);
    static sint32 initwithmem(sint32 fileCount, sint32 bufferSize, sint32 operationCount, void *memory);
    static sint32 overhead(sint32 fileCount, sint32 bufferSize, sint32 operationCount);
    static sint32 opstatus(sint32 handle);
    static sint32 operror(sint32 handle);
    static void callbackop(uint32 operation, NativeFileCompletion callback);
    static void priorityop(uint32 operation, sint32 priority);
    static void cancelop(uint32 operation);
    static sint32 waitop(uint32 operation);
    static sint32 completeop(sint32 handle);
    static uint32 open(const char *name, sint32 mode, sint32 priority, void *userData);
    static uint32 close(sint32 filehandle, sint32 priority, void *userData);
    static sint32 read(sint32 filehandle, sint32 offset, void *destination, sint32 size, sint32 priority, void *userData);
    static uint32 size(sint32 filehandle, sint32 priority, void *userData);
    static uint32 addbig(const char *name, uint32 flags, sint32 priority, void *userData);
    static uint32 delbig(sint32 handle, sint32 priority, void *userData);
    static NativeFileAtomicResult atomic(NativeFileAtomicResult (*callback)(sint32, void *), uint32 unused, sint32 priority, void *userData);
};
}

class iFILE
{
public:
    static void addbigreadcallback(uint32 operation, uint32 status, void *userData);
    static void addbigopencallback(uint32 operation, uint32 status, void *userData);
    static void delbigclosecallback(uint32 operation, uint32 status, void *userData);
    static void CommandCompleteCallback(sint32 successful);
};
#endif

#endif
