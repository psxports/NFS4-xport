#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDbankremovepat(sint32 bank, sint32 patchIndex, void *sampleTable)
{
    FUNCTION_MARKER(0x800FE5B4, "NFS4.EXE");
    uint8 *record = ((uint8 *)(sndgs.bankMemory)) + (sint32)(((uint32)(bank)) * 12u);
    uint8 *header;
    memcpy(&header, record, sizeof(header));
    uint8 *version4 = header[4] == 4 ? header : 0;
    if (iSNDvalidbank(bank) != 0 || patchIndex < 0)
        return -8;
    uint16 count;
    memcpy(&count, header + 6, sizeof(count));
    if (patchIndex >= count)
        return -8;
    uint8 *slot = header[4] == 4 ? version4 + 20 + patchIndex * 4 : header + 12 + patchIndex * 4;
    uint8 *patch;
    memcpy(&patch, slot, sizeof(patch));
    if (patch == 0)
        return -8;
    iSNDremovetaggedpatch(patch, sampleTable);
    slot = header[4] == 4 ? version4 + 20 + patchIndex * 4 : header + 12 + patchIndex * 4;
    patch = 0;
    memcpy(slot, &patch, sizeof(patch));
    return 0;
}
