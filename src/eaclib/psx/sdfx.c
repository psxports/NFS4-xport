#include "xport.h"
#include "eaclib/psx/internal.h"

static sint32 soundEffectDivide127(uint32 bits)
{
    sint32 value = (sint32)bits;
    sint64 product = (sint64)value * (sint32)0x81020409u;
    uint32 sum = (uint32)(sint32)(product >> 32) + bits;
    return (sint32)((uint32)((sint32)sum >> 6) - (uint32)(value >> 31));
}

void iSNDpsxfxinit(sint32 mode)
{
    FUNCTION_MARKER(0x8010020C, "NFS4.EXE");
    sint32 preset = mode;
    switch (mode)
    {
    case 5: preset = 1; break;
    case 10: preset = 2; break;
    case 20: preset = 3; break;
    case 30: preset = 4; break;
    case 40: preset = 5; break;
    case 50: preset = 6; break;
    case 100: preset = 9; break;
    case 110: preset = 8; break;
    case 120: preset = 7; break;
    }
    soundSpuMemory.limitUnit = (uint16)((65536 - (uint32)sndpe[preset][0]) >> 3);
    iSNDpsxeffectoff(0xFFFFFFu);
    iSNDpsxeffectvol(0, 0);
    uint16 row[33];
    blockmove(sndpe[preset], row, 66);
    if ((uint32)preset - 7u < 2u)
    {
        row[11] = (uint16)((uint32)soundEffectDivide127((uint32)sndgs.unknown164 << 13) - row[1]);
        row[12] = (uint16)((uint32)soundEffectDivide127((uint32)sndgs.unknown164 << 12) - row[2]);
        row[13] = (uint16)(row[14] + (uint32)soundEffectDivide127((uint32)sndgs.unknown164 << 12));
        row[17] = (uint16)(row[18] + (uint32)soundEffectDivide127((uint32)sndgs.unknown164 << 12));
        row[27] = (uint16)(row[29] + (uint32)soundEffectDivide127((uint32)sndgs.unknown164 << 12));
        row[28] = (uint16)(row[30] + (uint32)soundEffectDivide127((uint32)sndgs.unknown164 << 12));
        uint32 feedback = ((uint32)sndgs.unknown168 << 7) + (uint32)sndgs.unknown168;
        row[8] = (uint16)soundEffectDivide127(feedback << 8);
    }
    uint32 control;
    if (!spu_mmio_read(0, 0x1F801DAAu, 2, &control))
        abort();
    if (!spu_mmio_write(0, 0x1F801DAAu, 2, control & 0xFF7Fu, 3))
        abort();
    for (sint32 i = 0; i < 32; ++i)
        if (!spu_mmio_write(0, 0x1F801DC0u + (uint32)i * 2u, 2, row[i + 1], 3))
            abort();
    sint32 position = 65536 - row[0];
    if (!spu_mmio_write(0, 0x1F801DA2u, 2, (uint16)position, 3))
        abort();
    uint32 zeros[64];
    blockclear(zeros, 256);
    while (position < 65536)
    {
        sint32 units = 65536 - position;
        if (units > 32)
            units = 32;
        uint32 request = iSNDdmqueue(((const uint8 *)(zeros)), position << 3, units << 3, 1, 0);
        position += 32;
        while (iSNDdmcomplete(request) == 0)
        {
        }
    }
    if (!spu_mmio_read(0, 0x1F801DAAu, 2, &control))
        abort();
    if (!spu_mmio_write(0, 0x1F801DAAu, 2, control | 0x80u, 3))
        abort();
}

sint32 iSNDplatformfxinit(sint32 unused0, sint32 mode)
{
    FUNCTION_MARKER(0x80100584, "NFS4.EXE");
    sint32 initialized = sndgs.initialized;
    sndpd.fxMode = mode;
    if (initialized != 0)
        iSNDpsxfxinit(mode);
    return 0;
}

sint32 iSNDplatformfxmasterlevel(sint32 unused0, sint32 level)
{
    FUNCTION_MARKER(0x801005BC, "NFS4.EXE");
    uint32 scaled = ((uint32)level << 7) + (uint32)level;
    sint32 volume = (sint32)(scaled << 1);
    iSNDpsxeffectvol(volume, volume);
    return 0;
}
