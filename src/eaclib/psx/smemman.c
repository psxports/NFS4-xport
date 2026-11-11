#include "xport.h"
#include "eaclib/psx/internal.h"
#include "smemman.h"

SoundMemoryState sndmm;

void iSNDmemconstrain(sint32 *offset, sint32 *length)
{
    FUNCTION_MARKER(0x801061A8, "NFS4.EXE");
    sint32 start = *offset;
    sint32 available = *length;
    sint32 capacity = sndmm.capacityWords;
    if (capacity < (sint32)(((uint32)(start)) + ((uint32)(available))))
        *length = (sint32)(((uint32)(capacity)) - ((uint32)(start)));
}

void iSNDmeminit(void *memory, sint32 size)
{
    FUNCTION_MARKER(0x801061D4, "NFS4.EXE");
    sndmm.memory = ((uint8 *)(memory));
    sndmm.allocationCount = 0;
    sndmm.capacityWords = (uint16)(((uint32)(size)) >> 2);
    sndmm.highWaterWords = 0;
}

uint32 iSNDmemrestore(void)
{
    FUNCTION_MARKER(0x801061F4, "NFS4.EXE");
    uint32 used = ((uint32)(sndmm.highWaterWords)) * 100u;
    uint32 capacity = sndmm.capacityWords;
    if (capacity == 0)
        abort();
    return used / capacity;
}

void *iSNDmalloc(sint32 size)
{
    FUNCTION_MARKER(0x80106238, "NFS4.EXE");
    if (sndmm.allocationCount >= 128)
        return 0;
    sint32 rounded = (sint32)(((uint32)(size)) + 3u);
    sint32 words = rounded / 4;
    if (rounded < 0 && (((uint32)(rounded)) & 3u) != 0)
        --words;
    sint32 index = 0;
    sint32 offset = 0;
    sint32 length = 0;
    for (; index < sndmm.allocationCount; ++index)
    {
        if (index == 0)
            offset = 0;
        else
            offset = sndmm.allocations[index - 1].offsetWords + sndmm.allocations[index - 1].lengthWords;
        length = sndmm.allocations[index].offsetWords - offset;
        iSNDmemconstrain(&offset, &length);
        if (length >= words)
            break;
    }
    if (index == sndmm.allocationCount)
    {
        if (index == 0)
            offset = 0;
        else
            offset = sndmm.allocations[index - 1].offsetWords + sndmm.allocations[index - 1].lengthWords;
        length = sndmm.capacityWords - offset;
        iSNDmemconstrain(&offset, &length);
        if (length < words)
            return 0;
    }
    else
    {
        for (sint32 i = sndmm.allocationCount; i > index; --i)
            sndmm.allocations[i] = sndmm.allocations[i - 1];
    }
    sndmm.allocations[index].lengthWords = (uint16)words;
    sndmm.allocations[index].offsetWords = (uint16)offset;
    ++sndmm.allocationCount;
    uint8 *result = ((uint8 *)(((uintptr_t)(sndmm.memory)) + (((uint32)(offset)) << 2)));
    sint32 end = (sint32)(((uint32)(offset)) + ((uint32)(words)));
    if (sndmm.highWaterWords < end)
        sndmm.highWaterWords = end;
    return result;
}
