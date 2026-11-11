#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

void xformy(matrixtdef *matrix, sint32 angle)
{
    FUNCTION_MARKER(0x800E5AC4, "NFS4.EXE");
    sint32 sine;
    sint32 cosine;
    intsincos(angle, &sine, &cosine);
    matrix->m[1] = 0;
    matrix->m[3] = 0;
    matrix->m[4] = 65536;
    matrix->m[5] = 0;
    matrix->m[7] = 0;
    matrix->m[0] = cosine;
    matrix->m[2] = (sint32)(0U - (uint32)sine);
    matrix->m[6] = sine;
    matrix->m[8] = cosine;
}
