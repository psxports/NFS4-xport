#include "xport.h"
#include "eaclib/psx/internal.h"

void iSNDatolrv(sint32 angle, sint32 volume, sint32 *left, sint32 *right)
{
    FUNCTION_MARKER(0x8010B0A8, "NFS4.EXE");
    iSNDlibatodlrv(angle, volume, left, right);
    sint32 value = *left;
    if (value < 0) *left = (-value);
    value = *right;
    if (value < 0) *right = (-value);
}
