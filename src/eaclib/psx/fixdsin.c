#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

sint32 fixedsin(sint32 angle)
{
    FUNCTION_MARKER(0x800ED424, "NFS4.EXE");

    sint32 base = fastintsin(angle >> 6);
    sint32 slope = fastintcos(angle >> 6);
    sint32 factor = ((angle & 63) * 0x6487e) >> 9;
    return base + (((slope >> 2) * factor) >> 21);
}

sint32 fixedcos(sint32 angle)
{
    FUNCTION_MARKER(0x800ED4A4, "NFS4.EXE");

    sint32 base = fastintcos(angle >> 6);
    sint32 slope = -fastintsin(angle >> 6);
    sint32 factor = ((angle & 63) * 0x6487e) >> 9;
    return base + (((slope >> 2) * factor) >> 21);
}
