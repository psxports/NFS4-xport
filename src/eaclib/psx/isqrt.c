#include <stdbool.h>

#include "xport.h"

#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/globals.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/math64.h"
#include "lib/psx/sinfunc.h"

uint32 isqrt(uint32 value)
{
    FUNCTION_MARKER(0x800F32CC, "NFS4.EXE");
    if ((value & 0xFFFF0000) != 0)
    {
        sint32 shift = (value & 0xFF000000) != 0 ? 8 : 4;
        uint32 index = value >> (shift == 8 ? 24 : 16);
        uint32 lower = (uint32)isqrttbl[index - 1] << shift;
        uint32 upper = ((uint32)isqrttbl[index] + 1) << shift;
        while (upper - lower >= 2)
        {
            uint32 middle = (lower + upper) >> 1;
            if (value < middle * middle)
                upper = middle;
            else
                lower = middle;
        }
        return lower;
    }
    if ((value & 0xFF00) != 0)
    {
        uint32 index;
        sint32 shift;
        if ((value & 0xF000) != 0)
        {
            if ((value & 0xC000) != 0)
            {
                index = value >> 8;
                shift = 0;
            }
            else
            {
                index = value >> 6;
                shift = 1;
            }
        }
        else if ((value & 0xC00) != 0)
        {
            index = value >> 4;
            shift = 2;
        }
        else
        {
            index = value >> 2;
            shift = 3;
        }
        uint32 lower = isqrttbl[index - 1] >> shift;
        uint32 upper = (isqrttbl[index] >> shift) + 1;
        if (upper - lower < 2)
            return lower;
        uint32 middle = (lower + upper) >> 1;
        if (value < middle * middle)
            return lower;
        return middle;
    }
    if (value == 0)
        return 0;
    return isqrttbl[value - 1] >> 4;
}
