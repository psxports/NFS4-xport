#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDSTRM::setgreedylevel(sint32 handle, sint32 level)
{
    FUNCTION_MARKER(0x800E9D30, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle);
    if (stream == 0)
        return -8;
    STREAM::setgreedylevel(stream->streamHandle, level);
    return 0;
}
