#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDvalidbank(sint32 bank)
{
    FUNCTION_MARKER(0x800FE6BC, "NFS4.EXE");
    if (bank < 0 || bank >= sndgs.options.set.maxbanks)
        return -8;
    const NativeSoundBank *record = &sndgs.bankMemory[bank];
    if (record->header == 0)
        return -8;
    if (record->unavailable != 0)
        return -18;
    return 0;
}
