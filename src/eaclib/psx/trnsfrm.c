#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

void transform(coorddef *vector, matrixtdef *matrix, coorddef *result)
{
    FUNCTION_MARKER(0x800EAED8, "NFS4.EXE");

    sint32 values[3];
    for (sint32 i = 0; i < 3; ++i)
    {
        uint32 first = fixedmult(vector->x, matrix->m[i]);
        uint32 second = fixedmult(vector->y, matrix->m[i + 3]);
        uint32 third = fixedmult(vector->z, matrix->m[i + 6]);
        values[i] = (first + second + third);
    }
    result->x = values[0];
    result->y = values[1];
    result->z = values[2];
}
