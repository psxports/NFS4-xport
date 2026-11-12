#include "xport.h"
#include "lib/psx/fixdinv.h"

sint32 fixedinverse(sint32 value)
{
    FUNCTION_MARKER(0x800ED3EC, "NFS4.EXE");

    uint32 sign = value < 0 ? 0xffffffff : 0;
    uint32 magnitude = (value ^ sign) - sign;
    uint32 quotient;
    uint32 remainder;
    if (magnitude == 0)
    {
        quotient = 0xffffffff;
        remainder = 0xffffffff;
    }
    else
    {
        quotient = 0xffffffff / magnitude;
        remainder = 0xffffffff % magnitude;
    }
    if ((uint32)((sint32)magnitude >> 1) < remainder)
        ++quotient;
    return ((quotient ^ sign) - sign);
}
