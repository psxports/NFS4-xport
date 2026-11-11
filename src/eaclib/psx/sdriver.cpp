#include "xport.h"
#include "eaclib/psx/internal.h"

extern "C" void iSNDpsxkeyon(uint32 channels)
{
    FUNCTION_MARKER(0x800FF140, "NFS4.EXE");
    if (!spu_mmio_write(0, 0x1F801D88u, 2, channels & 0xFFFFu, 3))
        abort();
    if (!spu_mmio_write(0, 0x1F801D8Au, 2, channels >> 16, 3))
        abort();
}

extern "C" void iSNDpsxkeyoff(uint32 channels)
{
    FUNCTION_MARKER(0x800FF168, "NFS4.EXE");
    if (!spu_mmio_write(0, 0x1F801D8Cu, 2, channels & 0xFFFFu, 3))
        abort();
    if (!spu_mmio_write(0, 0x1F801D8Eu, 2, channels >> 16, 3))
        abort();
}

extern "C" void iSNDpsxeffecton(uint32 channels)
{
    FUNCTION_MARKER(0x800FF190, "NFS4.EXE");
    uint32 value;
    if (!spu_mmio_read(0, 0x1F801D98u, 2, &value))
        abort();
    if (!spu_mmio_write(0, 0x1F801D98u, 2, (value | channels) & 0xFFFFu, 3))
        abort();
    if (!spu_mmio_read(0, 0x1F801D9Au, 2, &value))
        abort();
    if (!spu_mmio_write(0, 0x1F801D9Au, 2, value | (channels >> 16), 3))
        abort();
}

extern "C" void iSNDpsxeffectoff(uint32 channels)
{
    FUNCTION_MARKER(0x800FF1D0, "NFS4.EXE");
    uint32 value;
    if (!spu_mmio_read(0, 0x1F801D98u, 2, &value))
        abort();
    if (!spu_mmio_write(0, 0x1F801D98u, 2, value & ~channels, 3))
        abort();
    if (!spu_mmio_read(0, 0x1F801D9Au, 2, &value))
        abort();
    if (!spu_mmio_write(0, 0x1F801D9Au, 2, value & ~(channels >> 16), 3))
        abort();
}

extern "C" void iSNDpsxeffectvol(sint32 left, sint32 right)
{
    FUNCTION_MARKER(0x800FF210, "NFS4.EXE");
    if (!spu_mmio_write(0, 0x1F801D84u, 2, (uint16)left, 3))
        abort();
    if (!spu_mmio_write(0, 0x1F801D86u, 2, (uint16)right, 3))
        abort();
}

extern "C" void iSNDsetvol(sint32 channel, sint32 left, sint32 right)
{
    FUNCTION_MARKER(0x800FF238, "NFS4.EXE");
    SoundPlatformChannel *voice = &sndpd.channels[channel];
    uint32 registers = 0x1F801C00u + ((uint32)channel << 4);
    uint32 leftScaled = ((uint32)left << 7) + (uint32)left;
    uint32 rightScaled = ((uint32)right << 7) + (uint32)right;
    if (voice->channelCount == 1)
    {
        if (sndgs.options.set.outputchannels == 2)
        {
            if (!spu_mmio_write(0, registers, 2, leftScaled & 0x7FFFu, 3))
                abort();
            if (!spu_mmio_write(0, registers + 2, 2, rightScaled & 0x7FFFu, 3))
                abort();
            return;
        }
        if (voice->unknown1E == 1)
        {
            if (left < 0)
                left = (sint32)(0u - (uint32)left);
            if (right < 0)
                right = (sint32)(0u - (uint32)right);
            if (left >= 128)
                left = 127;
            if (right >= 128)
                right = 127;
        }
        uint32 sum = (uint32)left + (uint32)right;
        uint32 scaled = (sum << 7) + sum;
        uint32 volume = (uint32)((sint32)scaled >> 1) & 0x7FFFu;
        if (!spu_mmio_write(0, registers, 2, volume, 3))
            abort();
        if (!spu_mmio_write(0, registers + 2, 2, volume, 3))
            abort();
        return;
    }
    if (sndgs.options.set.outputchannels == 2)
    {
        if (!spu_mmio_write(0, registers, 2, leftScaled & 0x7FFFu, 3))
            abort();
        if (!spu_mmio_write(0, registers + 2, 2, 0, 3))
            abort();
        registers = 0x1F801C00u + ((uint32)(sint32)voice->partner << 4);
        if (!spu_mmio_write(0, registers, 2, 0, 3))
            abort();
        if (!spu_mmio_write(0, registers + 2, 2, rightScaled & 0x7FFFu, 3))
            abort();
        return;
    }
    uint32 volume = (uint32)((sint32)leftScaled >> 1) & 0x7FFFu;
    if (!spu_mmio_write(0, registers, 2, volume, 3))
        abort();
    if (!spu_mmio_write(0, registers + 2, 2, volume, 3))
        abort();
    volume = (uint32)((sint32)rightScaled >> 1) & 0x7FFFu;
    registers = 0x1F801C00u + ((uint32)(sint32)voice->partner << 4);
    if (!spu_mmio_write(0, registers, 2, volume, 3))
        abort();
    if (!spu_mmio_write(0, registers + 2, 2, volume, 3))
        abort();
}

extern "C" void iSNDsetslot(sint32 channel, sint32 address, sint32 pitch)
{
    FUNCTION_MARKER(0x800FF394, "NFS4.EXE");
    uint32 registers = 0x1F801C00u + ((uint32)channel << 4);
    if (!spu_mmio_write(0, registers + 8, 2, 0xF, 3))
        abort();
    if (!spu_mmio_write(0, registers + 10, 2, 5, 3))
        abort();
    if (!spu_mmio_write(0, registers + 4, 2, (uint16)pitch, 3))
        abort();
    if (!spu_mmio_write(0, registers + 6, 2, (uint16)(address >> 3), 3))
        abort();
    if (!spu_mmio_write(0, registers + 14, 2, 0x200, 3))
        abort();
}

extern "C" uint32 iSNDstartvoice(sint32 channel)
{
    FUNCTION_MARKER(0x800FF3CC, "NFS4.EXE");
    SoundPlatformChannel *voice = &sndpd.channels[channel];
    voice->unknown24[0] = 0;
    voice->unknown24[1] = 0;
    uint32 pitch = (uint32)(((uint64)(uint32)voice->pitch * 0x4A4DC96Fu) >> 39);
    iSNDsetslot(channel, (sint32)voice->sampleAddress, (sint32)pitch);
    uint32 channels = 1u << ((uint32)channel & 31u);
    voice->state = 2;
    voice->unknown1D = 2;
    voice->envelopeSeen = 0;
    if (voice->channelCount == 2)
    {
        SoundPlatformChannel *other = &sndpd.channels[voice->partner];
        uint32 rightOffset;
        if (voice->stream >= 0)
            rightOffset = sndpd.packetBuffers[voice->stream]->rightOffset;
        else
        {
            uint32 length = voice->sampleLength;
            uint32 blocks = (uint32)(((uint64)(length >> 2) * 0x24924925u) >> 32);
            if (length != blocks * 28u)
                ++blocks;
            rightOffset = blocks << 4;
        }
        iSNDsetslot(voice->partner, (sint32)(voice->sampleAddress + rightOffset), (sint32)pitch);
        sint32 partner = voice->partner;
        other->state = 2;
        other->unknown1D = 2;
        other->envelopeSeen = 0;
        channels |= 1u << ((uint32)partner & 31u);
    }
    iSNDvol(channel, voice->volume);
    iSNDplatformfxlevel(channel, 0, voice->effectLevel);
    return channels;
}

extern "C" sint32 iSNDplatformoutputcaps(void)
{
    FUNCTION_MARKER(0x800FF5A8, "NFS4.EXE");
    SNDSYSOPTS *options = &sndgs.options;
    options->cap.input3dvoicesmax = 0;
    options->cap.eax = 0;
    options->cap.voicemanager = 0;
    options->cap.inputvoicesmax = 24;
    options->cap.outputratemin = 44100;
    options->cap.outputchannelsmin = 1;
    options->cap.outputratemax = 44100;
    options->cap.outputchannelsmax = 2;
    options->set.use3dacceleration = 0;
    options->set.use3dmixing = 0;
    options->set.useeax = 0;
    options->set.inputvoices = 24;
    options->set.outputrate = 44100;
    options->set.outputchannels = 2;
    return 0;
}

extern "C" sint32 iSNDplatformoutputset(void)
{
    FUNCTION_MARKER(0x800FF600, "NFS4.EXE");
    sndgs.options.set.inputvoices = 24;
    if (sndgs.options.set.outputchannels < sndgs.options.cap.outputchannelsmin)
        sndgs.options.set.outputchannels = sndgs.options.cap.outputchannelsmin;
    if (sndgs.options.set.outputchannels > sndgs.options.cap.outputchannelsmax)
        sndgs.options.set.outputchannels = sndgs.options.cap.outputchannelsmax;
    if (sndgs.initialized != 0 && sndpd.unknown0[2] != sndgs.options.set.outputchannels)
    {
        sint32 channel = 0;
        do
        {
            SoundPlatformChannel *voice = &sndpd.channels[channel];
            if (voice->state == 2 && voice->unknown21 == 0)
                iSNDsetvol(channel, (sint8)voice->unknown24[0], (sint8)voice->unknown24[1]);
            ++channel;
        } while (channel < sndgs.options.set.inputvoices);
    }
    sndpd.unknown0[2] = sndgs.options.set.outputchannels;
    return 0;
}

static void RestoreSoundAtExit(void)
{
    SNDSYS::restore();
}

void WriteSoundRegister(uint32 offset, uint32 value)
{
    if (!spu_mmio_write(0, 0x1F801C00u + offset, 2, value, 3))
        abort();
}

uint32 ReadSoundRegister(uint32 offset)
{
    uint32 value;
    if (!spu_mmio_read(0, 0x1F801C00u + offset, 2, &value))
        abort();
    return value;
}

extern "C" sint32 iSNDinit(void)
{
    FUNCTION_MARKER(0x800FF700, "NFS4.EXE");

    soundSpuMemory.firstUnit = 65;
    WriteSoundRegister(0x180, 0);
    WriteSoundRegister(0x182, 0);
    WriteSoundRegister(0x1AA, 0);
    WriteSoundRegister(0x180, 0);
    WriteSoundRegister(0x182, 0);
    uint32 status = ReadSoundRegister(0x1AE);
    soundSpuMemory.endUnit = 8192;
    while ((status & 0x7FFu) != 0)
        status = ReadSoundRegister(0x1AE);

    WriteSoundRegister(0x1AC, 4);
    WriteSoundRegister(0x190, 0);
    WriteSoundRegister(0x192, 0);
    WriteSoundRegister(0x194, 0);
    WriteSoundRegister(0x196, 0);
    WriteSoundRegister(0x1B0, 0);
    WriteSoundRegister(0x1B2, 0);
    WriteSoundRegister(0x1B4, 0);
    sint32 interruptsEnabled = xport_bios_enter_critical();
    WriteSoundRegister(0x1B6, 0);
    WriteSoundRegister(0x1AA, 0xC000);
    DMACallback(4, iSNDdmcallback);
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();

    static const uint8 safeLoopBlock[64] = { 0, 7, 0, 0, 0, 0, 0, 0,
                                                      0, 0, 0, 0, 0, 0, 0, 0, 2 };
    iSNDdmqueue(safeLoopBlock, 0x1000, sizeof(safeLoopBlock), 1, 0);
    for (sint32 channel = 0; channel < sndgs.options.set.inputvoices; ++channel)
    {
        uint32 voice = (uint32)channel << 4;
        WriteSoundRegister(voice, 0);
        WriteSoundRegister(voice + 2, 0);
        WriteSoundRegister(voice + 4, 0);
        WriteSoundRegister(voice + 6, 0x200);
        WriteSoundRegister(voice + 14, 0x200);
        WriteSoundRegister(voice + 8, 0);
        WriteSoundRegister(voice + 10, 0);
        sndpd.channels[channel].stream = -1;
    }
    iSNDpsxkeyon(0xFFFFFFu);
    WriteSoundRegister(0x1A2, 0xFFFE);
    WriteSoundRegister(0x180, 0x3FFF);
    WriteSoundRegister(0x182, 0x3FFF);
    WriteSoundRegister(0x1AA, ReadSoundRegister(0x1AA) | 1u);
    iSNDpsxfxinit(sndpd.fxMode);
    sndpd.unknown0[1] = 0;
    addtimer(iSNDserver);
    if (sndpd.unknown0[0] == 0)
    {
        addexit(RestoreSoundAtExit);
        sndpd.unknown0[0] = 1;
    }
    return 0;
}

extern "C" sint32 iSNDrestore(void)
{
    FUNCTION_MARKER(0x800FF9A0, "NFS4.EXE");
    while (sndgs.audioLockDepth != 0)
        iSNDleaveaudio();
    uint32 deadline = sndgs.clock + 100;
    bool complete;
    do
    {
        complete = true;
        for (sint32 channel = 0; channel < sndgs.options.set.inputvoices; ++channel)
        {
            SoundPlatformChannel *voice = &sndpd.channels[channel];
            if (voice->unknown1D != 0)
            {
                complete = false;
                if (deadline < sndgs.clock)
                {
                    iSNDpsxkeyoff(0xFFFFFF);
                    voice->unknown1D = 0;
                    iSNDfreechan(channel);
                }
            }
        }
        psx_root_counter_poll();
        SNDSYS::service();
    } while (!complete);
    deltimer(iSNDserver);
    iSNDpsxfxinit(0);
    sint32 interruptsEnabled = xport_bios_enter_critical();
    DMACallback(4, 0);
    if (interruptsEnabled != 0)
        xport_bios_exit_critical();
    return 0;
}

extern "C" void iSNDserve(void)
{
    FUNCTION_MARKER(0x800FFAF4, "NFS4.EXE");
    uint32 keyOff = 0;
    uint32 keyOn = 0;
    void (*callback)(void) = sndpd.serveCallback;
    if (callback != 0)
        callback();
    for (sint32 channel = 0; channel < sndgs.options.set.inputvoices; ++channel)
    {
        SoundPlatformChannel *voice = &sndpd.channels[channel];
        uint32 registers = 0x1F801C00u + ((uint32)channel << 4);
        if (voice->unknown1D == 2)
        {
            uint32 envelope;
            if (!spu_mmio_read(0, registers + 12, 2, &envelope))
                abort();
            if (envelope != 0)
                voice->envelopeSeen = 1;
            else if (voice->envelopeSeen != 0 && voice->unknown21 == 0 && voice->stream < 0)
            {
                uint32 remaining = voice->channelCount;
                do
                {
                    sint32 selected = channel;
                    if (remaining == 2)
                    {
                        selected = voice->partner;
                        voice = &sndpd.channels[selected];
                    }
                    else
                        voice = &sndpd.channels[channel];
                    voice->unknown1D = 0;
                    voice->state = 0;
                    --remaining;
                    iSNDfreechan(selected);
                    uint32 selectedRegisters = 0x1F801C00u + ((uint32)selected << 4);
                    if (!spu_mmio_write(0, selectedRegisters + 6, 2, 0x200, 3))
                        abort();
                    keyOn |= 1u << ((uint32)selected & 31u);
                    if (!spu_mmio_write(0, selectedRegisters, 2, 0, 3))
                        abort();
                    if (!spu_mmio_write(0, selectedRegisters + 2, 2, 0, 3))
                        abort();
                } while ((sint32)remaining > 0);
            }
            if (voice->pitchChanged != 0)
            {
                uint32 pitch = (uint32)(((uint64)(uint32)voice->pitch * 0x4A4DC96Fu) >> 39);
                if (!spu_mmio_write(0, registers + 4, 2, (uint16)pitch, 3))
                    abort();
                voice->pitchChanged = 0;
            }
            if ((sint32)voice->unknown10 > 0)
                voice->unknown10 -= (uint32)voice->pitch;
            else
            {
                voice->unknown0C += (uint32)voice->pitch;
                if (voice->unknown0C >= voice->unknown14)
                {
                    if (voice->stream >= 0)
                        voice->unknown0C -= voice->unknown14;
                    else
                        voice->unknown0C = voice->unknown14;
                }
            }
        }
        else if (voice->unknown1D == 3)
        {
            uint32 envelope;
            if (!spu_mmio_read(0, registers + 12, 2, &envelope))
                abort();
            if (envelope == 0)
            {
                keyOn |= 1u << ((uint32)channel & 31u);
                voice->unknown1D = 0;
                if (!spu_mmio_write(0, registers + 6, 2, 0x200, 3))
                    abort();
                if (!spu_mmio_write(0, registers, 2, 0, 3))
                    abort();
                if (!spu_mmio_write(0, registers + 2, 2, 0, 3))
                    abort();
            }
            else
                keyOff |= 1u << ((uint32)channel & 31u);
        }
        if (voice->state == 1 && voice->unknown1D == 0 &&
            (voice->channelCount < 2 || sndpd.channels[voice->partner].unknown1D == 0))
            keyOn |= iSNDstartvoice(channel);
    }
    if (keyOff != 0)
        iSNDpsxkeyoff(keyOff);
    if (keyOn != 0)
        iSNDpsxkeyon(keyOn);
}

extern "C" sint32 iSNDstop(sint32 channel)
{
    FUNCTION_MARKER(0x800FFE90, "NFS4.EXE");
    SoundPlatformChannel *voice = &sndpd.channels[channel];
    if (voice->stream >= 0)
        sndpd.stopCallback(voice);
    iSNDfreechan(channel);
    voice->unknown1D = 3;
    voice->state = 0;
    uint32 channels = 1u << ((uint32)channel & 31u);
    if (voice->channelCount >= 2)
    {
        sint32 partner = voice->partner;
        iSNDfreechan(partner);
        channels |= 1u << ((uint32)partner & 31u);
        sndpd.channels[partner].unknown1D = 3;
        sndpd.channels[partner].state = 0;
    }
    iSNDpsxkeyoff(channels);
    return 0;
}

extern "C" sint32 iSNDplatformpitch(sint32 channel, sint32 pitch)
{
    FUNCTION_MARKER(0x800FFF94, "NFS4.EXE");
    SoundPlatformChannel *voice = &sndpd.channels[channel];
    uint32 product = (uint32)voice->basePitch * (uint32)pitch;
    uint32 shifted = (uint32)((sint32)product >> 12);
    voice->pitch = (sint32)(shifted * 441u);
    voice->pitchChanged = 1;
    if (voice->channelCount == 2)
    {
        sndpd.channels[voice->partner].pitch = voice->pitch;
        sndpd.channels[voice->partner].pitchChanged = 1;
    }
    return 0;
}

extern "C" sint32 iSNDplatformfxlevel(sint32 channel, sint32, sint32 level)
{
    FUNCTION_MARKER(0x80100058, "NFS4.EXE");
    SoundPlatformChannel *voice = &sndpd.channels[channel];
    if (voice->state != 2)
        voice->effectLevel = (sint8)level;
    else
    {
        uint32 channels = 1u << ((uint32)channel & 31u);
        if (voice->channelCount == 2)
            channels |= 1u << ((uint32)(uint8)voice->partner & 31u);
        if (level != 0)
            iSNDpsxeffecton(channels);
        else
            iSNDpsxeffectoff(channels);
    }
    return 0;
}

extern "C" sint32 iSNDvol(sint32 channel, sint32 volume)
{
    FUNCTION_MARKER(0x801000F8, "NFS4.EXE");
    SoundPlatformChannel *voice = &sndpd.channels[channel];
    if (voice->state != 2)
    {
        voice->volume = (sint8)volume;
        return 0;
    }
    uint8 mode = voice->unknown1E;
    XASSERT(mode <= 1);
    sint32 left;
    sint32 right;
    if (mode == 0)
        iSNDatolrv(voice->pan, volume, &left, &right);
    else
        iSNDatodlrv(voice->pan, volume, &left, &right,
            (sint8)voice->unknown24[0], (sint8)voice->unknown24[1]);
    voice->unknown24[0] = (uint8)left;
    voice->unknown24[1] = (uint8)right;
    iSNDsetvol(channel, left, right);
    return 0;
}
