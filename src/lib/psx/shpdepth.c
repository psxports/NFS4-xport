#include "xport.h"
#include "lib/psx/shpdepth.h"

sint32 shapedepth(const void *shape)
{
    FUNCTION_MARKER(0x800F43E4, "NFS4.EXE");
    uint32 format = *(const uint8 *)shape & 0x77U;
    switch (format)
    {
    case 0x40:
        return 4;
    case 0x41:
    case 0x72:
        return 8;
    case 0x42:
    case 0x23:
        return 16;
    case 0x43:
        return 24;
    default:
        return 1;
    }
}
