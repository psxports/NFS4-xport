#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDbankalloc(void)
{
    FUNCTION_MARKER(0x801027BC, "NFS4.EXE");
    sint32 count = sndgs.options.set.maxbanks;
    if (count == 0)
        return -9;
    const NativeSoundBank *bank = sndgs.bankMemory;
    for (sint32 i = 0; i < count; ++i, ++bank)
    {
        if (bank->header == 0)
            return i;
    }
    return -9;
}
