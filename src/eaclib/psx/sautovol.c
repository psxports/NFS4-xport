#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDautovol(sint32 handle, sint32 ticks, sint32 volume)
{
    FUNCTION_MARKER(0x800E82D0, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    iSNDenteraudio();
    sint32 channel = iSNDgetchan(handle);
    if (channel >= 0)
    {
        if (ticks <= 0)
            ticks = 1;
        sint32 cursor = -1;
        uint32 target = ((uint32)(volume)) << 16;
        while (iSNDpatchkey(channel, &cursor) != 0)
        {
            SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[cursor];
            sint32 difference = (target - ((voice->volume)));
            sint32 ramp = difference / ticks;
            voice->volumeTarget = target;
            voice->volumeRamp = ramp;
        }
    }
    iSNDleaveaudio();
    return channel;
}
