#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDdownloadbank(uint8 *bank, const uint8 *sampleData)
{
    FUNCTION_MARKER(0x8010266C, "NFS4.EXE");
    uint32 sampleTable[256][2];
    for (sint32 i = 255; i >= 0; --i)
        sampleTable[i][0] = 0xFFFFFFFF;
    sint32 result = 7;
    uint16 count;
    memcpy(&count, bank + 6, sizeof(count));
    for (sint32 i = 0; i < count; ++i)
    {
        uint8 *slot = bank + (bank[4] == 4 ? 20 : 12) + i * 4;
        uint32 relative;
        memcpy(&relative, slot, sizeof(relative));
        if (relative != 0)
        {
            slot = bank + (bank[4] == 4 ? 20 : 12) + i * 4;
            memcpy(&relative, slot, sizeof(relative));
            uint8 *patch = slot + relative;
            memcpy(slot, &patch, sizeof(patch));
            if (iSNDresolvetaggedpatch(patch, sampleData, sampleTable) != 7)
                result = 8;
        }
        memcpy(&count, bank + 6, sizeof(count));
    }
    return result;
}
