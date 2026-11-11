#include "xport.h"
#include "eaclib/psx/internal.h"

SoundStreamRequestPrefix *iSNDstreamgetrequestptr(sint32 requestId)
{
    FUNCTION_MARKER(0x80103508, "NFS4.EXE");
    if (requestId < 0)
        return 0;
    SoundStreamPrefix *stream = iSNDstreamgetstreamptr(requestId & 0xFF);
    if (stream == 0 || (sint8)stream->requestCount <= 0)
        return 0;
    SoundStreamRequestPrefix *request = stream->requests;
    sint32 index = 0;
    do
    {
        if (request->id == ((uint32)(requestId)))
            return request;
        ++request;
        ++index;
    } while (index < (sint8)stream->requestCount);
    return 0;
}
