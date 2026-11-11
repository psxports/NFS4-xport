#include "xport.h"
#include "eaclib/psx/internal.h"

void SNDfxinitbus(sint32 bus, sint32 level, sint32 effect, sint32 parameter0, sint32 parameter1)
{
    FUNCTION_MARKER(0x800E6D58, "NFS4.EXE");
    XASSERT(bus == 0);
    uint8 *state = ((uint8 *)(&sndgs)) + 156 + ((uint32)bus << 4);
    memcpy(state, &effect, 4);
    memcpy(state + 8, &parameter0, 4);
    memcpy(state + 12, &parameter1, 4);
    iSNDplatformfxinit(bus, effect);
    if (effect == 0)
        level = 0;
    SNDfxmasterlevel(bus, level);
}

sint32 SNDfxmasterlevel(sint32 bus, sint32 level)
{
    FUNCTION_MARKER(0x800E6DD0, "NFS4.EXE");
    XASSERT(bus == 0);
    uint8 *state = ((uint8 *)(&sndgs)) + 160 + ((uint32)bus << 4);
    memcpy(state, &level, 4);
    sint32 result = iSNDplatformfxmasterlevel(bus, level);
    if (result < 0)
        return result;
    if (sndgs.initialized != 0)
    {
        for (sint32 index = 0; index < sndgs.options.set.inputvoices; ++index)
        {
            SoundVoicePrefix *voice = ((SoundVoicePrefix *)(sndgs.voiceMemory)) + index;
            const sint8 *effects = ((const sint8 *)(voice)) + 53;
            SNDfxlevel(voice->playHandle, bus, effects[bus]);
        }
    }
    return 0;
}
