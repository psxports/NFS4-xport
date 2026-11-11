#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 MEM::defaultevent(...)
{
    FUNCTION_MARKER(0x800F4178, "NFS4.EXE");
    return 0;
}

extern "C" sint32 initmemadr(void *memory, sint32 size)
{
    FUNCTION_MARKER(0x800F4180, "NFS4.EXE");
    sint32 available = creatememclass(0, "RAM", memory, size, 8, 32, 0, 0, 0, 0, 0, MEM::defaultevent);
    memclass[1] = memclass[0];
    return available;
}
