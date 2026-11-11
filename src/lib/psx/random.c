#include "xport.h"
#include "lib/psx/random.h"

uint32 seed[6] =
{
    0xF22D0E56U, 0x883126E9U, 0xC624DD2FU,
    0x0702C49CU, 0x9E353F7DU, 0x6FDF3B64U
};

uint32 random(void)
{
    FUNCTION_MARKER(0x800E77A8, "NFS4.EXE");
    uint32 next[6];
    next[5] = seed[5];
    uint32 carry = 0;
    for (sint32 i = 4; i >= 0; --i) {
        uint64 sum = (uint64)seed[i] + next[i + 1] + carry;
        next[i] = (uint32)sum;
        carry = (uint32)(sum >> 32);
    }
    for (sint32 i = 5; i >= 0; --i) {
        ++next[i];
        if (next[i] != 0)
            break;
    }
    for (sint32 i = 5; i >= 0; --i)
        seed[i] = next[i];
    return next[0];
}

void seedrandom(uint32 value)
{
    FUNCTION_MARKER(0x800E7878, "NFS4.EXE");
    seed[0] = value + 0xf22d0e56;
    seed[1] = value + 0x883126e9;
    seed[2] = value + 0xc624dd2f;
    seed[3] = value + 0x0702c49c;
    seed[4] = value + 0x9e353f7d;
    seed[5] = value + 0x6fdf3b64;
}
