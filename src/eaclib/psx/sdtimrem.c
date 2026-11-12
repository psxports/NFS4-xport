#include "xport.h"
#include "eaclib/psx/internal.h"

uint32 iSNDtimeremaining(sint32 channel)
{
    FUNCTION_MARKER(0x801049A8, "NFS4.EXE");
    SoundPlatformChannel *voice = &sndpd.channels[channel];
    if (voice->pitch == 0)
        return 0x7FFFFFFF;
    uint32 end = voice->unknown14;
    uint32 position = voice->unknown0C;
    uint32 rate = voice->pitch;
    if (rate == 0)
        abort();
    return (end - position) / rate;
}
