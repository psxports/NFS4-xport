#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDplatformplay(uint8 *sample, sint32 channel, sint32 volume, sint32 pitch,
    sint32 unused0, sint32 effectVolume, sint32 azimuth, sint32 unused1)
{
    FUNCTION_MARKER(0x8010BA48, "NFS4.EXE");
    SoundPlatformChannel *voice = &sndpd.channels[channel];
    uint8 *sampleData = 0;
    uint32 sampleRate = 0x5622;
    uint32 sampleLength = 0;
    voice->unknown21 = 0;
    voice->partner = -1;
    voice->unknown1E = 0;
    voice->channelCount = 1;
    uint32 tag;
    uint32 value;
    uint8 *payload;
    while (iSNDgettag(&sample, &tag, &value, &payload) != 0 && tag != 0xFE)
    {
        if (tag == 0x8A)
            sampleData = payload;
        else if (tag == 0x85)
            sampleLength = value;
        else if (tag == 0x82)
            voice->channelCount = (uint8)value;
        else if (tag == 0x84)
            sampleRate = value;
        else if (tag == 0x92)
            voice->unknown1E = (uint8)value;
    }
    voice->sampleLength = sampleLength;
    voice->volume = (sint8)volume;
    voice->pan = (uint16)azimuth;
    voice->effectLevel = (sint8)effectVolume;
    memcpy(&voice->sampleAddress, sampleData, sizeof(voice->sampleAddress));
    voice->unknown0C = 0;
    voice->unknown10 = 0;
    voice->unknown14 = sampleLength << 12;
    voice->basePitch = (uint16)((sampleRate * 0x17C7u) >> 16);
    if (voice->channelCount >= 2)
    {
        SoundVoicePrefix *logical = ((SoundVoicePrefix *)(sndgs.voiceMemory));
        voice->partner = (sint8)logical[channel].unknown04[0];
        sndpd.channels[voice->partner].unknown21 = 1;
        sndpd.channels[voice->partner].unknown0C = 0;
        sndpd.channels[voice->partner].unknown10 = 0;
        sndpd.channels[voice->partner].unknown14 = voice->unknown14;
    }
    iSNDplatformpitch(channel, pitch);
    voice->state = 1;
    return 0;
}
