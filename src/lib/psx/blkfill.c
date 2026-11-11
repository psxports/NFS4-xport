#include "xport.h"
#include "lib/psx/blkfill.h"

void blockclear(void *destination, sint32 length)
{
    FUNCTION_MARKER(0x800F17A0, "NFS4.EXE");

    blockfill(destination, length, 0);
}

void blockfill(void *destination, sint32 length, sint32 value)
{
    FUNCTION_MARKER(0x800F17A4, "NFS4.EXE");

    uint32 remaining = (uint32)length - 1;
    if ((sint32)remaining < 0)
        return;
    uint8 *bytes = (uint8 *)destination;
    do
    {
        *bytes++ = (uint8)value;
    } while (remaining-- != 0);
}
