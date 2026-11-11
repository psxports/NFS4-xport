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
    uint8 *record = ((uint8 *)(sndgs.bankMemory)) + (sint32)(((uint32)(handle)) * 12u);
    uint8 *header;
    memcpy(&header, record, sizeof(header));
    uint8 *copy = ((uint8 *)(destination));
    memcpy(copy, header, size);
    record = ((uint8 *)(sndgs.bankMemory)) + (sint32)(((uint32)(handle)) * 12u);
    memcpy(&header, record, sizeof(header));
    uint16 count;
    memcpy(&count, header + 6, sizeof(count));
    for (sint32 i = 0; i < count; ++i)
    {
        sint32 slot = (header[4] == 4 ? 20 : 12) + i * 4;
        uint8 *patch;
        memcpy(&patch, header + slot, sizeof(patch));
        if (patch != 0)
        {
            uint8 *relocated = copy + (patch - header);
            memcpy(copy + slot, &relocated, sizeof(relocated));
        }
        memcpy(&count, header + 6, sizeof(count));
    }
    record = ((uint8 *)(sndgs.bankMemory)) + (sint32)(((uint32)(handle)) * 12u);
    memcpy(record, &copy, sizeof(copy));
    return 0;
}
