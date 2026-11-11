#include "xport.h"
#include "eaclib/psx/internal.h"
#include "nasync.h"

sint32 asyncChunkSize;
uint32 asyncGeneration;
NativeAsyncQueue asyncFreeQueue;
NativeAsyncQueue asyncCompletionQueue;
sint32 asyncFile;
sint32 asyncFileOffset;
MemMutex *asyncMutex;

sint32 asyncRequestCount;
NativeAsyncRequest *asyncRequests;

static NativeAsyncRequest *locaterequest(uint32 handle)
{
    FUNCTION_MARKER(0x800F0BF4, "NFS4.EXE");
    if ((sint32)handle < 0x100)
        return 0;
    sint32 index = handle & 0xFF;
    if (index >= asyncRequestCount)
        return 0;
    NativeAsyncRequest *request = &asyncRequests[index];
    return request->handle == handle ? request : 0;
}

extern "C" void queueadd(NativeAsyncQueue *queue, NativeAsyncRequest *request)
{
    FUNCTION_MARKER(0x800F0B1C, "NFS4.EXE");
    sint32 interruptsWereEnabled = EnterCriticalSectionPSX();
    if (queue->head == 0)
        queue->head = request;
    else
        queue->tail->next = request;
    queue->tail = request;
    request->next = 0;
    if (interruptsWereEnabled != 0)
        ExitCriticalSection();
}

extern "C" NativeAsyncRequest *queuefetch(NativeAsyncQueue *queue)
{
    FUNCTION_MARKER(0x800F0B74, "NFS4.EXE");
    sint32 interruptsWereEnabled = EnterCriticalSectionPSX();
    NativeAsyncRequest *request = queue->head;
    if (request != 0)
        queue->head = request->next;
    if (interruptsWereEnabled != 0)
        ExitCriticalSection();
    return request;
}

extern "C" uint32 newrequestid(NativeAsyncRequest *request)
{
    FUNCTION_MARKER(0x800F0BC0, "NFS4.EXE");
    asyncGeneration += 0x100;
    if (asyncGeneration == 0)
        asyncGeneration = 0x100;
    uint8 index = (uint8)request->handle;
    uint32 generation = asyncGeneration;
    uint32 handle = index | generation;
    request->handle = handle;
    return handle;
}

extern "C" void cancelrequest(NativeAsyncRequest *request)
{
    FUNCTION_MARKER(0x800F0C50, "NFS4.EXE");
    sint32 interruptsWereEnabled = EnterCriticalSectionPSX();
    sint32 state = request->state;
    if (state == 1)
        request->state = 2;
    if (interruptsWereEnabled != 0)
        ExitCriticalSection();
    if (state != 1)
        return;
    void *storage = request->storage;
    if (reinterpret_cast<uintptr_t>(storage) > 1)
        purgememadr(storage);
    uint8 index = (uint8)request->handle;
    request->pending = 0;
    request->handle = index;
    queueadd(&asyncFreeQueue, request);
}

extern "C" void finishrequest(NativeAsyncRequest *request)
{
    FUNCTION_MARKER(0x800F0CE8, "NFS4.EXE");
    NativeAsyncCallback callback = request->callback;
    request->pending = 0;
    if (callback != 0)
        queueadd(&asyncCompletionQueue, request);
}

extern "C" void loadfileclosecallback(uint32, uint32, void *userData)
{
    FUNCTION_MARKER(0x800F0D24, "NFS4.EXE");
    NativeAsyncRequest *request = static_cast<NativeAsyncRequest *>(userData);
    XPORT_UNPLACED::FILE::completeop(request->pending);
    if (request->state == 0)
        finishrequest(request);
    else
        cancelrequest(request);
}

extern "C" void loadfilereadcallback(uint32, uint32, void *userData)
{
    FUNCTION_MARKER(0x800F0D80, "NFS4.EXE");
    NativeAsyncRequest *request = static_cast<NativeAsyncRequest *>(userData);
    sint32 bytes = XPORT_UNPLACED::FILE::completeop(request->pending);
    sint32 total = request->result;
    sint32 chunkSize = asyncChunkSize;
    request->result = (sint32)((uint32)total + (uint32)bytes);
    uint32 operation;
    NativeFileCompletion callback;
    if (bytes < chunkSize || request->state != 0)
    {
        operation = XPORT_UNPLACED::FILE::close(request->fileHandle, 99, request);
        callback = loadfileclosecallback;
    }
    else
    {
        sint32 offset = request->offset;
        char *cursor = request->cursor;
        request->offset = (sint32)((uint32)offset + (uint32)bytes);
        request->cursor = cursor + bytes;
        operation = XPORT_UNPLACED::FILE::read(request->fileHandle, request->offset, request->cursor, chunkSize, 99, request);
        callback = loadfilereadcallback;
    }
    request->pending = operation;
    if (operation != 0)
        XPORT_UNPLACED::FILE::callbackop(operation, callback);
}

extern "C" void loadfilesizecallback(uint32, uint32, void *userData)
{
    FUNCTION_MARKER(0x800F0E54, "NFS4.EXE");
    NativeAsyncRequest *request = static_cast<NativeAsyncRequest *>(userData);
    sint32 length = XPORT_UNPLACED::FILE::completeop(request->pending);
    uint32 operation;
    NativeFileCompletion callback;
    if (request->state != 0)
    {
        operation = XPORT_UNPLACED::FILE::close(request->fileHandle, 99, request);
        callback = loadfileclosecallback;
    }
    else
    {
        void *storage = reservememadr("ASYNCBUF", length, request->allocationFlags);
        request->storage = storage;
        request->cursor = static_cast<char *>(storage);
        operation = XPORT_UNPLACED::FILE::read(request->fileHandle, request->offset, request->cursor, asyncChunkSize, 99, request);
        callback = loadfilereadcallback;
    }
    request->pending = operation;
    if (operation != 0)
        XPORT_UNPLACED::FILE::callbackop(operation, callback);
}

extern "C" void loadfileopencallback(uint32, uint32, void *userData)
{
    FUNCTION_MARKER(0x800F0F18, "NFS4.EXE");
    NativeAsyncRequest *request = static_cast<NativeAsyncRequest *>(userData);
    sint32 file = XPORT_UNPLACED::FILE::completeop(request->pending);
    request->fileHandle = file;
    if (file == 0)
    {
        if (request->state != 0)
            cancelrequest(request);
        else
            finishrequest(request);
        return;
    }
    uint32 operation;
    NativeFileCompletion callback;
    if (request->state != 0)
    {
        operation = XPORT_UNPLACED::FILE::close(file, 99, request);
        callback = loadfileclosecallback;
    }
    else if (request->storage == 0)
    {
        operation = XPORT_UNPLACED::FILE::read(request->fileHandle, request->offset, request->cursor, asyncChunkSize, 99, request);
        callback = loadfilereadcallback;
    }
    else
    {
        operation = XPORT_UNPLACED::FILE::size(file, 99, request);
        callback = loadfilesizecallback;
    }
    request->pending = operation;
    if (operation != 0)
        XPORT_UNPLACED::FILE::callbackop(operation, callback);
}

extern "C" void loadsegreadcallback(uint32, uint32, void *userData)
{
    FUNCTION_MARKER(0x800F1024, "NFS4.EXE");
    NativeAsyncRequest *request = static_cast<NativeAsyncRequest *>(userData);
    sint32 bytes = XPORT_UNPLACED::FILE::completeop(request->pending);
    sint32 total = request->result;
    sint32 offset = request->offset;
    request->result = (sint32)((uint32)total + (uint32)bytes);
    sint32 state = request->state;
    offset = (sint32)((uint32)offset + (uint32)bytes);
    request->offset = offset;
    asyncFileOffset = offset;
    if (state != 0)
    {
        cancelrequest(request);
        return;
    }
    sint32 chunkSize = asyncChunkSize;
    if (bytes < chunkSize)
    {
        finishrequest(request);
        return;
    }
    request->remaining = (sint32)((uint32)request->remaining - (uint32)bytes);
    char *cursor = request->cursor;
    sint32 length = request->remaining;
    request->cursor = cursor + bytes;
    if (chunkSize < length)
        length = chunkSize;
    uint32 operation = XPORT_UNPLACED::FILE::read(asyncFile, request->offset, request->cursor, length, 99, request);
    request->pending = operation;
    if (operation != 0)
        XPORT_UNPLACED::FILE::callbackop(operation, loadsegreadcallback);
}

extern "C" sint32 asyncsystemtask(sint32, sint32)
{
    FUNCTION_MARKER(0x800F1120, "NFS4.EXE");
    NativeAsyncRequest *request;
    while ((request = queuefetch(&asyncCompletionQueue)) != 0)
    {
        if (request->state != 0)
            cancelrequest(request);
        else
        {
            request->callback(request->handle);
            request->handle = (uint8)request->handle;
            queueadd(&asyncFreeQueue, request);
        }
    }
    return 0;
}

extern "C" void initasync(sint32 count, sint32 chunkSize, uint32 flags)
{
    FUNCTION_MARKER(0x800F11B0, "NFS4.EXE");
    if (asyncRequests != 0 || count >= 257)
        return;
    asyncChunkSize = chunkSize;
    asyncRequestCount = count;
    asyncRequests = static_cast<NativeAsyncRequest *>(reservememadr("ASYNCREQ", (sint32)((uint32)count * 44), flags));
    asyncFreeQueue.head = asyncRequests;
    asyncFreeQueue.tail = asyncRequests + (count - 1);
    asyncCompletionQueue.head = 0;
    asyncCompletionQueue.tail = 0;
    asyncFile = 0;
    asyncMutex = allocmutex();
    NativeAsyncRequest *request = asyncRequests;
    for (sint32 index = 0; index < count; ++index, ++request)
    {
        request->handle = index;
        request->next = request + 1;
        request->pending = 0;
    }
    asyncRequests[count - 1].next = 0;
    addsystemtask(asyncsystemtask, 1, 1);
}

extern "C" uint32 asyncloadfilecallback(const char *name, uint32 flags, NativeAsyncCallback callback)
{
    FUNCTION_MARKER(0x800F12B0, "NFS4.EXE");
    NativeAsyncRequest *request = queuefetch(&asyncFreeQueue);
    if (request == 0)
        return 0;
    newrequestid(request);
    request->result = 0;
    request->state = 0;
    request->storage = reinterpret_cast<void *>(1);
    request->callback = callback;
    request->offset = 0;
    request->allocationFlags = flags;
    uint32 operation = XPORT_UNPLACED::FILE::open(name, 1, 100, request);
    request->pending = operation;
    if (operation == 0)
        return 0;
    XPORT_UNPLACED::FILE::callbackop(operation, loadfileopencallback);
    return request->handle;
}

extern "C" uint32 asyncloadfile(const char *name, uint32 flags)
{
    FUNCTION_MARKER(0x800F1368, "NFS4.EXE");
    return asyncloadfilecallback(name, flags, 0);
}

extern "C" uint32 asyncloadfileatcallback(const char *name, void *destination, NativeAsyncCallback callback)
{
    FUNCTION_MARKER(0x800F1388, "NFS4.EXE");
    NativeAsyncRequest *request = queuefetch(&asyncFreeQueue);
    if (request == 0)
        return 0;
    newrequestid(request);
    request->result = 0;
    request->state = 0;
    request->storage = 0;
    request->callback = callback;
    request->offset = 0;
    request->cursor = static_cast<char *>(destination);
    uint32 operation = XPORT_UNPLACED::FILE::open(name, 1, 100, request);
    request->pending = operation;
    if (operation == 0)
        return 0;
    XPORT_UNPLACED::FILE::callbackop(operation, loadfileopencallback);
    return request->handle;
}

extern "C" uint32 asyncloadfileat(const char *name, void *destination)
{
    FUNCTION_MARKER(0x800F143C, "NFS4.EXE");
    return asyncloadfileatcallback(name, destination, 0);
}

extern "C" void setasyncfile(const char *name)
{
    FUNCTION_MARKER(0x800F145C, "NFS4.EXE");
    if (asyncFile != 0)
        XPORT_UNPLACED::FILE::closesync(asyncFile, 100);
    if (name == 0)
        asyncFile = 0;
    else
    {
        XPORT_UNPLACED::FILE::opensync(name, 1, 100, &asyncFile);
        asyncFileOffset = 0;
    }
}

extern "C" uint32 asyncloadsegmentcallback(sint32 offset, void *destination, sint32 length, NativeAsyncCallback callback)
{
    FUNCTION_MARKER(0x800F14BC, "NFS4.EXE");
    NativeAsyncRequest *request = queuefetch(&asyncFreeQueue);
    if (request == 0)
        return 0;
    newrequestid(request);
    sint32 file = asyncFile;
    request->result = 0;
    request->state = 0;
    request->storage = 0;
    request->callback = callback;
    request->offset = offset;
    request->remaining = length;
    request->cursor = static_cast<char *>(destination);
    if (file == 0)
        finishrequest(request);
    else
    {
        sint32 chunkSize = asyncChunkSize;
        if (chunkSize < length)
            length = chunkSize;
        uint32 operation = XPORT_UNPLACED::FILE::read(file, request->offset, request->cursor, length, 100, request);
        request->pending = operation;
        if (operation == 0)
            return 0;
        XPORT_UNPLACED::FILE::callbackop(operation, loadsegreadcallback);
    }
    return request->handle;
}

extern "C" uint32 asyncloadsegment(sint32 offset, void *destination, sint32 length)
{
    FUNCTION_MARKER(0x800F15B0, "NFS4.EXE");
    return asyncloadsegmentcallback(offset, destination, length, 0);
}

extern "C" void cancelasyncload(uint32 handle)
{
    FUNCTION_MARKER(0x800F15D0, "NFS4.EXE");
    NativeAsyncRequest *request = locaterequest(handle);
    if (request == 0 || request->state != 0)
        return;
    uint32 operation = request->pending;
    request->state = 1;
    XPORT_UNPLACED::FILE::cancelop(operation);
    if (request->pending == 0 && request->callback == 0)
        cancelrequest(request);
}

extern "C" char *getasyncreadadr(uint32 handle)
{
    FUNCTION_MARKER(0x800F1640, "NFS4.EXE");
    NativeAsyncRequest *request = locaterequest(handle);
    if (request == 0 || request->state != 0 || request->pending != 0)
        return 0;
    char *address = static_cast<char *>(request->storage);
    if (reinterpret_cast<uintptr_t>(address) == 1)
        address = 0;
    if (request->storage != 0 && request->callback == 0)
    {
        uint8 index = (uint8)request->handle;
        request->handle = index;
        queueadd(&asyncFreeQueue, request);
    }
    return address;
}

extern "C" sint32 getasyncreadstatus(uint32 handle)
{
    FUNCTION_MARKER(0x800F16D8, "NFS4.EXE");
    NativeAsyncRequest *request = locaterequest(handle);
    if (request == 0 || request->state != 0)
        return -2;
    if (request->pending != 0)
        return 0;
    sint32 result = request->result;
    if (result == 0)
        result = -1;
    if (request->storage == 0 && request->callback == 0)
    {
        uint8 index = (uint8)request->handle;
        request->handle = index;
        queueadd(&asyncFreeQueue, request);
    }
    return result;
}
