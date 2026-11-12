#include "xport.h"
#include "eaclib/psx/internal.h"

extern "C" void synccallback(uint32 operation, uint32 result, void *userData)
{
    FUNCTION_MARKER(0x800EA6CC, "NFS4.EXE");
    NativeSyncReadState *state = static_cast<NativeSyncReadState *>(userData);
    sint32 bytes = static_cast<sint32>(XPORT_UNPLACED::FILE::completeop(operation));
    state->pending = 0;
    if (result != 1)
    {
        state->remaining = 0;
        return;
    }
    state->offset = (state->offset + bytes);
    state->transferred = (state->transferred + bytes);
    char *cursor = state->cursor;
    sint32 chunkSize = state->chunkSize;
    state->cursor = cursor + bytes;
    if (bytes < chunkSize)
        state->remaining = 0;
    else
        state->remaining = (state->remaining - bytes);
    if (state->remaining <= 0)
        return;
    sint32 length = state->remaining;
    if (length > 8192)
        length = 8192;
    state->chunkSize = length;
    uint32 nextOperation = state->reader(state->fileHandle, state->offset, state->cursor, state->chunkSize, state->priority, state);
    state->pending = nextOperation;
    if (nextOperation != 0)
        XPORT_UNPLACED::FILE::callbackop(nextOperation, synccallback);
    else
        state->remaining = 0;
}

extern "C" sint32 syncblockio(NativeFileSlot *fileHandle, sint32 offset, void *destination, sint32 length, sint32 priority, NativeReadOperation reader)
{
    FUNCTION_MARKER(0x800EA7E8, "NFS4.EXE");
    NativeSyncReadState state;
    state.priority = priority;
    state.fileHandle = fileHandle;
    state.offset = offset;
    state.remaining = length;
    state.transferred = 0;
    state.chunkSize = length;
    state.cursor = static_cast<char *>(destination);
    state.reader = reader;
    if (state.chunkSize > 8192)
        state.chunkSize = 8192;
    state.pending = reader(fileHandle, offset, state.cursor, state.chunkSize, priority, &state);
    if (state.pending != 0)
    {
        XPORT_UNPLACED::FILE::callbackop(state.pending, synccallback);
        while (state.remaining != 0 || state.pending != 0)
            XPORT_UNPLACED::FILE::waitop(state.pending);
    }
    return state.transferred;
}

sint32 XPORT_UNPLACED::FILE::opensync(const char *filename, sint32 mode, sint32 priority, NativeFileSlot **handle)
{
    FUNCTION_MARKER(0x800EA8A8, "NFS4.EXE");
    uint32 operation = open(filename, mode, priority, 0);
    if (operation == 0)
    {
        *handle = 0;
        return 0;
    }
    waitop(operation);
    sint32 succeeded = opstatus(operation) == 1 ? 1 : 0;
    *handle = reinterpret_cast<NativeFileSlot *>(completeop(operation));
    return succeeded;
}

sint32 XPORT_UNPLACED::FILE::readsync(NativeFileSlot *handle, sint32 offset, void *destination, sint32 size, sint32 priority)
{
    FUNCTION_MARKER(0x800EA920, "NFS4.EXE");
    return syncblockio(handle, offset, destination, size, priority, read);
}

sint32 XPORT_UNPLACED::FILE::closesync(NativeFileSlot *handle, sint32 priority)
{
    FUNCTION_MARKER(0x800EA950, "NFS4.EXE");
    uint32 operation = close(handle, priority, 0);
    if (operation == 0)
        return 0;
    waitop(operation);
    return static_cast<sint32>(completeop(operation));
}

sint32 XPORT_UNPLACED::FILE::sizesync(NativeFileSlot *handle, sint32 priority)
{
    FUNCTION_MARKER(0x800EA9A4, "NFS4.EXE");
    uint32 operation = size(handle, priority, 0);
    if (operation == 0)
        return 0;
    waitop(operation);
    return static_cast<sint32>(completeop(operation));
}

sint32 XPORT_UNPLACED::FILE::addbigsync(const char *name, uint32 flags, sint32 priority, NativeFileSlot **handle)
{
    FUNCTION_MARKER(0x800EA9F8, "NFS4.EXE");
    uint32 operation = addbig(name, flags, priority, 0);
    if (operation == 0)
    {
        *handle = 0;
        return 0;
    }
    waitop(operation);
    sint32 succeeded = opstatus(operation) == 1 ? 1 : 0;
    *handle = reinterpret_cast<NativeFileSlot *>(completeop(operation));
    return succeeded;
}

sint32 XPORT_UNPLACED::FILE::delbigsync(NativeFileSlot *handle, sint32 priority)
{
    FUNCTION_MARKER(0x800EAA70, "NFS4.EXE");
    uint32 operation = delbig(handle, priority, 0);
    if (operation == 0)
        return 0;
    waitop(operation);
    return static_cast<sint32>(completeop(operation));
}
