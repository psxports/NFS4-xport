#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 randrange(sint32 range)
{
    FUNCTION_MARKER(0x8010BCF4, "NFS4.EXE");
    if (range > 65536)
        range = 65536;
    else if (range < 0)
        range = 0;
    uint32 random = iSNDrandom();
    sint32 centered = (random & 32767) - 16384;
    return (centered * range) >> 14;
}
