#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDunsafevol(sint32 handle, sint32 volume)
{
    FUNCTION_MARKER(0x800E69D0, "NFS4.EXE");
    sint32 channel = iSNDgetchan(handle);
    if (channel >= 0)
    {
        sint32 cursor = -1;
        sint32 target = (sint32)(((uint32)(volume)) << 16);
        while (iSNDpatchkey(channel, &cursor) != 0)
        {
            SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[cursor];
            if (voice->volume == target)
                return 0;
            voice->volume = target;
            voice->volumeRamp = 0;
            iSNDcalcvol(cursor);
            iSNDvol(cursor, voice->outputVolume);
        }
    }
    return channel;
}

sint32 SNDvol(sint32 handle, sint32 volume)
{
    FUNCTION_MARKER(0x800E6A94, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    iSNDenteraudio();
    sint32 result = iSNDunsafevol(handle, volume);
    iSNDleaveaudio();
    return result;
}
