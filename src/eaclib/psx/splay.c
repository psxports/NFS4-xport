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
    (void)unused;
    if (iSNDvalidbank((sint8)options->bhandle) < 0)
        return -8;
    sint32 bankIndex = (sint8)options->bhandle;
    const uint8 *record = ((const uint8 *)(sndgs.bankMemory)) + bankIndex * 12;
    sint32 patchIndex = options->patnum;
    uint8 *bank;
    memcpy(&bank, record, sizeof(bank));
    if (patchIndex < 0)
        return -8;
    uint16 count;
    memcpy(&count, bank + 6, sizeof(count));
    if (patchIndex >= count)
        return -8;
    sint32 slot = (bank[4] == 4 ? 20 : 12) + patchIndex * 4;
    uint8 *patch;
    memcpy(&patch, bank + slot, sizeof(patch));
    return iSNDplaytaggedpatch(patch, options);
}
