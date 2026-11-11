#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDgetdata(const uint8 *data, uint32 count)
{
    FUNCTION_MARKER(0x8010C928, "NFS4.EXE");
    uint32 value = 0;
    for (uint32 i = 0; i < count; ++i)
        value = (value << 8) + *data++;
    if (count == 1 && value >= 128)
        value -= 256;
    else if (count == 2 && value > 0x7FFF)
        value -= 0x10000;
    else if (count == 3 && value > 0x7FFFFF)
        value -= 0x1000000;
    return (sint32)value;
}
