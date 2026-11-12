#include <string.h>
#include "xport.h"
#include "lib/psx/shpsubs.h"

uint32 shapecount(const void *shape)
{
    FUNCTION_MARKER(0x800F0AAC, "NFS4.EXE");
    uint32 count;
    memcpy(&count, (const uint8 *)shape + 8, sizeof(count));
    return count;
}

void *shapepointer(void *shape, uint32 index)
{
    FUNCTION_MARKER(0x800F0AB8, "NFS4.EXE");
    uint32 count;
    memcpy(&count, (const uint8 *)shape + 8, sizeof(count));
    if (index < count)
    {
        uint32 offset;
        memcpy(&offset, (const uint8 *)shape + index * 8U + 0x14U, sizeof(offset));
        return (uint8 *)shape + offset;
    }
    return 0;
}

sint32 shapename(const void *shape, uint32 index, char *name)
{
    FUNCTION_MARKER(0x800F0AE0, "NFS4.EXE");
    uint32 count;
    uint32 value = 0;
    memcpy(&count, (const uint8 *)shape + 8, sizeof(count));
    if (index < count)
    {
        uint32 offset = index * 8 + 16U;
        memcpy(&value, (const uint8 *)shape + offset, sizeof(value));
    }
    memcpy(name, &value, sizeof(value));
    return 0;
}
