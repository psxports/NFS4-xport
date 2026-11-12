#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 SNDSTRM::overheadtap(sint32 streams, sint32 chunks)
{
    FUNCTION_MARKER(0x800EA054, "NFS4.EXE");
    uint32 requestBytes = ((uint32)streams * sizeof(SoundStreamRequestPrefix) + sizeof(void *) - 1U) & (~((uint32)sizeof(void *) - 1));
    uint32 streamBytes = requestBytes + sizeof(SoundStreamPrefix);
    sint32 packetBytes = SNDPKTPLAY::overhead(chunks);
    return (streamBytes + packetBytes);
}

sint32 SNDSTRM::overhead(sint32 streams, sint32 chunks)
{
    FUNCTION_MARKER(0x800EA094, "NFS4.EXE");
    sint32 soundBytes = overheadtap(streams, chunks);
    sint32 streamBytes = STREAM::overhead((streams + 1), 1, 1);
    return (soundBytes + streamBytes);
}
