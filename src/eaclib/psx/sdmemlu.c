#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDmemlargestunused(sint32 *address)
{
    FUNCTION_MARKER(0x800E7CEC, "NFS4.EXE");
    *address = 65;
    sint32 largest = 0;
    if (soundSpuMemory.allocationCount == 0)
        largest = ((sint32)(soundSpuMemory.limitUnit)) - 65;
    else
    {
        sint32 index = 0;
        for (; index < soundSpuMemory.allocationCount; ++index)
        {
            if (index == 0)
            {
                sint32 gap = ((sint32)(soundSpuMemory.allocations[0].offsetUnits)) - 65;
                if (largest < gap)
                    largest = gap;
            }
            else
            {
                sint32 start = soundSpuMemory.allocations[index - 1].offsetUnits + soundSpuMemory.allocations[index - 1].lengthUnits;
                sint32 gap = ((sint32)(soundSpuMemory.allocations[index].offsetUnits)) - start;
                if (largest < gap)
                {
                    largest = gap;
                    *address = start;
                }
            }
        }
        sint32 start = soundSpuMemory.allocations[index - 1].offsetUnits + soundSpuMemory.allocations[index - 1].lengthUnits;
        sint32 gap = ((sint32)(soundSpuMemory.limitUnit)) - start;
        if (largest < gap)
        {
            largest = gap;
            *address = start;
        }
    }
    *address = (sint32)(((uint32)(*address)) << 6);
    return (sint32)(((uint32)(largest)) << 6);
}
