#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDSTRM::autovol(sint32 handle, sint32 ticks, sint32 volume)
{
    FUNCTION_MARKER(0x800E9DE8, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle);
    if (stream == 0)
        return -8;
    sint32 soundHandle = stream->requestHandle;
    stream->options.vol = (char)volume;
    SNDautovol(soundHandle, ticks / 10, volume);
    return 0;
}
