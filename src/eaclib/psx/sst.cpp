#include "xport.h"
#include "eaclib/psx/internal.h"
#include "sst.h"

static bool soundStreamPaused;
SoundStreamState sndss;

void StopSoundVoicesExcept(sint32 preservedHandle)
{
    sint32 channel = 0;
    while (channel < sndgs.options.set.inputvoices)
    {
        sint32 handle = static_cast<SoundVoicePrefix *>(sndgs.voiceMemory)[channel].playHandle;
        ++channel;
        if (preservedHandle < 0 || handle != preservedHandle)
            SNDstop(handle);
    }
}

extern "C" sint32 iSNDstreamdestroyall(...)
{
    FUNCTION_MARKER(0x800E8C14, "NFS4.EXE");
    SNDSTRM::destroy(0);
    return 0;
}

extern "C" SoundStreamPrefix *iSNDstreamgetstreamptr(sint32 handle)
{
    FUNCTION_MARKER(0x800E8C48, "NFS4.EXE");
    return handle == 0 ? sndss.stream : 0;
}

extern "C" void iSNDstreamremoverequest(uint32 requestId)
{
    FUNCTION_MARKER(0x800E8C64, "NFS4.EXE");
    SoundStreamPrefix *stream = (&sndss.stream)[requestId & 0xFF];
    sint32 kept = 0;
    for (sint32 i = 0; i < (sint8)stream->requestCount; ++i)
    {
        SoundStreamRequestPrefix *request = &stream->requests[i];
        if (request->id != requestId)
        {
            if ((sint8)stream->parseIndex == i)
                stream->parseIndex = (uint8)kept;
            stream->requests[kept] = *request;
            ++kept;
        }
    }
    --stream->requestCount;
    if ((sint8)stream->requestCount < (sint8)stream->parseIndex)
        --stream->parseIndex;
}

extern "C" void iSNDstreamreleasecallback(void *data)
{
    FUNCTION_MARKER(0x800E8D90, "NFS4.EXE");
    uint32 *chunk;
    memcpy(&chunk, static_cast<uint8 *>(data) - sizeof(chunk), sizeof(chunk));
    uint8 index = *reinterpret_cast<uint8 *>(chunk);
    SoundStreamPrefix *stream = (&sndss.stream)[index];
    STREAM::release(stream->streamHandle, chunk);
}

extern "C" void iSNDstreamnotifycallback(sint32 handle, uint32 frames)
{
    FUNCTION_MARKER(0x800E8DD4, "NFS4.EXE");
    SoundStreamPrefix *stream = (&sndss.stream)[sndss.packetStreams[handle]];
    uint32 remaining = frames;
    do
    {
        SoundStreamRequestPrefix *request = stream->requests;
        uint32 excess = 0;
        uint32 amount = remaining;
        if (request->queued < amount)
        {
            excess = amount - request->queued;
            amount -= excess;
        }
        request->consumed += amount;
        request->queued -= amount;
        if (request->consumed >= request->total)
            iSNDstreamremoverequest(request->id);
        remaining = excess;
    } while (remaining != 0);
}

extern "C" sint32 iSNDstreamparseheader(SoundStreamPrefix *stream, uint32 *chunk)
{
    FUNCTION_MARKER(0x800E8E9C, "NFS4.EXE");
    SoundStreamRequestPrefix *request = &stream->requests[(sint8)stream->parseIndex];
    uint32 frames;
    iSNDpatchtohdr(reinterpret_cast<uint8 *>(chunk) + 12, &stream->nextFormat, reinterpret_cast<SoundAttributesPrefix *>(stream->nextAttributes), &frames);
    request->total = frames;
    STREAM::release(stream->streamHandle, chunk);
    sint32 rounded = (sint32)(request->total + 27);
    request->total = uint32(rounded / 28) * 28;
    request->dataRate = iSNDplatformcalcdatarate(&stream->nextFormat);
    if (memcmp(&stream->format, &stream->nextFormat, 4) != 0 || memcmp(stream->attributes, stream->nextAttributes, 20) != 0)
    {
        if (stream->format.samplerate != 0)
        {
            stream->state = 2;
            return 0;
        }
        memcpy(&stream->format, &stream->nextFormat, 4);
        memcpy(stream->attributes, stream->nextAttributes, 20);
    }
    if (stream->state != 1)
    {
        stream->requestHandle = SNDPKTPLAY::start(stream->packetHandle, &stream->format, stream->attributes, &stream->options);
        stream->state = 1;
    }
    return 0;
}

extern "C" sint32 iSNDstreamparsenumchunks(SoundStreamPrefix *stream, uint32 *chunk)
{
    FUNCTION_MARKER(0x800E9044, "NFS4.EXE");
    STREAM::release(stream->streamHandle, chunk);
    sint32 index = (sint8)stream->parseIndex;
    uint32 chunks = chunk[3];
    stream->requests[index].chunks = chunks;
    return 1;
}

extern "C" sint32 iSNDstreamparsedata(SoundStreamPrefix *stream, uint32 *chunk)
{
    FUNCTION_MARKER(0x800E90AC, "NFS4.EXE");
    SoundPacketSubmit packet;
    packet.frames = chunk[3];
    uint8 *data = reinterpret_cast<uint8 *>(chunk) + 16;
    if (stream->attributes[11] == 0)
    {
        for (sint32 i = 0; i < stream->format.channels; ++i)
            packet.channels[i] = data;
    }
    else
    {
        uint8 *channelData = data + stream->format.channels * 4;
        for (sint32 i = 0; i < stream->format.channels; ++i)
            packet.channels[i] = channelData + (sint32)chunk[4 + i];
    }
    sint32 rounded = (sint32)(packet.frames + 27);
    packet.frames = uint32(rounded / 28) * 28;
    sint32 index = (sint8)stream->parseIndex;
    SoundStreamRequestPrefix *requests = stream->requests;
    memcpy(static_cast<uint8 *>(packet.channels[0]) - sizeof(chunk), &chunk, sizeof(chunk));
    SoundStreamRequestPrefix *request = &requests[index];
    chunk[0] = request->id;
    request->queued += packet.frames;
    ++request->queuedChunks;
    request->lastPacket = SNDPKTPLAY::submit(stream->packetHandle, &packet);
    if (request->firstPacket < 0)
        request->firstPacket = request->lastPacket;
    return 1;
}

extern "C" sint32 iSNDstreamparseend(SoundStreamPrefix *stream, void *chunk)
{
    FUNCTION_MARKER(0x800E9230, "NFS4.EXE");
    STREAM::release(stream->streamHandle, chunk);
    ++stream->parseIndex;
    return 1;
}

extern "C" sint32 iSNDstreamparsechunk(SoundStreamPrefix *stream, uint32 *chunk)
{
    FUNCTION_MARKER(0x800E9270, "NFS4.EXE");
    switch (chunk[0])
    {
    case 0x6C444353:
        return iSNDstreamparsedata(stream, chunk);
    case 0x6C484353:
        return iSNDstreamparseheader(stream, chunk);
    case 0x6C454353:
        return iSNDstreamparseend(stream, chunk);
    case 0x6C434353:
        return iSNDstreamparsenumchunks(stream, chunk);
    default:
        STREAM::release(stream->streamHandle, chunk);
        return 1;
    }
}

extern "C" sint32 iSNDstreamisheld(SoundStreamPrefix *stream)
{
    FUNCTION_MARKER(0x800E9318, "NFS4.EXE");
    SoundStreamRequestPrefix *request = &stream->requests[(sint8)stream->parseIndex];
    if (request->dataRate == 0)
        return 0;
    if (request->hold < 0)
        return 1;
    if (request->hold == 0)
        return 0;
    uint32 available = STREAM::gettable(stream->streamHandle);
    if (available > 4000000)
        available = 4000000;
    uint32 rate = request->dataRate;
    if (rate == 0)
        abort();
    if (available * 1000 / rate < uint32(request->hold) && STREAM::state(stream->streamHandle) != 2)
        return 1;
    request->hold = 0;
    return 0;
}

extern "C" void iSNDstreamhotroddatachunks(void)
{
    FUNCTION_MARKER(0x800E9438, "NFS4.EXE");
    SoundStreamPrefix *stream = sndss.stream;
    if (soundStreamPaused || stream == 0 || stream->requestCount == 0 ||
        stream->state != 1 || iSNDstreamisheld(stream) != 0)
        return;
    SoundStreamRequestPrefix *request = &stream->requests[(sint8)stream->parseIndex];
    if (request->chunks == 0 || request->queuedChunks == 0)
        return;
    sint32 remaining = (sint32)(request->chunks - request->queuedChunks);
    sint32 space = SNDPKTPLAY::submitspace(stream->packetHandle);
    if (remaining < space)
        space = remaining;
    if (space <= 0)
        return;
    sint32 bytes = 0;
    do
    {
        --space;
        uint32 *chunk = STREAM::get(stream->streamHandle);
        if (chunk != 0)
        {
            bytes = (sint32)((uint32)bytes + chunk[1]);
            iSNDstreamparsedata(stream, chunk);
        }
    } while (space > 0 && bytes < 0x4000);
}

extern "C" void iSNDstreamservice(...)
{
    FUNCTION_MARKER(0x800E9590, "NFS4.EXE");
    iSNDenteraudio();
    SoundStreamPrefix *stream = sndss.stream;
    if (soundStreamPaused || stream == 0 || stream->requestCount == 0)
        goto done;
    if (stream->state == 2)
    {
        if (SNDPKTPLAY::framesoutstanding(stream->packetHandle) > 0)
            goto done;
        memcpy(&stream->format, &stream->nextFormat, sizeof(stream->format));
        SNDPKTPLAY::stop(stream->packetHandle);
        stream->requestHandle = SNDPKTPLAY::start(stream->packetHandle, &stream->format, stream->attributes, &stream->options);
        stream->state = 1;
    }
    if (iSNDstreamisheld(stream) == 0)
    {
        sint32 remaining = 10;
        if (stream->state == 1)
        {
            remaining = SNDPKTPLAY::submitspace(stream->packetHandle);
            if (remaining == 0)
                goto done;
        }
        sint32 parsed = 0;
        do
        {
            --remaining;
            uint32 *chunk = STREAM::get(stream->streamHandle);
            if (chunk != 0)
                parsed = iSNDstreamparsechunk(stream, chunk);
        } while (parsed != 0 && remaining > 0);
    }
done:
    iSNDleaveaudio();
}

extern "C" sint32 iSNDstreamnumcreated(void)
{
    FUNCTION_MARKER(0x800E96F8, "NFS4.EXE");
    return sndss.stream != 0 ? 1 : 0;
}

extern "C" sint32 iSNDstreamcreate(SNDPLAYOPTS *options, sint32 streams, sint32 chunks, void *buffer, sint32 size, NativeStreamHandle *streamHandle, sint32 shared)
{
    FUNCTION_MARKER(0x800E9730, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    if (sndss.stream != 0)
        return -9;
    uint8 *data = static_cast<uint8 *>(buffer);
    SoundStreamPrefix *stream = reinterpret_cast<SoundStreamPrefix *>(data);
    uint8 *requestBuffer = data + 96;
    uint32 requestBytes = uint32(streams) * 44;
    stream->requests = reinterpret_cast<SoundStreamRequestPrefix *>(requestBuffer);
    uint8 *packetBuffer = requestBuffer + requestBytes;
    sint32 packetBytes = SNDPKTPLAY::overhead(chunks);
    uint8 *streamBuffer = packetBuffer + packetBytes;
    uint32 remainingBytes = uint32(size) - 96 - requestBytes - uint32(packetBytes);
    sint32 packetCapacity = SNDPKTPLAY::overhead(chunks);
    stream->packetHandle = SNDPKTPLAY::create(packetBuffer, packetCapacity, iSNDstreamreleasecallback, iSNDstreamnotifycallback);
    if (stream->packetHandle < 0)
        return stream->packetHandle;
    sndss.packetStreams[stream->packetHandle] = 0;
    if (shared == 0)
    {
        stream->streamHandle = STREAM::create((sint32)(uint32(streams) + 1), 1, 1, streamBuffer, (sint32)remainingBytes);
        stream->sharedStream = 0;
        sint32 bufferSize = STREAM::buffersize(stream->streamHandle);
        SNDSTRM::setgreedylevel((sint32)reinterpret_cast<uintptr_t>(stream->streamHandle), bufferSize / 3);
    }
    else
    {
        stream->streamHandle = streamHandle;
        stream->sharedStream = 1;
    }
    stream->unknown16 = 0;
    stream->requestHandle = -1;
    stream->streamCount = (uint8)streams;
    uint32 optionWords[5];
    memcpy(optionWords, options, 16);
    memcpy(&stream->options, optionWords, 16);
    memcpy(&optionWords[4], reinterpret_cast<uint8 *>(options) + 16, 4);
    memcpy(reinterpret_cast<uint8 *>(&stream->options) + 16, &optionWords[4], 4);
    if (iSNDstreamnumcreated() == 0)
    {
        iSNDserveraddclient(iSNDstreamservice);
        sndgs.streamService = iSNDstreamdestroyall;
    }
    sndss.stream = stream;
    SNDSTRM::purge(0);
    return 0;
}

extern "C" sint32 iSNDstreamqueue(sint32 handle, sint32 delay, const void *source, sint32 offset, sint32 mode)
{
    FUNCTION_MARKER(0x800E9970, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle);
    if (stream == 0)
        return -8;
    iSNDenteraudio();
    if ((sint8)stream->requestCount >= (sint8)stream->streamCount)
    {
        iSNDleaveaudio();
        return -13;
    }
    SoundStreamRequestPrefix *request = &stream->requests[(sint8)stream->requestCount];
    if (mode == 0)
        request->streamRequest = STREAM::queuefile(stream->streamHandle, static_cast<const char *>(source), offset, 0x6C454353);
    else if (mode == 1)
        request->streamRequest = STREAM::queuemem(stream->streamHandle, source, 0, 0x6C454353);
    else
        request->streamRequest = offset;
    if (request->streamRequest == 0)
    {
        iSNDleaveaudio();
        return -1;
    }
    stream->unknown16 += 256;
    if ((sint32)stream->unknown16 < 0)
        stream->unknown16 = 0;
    request->id = stream->unknown16 | uint32(handle);
    request->hold = (sint16)delay;
    ++stream->requestCount;
    request->dataRate = 0;
    request->firstPacket = -1;
    request->lastPacket = -1;
    request->consumed = 0;
    request->queued = 0;
    request->queuedChunks = 0;
    request->chunks = 0;
    iSNDleaveaudio();
    return (sint32)request->id;
}

sint32 SNDSTRM::create(SNDPLAYOPTS *options, sint32 streams, sint32 chunks, void *buffer, sint32 size)
{
    FUNCTION_MARKER(0x800E9B44, "NFS4.EXE");
    return iSNDstreamcreate(options, streams, chunks, buffer, size, 0, 0);
}

sint32 SNDSTRM::destroy(sint32 handle)
{
    FUNCTION_MARKER(0x800E9B70, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle);
    if (stream == 0)
        return -8;
    purge(handle);
    if (iSNDstreamnumcreated() == 1)
    {
        iSNDserverremoveclient(iSNDstreamservice);
        sndgs.streamService = 0;
    }
    SNDPKTPLAY::destroy(stream->packetHandle);
    if (stream->sharedStream == 0)
        STREAM::destroy(stream->streamHandle);
    sndss.stream = 0;
    return 0;
}

sint32 SNDSTRM::queuefile(sint32 handle, sint32 delay, const char *filename, sint32 offset)
{
    FUNCTION_MARKER(0x800E9C38, "NFS4.EXE");
    return iSNDstreamqueue(handle, delay, filename, offset, 0);
}

sint32 SNDSTRM::purge(sint32 handle)
{
    FUNCTION_MARKER(0x800E9C58, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle);
    if (stream == 0)
        return -8;
    if (stream->requestHandle >= 0)
        SNDPKTPLAY::stop(stream->packetHandle);
    stream->requestHandle = -1;
    soundStreamPaused = false;
    if (stream->sharedStream == 0)
        STREAM::kill(stream->streamHandle);
    stream->requestCount = 0;
    stream->parseIndex = 0;
    stream->state = 0;
    memset(&stream->format, 0, sizeof(stream->format));
    memset(&stream->nextFormat, 0, sizeof(stream->nextFormat));
    memset(stream->attributes, 0, sizeof(stream->attributes));
    memset(stream->nextAttributes, 0, sizeof(stream->nextAttributes));
    return 0;
}

bool SNDSTRM::IsPaused(void)
{
    return soundStreamPaused;
}

sint32 SNDSTRM::SetPaused(sint32 handle, bool paused)
{
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle);
    if (stream == 0)
        return -1;

    iSNDenteraudio();
    sint32 voice = stream->requestHandle;
    if (soundStreamPaused != paused)
    {
        soundStreamPaused = paused;
        if (voice >= 0)
        {
            SNDpitchmult(voice, paused ? 0 : stream->options.pitchmult);
            SNDvol(voice, paused ? 0 : (uint8)stream->options.vol);
        }
    }
    iSNDleaveaudio();
    return voice;
}
