#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDpan(sint32 handle, sint32 pan)
{
    FUNCTION_MARKER(0x800E6884, "NFS4.EXE");
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
            if ((sint8)voice->inputPan == pan)
                break;
            sint32 adjusted = pan;
            sint32 factor = (sint8)voice->unknown33;
            voice->inputPan = (uint8)pan;
            if (factor != 1)
                adjusted = ((((pan) - 64) * ((factor))) + 64);
            adjusted = ((((((sint8)voice->unknown2E))) + (adjusted)) - 64);
            if (adjusted >= 128)
                adjusted = 127;
            else if (adjusted < 0)
                adjusted = 0;
            if (voice->unknown48 != 0)
                adjusted = voice->unknown48[adjusted];
            sint32 angle = ((((uint32)(adjusted)) - 64) << 8) & 0xFF00;
            iSNDplatform3dpos(cursor, angle, 0);
        }
    }
    iSNDleaveaudio();
    return channel;
}
