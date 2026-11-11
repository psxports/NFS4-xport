#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDbankheadersize(sint32 handle)
{
    FUNCTION_MARKER(0x800E7B64, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    const uint8 *record = ((const uint8 *)(sndgs.bankMemory)) + (sint32)(((uint32)(handle)) * 12u);
    const uint8 *header;
    memcpy(&header, record, sizeof(header));
    sint32 size;
    memcpy(&size, header + 8, sizeof(size));
    return size;
}
