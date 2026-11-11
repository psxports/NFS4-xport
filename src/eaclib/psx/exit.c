#include "xport.h"
#include "eaclib/psx/internal.h"

void addexit(GameTimerCallback callback)
{
    FUNCTION_MARKER(0x800F1CF8, "NFS4.EXE");
    for (sint32 i = 0; i < 32; ++i)
        if (gameExitCallbacks[i] == callback)
            return;
    for (sint32 i = 0; i < 32; ++i)
        if (gameExitCallbacks[i] == 0)
        {
            gameExitCallbacks[i] = callback;
            return;
        }
}
