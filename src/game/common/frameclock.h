#ifndef FRAMECLOCK_H
#define FRAMECLOCK_H

#include "xport.h"

class FrameClock
{
    uint64 processedTick;
    uint64 lastTick;
    bool seeded;

public:
    FrameClock(void);
    void Reset(uint64 tick);
    uint32 Pending(uint64 tick, uint32 ticksPerStep, uint32 batchLimit, uint32 debtLimit, bool *reset);
    void Consume(uint32 ticksPerStep);
    uint64 ProcessedTick(void) const;
    float Phase(uint64 tick, float fraction, uint32 ticksPerStep) const;
};

#endif
