#include "xport.h"
#include "game/common/frameclock.h"

FrameClock::FrameClock(void)
{
    processedTick = 0;
    lastTick = 0;
    seeded = false;
}

void FrameClock::Reset(uint64 tick)
{
    processedTick = tick;
    lastTick = tick;
    seeded = true;
}

uint32 FrameClock::Pending(uint64 tick, uint32 ticksPerStep, uint32 batchLimit, uint32 debtLimit, bool *reset)
{
    *reset = false;
    if (!seeded || tick < lastTick)
    {
        Reset(tick);
        *reset = true;
        return 0;
    }
    lastTick = tick;
    if (ticksPerStep == 0)
        return 0;
    uint64 pending = (tick - processedTick) / ticksPerStep;
    if (pending > debtLimit)
    {
        Reset(tick);
        *reset = true;
        return 0;
    }
    return pending < batchLimit ? (uint32)pending : batchLimit;
}

void FrameClock::Consume(uint32 ticksPerStep)
{
    processedTick += ticksPerStep;
}

uint64 FrameClock::ProcessedTick(void) const
{
    return processedTick;
}

float FrameClock::Phase(uint64 tick, float fraction, uint32 ticksPerStep) const
{
    if (!seeded || ticksPerStep == 0 || tick < processedTick)
        return 0;
    uint64 delta = tick - processedTick;
    if (delta >= ticksPerStep)
        return 1;
    if (!(fraction >= 0))
        fraction = 0;
    if (fraction > 1)
        fraction = 1;
    return ((float)delta + fraction) / ticksPerStep;
}
