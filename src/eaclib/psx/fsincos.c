#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

void fixedsincos(sint32 angle, sint32 *sine, sint32 *cosine)
{
    FUNCTION_MARKER(0x800F3670, "NFS4.EXE");

    sint32 baseSine;
    sint32 baseCosine;
    intsincos(angle >> 6, &baseSine, &baseCosine);
    sint32 factor = ((angle & 63) * 0x6487e) >> 9;
    *sine = baseSine + (((baseCosine >> 2) * factor) >> 21);
    *cosine = baseCosine - (((baseSine >> 2) * factor) >> 21);
}
