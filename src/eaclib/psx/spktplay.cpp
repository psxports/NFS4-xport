#include "xport.h"
#include "eaclib/psx/internal.h"
#include "spktplay.h"

SoundPacketPlayPrefix *sndpps[1];

extern "C" sint32 iSNDpacketplayoverhead(sint32 chunks)
{
    FUNCTION_MARKER(0x801028BC, "NFS4.EXE");
    return (sint32)((uint32)chunks * 24 + 64);
}

extern "C" const uint8 *iSNDpacketget(sint32 handle, sint32 channel, uint32 *frames)
{
    FUNCTION_MARKER(0x80103248, "NFS4.EXE");
    SoundPacketPlayPrefix *packet = sndpps[handle];
    if ((uint32)channel + 1u == packet->format.channels && packet->current >= 0)
    {
        sint32 current = packet->current;
        packet->current = -1;
        SoundPacketReleaseCallback release = packet->release;
        if (release != 0)
        {
            SoundPacketRecord *records = reinterpret_cast<SoundPacketRecord *>(reinterpret_cast<uint8 *>(packet) + 40);
            release(records[current].channels[0]);
        }
    }
    if (packet->queued == 0)
        return 0;
    SoundPacketRecord *records = reinterpret_cast<SoundPacketRecord *>(reinterpret_cast<uint8 *>(packet) + 40);
    SoundPacketRecord *record = &records[packet->head];
    *frames = record->frames;
    if ((uint32)channel + 1u == packet->format.channels)
    {
        packet->current = packet->head;
        packet->queued = (sint16)(packet->queued - 1);
        packet->head = (sint16)(packet->head + 1);
        packet->inFlightFrames += record->frames;
        packet->frames -= record->frames;
        if (packet->head >= packet->capacity)
            packet->head = 0;
    }
    return static_cast<const uint8 *>(record->channels[channel]);
}

extern "C" void iSNDpacketfreeframes(sint32 handle, sint32 channel, uint32 frames)
{
    FUNCTION_MARKER(0x801033C4, "NFS4.EXE");
    SoundPacketPlayPrefix *packet = sndpps[handle];
    if (uint32(channel) + 1u == packet->format.channels)
    {
        SoundPacketNotifyCallback notify = packet->notify;
        packet->inFlightFrames -= frames;
        if (notify != 0)
            notify(handle, frames);
    }
}

sint32 SNDPKTPLAY::overhead(sint32 chunks)
{
    FUNCTION_MARKER(0x801028D0, "NFS4.EXE");
    sint32 packetBytes = iSNDpacketplayoverhead(chunks);
    sint32 platformBytes = iSNDplatformpacketoverhead();
    return (sint32)((uint32)packetBytes + (uint32)platformBytes);
}

sint32 SNDPKTPLAY::create(void *buffer, sint32 size, SoundPacketReleaseCallback release, SoundPacketNotifyCallback notify)
{
    FUNCTION_MARKER(0x801028FC, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    if (sndpps[0] != 0)
        return -9;
    if (iSNDplatformpacketplaycreate(0, buffer) < 0)
        return -6;
    uint8 *data = static_cast<uint8 *>(buffer);
    sint32 platformBytes = iSNDplatformpacketoverhead();
    SoundPacketPlayPrefix *packet = reinterpret_cast<SoundPacketPlayPrefix *>(data + platformBytes);
    platformBytes = iSNDplatformpacketoverhead();
    uint32 bytes = uint32(size) - uint32(platformBytes) - 64;
    sndpps[0] = packet;
    packet->platformBuffer = data;
    packet->release = release;
    packet->notify = notify;
    packet->capacity = (sint16)(bytes / 24);
    packet->playHandle = -1;
    return 0;
}

sint32 SNDPKTPLAY::start(sint32 handle, SNDSAMPLEFORMAT *format, void *attributes, SNDPLAYOPTS *options)
{
    FUNCTION_MARKER(0x80102A10, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    sint32 channels = format->channels;
    SoundPacketPlayPrefix *packet = sndpps[handle];
    iSNDenteraudio();
    SoundAttributesPrefix *attrs = static_cast<SoundAttributesPrefix *>(attributes);
    sint32 playHandle;
    sint32 channel = iSNDallocchan(uint32(attrs->unknown0), channels, (sint8)attrs->unknown6, &playHandle);
    if (channel < 0)
    {
        iSNDleaveaudio();
        return -9;
    }
    packet->playHandle = playHandle;
    packet->head = 0;
    packet->queued = 0;
    packet->frames = 0;
    packet->sequence = 0;
    packet->inFlightFrames = 0;
    packet->current = -1;
    memcpy(&packet->format, format, 4);
    SoundVoicePrefix *voice = &static_cast<SoundVoicePrefix *>(sndgs.voiceMemory)[channel];
    sint32 pan;
    if ((sint8)options->use3dpos != 0)
        pan = options->azimuth;
    else
        pan = (uint16)(((sint8)options->pan - 64) * 256);
    voice->unknown0A = -1;
    voice->basePitch = attrs->unknown4;
    uint16 pitchMultiplier = options->pitchmult;
    voice->volumeRamp = 0;
    voice->pitchMultiplier = pitchMultiplier;
    sint32 volume = (sint8)options->vol;
    voice->modulationDelta = 0;
    voice->modulationTicks = 0x7FFFFFFF;
    voice->modulationVolume = 0x7F0000;
    voice->volume = volume * 65536;
    voice->unknown2C = attrs->volume;
    voice->unknown2E = attrs->pan;
    uint8 bend = options->bend;
    voice->envelopeCount = 1;
    voice->envelopeIndex = 0;
    voice->unknown32 = 0;
    voice->unknown33 = 1;
    voice->unknown2F = bend;
    voice->effectScale = (sint8)attrs->unknown9;
    voice->effectLevel = (sint8)options->fxlevel0;
    sint32 adjustment = (sint8)attrs->unknown10;
    voice->unknown37 = 0;
    voice->unknown36 = 0;
    voice->unknown5A = (sint16)(adjustment * 100);
    uint8 inputPan = options->pan;
    voice->envelope = 0;
    voice->unknown44 = 0;
    voice->inputPan = inputPan;
    voice->unknown48 = 0;
    voice->unknown4C = 0;
    voice->unknown50 = 0;
    voice->unknown54 = 0;
    voice->pitchAdjustment = 0;
    iSNDcalcpitch(channel);
    iSNDcalcvol(channel);
    sint32 pitch = voice->pitch;
    sint32 outputVolume = voice->outputVolume;
    sint32 time = options->timemult;
    uint32 effect = uint32(sndgs.unknown160) * uint32(voice->effectScale);
    effect *= uint32(voice->effectLevel);
    sint32 result = iSNDplatformpacketplay(handle, channel, pan, outputVolume, pitch, time, (sint32)effect / 0x3F01, format, static_cast<uint8 *>(attributes) + 12);
    if (result < 0)
    {
        iSNDfreechan(channel);
        iSNDleaveaudio();
        return result;
    }
    iSNDleaveaudio();
    return packet->playHandle;
}

sint32 SNDPKTPLAY::submit(sint32 handle, const SoundPacketSubmit *data)
{
    FUNCTION_MARKER(0x80102CFC, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundPacketPlayPrefix *packet = sndpps[handle];
    iSNDenteraudio();
    sint32 result = -13;
    if (packet->queued < packet->capacity - 1)
    {
        sint32 index = packet->head + packet->queued;
        if (index >= packet->capacity)
            index -= packet->capacity;
        SoundPacketRecord *records = reinterpret_cast<SoundPacketRecord *>(reinterpret_cast<uint8 *>(packet) + 40);
        SoundPacketRecord *record = &records[index];
        record->frames = data->frames;
        record->id = (sint32)packet->sequence;
        for (sint32 i = 0; i < packet->format.channels; ++i)
            record->channels[i] = data->channels[i];
        packet->queued = (sint16)(packet->queued + 1);
        packet->frames += data->frames;
        result = (sint32)packet->sequence;
        ++packet->sequence;
    }
    iSNDleaveaudio();
    return result;
}

sint32 SNDPKTPLAY::submitspace(sint32 handle)
{
    FUNCTION_MARKER(0x80102E70, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundPacketPlayPrefix *packet = sndpps[handle];
    return packet->capacity - packet->queued - 1;
}

sint32 SNDPKTPLAY::unsafeframesoutstanding(sint32 handle)
{
    FUNCTION_MARKER(0x80102EC4, "NFS4.EXE");
    SoundPacketPlayPrefix *packet = sndpps[handle];
    return (sint32)(packet->frames + packet->inFlightFrames);
}

sint32 SNDPKTPLAY::framesoutstanding(sint32 handle)
{
    FUNCTION_MARKER(0x80102EEC, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    iSNDenteraudio();
    sint32 frames = unsafeframesoutstanding(handle);
    iSNDleaveaudio();
    return frames;
}

sint32 SNDPKTPLAY::purge(sint32 handle, sint32 first, sint32 last)
{
    FUNCTION_MARKER(0x80102F3C, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundPacketPlayPrefix *packet = sndpps[handle];
    iSNDenteraudio();
    sint32 writeIndex = packet->head;
    sint32 readIndex = packet->head;
    sint32 count = packet->queued;
    SoundPacketRecord *records = reinterpret_cast<SoundPacketRecord *>(reinterpret_cast<uint8 *>(packet) + 40);
    for (sint32 i = 0; i < count; ++i)
    {
        SoundPacketRecord *record = &records[readIndex];
        if (record->id < first || last < record->id)
        {
            records[writeIndex] = *record;
            ++writeIndex;
            if (writeIndex >= packet->capacity)
                writeIndex = 0;
        }
        else
        {
            packet->queued = (sint16)(packet->queued - 1);
            packet->frames -= record->frames;
            if (packet->release != 0)
                packet->release(record->channels[0]);
        }
        ++readIndex;
        if (readIndex >= packet->capacity)
            readIndex = 0;
    }
    iSNDleaveaudio();
    return 0;
}

sint32 SNDPKTPLAY::stop(sint32 handle)
{
    FUNCTION_MARKER(0x80103118, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundPacketPlayPrefix *packet = sndpps[handle];
    iSNDenteraudio();
    SNDstop(packet->playHandle);
    purge(handle, 0, 0x7FFFFFFF);
    if (packet->release != 0 && packet->current >= 0)
    {
        SoundPacketRecord *records = reinterpret_cast<SoundPacketRecord *>(reinterpret_cast<uint8 *>(packet) + 40);
        packet->release(records[packet->current].channels[0]);
    }
    packet->playHandle = -1;
    iSNDleaveaudio();
    return 0;
}

sint32 SNDPKTPLAY::destroy(sint32 handle)
{
    FUNCTION_MARKER(0x801031F4, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    iSNDplatformpacketplaydestroy(handle);
    sndpps[handle] = 0;
    return 0;
}
