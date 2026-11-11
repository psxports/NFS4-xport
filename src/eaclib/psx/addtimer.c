#include "xport.h"
#include "eaclib/psx/internal.h"

void addtimer(void (*callback)(void))
{
    FUNCTION_MARKER(0x800EAFE8, "NFS4.EXE");
    for (sint32 i = 0; i < 8; ++i)
        if (tmrsub[i] == callback)
            return;
    for (sint32 i = 0; i < 8; ++i)
        if (tmrsub[i] == 0)
        {
            tmrsub[i] = callback;
            return;
        }
}

void deltimer(void (*callback)(void))
{
    FUNCTION_MARKER(0x800EB048, "NFS4.EXE");
    for (sint32 i = 0; i < 8; ++i)
        if (tmrsub[i] == callback)
        {
            tmrsub[i] = 0;
            return;
        }
}
