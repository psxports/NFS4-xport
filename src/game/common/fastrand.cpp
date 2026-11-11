#include "xport.h"
#include "game/common/fastrand.h"

uint32 fastRandom = 4168396545U;
uint32 randtemp;
uint32 randSeed = 1U;
uint32 numRandCalls;

void FastRandom::CleanUp(void)
{
    FUNCTION_MARKER(0x8009B3C8, "NFS4.EXE");
}

void FastRandom::StartUp(uint32 seeder)
{
    FUNCTION_MARKER(0x8009B3D0, "NFS4.EXE");

    randSeed = 123456789;
    fastRandom = 987654321;
    sint32 seedIterations = seeder % 500;
    for (sint32 i = 0; i < seedIterations; ++i)
    {
        randtemp = fastRandom * 123456789;
        fastRandom = randtemp & 0xffff;
    }
}
