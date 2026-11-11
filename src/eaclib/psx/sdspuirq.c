#include "eaclib/psx/internal.h"

void iSNDpsxenablespuirq(void)
{
    FUNCTION_MARKER(0x8010BF80, "NFS4.EXE");
    uint32 control;
    if (!spu_mmio_read(0, 0x1F801DAAu, 2, &control))
        abort();
    if (!spu_mmio_write(0, 0x1F801DAAu, 2, control | 0x40u, 3))
        abort();
}

void iSNDpsxdisablespuirq(void)
{
    FUNCTION_MARKER(0x8010BFA4, "NFS4.EXE");
    uint32 control;
    if (!spu_mmio_read(0, 0x1F801DAAu, 2, &control))
        abort();
    if (!spu_mmio_write(0, 0x1F801DAAu, 2, control & 0xFFBFu, 3))
        abort();
}
