#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

sint32 fixedatan(sint32 x, sint32 z)
{
    FUNCTION_MARKER(0x800ED528, "NFS4.EXE");
    uint32 numerator = x;
    uint32 denominator = z;
    sint32 quadrant = 0;
    if (z < 0) {
        quadrant = 2;
        denominator = (~(denominator) + 1);
    }
    if (x < 0) {
        quadrant |= 4;
        numerator = (~(numerator) + 1);
    }
    sint32 angle;
    if (numerator == denominator) {
        angle = 0x2000;
    } else {
        if ((sint32)denominator < (sint32)numerator) {
            uint32 swap = numerator;
            numerator = denominator;
            denominator = swap;
            quadrant |= 1;
        }
        sint64 scaled;
        make64(&scaled, numerator, 32);
        uint32 quotient = divu64(scaled, denominator);
        sint32 index = quotient >> 24;
        sint32 fraction = (quotient >> 8) & 0xFFFF;
        sint32 first = fatantbl[index];
        sint32 difference = fatantbl[index + 1] - first;
        angle = first + ((difference * fraction) >> 16);
    }
    switch (quadrant) {
    case 1:
        return 0x4000 - angle;
    case 2:
        return 0x8000 - angle;
    case 3:
        return angle + 0x4000;
    case 4:
        return -angle;
    case 5:
        return angle - 0x4000;
    case 6:
        return angle - 0x8000;
    case 7:
        return -angle - 0x4000;
    default:
        return angle;
    }
}
