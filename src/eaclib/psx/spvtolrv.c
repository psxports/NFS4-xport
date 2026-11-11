#include "xport.h"
#include "eaclib/psx/internal.h"

void iSNDpvtolrv(sint32 pan, sint32 volume, sint32 *left, sint32 *right)
{
    FUNCTION_MARKER(0x8010C738, "NFS4.EXE");
    *left = volume;
    *right = volume;
    if (pan < 64)
        *right = (sint32)(((uint32)(volume)) * ((uint32)(pan))) >> 6;
    else if (pan > 64)
        *left = (sint32)(((uint32)(volume)) * (127u - ((uint32)(pan)))) >> 6;
}
