#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDbankadd(sint32 *handle, void *bank)
{
    FUNCTION_MARKER(0x800E7DEC, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    sint32 index = iSNDbankalloc();
    *handle = index;
    NativeSoundBank *record = &sndgs.bankMemory[index];
    record->header = (uint8 *)bank;
    record->storage.sampleData = (uint8 *)bank;
    sndgs.removeBanks = cSNDbankremove;
    return iSNDdownloadbank(((uint8 *)(bank)), ((uint8 *)(bank)));
}
