#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDSTRM::vol(sint32 handle, sint32 volume)
{
    FUNCTION_MARKER(0x800EA0D8, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle);
    if (stream == 0)
        return -8;
    sint32 soundHandle = stream->requestHandle;
    stream->options.vol = (char)volume;
    SNDvol(soundHandle, volume);
    return 0;
}
