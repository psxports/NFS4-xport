#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDremovepatches(sint32 bank, sint32 count)
{
    FUNCTION_MARKER(0x800E65F8, "NFS4.EXE");
    uint32 sampleTable[256][2];
    for (sint32 i = 255; i >= 0; --i)
        sampleTable[i][0] = 0xFFFFFFFF;
    for (sint32 i = 0; i < count; ++i)
        iSNDbankremovepat(bank, i, sampleTable);
    return 0;
}

sint32 cSNDbankremove(sint32 bank, sint32 unused)
{
    FUNCTION_MARKER(0x800E6694, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    if (bank == -1)
    {
        for (sint32 i = 0; i < sndgs.options.set.maxbanks; ++i)
            cSNDbankremove(i, 0);
        return 0;
    }
    if (iSNDvalidbank(bank) != 0)
        return -8;
    NativeSoundBank *record = &sndgs.bankMemory[bank];
    uint8 *header = record->header;
    for (sint32 i = 0; i < sndgs.options.set.inputvoices; ++i)
    {
        SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[i];
        if (voice->unknown0A == bank)
            SNDstop(voice->playHandle);
    }
    record = &sndgs.bankMemory[bank];
    if (record->wholeSpuAllocation != 0)
    {
        sint32 address = record->storage.spuAddress;
        iSNDpsxfree(address);
        record = &sndgs.bankMemory[bank];
        record->wholeSpuAllocation = 0;
    }
    else
    {
        uint16 count;
        memcpy(&count, header + 6, sizeof(count));
        iSNDremovepatches(bank, count);
    }
    record = &sndgs.bankMemory[bank];
    record->header = 0;
    record = &sndgs.bankMemory[bank];
    record->unavailable = 0;
    return 0;
}
