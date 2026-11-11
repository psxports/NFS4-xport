#include "xport.h"
#include "eaclib/psx/internal.h"

void iSNDmulu64(uint32 *result, uint32 left, uint32 right)
{
    FUNCTION_MARKER(0x80103424, "NFS4.EXE");
    uint64 product = (uint64)left * right;
    result[0] = (uint32)product;
    result[1] = (uint32)(product >> 32);
}

uint32 iSNDdivu64(uint32 low, uint32 high, uint32 divisor)
{
    FUNCTION_MARKER(0x801034C0, "NFS4.EXE");
    uint32 quotient = 0;
    for (sint32 bit = 0; bit < 32; ++bit)
    {
        quotient <<= 1;
        high = (high << 1) + (low >> 31);
        low <<= 1;
        if (high >= divisor)
        {
            high -= divisor;
            ++quotient;
        }
    }
    return quotient;
}
