#include "eaclib/psx/internal.h"

uint32 sndseed[6] = { 0xF22D0E56, 0x883126E9, 0xC624DD2F, 0x0702C49C, 0x9E353F7D, 0x6FDF3B64 };

uint32 iSNDrandom(void)
{
    FUNCTION_MARKER(0x8010BE80, "NFS4.EXE");
    uint32 low = sndseed[5] + sndseed[4];
    uint32 carry = low < sndseed[5] || low < sndseed[4];
    uint32 oldThird = sndseed[3];
    uint32 oldSecond = sndseed[2];
    uint32 oldFirst = sndseed[1];
    sndseed[4] = low;
    uint32 third = low + oldThird + carry;
    carry = third < oldThird;
    sndseed[3] = third;
    uint32 second = third + oldSecond + carry;
    carry = second < oldSecond;
    sndseed[2] = second;
    uint32 first = second + oldFirst + carry;
    carry = first < oldFirst;
    sndseed[1] = first;
    uint32 result = first + sndseed[0] + carry;
    sndseed[0] = result;
    if (++sndseed[5] == 0)
        if (++sndseed[4] == 0)
            if (++sndseed[3] == 0)
                if (++sndseed[2] == 0)
                    if (++sndseed[1] == 0)
                    {
                        ++result;
                        sndseed[0] = result;
                    }
    return result;
}
