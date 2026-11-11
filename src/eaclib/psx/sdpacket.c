#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 sndpp;

void iSNDpacketgetirq(void)
{
    FUNCTION_MARKER(0x80103784, "NFS4.EXE");
    SoundPlatformPacketBufferPrefix *packet = sndpd.packetBuffers[sndpp];
    if (packet->channel < 0)
        return;
    SoundPlatformChannel *voice = &sndpd.channels[packet->channel];
    uint32 position = voice->unknown0C >> 12;
    uint32 margin = packet->irqMargin;
    if (position > margin && position < packet->unknown0C - margin)
        return;
    if ((voice->unknown0C >> 11) < packet->unknown0C)
    {
        voice->unknown10 = voice->unknown0C;
        if (voice->partner >= 0)
            sndpd.channels[voice->partner].unknown10 = voice->unknown0C;
    }
    else
    {
        voice->unknown0C = 0;
        if (voice->partner >= 0)
            sndpd.channels[voice->partner].unknown0C = 0;
    }
}

void iSNDpacketsetirq(void)
{
    FUNCTION_MARKER(0x801038BC, "NFS4.EXE");
    iSNDpsxdisablespuirq();
    if (sndpd.unknown0[1] != 0)
        return;
    for (sint32 attempt = 0; attempt < 2; ++attempt)
    {
        sndpp = (sint32)((uint32)sndpp + 1u);
        if (sndpp > 0)
            sndpp = 0;
        SoundPlatformPacketBufferPrefix *packet = sndpd.packetBuffers[sndpp];
        if (packet != 0 && packet->channel >= 0)
        {
            uint16 address = (uint16)((sint32)((uint32)packet->spuAddress + 8u) >> 3);
            if (!spu_mmio_write(0, 0x1F801DA4u, 2, address, 3))
                abort();
            InterruptCallback(9, iSNDpacketirqcallback);
            iSNDpsxenablespuirq();
            return;
        }
    }
}

void iSNDpacketirqcallback(void)
{
    FUNCTION_MARKER(0x8010399C, "NFS4.EXE");
    iSNDpacketgetirq();
    iSNDpacketsetirq();
}

void iSNDpsxzerospu(uint32 *buffer, sint32 length)
{
    FUNCTION_MARKER(0x801039C4, "NFS4.EXE");
    sint32 words = length >> 2;
    for (sint32 word = 0; word < words; word += 4)
    {
        buffer[0] = 0x200;
        buffer[1] = 0;
        buffer[2] = 0;
        buffer[3] = 0;
        buffer += 4;
    }
}

void iSNDpacketpurgeframes(sint32 handle, uint32 position, uint32 frames)
{
    FUNCTION_MARKER(0x801039FC, "NFS4.EXE");
    SoundPlatformPacketBufferPrefix *packet = sndpd.packetBuffers[handle];
    SoundPlatformChannel *voice = &sndpd.channels[packet->channel];
    do
    {
        uint32 perPacket = packet->framesPerPacket;
        if (perPacket == 0)
            abort();
        uint32 index = position / perPacket;
        uint32 chunk = (index + 1u) * perPacket - position;
        if ((sint32)frames < (sint32)chunk)
            chunk = frames;
        uint16 *counts = ((uint16 *)(packet->data));
        uint32 available = counts[index];
        uint32 consumed = chunk;
        if ((sint32)available < (sint32)consumed)
            consumed = available;
        counts[index] = (uint16)(available - consumed);
        if (consumed != 0)
            for (sint32 channel = 0; channel < voice->channelCount; ++channel)
                iSNDpacketfreeframes(handle, channel, consumed);
        position += chunk;
        frames -= chunk;
        if (position >= packet->unknown0C)
            position = 0;
    } while ((sint32)frames > 0);
}

void iSNDfillspuwithpackets(sint32 handle, uint32 packetIndex)
{
    FUNCTION_MARKER(0x80103B54, "NFS4.EXE");
    SoundPlatformPacketBufferPrefix *packet = sndpd.packetBuffers[handle];
    SoundPlatformChannel *voice = &sndpd.channels[packet->channel];
    if (packet->unknown3C == 0)
    {
        packet->unknown3E = 0;
        packet->unknown3C = packet->packetSize;
        ((uint16 *)(packet->data))[packetIndex] = 0;
    }
    if (packet->writtenFrames == 0)
    {
        for (sint32 channel = 0; channel < voice->channelCount; ++channel)
            iSNDpsxzerospu(((uint32 *)(packet->work[channel])), 16);
        packet->unknown3E = (uint16)(packet->unknown3E + 16);
        packet->unknown3C = (uint16)(packet->unknown3C - 16);
    }
    while (packet->unknown3C != 0)
    {
        if (packet->unknown34 == 0)
        {
            uint32 frames;
            for (sint32 channel = 0; channel < voice->channelCount; ++channel)
                packet->packetData[channel] = iSNDpacketget(handle, channel, &frames);
            if (packet->packetData[0] == 0)
            {
                if ((sint32)(packet->writtenFrames - packet->playedFrames) > packet->irqMargin)
                    return;
                if (packet->unknown36 > packet->packetCount)
                    goto advanceWritten;
                if (packet->unknown36 < 2)
                {
                    for (sint32 channel = 0; channel < voice->channelCount; ++channel)
                        iSNDpsxzerospu(((uint32 *)(packet->work[channel] + packet->unknown3E)), packet->unknown3C);
                }
                else
                {
                    for (sint32 channel = 0; channel < voice->channelCount; ++channel)
                    {
                        packet->work[channel][1] = 2;
                        packet->work[channel][packet->packetSize - 15] = 2;
                    }
                }
                if (packet->unknown3E == 0)
                    packet->unknown36 = (uint16)(packet->unknown36 + 1);
                packet->unknown3C = 0;
                break;
            }
            sint32 scaled = (sint32)(frames << 2);
            sint64 product = (sint64)scaled * (sint32)0x92492493u;
            uint32 sum = (uint32)(sint32)(product >> 32) + (uint32)scaled;
            uint32 bytes = (uint32)((sint32)sum >> 2) - (uint32)(scaled >> 31);
            packet->unknown36 = 0;
            packet->unknown32 = 0;
            packet->unknown34 = (uint16)bytes;
            packet->unknown30 = (uint16)bytes;
        }
        uint32 chunk = packet->unknown34;
        if (chunk >= packet->unknown3C)
            chunk = packet->unknown3C;
        uint16 *counts = ((uint16 *)(packet->data));
        counts[packetIndex] = (uint16)(counts[packetIndex] + (chunk * 7u >> 2));
        for (sint32 channel = 0; channel < voice->channelCount; ++channel)
        {
            const uint8 *source = packet->packetData[0] + packet->unknown32 + packet->unknown30 * channel;
            blockmove(source, packet->work[channel] + packet->unknown3E, (sint32)chunk);
        }
        packet->unknown3E = (uint16)(packet->unknown3E + chunk);
        packet->unknown32 = (uint16)(packet->unknown32 + chunk);
        packet->unknown3C = (uint16)(packet->unknown3C - chunk);
        packet->unknown34 = (uint16)(packet->unknown34 - chunk);
    }
    if (packetIndex == 0)
    {
        for (sint32 channel = 0; channel < voice->channelCount; ++channel)
            packet->work[channel][1] |= 4;
    }
    else if (packetIndex == (uint32)packet->packetCount - 1u)
    {
        for (sint32 channel = 0; channel < voice->channelCount; ++channel)
            packet->work[channel][packet->packetSize - 15] |= 1;
    }
    for (sint32 channel = 0; channel < voice->channelCount; ++channel)
    {
        uint32 address = (uint32)packet->spuAddress + (packetIndex << (packet->shift & 31)) +
            packet->rightOffset * (uint32)channel;
        sint32 notify = packetIndex == 0 && channel == 0;
        packet->dmaRequest = iSNDdmqueue(packet->work[channel], (sint32)address, packet->packetSize, 2, notify);
    }
advanceWritten:
    packet->packetCurrent = (uint16)packetIndex;
    packet->writtenFrames += packet->framesPerPacket;
}

void iSNDpacketserve(void)
{
    FUNCTION_MARKER(0x80104024, "NFS4.EXE");
    iSNDstreamhotroddatachunks();
    for (sint32 handle = 0; handle < 1; ++handle)
    {
        SoundPlatformPacketBufferPrefix *packet = sndpd.packetBuffers[handle];
        if (packet == 0 || packet->channel < 0)
            continue;
        uint32 position = 0;
        if (sndpd.channels[packet->channel].unknown1D == 2)
            position = (uint32)(spu_voice_sample_position(packet->channel) % packet->unknown0C);
        position -= position % 28u;
        uint32 delta;
        if ((sint32)position < (sint32)packet->lastPosition)
            delta = position + packet->unknown0C - packet->lastPosition;
        else
            delta = position - packet->lastPosition;
        if ((sint32)packet->playedFrames > 0x70FFFFFF)
        {
            packet->playedFrames += 0x90000000u;
            packet->writtenFrames += 0x90000000u;
        }
        packet->playedFrames += delta;
        iSNDpacketpurgeframes(handle, packet->lastPosition, delta);
        bool refill = (sint32)(packet->writtenFrames - packet->playedFrames) < (sint32)packet->refillThreshold;
        packet->lastPosition = position;
        if (refill && iSNDdmcomplete(packet->dmaRequest) != 0)
        {
            uint32 next = (uint32)packet->packetCurrent + 1u;
            if (next >= packet->packetCount)
                next -= packet->packetCount;
            iSNDfillspuwithpackets(handle, next);
        }
    }
}

sint32 iSNDplatformpacketoverhead(void)
{
    FUNCTION_MARKER(0x801041B8, "NFS4.EXE");
    sint32 blocks = sndpsxlimits.packetbufsize / 4096;
    return (sint32)((((uint32)blocks + 3) & ~3) * 2 + 0x1050);
}

sint32 iSNDplatformcalcdatarate(const SNDSAMPLEFORMAT *format)
{
    FUNCTION_MARKER(0x801041EC, "NFS4.EXE");
    uint32 scaled = (uint32)format->samplerate * format->channels * 146;
    return (sint32)scaled >> 8;
}

sint32 iSNDplatformpacketplaycreate(sint32 handle, void *buffer)
{
    FUNCTION_MARKER(0x8010421C, "NFS4.EXE");
    SoundPlatformPacketBufferPrefix *packet = ((SoundPlatformPacketBufferPrefix *)(buffer));
    sndpd.stopCallback = iSNDpsxpacketstop;
    packet->channel = -1;
    packet->data = ((uint8 *)(buffer)) + 0x1050;
    packet->spuAddress = iSNDpsxmalloc(sndpsxlimits.packetbufsize);
    packet->bufferSize = sndpsxlimits.packetbufsize;
    if (packet->spuAddress == 0)
        return -9;
    sndpd.packetBuffers[handle] = packet;
    return 0;
}

void iSNDplatformpacketplaydestroy(sint32 handle)
{
    FUNCTION_MARKER(0x801042C0, "NFS4.EXE");
    iSNDpsxfree(sndpd.packetBuffers[handle]->spuAddress);
    sndpd.packetBuffers[handle] = 0;
}

sint32 iSNDplatformpacketplay(sint32 handle, sint32 channel, sint32 pan, sint32 volume, sint32 pitch,
    sint32 unused0, sint32 effect, SNDSAMPLEFORMAT *format, void * unused1)
{
    FUNCTION_MARKER(0x80104304, "NFS4.EXE");
    SoundPlatformChannel *voice = &sndpd.channels[channel];
    SoundPlatformPacketBufferPrefix *packet = sndpd.packetBuffers[handle];
    voice->unknown21 = 0;
    voice->partner = -1;
    voice->unknown1E = 0;
    voice->channelCount = format->channels;
    voice->sampleLength = 0;
    voice->pan = (uint16)pan;
    voice->volume = (sint8)volume;
    voice->effectLevel = (sint8)effect;
    voice->sampleAddress = (uint32)packet->spuAddress;
    voice->stream = (sint8)handle;
    sint32 channels = voice->channelCount;
    if (channels == 0)
        abort();
    packet->packetSize = (uint16)(4096 / channels);
    packet->framesPerPacket = (uint16)((packet->packetSize * 28) / 16);
    packet->work[0] = ((uint8 *)(packet)) + 0x50;
    packet->shift = (uint8)(13 - voice->channelCount);
    packet->work[1] = ((uint8 *)(packet)) + 0x50 + packet->packetSize;
    packet->packetCount = (uint16)(packet->bufferSize >> (packet->shift & 31));
    channels = voice->channelCount;
    if (channels == 0)
        abort();
    packet->packetCount = (uint16)(packet->packetCount / channels);
    uint32 capacity = (uint32)packet->packetCount * packet->framesPerPacket;
    packet->irqMargin = packet->framesPerPacket;
    packet->channel = (sint8)channel;
    packet->unknown36 = 0;
    packet->unknown30 = 0;
    packet->unknown34 = 0;
    packet->unknown32 = 0;
    packet->unknown3C = 0;
    packet->packetData[0] = 0;
    packet->dmaRequest = 0;
    packet->lastPosition = 0;
    packet->playedFrames = 0;
    packet->writtenFrames = 0;
    packet->rightOffset = (uint32)packet->packetCount << (packet->shift & 31);
    packet->unknown0C = capacity;
    packet->refillThreshold = capacity - packet->framesPerPacket;
    for (sint32 i = 0; i < 2; ++i)
    {
        while (iSNDdmcomplete(packet->dmaRequest) == 0)
        {
        }
        iSNDfillspuwithpackets(handle, (uint32)i);
    }
    sndpd.serveCallback = iSNDpacketserve;
    sndpd.irqCallback = iSNDpacketsetirq;
    voice->unknown0C = 0;
    voice->unknown10 = 0;
    voice->unknown14 = packet->unknown0C << 12;
    voice->basePitch = (uint16)(((uint32)format->samplerate * 0x17C7u) >> 16);
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
    sint32 interruptsEnabled = xport_bios_enter_critical();
    iSNDpacketsetirq();
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    voice->state = 1;
    return 0;
}

void iSNDpsxpacketstop(SoundPlatformChannel *channel)
{
    FUNCTION_MARKER(0x801046C8, "NFS4.EXE");
    sndpd.packetBuffers[channel->stream]->channel = -1;
    channel->stream = -1;
    sint32 count = 0;
    sint32 voices = sndgs.options.set.inputvoices;
    for (sint32 i = 0; i < voices; ++i)
        if (sndpd.channels[i].stream >= 0)
            ++count;
    if (count == 0)
    {
        sndpd.serveCallback = 0;
        sndpd.irqCallback = iSNDpacketsetirq;
    }
    sint32 interruptsEnabled = xport_bios_enter_critical();
    iSNDpacketsetirq();
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
}
