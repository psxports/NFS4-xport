#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

matrixtdef identitymatrix = {{65536, 0, 0, 0, 65536, 0, 0, 0, 65536}};
sint32 reorthogonalizeCoefficients[4] = {16384, -8192, 6144, -5120};

matrixtdef *addmatrix(matrixtdef *left, matrixtdef *right, matrixtdef *result)
{
    FUNCTION_MARKER(0x800F01FC, "NFS4.EXE");
    for (sint32 i = 0; i < 9; ++i)
    {
        sint32 rightValue = right->m[i];
        sint32 leftValue = left->m[i];
        result->m[i] = (leftValue + rightValue);
    }
    return result;
}

matrixtdef *submatrix(matrixtdef *left, matrixtdef *right, matrixtdef *result)
{
    FUNCTION_MARKER(0x800F0234, "NFS4.EXE");
    for (sint32 i = 0; i < 9; ++i)
    {
        sint32 rightValue = right->m[i];
        sint32 leftValue = left->m[i];
        result->m[i] = (leftValue - rightValue);
    }
    return result;
}

matrixtdef *scalematrix(matrixtdef *matrix, sint32 scale, matrixtdef *result)
{
    FUNCTION_MARKER(0x800F026C, "NFS4.EXE");
    for (sint32 i = 0; i < 9; ++i)
        result->m[i] = fixedmult(matrix->m[i], scale);
    return result;
}

void reorthogonalize(matrixtdef *matrix)
{
    FUNCTION_MARKER(0x800F02E4, "NFS4.EXE");
    matrixtdef temporary;
    matrixtdef gram;
    matrixtdef error;
    matrixtdef power;
    matrixtdef correction;
    for (sint32 iteration = 0; iteration < 4; ++iteration)
    {
        transpose(matrix, &temporary);
        transmult(&temporary, matrix, &gram);
        submatrix(&gram, &identitymatrix, &error);
        power = identitymatrix;
        correction = identitymatrix;
        for (sint32 coefficient = 1; coefficient < 4; ++coefficient)
        {
            transmult(&power, &error, &temporary);
            power = temporary;
            scalematrix(&power, reorthogonalizeCoefficients[coefficient], &gram);
            addmatrix(&correction, &gram, &correction);
        }
        temporary = *matrix;
        transmult(&temporary, &correction, matrix);
    }
}
