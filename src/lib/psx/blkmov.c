#include <string.h>
#include "xport.h"
#include "lib/psx/blkmov.h"

void blockmove(const void *source, void *destination, sint32 length)
{
    FUNCTION_MARKER(0x800E62DC, "NFS4.EXE");
    const uint8 *read = (const uint8 *)source;
    uint8 *write = (uint8 *)destination;
    uint32 words[8];
    uint32 remaining;
    if (length > 0 && (uintptr_t)read < (uintptr_t)write && (uintptr_t)write - (uintptr_t)read < (uint32)length)
    {
        read += length;
        write += length;
        remaining = (uint32)length - 16;
        while ((sint32)remaining >= 0)
        {
            memcpy(words, read - 16, 16);
            memcpy(write - 16, words, 16);
            read -= 16;
            write -= 16;
            remaining -= 16;
        }
        remaining += 12;
        while ((sint32)remaining >= 0)
        {
            memcpy(words, read - 4, 4);
            memcpy(write - 4, words, 4);
            read -= 4;
            write -= 4;
            remaining -= 4;
        }
        remaining += 3;
        if ((sint32)remaining >= 0)
        {
            do
            {
                --remaining;
                --read;
                --write;
                *write = *read;
            } while ((sint32)remaining >= 0);
        }
        return;
    }
    if ((((uintptr_t)read | (uintptr_t)write) & 3) == 0)
    {
        remaining = (uint32)length - 64;
        while ((sint32)remaining >= 0)
        {
            memcpy(words, read, 32);
            memcpy(write, words, 32);
            memcpy(words, read + 32, 32);
            memcpy(write + 32, words, 32);
            read += 64;
            write += 64;
            remaining -= 64;
        }
        remaining += 48;
    }
    else
        remaining = (uint32)length - 16;
    while ((sint32)remaining >= 0)
    {
        memcpy(words, read, 16);
        memcpy(write, words, 16);
        read += 16;
        write += 16;
        remaining -= 16;
    }
    remaining += 12;
    while ((sint32)remaining >= 0)
    {
        memcpy(words, read, 4);
        memcpy(write, words, 4);
        read += 4;
        write += 4;
        remaining -= 4;
    }
    remaining += 3;
    while ((sint32)remaining >= 0)
    {
        *write++ = *read++;
        --remaining;
    }
}
