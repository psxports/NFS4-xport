#include "xport.h"
#include "eaclib/psx/internal.h"

void iSPCH::InitSample(SPCHSample *sample)
{
    FUNCTION_MARKER(0x8010B5AC, "NFS4.EXE");
    sample->offset = -1;
    sample->size = 0;
    sample->ruleCount = 0;
    sample->rules[0] = 0xFF;
    sample->rules[1] = 0xFF;
    sample->rules[2] = 0xFF;
    sample->rules[3] = 0xFF;
}

sint32 iSPCH::UnPackSample(uint8 *bank, sint32 index, SPCHSample *sample)
{
    FUNCTION_MARKER(0x8010B5D4, "NFS4.EXE");
    InitSample(sample);
    if (index >= bank[3])
        return 0;
    sint32 rules = bank[2] & 15;
    sint32 stride = rules + 2;
    uint8 *record = bank + 8 + index * stride;
    sample->ruleCount = rules;
    for (sint32 i = 0; i < rules; ++i)
        sample->rules[i] = record[2 + i];
    if (record[0] & 0x80)
    {
        index = record[1];
        record = bank + 8 + index * stride;
    }
    sample->offset = ((record[0] << 8) + record[1]) << 8;
    uint8 *next = record + stride;
    sint32 nextIndex = index + 1;
    sint32 end;
    for (;;)
    {
        if (nextIndex >= bank[3])
        {
            end = (bank[4] + (bank[5] << 8)) << 8;
            break;
        }
        if (!(next[0] & 0x80))
        {
            end = ((next[0] << 8) + next[1]) << 8;
            break;
        }
        next += stride;
        ++nextIndex;
    }
    sample->size = end - sample->offset;
    return 1;
}
