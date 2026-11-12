#include "xport.h"
#include "eaclib/psx/internal.h"

uint32 seedX[6] = { 0xF22D0E56, 0x883126E9, 0xC624DD2F, 0x0702C49C, 0x9E353F7D, 0x6FDF3B64 };

uint32 iSPCH::EACrandom(void)
{
    FUNCTION_MARKER(0x800EB9C4, "NFS4.EXE");
    uint32 value = seedX[5] + seedX[4];
    uint32 carry = value < seedX[5] || value < seedX[4];
    uint32 old3 = seedX[3];
    uint32 old2 = seedX[2];
    uint32 old1 = seedX[1];
    seedX[4] = value;
    value += old3 + carry;
    carry = value < old3;
    seedX[3] = value;
    value += old2 + carry;
    carry = value < old2;
    seedX[2] = value;
    value += old1 + carry;
    carry = value < old1;
    seedX[1] = value;
    value += seedX[0] + carry;
    seedX[0] = value;
    ++seedX[5];
    if (seedX[5] == 0)
    {
        ++seedX[4];
        if (seedX[4] == 0)
        {
            ++seedX[3];
            if (seedX[3] == 0)
            {
                ++seedX[2];
                if (seedX[2] == 0)
                {
                    ++seedX[1];
                    if (seedX[1] == 0)
                        ++seedX[0];
                }
            }
        }
    }
    return seedX[0];
}

void iSPCH::EACseedrandom(uint32 seed)
{
    FUNCTION_MARKER(0x800EBAC4, "NFS4.EXE");
    uint32 value = seed + 0xF22D0E56;
    seedX[0] = value;
    value += 0x96041893;
    seedX[1] = value;
    value += 0x3DF3B646u;
    seedX[2] = value;
    value += 0x40DDE76Du;
    seedX[3] = value;
    value += 0x97327AE1;
    seedX[4] = value;
    value += 0xD1A9FBE7;
    seedX[5] = value;
}

sint32 iSPCH::Rand(sint32 limit)
{
    FUNCTION_MARKER(0x800EBB30, "NFS4.EXE");
    sint32 value = EACrandom() & 0xFFFF;
    if (limit == 0)
        abort();
    return value % limit;
}
