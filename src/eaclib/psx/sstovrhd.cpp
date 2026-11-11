#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDSTRM::overheadtap(sint32 streams, sint32 chunks)
{
    FUNCTION_MARKER(0x800EA054, "NFS4.EXE");
    uint32 streamBytes = (uint32)streams * 44 + 96;
    sint32 packetBytes = SNDPKTPLAY::overhead(chunks);
    return (sint32)(streamBytes + (uint32)packetBytes);
}

sint32 SNDSTRM::overhead(sint32 streams, sint32 chunks)
{
    FUNCTION_MARKER(0x800EA094, "NFS4.EXE");
    sint32 soundBytes = overheadtap(streams, chunks);
    sint32 streamBytes = STREAM::overhead((sint32)((uint32)streams + 1), 1, 1);
    return (sint32)((uint32)soundBytes + (uint32)streamBytes);
}
