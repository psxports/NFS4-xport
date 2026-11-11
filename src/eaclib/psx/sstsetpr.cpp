#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDSTRM::setpriority(sint32 handle, sint32 mask, sint32 priority)
{
    FUNCTION_MARKER(0x800E6CF0, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle);
    if (stream == 0)
        return -8;
    STREAM::setpriority(stream->streamHandle, mask, priority);
    return 0;
}
