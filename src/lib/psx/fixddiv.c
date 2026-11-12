#include <stdlib.h>
#include "xport.h"
#include "lib/psx/fixddiv.h"

sint32 fixeddiv(sint32 numerator, sint32 denominator)
{
    FUNCTION_MARKER(0x800E4404, "NFS4.EXE");

    uint32 magnitudeNumerator = numerator < 0 ? (~(((uint32)numerator)) + 1) : (uint32)numerator;
    uint32 magnitudeDenominator = denominator < 0 ? (~(((uint32)denominator)) + 1) : (uint32)denominator;
    if (magnitudeDenominator == 0)
        return 0;
    uint32 remainder = magnitudeNumerator % magnitudeDenominator;
    uint32 quotient = (magnitudeNumerator / magnitudeDenominator) << 16;
    uint32 bit = 0x10000;
    if ((sint32)remainder < 0)
        magnitudeDenominator >>= 1;
    else
        remainder <<= 1;
    do
    {
        sint32 high = (sint32)remainder < 0;
        bit >>= 1;
        if (remainder >= magnitudeDenominator)
        {
            quotient |= bit;
            remainder -= magnitudeDenominator;
        }
        if (high)
            magnitudeDenominator >>= 1;
        else
            remainder <<= 1;
    } while (bit != 1);
    if ((numerator < 0) != (denominator < 0))
    {
        if (quotient == 0x80000000)
            abort();
        quotient = (~(quotient) + 1);
    }
    return quotient;
}
