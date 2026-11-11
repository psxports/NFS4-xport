#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDSTRM::getvol(sint32 handle)
{
    FUNCTION_MARKER(0x800E83EC, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle);
    if (stream == 0)
        return -8;
    sint32 volume = SNDgetvol(stream->requestHandle);
    if (volume < 0)
        volume = (sint8)stream->options.vol;
    return volume;
}
