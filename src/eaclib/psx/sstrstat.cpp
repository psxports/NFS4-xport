#include "xport.h"
#include "eaclib/psx/internal.h"
#include "sstrstat.h"

sint32 SNDSTRM::requeststatus(sint32 handle, SNDREQUESTSTATUS *result)
{
    FUNCTION_MARKER(0x800E87D0, "NFS4.EXE");
    result->state = 0;
    result->currenttime = 0;
    result->timetoend = 0;
    result->timebuffered = 0;
    if (sndgs.initialized == 0)
        return -10;
    if (handle < 0)
        return -8;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(handle & 0xFF);
    if (stream == 0)
        return -8;
    iSNDenteraudio();
    SoundStreamRequestPrefix *request = iSNDstreamgetrequestptr(handle);
    if (request == 0)
        result->state = 3;
    else if (request->firstPacket < 0)
        result->state = 0;
    else
    {
        uint32 rate;
        if (stream->requests == request)
        {
            result->state = 2;
            rate = stream->format.samplerate;
        }
        else
        {
            result->state = 1;
            rate = stream->nextFormat.samplerate;
        }
        uint32 product[2];
        iSNDmulu64(product, request->consumed, 1000);
        result->currenttime = iSNDdivu64(product[0], product[1], rate);
        iSNDmulu64(product, request->total - request->consumed, 1000);
        result->timetoend = iSNDdivu64(product[0], product[1], rate);
        uint32 buffered = request->queued * 1000;
        if (rate == 0)
            abort();
        result->timebuffered = (buffered / rate);
    }
    iSNDleaveaudio();
    return 0;
}
