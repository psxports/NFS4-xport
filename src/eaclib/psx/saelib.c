#include "xport.h"
#include "eaclib/psx/internal.h"

void iSNDlibatodlrv(sint32 angle, sint32 volume, sint32 *left, sint32 *right)
{
    FUNCTION_MARKER(0x8010C894, "NFS4.EXE");
    sint32 sine = iSNDsin(angle >> 6);
    sint32 leftValue = (volume * (65536 - (uint32)sine)) >> 16;
    *left = leftValue;
    if (leftValue >= 128) *left = 127;
    sint32 rightValue = (volume * ((uint32)sine + 65536)) >> 16;
    *right = rightValue;
    if (rightValue >= 128) *right = 127;
}
