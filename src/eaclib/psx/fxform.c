#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

void fixedxformx(matrixtdef *matrix, sint32 angle)
{
    FUNCTION_MARKER(0x800EABAC, "NFS4.EXE");

    sint32 sine;
    sint32 cosine;
    fixedsincos(angle, &sine, &cosine);
    matrix->m[0] = 65536;
    matrix->m[1] = 0;
    matrix->m[2] = 0;
    matrix->m[3] = 0;
    matrix->m[6] = 0;
    matrix->m[5] = sine;
    matrix->m[4] = cosine;
    matrix->m[7] = (-sine);
    matrix->m[8] = cosine;
}

void fixedxformy(matrixtdef *matrix, sint32 angle)
{
    FUNCTION_MARKER(0x800EAC10, "NFS4.EXE");

    sint32 sine;
    sint32 cosine;
    fixedsincos(angle, &sine, &cosine);
    matrix->m[1] = 0;
    matrix->m[3] = 0;
    matrix->m[4] = 65536;
    matrix->m[5] = 0;
    matrix->m[7] = 0;
    matrix->m[0] = cosine;
    matrix->m[2] = (-sine);
    matrix->m[6] = sine;
    matrix->m[8] = cosine;
}

void fixedxformz(matrixtdef *matrix, sint32 angle)
{
    FUNCTION_MARKER(0x800EAC74, "NFS4.EXE");

    sint32 sine;
    sint32 cosine;
    fixedsincos(angle, &sine, &cosine);
    matrix->m[2] = 0;
    matrix->m[5] = 0;
    matrix->m[6] = 0;
    matrix->m[7] = 0;
    matrix->m[8] = 65536;
    matrix->m[1] = sine;
    matrix->m[0] = cosine;
    matrix->m[3] = (-sine);
    matrix->m[4] = cosine;
}
