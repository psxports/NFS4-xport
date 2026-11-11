#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDpatchkey(sint32 channel, sint32 *cursor)
{
    FUNCTION_MARKER(0x800FEF4C, "NFS4.EXE");
    uint8 group = ((SoundVoicePrefix *)(sndgs.voiceMemory))[channel].unknown37;
    if (group == 0)
    {
        if (*cursor < 0)
        {
            *cursor = channel;
            return 1;
        }
        return 0;
    }
    *cursor = (sint32)(((uint32)(*cursor)) + 1);
    while (*cursor < sndgs.options.set.inputvoices)
    {
        SoundVoicePrefix *voice = &((SoundVoicePrefix *)(sndgs.voiceMemory))[*cursor];
        if (voice->unknown37 == group && voice->state == 1 && voice->playHandle >= 0)
            return 1;
        *cursor = (sint32)(((uint32)(*cursor)) + 1);
    }
    return 0;
}
