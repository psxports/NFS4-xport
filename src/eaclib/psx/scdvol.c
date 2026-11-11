#include "xport.h"
#include "eaclib/psx/internal.h"

NativeCdLevels sndcdvs = { 64, 127 };

sint32 SNDcdvol(sint32 volume)
{
    FUNCTION_MARKER(0x800FAA44, "NFS4.EXE");
    uint32 product = (uint32)volume * (sint8)sndgs.masterVolume;
    sint32 scaled = (sint32)product / 127;
    sndcdvs.volume = (sint16)scaled;
    iSNDplatformcdpanvol(sndcdvs.pan, (sint16)scaled);
    return 0;
}
