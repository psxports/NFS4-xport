#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

void intsincos(sint32 angle, sint32 *sine, sint32 *cosine)
{
    FUNCTION_MARKER(0x800EADBC, "NFS4.EXE");

    sint32 index = angle & 255;
    switch (((uint32)angle >> 8) & 3)
    {
    case 0:
        *sine = sintbl[index];
        *cosine = sintbl[256 - index];
        break;
    case 1:
        *sine = sintbl[256 - index];
        *cosine = -sintbl[index];
        break;
    case 2:
        *sine = -sintbl[index];
        *cosine = -sintbl[256 - index];
        break;
    case 3:
        *sine = -sintbl[256 - index];
        *cosine = sintbl[index];
        break;
    }
}
