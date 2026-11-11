#include "xport.h"
#include "eaclib/psx/internal.h"

void *locateshape(void *file, const char *name)
{
    FUNCTION_MARKER(0x800EB110, "NFS4.EXE");
    uint8 *base = ((uint8 *)(file));
    uint32 count;
    uint32 wanted;
    memcpy(&count, base + 8, 4);
    memcpy(&wanted, name, 4);
    if (count == 0)
        return 0;
    uint32 index = count - 1U;
    while (index != 0)
    {
        uint32 id;
        uint32 entryOffset = index * 8U + 16U;
        memcpy(&id, base + entryOffset, 4);
        if (id == wanted)
            break;
        --index;
    }
    uint32 entryOffset = index * 8U + 16U;
    uint32 id;
    memcpy(&id, base + entryOffset, 4);
    if (id != wanted)
        return 0;
    sint32 offset;
    memcpy(&offset, base + entryOffset + 4U, 4);
    return base + offset;
}
