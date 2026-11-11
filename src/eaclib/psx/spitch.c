#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDpitchmult(sint32 handle, sint32 multiplier)
{
    FUNCTION_MARKER(0x800E7F08, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    iSNDenteraudio();
    sint32 channel = iSNDgetchan(handle);
    if (channel >= 0)
    {
        sint32 cursor = -1;
        while (iSNDpatchkey(channel, &cursor) != 0)
        {
            SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[cursor];
            if (voice->pitchMultiplier == multiplier)
                break;
            voice->pitchMultiplier = (uint16)multiplier;
            iSNDcalcpitch(cursor);
            iSNDplatformpitch(cursor, voice->pitch);
        }
    }
    iSNDleaveaudio();
    return channel;
}
