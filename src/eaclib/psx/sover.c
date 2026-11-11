#include "xport.h"
#include "eaclib/psx/internal.h"

uint32 SNDover(sint32 handle)
{
    FUNCTION_MARKER(0x800E7B14, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return ((uint32)(-10));
    iSNDenteraudio();
    uint32 result = ((uint32)(iSNDgetchan(handle))) >> 31;
    iSNDleaveaudio();
    return result;
}
