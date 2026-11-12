#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDgettag(uint8 **cursor, uint32 *tag, uint32 *value, uint8 **payload)
{
    FUNCTION_MARKER(0x8010BD50, "NFS4.EXE");
    uint8 *current = *cursor;
    while (*current == 0xFC)
    {
        *cursor = current + 1;
        ++current;
    }
    uint32 tagByte = **cursor;
    *tag = tagByte;
    if (tagByte == 0xFF)
        return 0;
    current = *cursor;
    *cursor = current + 1;
    uint32 id = *tag;
    if (id == 0xFD || id == 0xFE)
        return 1;
    uint32 length = current[1];
    current += 2;
    if (length == 0xFF)
    {
        *cursor = current;
        length = iSNDgetdata(current, 4);
        *cursor += 3;
    }
    current = *cursor + 1;
    *cursor = current;
    *payload = current;
    if (length < 5) *value = iSNDgetdata(*cursor, length);
    *cursor = (uint8 *)((uintptr_t)*cursor + length);
    return 1;
}
