#include "xport.h"
#include "eaclib/psx/internal.h"

extern "C" void iSNDserver(void)
{
    FUNCTION_MARKER(0x800EA138, "NFS4.EXE");
    if (sndgs.serverRunning != 0)
        return;
    if (sndgs.audioLockDepth != 0)
    {
        ++sndgs.pendingServerTicks;
        return;
    }
    sndgs.serverRunning = 1;
    sint32 frequency = timerhz;
    if (frequency != sndgs.serverTimerHz)
    {
        sndgs.serverTimerHz = (sint16)frequency;
        sndgs.clock = 0;
        sndgs.unknown172 = 0;
    }
    sndgs.unknown172 = (sint32)((uint32)sndgs.unknown172 + 1);
    uint32 elapsed = (uint32)sndgs.unknown172 * 100;
    if (frequency == 0)
        abort();
    uint32 target = elapsed / (uint32)frequency;
    while (sndgs.clock <= target)
        sndgs.server100hz();
    sndgs.serverRunning = 0;
}

extern "C" void iSND100hzserver(...)
{
    FUNCTION_MARKER(0x800EA254, "NFS4.EXE");
    ++sndgs.clock;
    if (sndpd.completed != 0)
        iSNDdmservice();
    iSNDserve();
    for (sint32 client = 0; client < sndgs.client100hzCount; ++client)
        sndgs.clients100hz[client]();
    for (sint32 channel = 0; channel < sndgs.options.set.inputvoices; ++channel)
    {
        SoundVoicePrefix *voice = &static_cast<SoundVoicePrefix *>(sndgs.voiceMemory)[channel];
        if (voice->state != 1 || voice->playHandle < 0)
            continue;
        if (voice->unknown54 != 0)
        {
            ++voice->pitchEnvelopeIndex;
            if (voice->pitchEnvelopeIndex >= voice->unknown39)
                voice->pitchEnvelopeIndex = 0;
            voice->pitchAdjustment = 0;
            iSNDcalcpitch(channel);
            iSNDplatformpitch(channel, voice->pitch);
        }
        bool volumeChanged = voice->unknown50 != 0;
        if (volumeChanged)
        {
            ++voice->volumeEnvelopeIndex;
            if (voice->volumeEnvelopeIndex >= voice->unknown38)
                voice->volumeEnvelopeIndex = 0;
        }
        sint32 ramp = voice->volumeRamp;
        if (ramp != 0)
        {
            volumeChanged = true;
            sint32 volume = (sint32)((uint32)voice->volume + (uint32)ramp);
            voice->volume = volume;
            sint32 target = voice->volumeTarget;
            if ((ramp < 0 && target >= volume) || (ramp >= 0 && volume >= target))
            {
                voice->volume = target;
                voice->volumeRamp = 0;
            }
            if (voice->volume < 0)
            {
                SNDstop(voice->playHandle);
                continue;
            }
        }
        sint32 delta = voice->modulationDelta;
        if (delta != 0)
        {
            volumeChanged = true;
            voice->modulationVolume = (sint32)((uint32)voice->modulationVolume + (uint32)delta);
        }
        if (voice->modulationTicks == 0)
        {
            voice->envelopeIndex = (sint8)((uint8)voice->envelopeIndex + 1);
            if (voice->envelopeIndex >= voice->envelopeCount)
            {
                SNDstop(voice->playHandle);
                continue;
            }
            const uint8 *point = static_cast<const uint8 *>(voice->envelope) + voice->envelopeIndex * 8;
            uint32 duration;
            memcpy(&duration, point, sizeof(duration));
            voice->modulationTicks = (sint32)duration;
            if (voice->modulationTicks < 0)
                voice->modulationTicks = 0x7FFFFFFF;
            uint32 target;
            memcpy(&target, point + 4, sizeof(target));
            sint32 difference = (sint32)((target << 16) - (uint32)voice->modulationVolume);
            sint32 ticks = voice->modulationTicks;
            if (ticks == 0)
                abort();
            voice->modulationDelta = difference / ticks;
        }
        voice->modulationTicks = (sint32)((uint32)voice->modulationTicks - 1);
        if (volumeChanged)
        {
            iSNDcalcvol(channel);
            if (voice->playHandle >= 0)
                iSNDvol(channel, voice->outputVolume);
        }
    }
}

extern "C" void iSNDenteraudio(void)
{
    FUNCTION_MARKER(0x800EA534, "NFS4.EXE");
    ++sndgs.audioLockDepth;
}

extern "C" void iSNDleaveaudio(void)
{
    FUNCTION_MARKER(0x800EA56C, "NFS4.EXE");
    --sndgs.audioLockDepth;
    if (sndgs.audioLockDepth == 0)
    {
        while (sndgs.pendingServerTicks != 0)
        {
            --sndgs.pendingServerTicks;
            iSNDserver();
        }
    }
}

extern "C" void iSNDserveradd100hzclient(void (*callback)(void))
{
    FUNCTION_MARKER(0x800EA5F0, "NFS4.EXE");
    sndgs.clients100hz[sndgs.client100hzCount] = callback;
    ++sndgs.client100hzCount;
}

extern "C" void iSNDserverremove100hzclient(void (*callback)(void))
{
    FUNCTION_MARKER(0x800EA620, "NFS4.EXE");
    for (sint32 i = 0; i < sndgs.client100hzCount; ++i)
    {
        if (sndgs.clients100hz[i] == callback)
        {
            --sndgs.client100hzCount;
            while (i < sndgs.client100hzCount)
            {
                sndgs.clients100hz[i] = sndgs.clients100hz[i + 1];
                ++i;
            }
            return;
        }
    }
}
