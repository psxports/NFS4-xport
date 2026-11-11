#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDvalidbank(sint32 bank)
{
    FUNCTION_MARKER(0x800FE6BC, "NFS4.EXE");
    if (bank < 0 || bank >= sndgs.options.set.maxbanks)
        return -8;
    const uint8 *record = ((const uint8 *)(sndgs.bankMemory)) + bank * 12;
    uint32 occupied;
    memcpy(&occupied, record, sizeof(occupied));
    if (occupied == 0)
        return -8;
    if ((sint8)record[8] != 0)
        return -18;
    return 0;
}
