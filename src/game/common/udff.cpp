#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/udff.h"

Udff::tInfo * Udff::Opena(char *name, char *mem, sint32 abortFlag)
{
    FUNCTION_MARKER(0x800BBA08, "NFS4.EXE");

    (void)abortFlag;
    tInfo *newHandle = (tInfo *)reservememadr("udffhandle", sizeof(tInfo), 0x10);
    if (newHandle == 0)
        return 0;
    if (name == 0 && mem != 0)
    {
        newHandle->type = UDFF_MEMORY;
        newHandle->handle = 0;
        newHandle->memPtr = mem;
        return newHandle;
    }
    if (name != 0 && mem == 0)
    {
        newHandle->type = UDFF_FILE;
        char *data = (char *)loadfileadrz(name, 0);
        newHandle->memPtr = data;
        newHandle->handle = data;
        if (data != 0)
            return newHandle;
    }
    purgememadr(newHandle);
    return 0;
}

void Udff::Close(Udff::tInfo *handle)
{
    FUNCTION_MARKER(0x800BBAD0, "NFS4.EXE");

    if (handle != 0)
    {
        if (handle->type == UDFF_FILE && handle->handle != 0)
            purgememadr(handle->handle);
        purgememadr(handle);
    }
}

sint32 Udff::GetInt(Udff::tInfo *handle)
{
    FUNCTION_MARKER(0x800BBB24, "NFS4.EXE");

    char *mem = handle->memPtr;
    sint32 value;
    memcpy(&value, mem, sizeof(value));
    handle->memPtr = mem + 4;
    return value;
}

void Udff::GetBuffer(Udff::tInfo *handle, char *mem, sint32 size)
{
    FUNCTION_MARKER(0x800BBB3C, "NFS4.EXE");

    memcpy(mem, handle->memPtr, size);
    handle->memPtr += size;
}
