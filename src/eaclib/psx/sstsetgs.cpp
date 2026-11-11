#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDSTRM::setgreedystate(sint32 handle, sint32 enabled)
{
    FUNCTION_MARKER(0x800E9D8C, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle);
    if (stream == 0)
        return -8;
    STREAM::setgreedystate(stream->streamHandle, enabled);
    return 0;
}
