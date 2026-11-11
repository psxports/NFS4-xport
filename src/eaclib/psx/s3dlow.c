#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SND3dpos(sint32 handle, sint32 azimuth, sint32 elevation)
{
    FUNCTION_MARKER(0x800E80E4, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    azimuth = ((sint32)(((uint32)(azimuth)) & 0xFFFFu));
    if (elevation >= 0x4000)
        elevation = 0x3FFF;
    else if (elevation < -0x4000)
        elevation = -0x4000;
    iSNDenteraudio();
    sint32 channel = iSNDgetchan(handle);
    if (channel >= 0)
    {
        sint32 cursor = -1;
        while (iSNDpatchkey(channel, &cursor) != 0)
            iSNDplatform3dpos(cursor, azimuth, elevation);
    }
    iSNDleaveaudio();
    return channel;
}
