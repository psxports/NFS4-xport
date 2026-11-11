#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDmastervol(sint32 volume)
{
    FUNCTION_MARKER(0x800E7FF0, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    iSNDenteraudio();
    sint32 count = sndgs.options.set.inputvoices;
    sndgs.masterVolume = (uint8)volume;
    if (count != 0)
    {
        sint32 channel = 0;
        do
        {
            SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[channel];
            if (voice->state == 1 && voice->playHandle >= 0)
            {
                iSNDcalcvol(channel);
                iSNDvol(channel, voice->outputVolume);
            }
            ++channel;
        } while (channel < sndgs.options.set.inputvoices);
    }
    iSNDleaveaudio();
    return 0;
}
