#include "xport.h"
#include "eaclib/psx/internal.h"
#include "sdmemman.h"

void iSNDpsxmemconstrain(uint32 *offset, uint32 *length)
{
    FUNCTION_MARKER(0x8010A550, "NFS4.EXE");
    uint32 first = soundSpuMemory.firstUnit;
    uint32 position = *offset;
    if ((sint32)position < (sint32)first)
    {
        *offset = first;
        *length -= first - position;
        position = *offset;
    }
    uint32 available = *length;
    uint32 end = soundSpuMemory.endUnit;
    if ((sint32)end < (sint32)(position + available))
        *length = end - position;
    position = *offset;
    available = *length;
    uint32 limit = soundSpuMemory.limitUnit;
    if ((sint32)limit < (sint32)(position + available))
        *length = limit - position;
}

sint32 iSNDpsxmalloc(sint32 size)
{
    FUNCTION_MARKER(0x8010A5CC, "NFS4.EXE");
    if (soundSpuMemory.allocationCount >= 128)
        return 0;
    sint32 units = (sint32)(((uint32)(size)) + 63u) >> 6;
    sint32 index = 0;
    uint32 offset;
    uint32 length;
    for (; index < soundSpuMemory.allocationCount; ++index)
    {
        if (index == 0)
            offset = soundSpuMemory.firstUnit;
        else
            offset = soundSpuMemory.allocations[index - 1].offsetUnits + soundSpuMemory.allocations[index - 1].lengthUnits;
        length = ((uint32)(soundSpuMemory.allocations[index].offsetUnits)) - offset;
        iSNDpsxmemconstrain(&offset, &length);
        if ((sint32)length >= units)
            break;
    }
    if (index == soundSpuMemory.allocationCount)
    {
        if (index == 0)
            offset = soundSpuMemory.firstUnit;
        else
            offset = soundSpuMemory.allocations[index - 1].offsetUnits + soundSpuMemory.allocations[index - 1].lengthUnits;
        length = ((uint32)(soundSpuMemory.endUnit)) - offset;
        iSNDpsxmemconstrain(&offset, &length);
        if ((sint32)length < units)
            return 0;
    }
    else
    {
        for (sint32 i = soundSpuMemory.allocationCount; i > index; --i)
            soundSpuMemory.allocations[i] = soundSpuMemory.allocations[i - 1];
    }
    soundSpuMemory.allocations[index].lengthUnits = (uint16)units;
    soundSpuMemory.allocations[index].offsetUnits = (uint16)offset;
    ++soundSpuMemory.allocationCount;
    return (sint32)(offset << 6);
}

sint32 iSNDpsxfree(sint32 address)
{
    FUNCTION_MARKER(0x8010A7C8, "NFS4.EXE");
    sint32 offset = address >> 6;
    for (sint32 i = 0; i < soundSpuMemory.allocationCount; ++i)
    {
        if (soundSpuMemory.allocations[i].offsetUnits == offset)
        {
            --soundSpuMemory.allocationCount;
            while (i < soundSpuMemory.allocationCount)
            {
                soundSpuMemory.allocations[i] = soundSpuMemory.allocations[i + 1];
                ++i;
            }
            return 0;
        }
    }
    return -8;
}
