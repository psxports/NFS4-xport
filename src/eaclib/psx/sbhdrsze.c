#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDbankheadersize(sint32 handle)
{
    FUNCTION_MARKER(0x800E7B64, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    const uint8 *header = sndgs.bankMemory[handle].header;
    sint32 size;
    memcpy(&size, header + 8, sizeof(size));
    return size;
}
