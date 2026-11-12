#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

coorddef *crossproduct(coorddef *left, coorddef *right, coorddef *result)
{
    FUNCTION_MARKER(0x800EAAE4, "NFS4.EXE");

    sint32 first = fixedmult(left->y, right->z);
    sint32 second = fixedmult(left->z, right->y);
    result->x = (first - second);
    first = fixedmult(left->x, right->z);
    second = fixedmult(left->z, right->x);
    result->y = (second - first);
    first = fixedmult(left->x, right->y);
    second = fixedmult(left->y, right->x);
    result->z = (first - second);
    return result;
}
