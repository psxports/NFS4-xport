#include "xport.h"
#include "lib/psx/fixdmult.h"

sint32 fixedmult(sint32 left, sint32 right)
{
    FUNCTION_MARKER(0x800E4328, "NFS4.EXE");

    sint64 product = (sint64)left * right + 0x8000;
    return (sint32)((uint64)product >> 16);
}
