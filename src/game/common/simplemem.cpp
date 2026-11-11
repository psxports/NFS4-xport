#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/simplemem.h"

void *SimpleMem::Alloc(sint32 len, sint32 feign)
{
    FUNCTION_MARKER(0x800B7250, "NFS4.EXE");
    len = (sint32)(((uint32)len + 3U) & 0xfffffffcU);
    if (len > freeMemSize)
        return 0;
    void *ret = freeMem;
    if (feign == 0)
    {
        sint32 remaining = freeMemSize;
        freeMem = static_cast<uint8 *>(ret) + len;
        freeMemSize = (sint32)((uint32)remaining - (uint32)len);
    }
    return ret;
}

void *SimpleMem::FeignAlloc(sint32 len)
{
    FUNCTION_MARKER(0x800B729C, "NFS4.EXE");
    return Alloc(len, 1);
}

void SimpleMem::ResizeToFit(void)
{
    FUNCTION_MARKER(0x800B72BC, "NFS4.EXE");
    uint8 *end = static_cast<uint8 *>(freeMem);
    void *start = heap;
    freeMem = 0;
    sint32 newSize = (sint32)(end - static_cast<uint8 *>(start));
    resizememadr(start, newSize);
}
