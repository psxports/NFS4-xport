#include "xport.h"
#include "eaclib/psx/internal.h"

uint8 *memcpyl(uint8 *destination, const uint8 *source, sint32 length)
{
    FUNCTION_MARKER(0x800F51C0, "NFS4.EXE");
    uint8 *end = destination + length;
    do
    {
        uint32 word = geti(source, 4);
        puti(destination, word, 4);
        destination += 4;
        length = (sint32)(((uint32)(length)) - 4u);
        source += 4;
    } while (length > 0);
    return end;
}

void memcpyb(uint8 *destination, const uint8 *source, uint32 length)
{
    FUNCTION_MARKER(0x800F5234, "NFS4.EXE");
    do
    {
        uint8 value = *source++;
        --length;
        *destination++ = value;
    } while (length != 0);
}

uint8 *refcpy(uint8 *destination, uint32 distance, sint32 length)
{
    FUNCTION_MARKER(0x800F5254, "NFS4.EXE");
    if (distance < 4)
    {
        uint8 *end = destination + length;
        if (distance == 1)
            memset(destination, destination[-1], ((uint32)(length)));
        else
            memcpyb(destination, destination - distance, ((uint32)(length)));
        return end;
    }
    return memcpyl(destination, destination - distance, length);
}

sint32 unrefpack(const uint8 *data, uint8 *destination, sint32 unpack)
{
    FUNCTION_MARKER(0x800F52B8, "NFS4.EXE");
    if (data == 0)
        return 0;
    uint32 flags = geti(data, 4);
    const uint8 *cursor = data + ((flags & 1u) != 0 ? 5 : 2);
    sint32 size = (cursor[0] << 16) + (cursor[1] << 8) + cursor[2];
    cursor += 3;
    if (unpack == 0)
        return size;
    uint8 saved[4];
    puti(saved, geti(destination + size, 4), 4);
    for (;;)
    {
        uint32 token = geti(cursor, 4);
        uint32 distance;
        sint32 length;
        if ((token & 0x80u) == 0)
        {
            cursor += 2;
            sint32 literals = token & 3u;
            puti(destination, geti(cursor, 4), 4);
            destination += literals;
            cursor += literals;
            distance = ((token << 3) & 0x300u) + ((token >> 8) & 0xFFu) + 1u;
            length = ((sint32)((token >> 2) & 7u)) + 3;
        }
        else if ((token & 0x40u) == 0)
        {
            cursor += 3;
            sint32 literals = (token >> 14) & 3u;
            puti(destination, geti(cursor, 4), 4);
            destination += literals;
            cursor += literals;
            distance = (token & 0x3F00u) + ((token >> 16) & 0xFFu) + 1u;
            length = ((sint32)(token & 0x3Fu)) + 4;
        }
        else if ((token & 0x20u) == 0)
        {
            cursor += 4;
            sint32 literals = token & 3u;
            puti(destination, geti(cursor, 4), 4);
            destination += literals;
            cursor += literals;
            distance = ((token << 12) & 0x10000u) + (token & 0xFF00u) + 1u + ((token >> 16) & 0xFFu);
            length = ((sint32)(((token << 6) & 0x300u) + (token >> 24))) + 5;
        }
        else
        {
            ++cursor;
            if ((token & 0xFFu) < 0xFCu)
            {
                sint32 literals = ((sint32)(((token & 0x1Fu) + 1u) << 2));
                memcpyl(destination, cursor, literals);
                destination += literals;
                cursor += literals;
                continue;
            }
            sint32 literals = token & 3u;
            while (literals != 0)
            {
                uint8 value = *cursor++;
                --literals;
                *destination++ = value;
            }
            puti(destination, geti(saved, 4), 4);
            return size;
        }
        destination = refcpy(destination, distance, length);
    }
}
