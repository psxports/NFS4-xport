#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDgetvol(sint32 handle)
{
    FUNCTION_MARKER(0x80102810, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    iSNDenteraudio();
    sint32 channel = iSNDgetchan(handle);
    if (channel < 0)
    {
        iSNDleaveaudio();
        return -8;
    }
    iSNDleaveaudio();
    return ((SoundVoicePrefix *)(sndgs.voiceMemory))[channel].outputVolume;
}
