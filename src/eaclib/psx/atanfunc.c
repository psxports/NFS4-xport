#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

sint32 intatan(sint32 x, sint32 z)
{
    FUNCTION_MARKER(0x800E5B38, "NFS4.EXE");
    uint32 numerator = (uint32)x;
    uint32 denominator = (uint32)z;
    sint32 reverse = 0;
    sint32 quadrant = 0;
    if ((sint32)denominator < 0)
    {
        denominator = 0U - denominator;
        reverse = 1;
        quadrant = 0x200;
    }
    if ((sint32)numerator < 0)
    {
        numerator = 0U - numerator;
        reverse = 1 - reverse;
        quadrant = -quadrant;
    }
    if ((sint32)denominator < (sint32)numerator)
    {
        uint32 temp = numerator;
        numerator = denominator;
        denominator = temp;
        quadrant += reverse ? -0x100 : 0x100;
        reverse = 1 - reverse;
    }
    uint32 angle = 0x80;
    if (numerator != denominator)
    {
        uint32 index;
        if ((numerator & 0xff800000U) != 0)
        {
            sint64 scaledNumerator;
            make64(&scaledNumerator, (sint32)numerator, 32);
            uint32 quotient = divu64((uint64)scaledNumerator, denominator);
            index = quotient >> 24;
            if ((quotient & 0x800000U) != 0)
                ++index;
        }
        else
        {
            uint32 scaledNumerator = (numerator << 8) + (uint32)((sint32)denominator >> 1);
            if (denominator == 0)
                abort();
            index = scaledNumerator / denominator;
        }
        angle = atantbl[index];
    }
    return reverse ? (sint32)((uint32)quadrant - angle) : (sint32)((uint32)quadrant + angle);
}
