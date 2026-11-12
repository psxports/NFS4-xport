#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDbankheadercopy(void *destination, sint32 handle)
{
    FUNCTION_MARKER(0x800E7BA8, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    sint32 size = SNDbankheadersize(handle);
    if (size < 0)
        return size;
    NativeSoundBank *record = &sndgs.bankMemory[handle];
    uint8 *copy = (uint8 *)destination;
    memcpy(copy, record->header, size);
    record->header = copy;
    return 0;
}
