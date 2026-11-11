#include <limits.h>
#include <stdlib.h>
#include "xport.h"
#include "lib/psx/math64.h"

void make64(sint64 *result, sint32 value, sint32 shift)
{
    FUNCTION_MARKER(0x800FE488, "NFS4.EXE");
    if (shift < INT_MIN + 32)
        abort();
    uint32 low;
    uint32 high;
    if (shift >= 32)
    {
        low = 0;
        high = (uint32)value << (((uint32)shift - 32U) & 31U);
    }
    else if (shift > 0)
    {
        low = (uint32)value << ((uint32)shift & 31U);
        high = (uint32)(value >> ((32U - (uint32)shift) & 31U));
    }
    else
    {
        low = (uint32)value;
        high = (uint32)(value >> 31);
    }
    *result = (sint64)(((uint64)high << 32) | low);
}

uint32 divu64(uint64 dividend, uint32 divisor)
{
    FUNCTION_MARKER(0x800FE4E0, "NFS4.EXE");
    uint32 low = (uint32)dividend;
    uint32 remainder = (uint32)(dividend >> 32);
    uint32 quotient = 0;
    for (sint32 bit = 0; bit < 32; ++bit)
    {
        quotient <<= 1;
        remainder = (remainder << 1) + (low >> 31);
        low <<= 1;
        if (remainder >= divisor)
        {
            remainder -= divisor;
            ++quotient;
        }
    }
    return quotient;
}
