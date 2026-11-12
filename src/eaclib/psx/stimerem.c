#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDtimeremaining(sint32 handle)
{
    FUNCTION_MARKER(0x800ED338, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    iSNDenteraudio();
    sint32 channel = iSNDgetchan(handle);
    if (channel < 0)
    {
        iSNDleaveaudio();
        return -8;
    }
    sint32 longest = -1024;
    sint32 cursor = -1;
    while (iSNDpatchkey(channel, &cursor) != 0)
    {
        sint32 remaining = iSNDtimeremaining(cursor);
        if (longest < remaining)
            longest = remaining;
    }
    iSNDleaveaudio();
    return longest;
}
