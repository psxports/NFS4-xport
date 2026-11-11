#include "xport.h"
#include "eaclib/psx/internal.h"

extern "C" void iSNDserveraddclient(void (*callback)(...))
{
    FUNCTION_MARKER(0x8010479C, "NFS4.EXE");
    sndgs.serviceClients[sndgs.serviceClientCount] = callback;
    ++sndgs.serviceClientCount;
}

extern "C" void iSNDserverremoveclient(void (*callback)(...))
{
    FUNCTION_MARKER(0x801047CC, "NFS4.EXE");
    for (sint32 i = 0; i < sndgs.serviceClientCount; ++i)
    {
        if (sndgs.serviceClients[i] == callback)
        {
            --sndgs.serviceClientCount;
            while (i < sndgs.serviceClientCount)
            {
                sndgs.serviceClients[i] = sndgs.serviceClients[i + 1];
                ++i;
            }
            return;
        }
    }
}

void SNDSYS::service(void)
{
    FUNCTION_MARKER(0x80104878, "NFS4.EXE");
    for (sint32 i = 0; i < sndgs.serviceClientCount; ++i)
        sndgs.serviceClients[i]();
}
