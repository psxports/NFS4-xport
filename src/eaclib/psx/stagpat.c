#include "xport.h"
#include "eaclib/psx/internal.h"
#include "stagpat.h"

uint8 soundGroupKey;
uint32 snddefaultenvelope[2] = { 0x7FFFFFFF, 127 };

void iSNDresetpatch(SoundTimbreParameters *parameters)
{
    FUNCTION_MARKER(0x80101ABC, "NFS4.EXE");
    parameters->values[36].value = 0;
}

void iSNDresettimbre(SoundTimbreParameters *parameters, SoundTimbreParameters *bases)
{
    FUNCTION_MARKER(0x80101AC4, "NFS4.EXE");
    for (sint32 i = 37; i >= 0; --i)
        bases->values[i].pointer = 0;
    parameters->values[5].value = 0xFFFFFFFF;
    parameters->values[8].value = 0xFFFFFFFF;
    parameters->values[7].value = 60;
    parameters->values[12].value = 64;
    parameters->values[1].value = 0;
    parameters->values[2].value = 127;
    parameters->values[3].value = 0;
    parameters->values[4].value = 127;
    parameters->values[6].value = 0;
    parameters->values[9].value = 1;
    parameters->values[10].value = 0;
    parameters->values[11].value = 1;
    parameters->values[13].value = 0;
    parameters->values[14].value = 127;
    parameters->values[15].value = 0;
    parameters->values[16].value = 0;
    parameters->values[17].value = 0;
    parameters->values[18].pointer = 0;
    parameters->values[19].value = 0;
    parameters->values[21].pointer = 0;
    parameters->values[23].pointer = 0;
    parameters->values[28].value = 127;
    parameters->values[29].pointer = 0;
    parameters->values[30].value = 0;
    parameters->values[31].value = 0;
    parameters->values[32].pointer = 0;
    parameters->values[33].value = 0;
    parameters->values[35].value = 0;
    parameters->values[34].value = 0;
    parameters->values[37].value = 1;
    parameters->values[25].pointer = snddefaultenvelope;
}

void iSNDresolveheader(SoundTimbreParameters *parameters, SoundTimbreParameters *bases)
{
    FUNCTION_MARKER(0x80101B7C, "NFS4.EXE");
    const sint32 pointerSlots[5] = { 18, 21, 23, 29, 32 };
    for (sint32 i = 0; i < 5; ++i)
    {
        sint32 slot = pointerSlots[i];
        if (bases->values[slot].pointer != 0)
        {
            sint32 relative = parameters->values[slot].value;
            parameters->values[slot].pointer = ((uint8 *)(bases->values[slot].pointer)) + relative;
        }
    }
    if (parameters->values[25].value != 0 && bases->values[25].pointer != 0)
    {
        sint32 relative = parameters->values[25].value;
        parameters->values[25].pointer = ((uint8 *)(bases->values[25].pointer)) + relative;
    }
}

sint32 iSNDfindfreekey(void)
{
    FUNCTION_MARKER(0x80101BFC, "NFS4.EXE");
    for (;;)
    {
        uint8 previous = soundGroupKey;
        soundGroupKey = (uint8)(previous + 1);
        if (soundGroupKey == 0)
            soundGroupKey = (uint8)(previous + 2);
        sint32 count = sndgs.options.set.inputvoices;
        if (count == 0)
            return soundGroupKey;
        uint8 candidate = soundGroupKey;
        SoundVoicePrefix *voices = ((SoundVoicePrefix *)(sndgs.voiceMemory));
        sint32 channel = 0;
        while (channel < count)
        {
            if (voices[channel].state != 0 && voices[channel].unknown37 == candidate)
                break;
            ++channel;
        }
        if (channel == count)
            return soundGroupKey;
    }
}

sint32 iSNDplaytaggedtimbre(uint8 *sample, SNDPLAYOPTS *options, SoundTimbreParameters *parameters, SoundTimbreParameters *bases, sint32 key, sint32 velocity, sint32 randomPitch)
{
    FUNCTION_MARKER(0x80101C8C, "NFS4.EXE");
    sint32 pan = (sint8)options->pan;
    sint32 volume = (sint8)options->vol;
    sint32 bend = (sint8)options->bend;
    sint32 effectLevel = (sint8)options->fxlevel0;
    iSNDresolveheader(parameters, bases);
    sint32 basePan = parameters->values[12].value;
    if (parameters->values[13].value != 0)
        basePan = (((basePan)) + (randrange((sint32)parameters->values[13].value)));
    if (basePan >= 128)
        basePan = 127;
    else if (basePan < 0)
        basePan = 0;
    sint32 playHandle;
    sint32 channel = iSNDallocchan(parameters->values[5].value, parameters->values[11].value, parameters->values[6].value, &playHandle);
    if (channel < 0)
        return channel;
    SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[channel];
    voice->unknown33 = (uint8)parameters->values[37].value;
    voice->basePitch = (uint16)parameters->values[16].value;
    uint32 keyDifference = parameters->values[7].value - ((key));
    voice->basePitch = (uint16)(voice->basePitch - keyDifference * 100);
    if (parameters->values[36].value != 0)
        voice->basePitch = (uint16)(voice->basePitch + ((uint32)(randomPitch)));
    if (parameters->values[17].value != 0)
        voice->basePitch = (uint16)(voice->basePitch + ((uint32)(randrange(parameters->values[17].value))));
    voice->unknown44 = ((const uint8 *)(parameters->values[18].pointer));
    voice->unknown48 = ((const sint8 *)(parameters->values[21].pointer));
    voice->volume = (((uint32)(volume)) << 16);
    voice->unknown4C = ((const sint8 *)(parameters->values[23].pointer));
    voice->volumeRamp = 0;
    voice->inputPan = (uint8)pan;
    sint32 panScale = (sint8)voice->unknown33;
    if (panScale != 1)
        pan = (pan - 64) * panScale + 64;
    pan = basePan + pan - 64;
    if (pan < 0)
        pan = 0;
    else if (pan >= 128)
        pan = 127;
    if (voice->unknown48 != 0)
        pan = voice->unknown48[pan];
    sint32 layerVolume = parameters->values[14].value;
    if (parameters->values[15].value != 0)
        layerVolume = (((layerVolume)) + (randrange((sint32)parameters->values[15].value)));
    if (layerVolume >= 128)
        layerVolume = 127;
    if (layerVolume < -128)
        layerVolume = -128;
    voice->unknown2E = (uint8)basePan;
    sint32 scaledVelocity = (((layerVolume)) * (velocity));
    voice->unknown2C = (uint8)(scaledVelocity / 127);
    voice->unknown2F = (uint8)bend;
    voice->unknown5A = (sint16)(parameters->values[10].value * 100);
    voice->envelope = parameters->values[25].pointer;
    voice->envelopeIndex = 0;
    voice->modulationVolume = (parameters->values[28].value << 16);
    voice->envelopeCount = (sint8)parameters->values[9].value;
    voice->unknown32 = (uint8)parameters->values[8].value;
    uint32 envelopeValue;
    memcpy(&envelopeValue, voice->envelope, sizeof(envelopeValue));
    voice->modulationTicks = envelopeValue;
    if (voice->modulationTicks < 0)
        voice->modulationTicks = 0x7FFFFFFF;
    memcpy(&envelopeValue, ((uint8 *)(voice->envelope)) + 4, sizeof(envelopeValue));
    sint32 delta = ((envelopeValue << 16) - ((voice->modulationVolume)));
    if (voice->modulationTicks == 0)
        abort();
    voice->modulationDelta = delta / voice->modulationTicks;
    voice->unknown50 = ((const sint8 *)(parameters->values[29].pointer));
    voice->unknown54 = ((const sint8 *)(parameters->values[32].pointer));
    voice->unknown38 = (uint8)parameters->values[30].value;
    voice->unknown39 = (uint8)parameters->values[33].value;
    voice->unknown58 = (uint8)parameters->values[34].value;
    if (parameters->values[31].value != 0)
    {
        uint32 random = iSNDrandom();
        uint32 count = parameters->values[30].value;
        if (count == 0)
            abort();
        voice->volumeEnvelopeIndex = (uint8)(random % count);
    }
    else
        voice->volumeEnvelopeIndex = 0;
    if (parameters->values[35].value != 0)
    {
        uint32 random = iSNDrandom();
        uint32 count = parameters->values[33].value;
        if (count == 0)
            abort();
        voice->pitchEnvelopeIndex = (uint8)(random % count);
    }
    else
        voice->pitchEnvelopeIndex = 0;
    memcpy(voice->unknown3E, &options->timemult, sizeof(options->timemult));
    voice->pitchAdjustment = 0;
    voice->pitchMultiplier = options->pitchmult;
    iSNDcalcpitch(channel);
    uint8 effectScale = (uint8)parameters->values[19].value;
    voice->effectLevel = (sint8)effectLevel;
    voice->effectScale = effectScale;
    voice->unknown0A = (sint8)options->bhandle;
    memcpy(voice->unknown04 + 4, &options->patnum, sizeof(uint16));
    iSNDcalcvol(channel);
    sint32 azimuth;
    sint32 elevation;
    if ((sint8)options->use3dpos != 0)
    {
        azimuth = options->azimuth;
        elevation = options->elevation;
    }
    else
    {
        azimuth = (((((uint32)(pan - 64)) << 8) & 0xFFFFu));
        elevation = 0;
    }
    sint32 timeMultiplier = options->timemult;
    uint32 effectVolume = ((sndgs.unknown160)) * ((uint32)(voice->effectScale));
    effectVolume *= ((uint32)(voice->effectLevel));
    sint32 result = iSNDplatformplay(sample, channel, voice->outputVolume, voice->pitch, timeMultiplier, (sint32)effectVolume / 0x3F01, azimuth, elevation);
    if (result < 0)
    {
        iSNDfreechan(channel);
        return result;
    }
    return playHandle;
}

sint32 iSNDplaytaggedpatch(uint8 *patch, SNDPLAYOPTS *options)
{
    FUNCTION_MARKER(0x8010218C, "NFS4.EXE");
    if (patch == 0)
        return -8;
    sint32 key = (sint8)options->keynum;
    sint32 velocity = (sint8)options->velocity;
    uint8 *sample = 0;
    uint8 *cursor = patch + ((patch[3] & 2) != 0 ? 8 : 4);
    SoundTimbreParameters parameters;
    SoundTimbreParameters bases;
    sint32 handles[12];
    sint32 created = 0;
    sint32 result = -9;
    sint32 randomPitch = 0;
    SoundVoicePrefix *lastVoice = 0;
    uint32 tag;
    uint32 value;
    uint8 *payload;
    iSNDresetpatch(&parameters);
    iSNDresettimbre(&parameters, &bases);
    iSNDenteraudio();
    sint32 groupKey = iSNDfindfreekey();
    while (iSNDgettag(&cursor, &tag, &value, &payload) != 0)
    {
        if (tag == 0xFD)
            sample = cursor;
        else if (tag == 0xFE)
        {
            if (velocity >= (sint32)parameters.values[1].value && velocity <= (sint32)parameters.values[2].value && key >= (sint32)parameters.values[3].value && key <= (sint32)parameters.values[4].value)
            {
                result = iSNDplaytaggedtimbre(sample, options, &parameters, &bases, key, velocity, randomPitch);
                if (result < 0)
                    goto fail;
                handles[created++] = result;
            }
            iSNDresettimbre(&parameters, &bases);
        }
        else if (tag < 38)
        {
            parameters.values[tag].value = value;
            bases.values[tag].pointer = payload;
            if (tag == 36)
                randomPitch = randrange(value);
        }
    }
    if (velocity >= (sint32)parameters.values[1].value && velocity <= (sint32)parameters.values[2].value && key >= (sint32)parameters.values[3].value && key <= (sint32)parameters.values[4].value)
    {
        result = iSNDplaytaggedtimbre(sample, options, &parameters, &bases, key, velocity, randomPitch);
        if (result < 0)
            goto fail;
        handles[created++] = result;
    }
    if (created != 0)
    {
        if (created == 1)
        {
            sint32 channel = iSNDgetchan(handles[0]);
            if (channel >= 0)
            {
                lastVoice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[channel];
                lastVoice->unknown37 = 0;
                lastVoice->unknown36 = 0;
            }
        }
        else
        {
            for (sint32 i = 0; i < created; ++i)
            {
                sint32 channel = iSNDgetchan(handles[i]);
                if (channel >= 0)
                {
                    lastVoice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[channel];
                    lastVoice->unknown37 = (uint8)groupKey;
                    lastVoice->unknown36 = 0;
                }
            }
            lastVoice->unknown36 = 1;
        }
        iSNDleaveaudio();
        return result;
    }
fail:
    iSNDleaveaudio();
    for (sint32 i = 0; i < created; ++i)
        SNDstop(handles[i]);
    return result;
}

sint32 iSNDresolvetaggedpatch(uint8 *patch, const uint8 *sampleData, void *sampleTable)
{
    FUNCTION_MARKER(0x801024EC, "NFS4.EXE");
    sint32 result = -1;
    uint8 *cursor = patch + ((patch[3] & 2) != 0 ? 8 : 4);
    uint32 tag;
    uint32 value;
    uint8 *payload;
    while (iSNDgettag(&cursor, &tag, &value, &payload) != 0)
    {
        if (tag == 0xFD)
            result = iSNDplatformresolve(cursor, sampleData, sampleTable);
    }
    if (result >= 0)
        patch[3] |= 1;
    return result;
}

sint32 iSNDremovetaggedpatch(uint8 *patch, void *sampleTable)
{
    FUNCTION_MARKER(0x801025C0, "NFS4.EXE");
    uint8 *cursor = patch + ((patch[3] & 2) != 0 ? 8 : 4);
    uint32 tag;
    uint32 value;
    uint8 *payload;
    while (iSNDgettag(&cursor, &tag, &value, &payload) != 0)
    {
        if (tag == 0xFD)
            iSNDplatformremove(cursor, sampleTable);
    }
    patch[3] &= 0xFE;
    return 0;
}
