#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDplatform3dpos(sint32 channel, sint32 pan, sint32 elevation)
{
    FUNCTION_MARKER(0x800FEE30, "NFS4.EXE");
    (void)elevation;
    SoundPlatformChannel *voice = &sndpd.channels[channel];
    voice->pan = (uint16)pan;
    if (voice->state == 2)
    {
        SoundVoicePrefix *logical = &((SoundVoicePrefix *)(sndgs.voiceMemory))[channel];
        uint8 mode = voice->unknown1E;
        XASSERT(mode <= 1);
        sint32 left;
        sint32 right;
        if (mode == 1)
            iSNDatodlrv(pan, logical->outputVolume, &left, &right,
                (sint8)voice->unknown24[0], (sint8)voice->unknown24[1]);
        else
            iSNDatolrv(pan, logical->outputVolume, &left, &right);
        voice->unknown24[0] = (uint8)left;
        voice->unknown24[1] = (uint8)right;
        iSNDsetvol(channel, left, right);
    }
    return 0;
}
