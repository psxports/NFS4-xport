#include "xport.h"
#include "eaclib/psx/internal.h"
#include "sststat.h"

sint32 SNDSTRM::status(sint32 handle, SNDSTREAMSTATUS *result)
{
    FUNCTION_MARKER(0x800E86D8, "NFS4.EXE");
    result->timebuffered = 0;
    result->currentrequest = 0;
    result->outstandingrequests = 0;
    if (sndgs.initialized == 0)
        return -10;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle);
    if (stream == 0)
        return -8;
    iSNDenteraudio();
    result->outstandingrequests = (sint8)stream->requestCount;
    if (stream->requestCount != 0)
    {
        result->currentrequest = stream->requests->id;
        if (stream->format.samplerate != 0)
        {
            uint32 frames = uint32(SNDPKTPLAY::unsafeframesoutstanding(stream->packetHandle));
            uint32 rate = stream->format.samplerate;
            if (rate == 0)
                abort();
            result->timebuffered = ((frames * 1000u) / rate);
        }
    }
    iSNDleaveaudio();
    return 0;
}
