#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSNDabs(sint32 value)
{
    FUNCTION_MARKER(0x8010AEDC, "NFS4.EXE");
    return value < 0 ? (sint32)(0 - (uint32)value) : value;
}

void iSNDatodlrv(sint32 angle, sint32 volume, sint32 *left, sint32 *right, sint32 previousLeft, sint32 previousRight)
{
    FUNCTION_MARKER(0x8010AEF4, "NFS4.EXE");
    iSNDlibatodlrv(angle, volume, left, right);
    sint32 sourceQuadrant;
    if (previousLeft < 0 && previousRight < 0)
        sourceQuadrant = previousLeft < previousRight ? 1 : 2;
    else if (previousLeft >= 0 && previousRight >= 0)
        sourceQuadrant = previousLeft < previousRight ? 2 : 1;
    else
    {
        sint32 leftMagnitude = iSNDabs(previousLeft);
        sint32 rightMagnitude = iSNDabs(previousRight);
        sourceQuadrant = rightMagnitude < leftMagnitude ? 4 : 3;
    }
    sint32 flip = 0;
    if ((uint32)angle - 0x4000 < 0x8000)
    {
        sint32 targetQuadrant = *left < *right ? 3 : 4;
        if (sourceQuadrant < 3)
            flip = targetQuadrant == 3 && sourceQuadrant == 2 ? 1 : 2;
    }
    else
    {
        sint32 targetQuadrant = *left < *right ? 2 : 1;
        if (sourceQuadrant >= 3)
            flip = targetQuadrant == 2 && sourceQuadrant == 3 ? 1 : 2;
    }
    if (previousLeft < 0)
        *left = (sint32)(0 - (uint32)*left);
    if (previousRight < 0)
        *right = (sint32)(0 - (uint32)*right);
    if (flip == 1)
        *left = (sint32)(0 - (uint32)*left);
    else if (flip == 2)
        *right = (sint32)(0 - (uint32)*right);
}
