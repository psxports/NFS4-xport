#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDstop(sint32 handle)
{
    FUNCTION_MARKER(0x800E7E80, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    iSNDenteraudio();
    sint32 channel = iSNDgetchan(handle);
    if (channel >= 0)
    {
        sint32 cursor = -1;
        while (iSNDpatchkey(channel, &cursor) != 0)
            iSNDstop(cursor);
    }
    iSNDleaveaudio();
    return channel;
}
