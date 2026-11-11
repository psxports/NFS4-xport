#ifndef FASTRAND_H
#define FASTRAND_H

#include "xport.h"

class FastRandom
{
public:
    static void CleanUp(void);
    static void StartUp(uint32 seeder);
};

extern uint32 fastRandom;
extern uint32 randtemp;
extern uint32 randSeed;
extern uint32 numRandCalls;

#endif
