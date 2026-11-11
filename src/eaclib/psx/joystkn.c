#include "xport.h"
#include "eaclib/psx/internal.h"

uint32 numjoy;
uint32 joy_inited;

uint32 initjoy(uint32 count)
{
    FUNCTION_MARKER(0x800F39F8, "NFS4.EXE");
    uint32 initialized = joy_inited;
    numjoy = count;
    if (initialized == 0)
    {
        padinit();
        joy_inited = 1;
        return 1;
    }
    return initialized;
}
