#include "xport.h"
#include "eaclib/psx/internal.h"

void iSNDcalcvol(sint32 channel)
{
    FUNCTION_MARKER(0x800FF050, "NFS4.EXE");
    SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[channel];
    sint32 volume = (sint16)(((uint32)(voice->volume)) >> 16);
    sint32 modulation = (sint16)(((uint32)(voice->modulationVolume)) >> 16);
    uint32 product = ((uint32)((sint8)voice->unknown2C)) * ((uint32)(volume));
    product *= ((uint32)(modulation));
    product *= ((uint32)((sint8)sndgs.masterVolume));
    sint32 result = (sint32)product / 0x1F417F;
    voice->outputVolume = (sint8)result;
    if (voice->unknown50 != 0)
    {
        sint32 envelope = voice->unknown50[voice->volumeEnvelopeIndex];
        voice->outputVolume = (sint8)(((sint8)result * envelope) / 127);
    }
    if (voice->unknown44 != 0)
        voice->outputVolume = (sint8)voice->unknown44[voice->outputVolume];
}
