#include "xport.h"
#include "eaclib/psx/internal.h"
#include "ssysinit.h"

uint32 soundCapsInitialized;

sint32 SNDSYS::getopts(SNDSYSOPTS *options)
{
    FUNCTION_MARKER(0x800F1D58, "NFS4.EXE");
    sint32 result = 0;
    if (soundCapsInitialized == 0)
    {
        result = iSNDplatformoutputcaps();
        soundCapsInitialized = 1;
    }
    if (sndgs.options.set.maxbanks == 0)
        sndgs.options.set.maxbanks = 16;
    uint8 *source = reinterpret_cast<uint8 *>(&sndgs.options);
    uint8 *destination = reinterpret_cast<uint8 *>(options);
    for (sint32 i = 0; i < 48; i += 16)
    {
        uint8 block[16];
        memcpy(block, source + i, 16);
        memcpy(destination + i, block, 16);
    }
    uint8 last[12];
    memcpy(last, source + 48, 12);
    memcpy(destination + 48, last, 12);
    return result;
}

sint32 SNDSYS::setopts(SNDSYSOPTS *options)
{
    FUNCTION_MARKER(0x800F1E14, "NFS4.EXE");
    uint8 *source = reinterpret_cast<uint8 *>(options) + 12;
    uint8 *destination = reinterpret_cast<uint8 *>(&sndgs.options.set);
    for (sint32 i = 0; i < 32; i += 16)
    {
        uint8 block[16];
        memcpy(block, source + i, 16);
        memcpy(destination + i, block, 16);
    }
    uint8 last[12];
    memcpy(last, source + 32, 12);
    memcpy(destination + 32, last, 12);
    sndgs.options.vec.issurfacelocked = options->vec.issurfacelocked;
    iSNDplatformoutputset();
    return 0;
}

sint32 SNDSYS::init(void *heap, sint32 size)
{
    FUNCTION_MARKER(0x800F1F10, "NFS4.EXE");
    if (sndgs.initialized != 0)
        return 0;
    iSNDmeminit(heap, size);
    if (sndgs.options.set.inputvoices == 0)
    {
        getopts(&sndgs.options);
        setopts(&sndgs.options);
    }
    void *voices = iSNDmalloc(sndgs.options.set.inputvoices * 100);
    sint32 voiceBytes = sndgs.options.set.inputvoices * 100;
    sndgs.voiceMemory = voices;
    memset(voices, 0, voiceBytes);
    void *banks = iSNDmalloc(sndgs.options.set.maxbanks * 12);
    sint32 bankBytes = sndgs.options.set.maxbanks * 12;
    sndgs.bankMemory = banks;
    memset(banks, 0, bankBytes);
    if (sndgs.server100hz == 0)
        sndgs.server100hz = iSND100hzserver;
    sndgs.clock = 0;
    sndgs.unknown172 = 0;
    sndgs.masterVolume = 127;
    sint32 result = iSNDinit();
    if (result < 0)
    {
        iSNDrestore();
        return result;
    }
    sndgs.initialized = 1;
    return 0;
}

sint32 SNDSYS::restore(void)
{
    FUNCTION_MARKER(0x800F204C, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    if (sndgs.restoreHook140 != 0)
        sndgs.restoreHook140();
    if (sndgs.restoreHook144 != 0)
        sndgs.restoreHook144();
    if (sndgs.restoreHook132 != 0)
        sndgs.restoreHook132();
    if (sndgs.streamService != 0)
        sndgs.streamService();
    if (sndgs.restoreHook128 != 0)
        sndgs.restoreHook128();
    SNDstopall();
    if (sndgs.removeBanks != 0)
        sndgs.removeBanks(-1, 0);
    iSNDrestore();
    sndgs.initialized = 0;
    return (sint32)iSNDmemrestore();
}
