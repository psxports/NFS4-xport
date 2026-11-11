#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDplatformcdpanvol(sint32 pan, sint32 volume)
{
    FUNCTION_MARKER(0x801094EC, "NFS4.EXE");
    sint32 left;
    sint32 right;
    iSNDpvtolrv(pan, volume, &left, &right);
    uint32 leftScaled = ((uint32)left * 258) & 0x7FFF;
    if (!spu_mmio_write(0, 0x1F801DB0, 2, leftScaled, 3))
        abort();
    uint32 rightScaled = ((uint32)right * 258) & 0x7FFF;
    if (!spu_mmio_write(0, 0x1F801DB2, 2, rightScaled, 3))
        abort();
    return 0;
}
