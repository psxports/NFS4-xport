#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

void transpose(matrixtdef *source, matrixtdef *destination)
{
    FUNCTION_MARKER(0x800E4358, "NFS4.EXE");

    if (source == destination)
    {
        sint32 m01 = destination->m[1];
        sint32 m02 = destination->m[2];
        sint32 m12 = destination->m[5];
        destination->m[1] = destination->m[3];
        destination->m[2] = destination->m[6];
        sint32 m21 = destination->m[7];
        destination->m[3] = m01;
        destination->m[6] = m02;
        destination->m[7] = m12;
        destination->m[5] = m21;
    }
    else
    {
        destination->m[0] = source->m[0];
        destination->m[1] = source->m[3];
        destination->m[2] = source->m[6];
        destination->m[3] = source->m[1];
        destination->m[4] = source->m[4];
        destination->m[5] = source->m[7];
        destination->m[6] = source->m[2];
        destination->m[7] = source->m[5];
        destination->m[8] = source->m[8];
    }
}
