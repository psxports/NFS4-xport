#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDfxlevel(sint32 voice, sint32 bus, sint32 level)
{
    FUNCTION_MARKER(0x801005E8, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    XASSERT(bus == 0);
    iSNDenteraudio();
    sint32 channel = iSNDgetchan(voice);
    if (channel >= 0)
    {
        sint32 index = -1;
        const uint8 *masterState = ((const uint8 *)(&sndgs)) + 160 + ((uint32)bus << 4);
        while (iSNDpatchkey(channel, &index) != 0)
        {
            SoundVoicePrefix *current = ((SoundVoicePrefix *)(sndgs.voiceMemory)) + index;
            sint8 *effects = ((sint8 *)(current)) + 52 + bus;
            sint32 scale = effects[0];
            effects[1] = (sint8)level;
            sint32 master;
            memcpy(&master, masterState, 4);
            sint32 value = ((level * scale) * master);
            sint64 product = (sint64)value * (sint32)0x82061029;
            sint32 high = (sint32)(product >> 32);
            sint32 output = ((high + value) >> 13) - (value >> 31);
            iSNDplatformfxlevel(index, bus, output);
        }
    }
    iSNDleaveaudio();
    return channel;
}
