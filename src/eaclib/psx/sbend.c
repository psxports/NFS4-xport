#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDunsafebend(sint32 handle, sint32 bend)
{
    FUNCTION_MARKER(0x800E78F8, "NFS4.EXE");
    sint32 channel = iSNDgetchan(handle);
    if (channel >= 0)
    {
        sint32 cursor = -1;
        while (iSNDpatchkey(channel, &cursor) != 0)
        {
            SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[cursor];
            if ((sint8)voice->unknown2F == bend || voice->unknown5A == 0)
                return 0;
            voice->unknown2F = (uint8)bend;
            voice->pitchAdjustment = 0;
            iSNDcalcpitch(cursor);
            iSNDplatformpitch(cursor, voice->pitch);
        }
    }
    return channel;
}

sint32 SNDpitchbend(sint32 handle, sint32 bend)
{
    FUNCTION_MARKER(0x800E79D0, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    iSNDenteraudio();
    sint32 result = iSNDunsafebend(handle, bend);
    iSNDleaveaudio();
    return result;
}
