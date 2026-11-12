#include "xport.h"
#include "eaclib/psx/internal.h"
#include "salloc.h"

uint32 soundChannelGeneration;

sint32 iSNDischanreserved(sint32 channel, sint32 count)
{
    FUNCTION_MARKER(0x800FE724, "NFS4.EXE");
    for (sint32 i = 0; i < count; ++i)
    {
        if (sndchanreserved[i] == channel)
            return 1;
    }
    return 0;
}

sint32 iSNDallocchan(uint32 mask, sint32 channels, sint32 priority, sint32 *playHandle)
{
    FUNCTION_MARKER(0x800FE764, "NFS4.EXE");
    sint32 selected = 0;
    for (sint32 i = 0; i < channels; ++i)
        sndchanreserved[i] = -1;
    soundChannelGeneration += 32;
    if ((sint32)soundChannelGeneration < 0)
        soundChannelGeneration = 0;
    if (channels > 0)
    {
        for (sint32 attempt = 0; attempt < channels; ++attempt)
        {
            sint32 best = -1;
            uint32 bestAge = 0xFFFFFFFF;
            for (sint32 channel = 0; channel < sndgs.options.set.inputvoices; ++channel)
            {
                if ((mask & (((uint32)(1)) << (channel & 31))) != 0)
                {
                    SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[channel];
                    if (voice->state == 0 && iSNDischanreserved(channel, selected) == 0 && voice->age < bestAge)
                    {
                        bestAge = voice->age;
                        best = channel;
                    }
                }
            }
            if (best >= 0)
            {
                sndchanreserved[selected] = (sint8)best;
                ++selected;
            }
        }
        for (sint32 attempt = selected; attempt < channels; ++attempt)
        {
            sint32 bestPriority = 102;
            sint32 best = -1;
            uint32 bestAge = 0xFFFFFFFF;
            for (sint32 channel = 0; channel < sndgs.options.set.inputvoices; ++channel)
            {
                if ((mask & (((uint32)(1)) << (channel & 31))) != 0 && iSNDischanreserved(channel, selected) == 0)
                {
                    SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[channel];
                    if (voice->priority < 101)
                    {
                        if (voice->priority < bestPriority)
                        {
                            bestPriority = voice->priority;
                            bestAge = voice->age;
                            best = channel;
                        }
                        else if (voice->priority == bestPriority && voice->age < bestAge)
                        {
                            bestAge = voice->age;
                            best = channel;
                        }
                    }
                }
            }
            if (best >= 0)
            {
                sndchanreserved[selected] = (sint8)best;
                ++selected;
                if (selected >= channels)
                    break;
            }
        }
    }
    if (selected != channels)
        return -9;
    *playHandle = (soundChannelGeneration | ((sndchanreserved[0])));
    sint32 result = sndchanreserved[0];
    for (sint32 i = 0; i < selected; ++i)
    {
        SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[sndchanreserved[i]];
        sint32 oldHandle = voice->playHandle;
        if (voice->state == 1)
        {
            if (oldHandle < 0)
                oldHandle = ((SoundVoicePrefix *)(sndgs.voiceMemory))[(sint8)voice->unknown3C].playHandle;
            SNDstop(oldHandle);
            if (SNDover(oldHandle) != 1)
            {
                for (sint32 previous = i - 1; previous >= 0; --previous)
                    ((SoundVoicePrefix *)(sndgs.voiceMemory))[sndchanreserved[previous]].state = 0;
                return -9;
            }
        }
        voice->state = 1;
        voice->age = sndgs.clock;
        voice->priority = (uint8)priority;
    }
    ((SoundVoicePrefix *)(sndgs.voiceMemory))[sndchanreserved[0]].playHandle = *playHandle;
    for (sint32 i = 1; i < selected; ++i)
    {
        SoundVoicePrefix *parent = &((SoundVoicePrefix *)(sndgs.voiceMemory))[sndchanreserved[0]];
        parent->unknown04[i - 1] = sndchanreserved[i];
        SoundVoicePrefix *child = &((SoundVoicePrefix *)(sndgs.voiceMemory))[sndchanreserved[i]];
        child->playHandle = -1;
        child->unknown3C = sndchanreserved[0];
    }
    return result;
}

void iSNDfreechan(sint32 channel)
{
    FUNCTION_MARKER(0x800FEC0C, "NFS4.EXE");
    SoundVoicePrefix *voices = ((SoundVoicePrefix *)(sndgs.voiceMemory));
    SoundVoicePrefix *voice = &voices[channel];
    uint8 group = voice->unknown37;
    if (group != 0)
    {
        sint32 count = 0;
        sint32 leader = -1;
        for (sint32 i = 0; i < sndgs.options.set.inputvoices; ++i)
            if (voices[i].unknown37 == group && voices[i].playHandle >= 0 && voices[i].state != 0)
            {
                ++count;
                if (voices[i].unknown36 != 0)
                    leader = i;
            }
        if (count != 1)
        {
            if (voices[leader].state == 2 && channel != leader && count == 2)
            {
                voice->state = 0;
                voice->age = sndgs.clock;
                voices[leader].state = 0;
                voices[leader].age = sndgs.clock;
                return;
            }
            if (voices[leader].state == 1 && channel == leader)
            {
                voices[leader].state = 2;
                return;
            }
        }
    }
    voice->state = 0;
    voice->age = sndgs.clock;
}

sint32 iSNDgetchan(sint32 handle)
{
    FUNCTION_MARKER(0x800FEDC4, "NFS4.EXE");
    if (handle < 0)
        return -8;
    sint32 channel = handle & 31;
    if (channel >= sndgs.options.set.inputvoices)
        return -8;
    SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[channel];
    if (voice->state == 0 || voice->playHandle != handle)
        return -8;
    return channel;
}
