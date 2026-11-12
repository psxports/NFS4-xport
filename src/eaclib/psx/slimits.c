#include "xport.h"
#include "eaclib/psx/internal.h"
#include "slimits.h"

sint32 SNDgetlimits(struct SNDLIMITS *limits)
{
    FUNCTION_MARKER(0x800E9F84, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -14;
    uint8 first[12];
    memcpy(first, &sndpsxlimits, 12);
    memcpy(limits, first, 12);
    uint8 last[8];
    memcpy(last, ((uint8 *)(&sndpsxlimits)) + 12, 8);
    memcpy(((uint8 *)(limits)) + 12, last, 8);
    return 0;
}

sint32 SNDsetlimits(struct SNDLIMITS *limits)
{
    FUNCTION_MARKER(0x800E9FD4, "NFS4.EXE");
    if (sndgs.initialized == 0)
        return -10;
    if (limits->packetbufsize < 0x2000)
        return -1;
    uint8 first[12];
    memcpy(first, limits, 12);
    memcpy(&sndpsxlimits, first, 12);
    uint8 last[8];
    memcpy(last, ((uint8 *)(limits)) + 12, 8);
    memcpy(((uint8 *)(&sndpsxlimits)) + 12, last, 8);
    sndpsxlimits.packetbufsize = (((sndpsxlimits.packetbufsize)) & 0xFFFFE000u);
    return 0;
}
