#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDbankalloc(void)
{
    FUNCTION_MARKER(0x801027BC, "NFS4.EXE");
    sint32 count = sndgs.options.set.maxbanks;
    if (count == 0)
        return -9;
    const uint8 *bank = ((const uint8 *)(sndgs.bankMemory));
    for (sint32 i = 0; i < count; ++i, bank += 12)
    {
        uint32 state;
        memcpy(&state, bank, sizeof(state));
        if (state == 0)
            return i;
    }
    return -9;
}
