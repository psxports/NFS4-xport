#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDplay(SNDPLAYOPTS *options)
{
    FUNCTION_MARKER(0x800E7A30, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    return cSNDplay(options, 0);
}

sint32 cSNDplay(SNDPLAYOPTS *options, sint32 unused)
{
    FUNCTION_MARKER(0x800E7A68, "NFS4.EXE");
    if (iSNDvalidbank((sint8)options->bhandle) < 0)
        return -8;
    sint32 bankIndex = (sint8)options->bhandle;
    sint32 patchIndex = options->patnum;
    uint8 *bank = sndgs.bankMemory[bankIndex].header;
    if (patchIndex < 0)
        return -8;
    uint16 count;
    memcpy(&count, bank + 6, sizeof(count));
    if (patchIndex >= count)
        return -8;
    sint32 slot = (bank[4] == 4 ? 20 : 12) + patchIndex * 4;
    uint32 relative;
    memcpy(&relative, bank + slot, sizeof(relative));
    uint8 *patch = relative != 0 ? bank + slot + relative : 0;
    return iSNDplaytaggedpatch(patch, options);
}
