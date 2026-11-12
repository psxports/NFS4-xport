#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

void transmult(matrixtdef *left, matrixtdef *right, matrixtdef *result)
{
    FUNCTION_MARKER(0x80105F40, "NFS4.EXE");
    matrixtdef product;
    for (sint32 row = 0; row < 3; ++row)
    {
        for (sint32 column = 0; column < 3; ++column)
        {
            uint32 first = fixedmult(left->m[row * 3], right->m[column]);
            uint32 second = fixedmult(left->m[row * 3 + 1], right->m[column + 3]);
            uint32 third = fixedmult(left->m[row * 3 + 2], right->m[column + 6]);
            product.m[row * 3 + column] = (first + second + third);
        }
    }
    *result = product;
}
