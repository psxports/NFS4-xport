#include <string.h>
#include "xport.h"
#include "lib/psx/getm.h"

uint32 getm(const void *data, sint32 length)
{
    FUNCTION_MARKER(0x800F3024, "NFS4.EXE");

    const volatile uint8 *bytes = (const volatile uint8 *)data;
    uint32 first = bytes[0];
    uint32 second = bytes[1];
    if ((length - 3) < 0)
    {
        if ((length - 2) < 0)
            return first;
        return (first << 8) | second;
    }
    uint32 third = bytes[2];
    if (length == 3)
        return (first << 16) | (second << 8) | third;
    uint32 fourth = bytes[3];
    return (first << 24) | (second << 16) | (third << 8) | fourth;
}

uint32 geti(const void *data, sint32 length)
{
    FUNCTION_MARKER(0x800F308C, "NFS4.EXE");

    uint32 value;
    memcpy(&value, data, sizeof(value));
    uint32 shift = (32 - ((uint32)length << 3)) & 31;
    return (value << shift) >> shift;
}

void putm(void *data, uint32 value, sint32 length)
{
    FUNCTION_MARKER(0x800F30AC, "NFS4.EXE");

    uint8 *bytes = (uint8 *)data;
    sint32 remaining = (length - 1);
    while (remaining >= 0)
    {
        bytes[remaining] = value;
        value >>= 8;
        --remaining;
    }
}

void puti(void *data, uint32 value, sint32 length)
{
    FUNCTION_MARKER(0x800F30D4, "NFS4.EXE");

    uint8 *bytes = (uint8 *)data;
    sint32 remaining = (length - 1);
    while (remaining >= 0)
    {
        *bytes++ = value;
        value >>= 8;
        --remaining;
    }
}
