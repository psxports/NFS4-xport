#include "xport.h"
#include "eaclib/psx/internal.h"

uint32 abortflag;
GameFileLoadCallback loadfilecallback;

NativeFileAtomicResult filesizeatomic(sint32 priority, void *userData)
{
    FUNCTION_MARKER(0x800E5608, "NFS4.EXE");
    NativeFileSizeRequest *request = static_cast<NativeFileSizeRequest *>(userData);
    NativeFileSlot *handle;
    sint32 succeeded = XPORT_UNPLACED::FILE::opensync(request->name, 1, priority, &handle);
    sint32 nestedPriority = (priority - 1);
    NativeFileAtomicResult result;
    result.value = 0;
    if (succeeded != 0)
    {
        result.value = XPORT_UNPLACED::FILE::sizesync(handle, nestedPriority);
        XPORT_UNPLACED::FILE::closesync(handle, nestedPriority);
    }
    return result;
}

extern "C" sint32 filesize(const char *name)
{
    FUNCTION_MARKER(0x800E566C, "NFS4.EXE");
    NativeFileSizeRequest request;
    request.name = name;
    NativeFileAtomicResult result = XPORT_UNPLACED::FILE::atomic(filesizeatomic, 0, 100, &request);
    return result.value;
}

NativeFileAtomicResult loadfileadratomic(sint32 priority, void *userData)
{
    FUNCTION_MARKER(0x800E56B0, "NFS4.EXE");
    NativeFileLoadRequest *request = static_cast<NativeFileLoadRequest *>(userData);
    NativeFileAtomicResult result;
    result.memory = 0;
    NativeFileSlot *file;
    sint32 opened = XPORT_UNPLACED::FILE::opensync(request->name, 1, priority, &file);
    sint32 nestedPriority = (priority - 1);
    if (opened == 0)
        return result;
    sint32 size = XPORT_UNPLACED::FILE::sizesync(file, nestedPriority);
    void *memory = reservememadr(request->name, size, request->flags);
    if (memory != 0)
    {
        XPORT_UNPLACED::FILE::readsync(file, 0, memory, size, nestedPriority);
        XPORT_UNPLACED::FILE::closesync(file, nestedPriority);
        if (loadfilecallback != 0)
        {
            void *converted = loadfilecallback(memory, request->name, request->flags);
            if (converted == 0)
                purgememadr(memory);
            memory = converted;
        }
        result.memory = memory;
    }
    else
        XPORT_UNPLACED::FILE::closesync(file, nestedPriority);
    return result;
}

extern "C" void *loadfileadrz(const char *name, uint32 flags)
{
    FUNCTION_MARKER(0x800E57A8, "NFS4.EXE");
    NativeFileLoadRequest request;
    request.name = name;
    request.flags = flags;
    request.abortFlags = 0;
    return XPORT_UNPLACED::FILE::atomic(loadfileadratomic, 0, 100, &request).memory;
}

extern "C" void *loadfileadr(const char *name, uint32 flags)
{
    FUNCTION_MARKER(0x800E57E8, "NFS4.EXE");
    NativeFileLoadRequest request;
    request.name = name;
    request.flags = flags;
    request.abortFlags = abortflag;
    return XPORT_UNPLACED::FILE::atomic(loadfileadratomic, 0, 100, &request).memory;
}

NativeFileAtomicResult loadfileatadratomic(sint32 priority, void *userData)
{
    FUNCTION_MARKER(0x800E5830, "NFS4.EXE");
    NativeFileLoadRequest *request = static_cast<NativeFileLoadRequest *>(userData);
    NativeFileAtomicResult result;
    result.memory = 0;
    NativeFileSlot *file;
    if (XPORT_UNPLACED::FILE::opensync(request->name, 1, priority, &file) == 0)
        return result;
    sint32 nestedPriority = (priority - 1);
    XPORT_UNPLACED::FILE::readsync(file, 0, request->destination, 0x7FFFFFFF, nestedPriority);
    XPORT_UNPLACED::FILE::closesync(file, nestedPriority);
    result.memory = request->destination;
    return result;
}

extern "C" void *loadfileatadrz(const char *name, void *destination)
{
    FUNCTION_MARKER(0x800E58B0, "NFS4.EXE");
    NativeFileLoadRequest request;
    request.name = name;
    request.destination = destination;
    request.abortFlags = 0;
    return XPORT_UNPLACED::FILE::atomic(loadfileatadratomic, 0, 100, &request).memory;
}

extern "C" void *loadfileatadr(const char *name, void *destination)
{
    FUNCTION_MARKER(0x800E58F0, "NFS4.EXE");
    NativeFileLoadRequest request;
    request.name = name;
    request.destination = destination;
    request.abortFlags = abortflag;
    return XPORT_UNPLACED::FILE::atomic(loadfileatadratomic, 0, 100, &request).memory;
}

NativeFileAtomicResult loadbigfileheaderatomic(sint32 priority, void *userData)
{
    FUNCTION_MARKER(0x800E5938, "NFS4.EXE");
    NativeFileLoadRequest *request = static_cast<NativeFileLoadRequest *>(userData);
    NativeFileAtomicResult result;
    result.memory = 0;
    NativeFileSlot *file;
    if (XPORT_UNPLACED::FILE::opensync(request->name, 1, priority, &file) == 0)
        return result;
    sint32 nestedPriority = (priority - 1);
    char *header = static_cast<char *>(reservememadr(request->name, 0xA90, request->flags));
    if (header != 0)
    {
        XPORT_UNPLACED::FILE::readsync(file, 0, header, 0xA90, nestedPriority);
        if (typeofbigfile(header) != 0)
        {
            uint32 size = sizeofbigfileheader(header);
            if (size <= 0xA90)
                result.memory = header;
            else
            {
                char *expanded = static_cast<char *>(reservememadr(request->name, size, request->flags));
                if (expanded != 0)
                {
                    blockmove(header, expanded, 0xA90);
                    purgememadr(header);
                    XPORT_UNPLACED::FILE::readsync(file, 0xA90, expanded + 0xA90, (sint32)(size - 0xA90), nestedPriority);
                    result.memory = expanded;
                }
            }
        }
        if (result.memory == 0)
            purgememadr(header);
    }
    XPORT_UNPLACED::FILE::closesync(file, nestedPriority);
    return result;
}

extern "C" char *loadbigfileheader(const char *filename, uint32 flags)
{
    FUNCTION_MARKER(0x800E5A7C, "NFS4.EXE");
    NativeFileLoadRequest request;
    request.name = filename;
    request.flags = flags;
    request.abortFlags = abortflag;
    return static_cast<char *>(XPORT_UNPLACED::FILE::atomic(loadbigfileheaderatomic, 0, 100, &request).memory);
}
