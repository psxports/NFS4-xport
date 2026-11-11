#include "xport.h"
#include "eaclib/psx/internal.h"
#include "unbtree.h"

sint8 *SQVclue;
uint8 *SQVleft;
uint8 *SQVright;
const uint8 *SQVs;
uint8 *SQVd;

void chase(uint32 symbol)
{
    FUNCTION_MARKER(0x800F5530, "NFS4.EXE");
    sint32 index = symbol & 0xFFu;
    if (SQVclue[index] != 0)
    {
        chase(SQVleft[index]);
        chase(SQVright[index]);
    }
    else
    {
        *SQVd = (uint8)symbol;
        ++SQVd;
    }
}

sint32 unbtree(const uint8 *data, uint8 *destination)
{
    FUNCTION_MARKER(0x800F55B4, "NFS4.EXE");
    sint8 kinds[256];
    uint8 left[256];
    uint8 right[256];
    SQVclue = kinds;
    SQVleft = left;
    SQVs = data;
    SQVd = destination;
    SQVright = right;
    if (data == 0)
        return 0;
    uint32 first = *SQVs++;
    uint32 second = *SQVs++;
    if ((first << 8) + second == 0x47FBu)
        SQVs = data + 5;
    first = *SQVs++;
    second = *SQVs++;
    uint32 third = *SQVs++;
    sint32 size = ((sint32)((first << 16) + (second << 8) + third));
    for (sint32 i = 0; i < 256; ++i)
        SQVclue[i] = 0;
    uint32 escape = *SQVs++;
    SQVclue[escape] = 1;
    uint32 count = *SQVs++;
    for (uint32 i = 0; i < count; ++i)
    {
        uint32 index = *SQVs++;
        SQVleft[index] = *SQVs;
        ++SQVs;
        SQVright[index] = *SQVs;
        ++SQVs;
        SQVclue[index] = -1;
    }
    for (;;)
    {
        uint32 symbol = *SQVs++;
        sint32 kind = SQVclue[symbol];
        if (kind < 0)
        {
            chase(SQVleft[symbol]);
            chase(SQVright[symbol]);
            continue;
        }
        if (kind != 0)
        {
            symbol = *SQVs++;
            if (symbol == 0)
                return size;
        }
        *SQVd = (uint8)symbol;
        ++SQVd;
    }
}
