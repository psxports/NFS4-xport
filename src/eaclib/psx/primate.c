#include "xport.h"
#include "eaclib/psx/internal.h"

void *otbuf;
void *primbuf;
uint32 linkmodeflag;
sint32 oti;
sint32 otp;
uint32 *otbl;
uint32 *alternateOT;
sint32 maxot;
char *primbase;
char *alternatePrimitives;
sint32 maxprim;

void *initlinkmode(sint32 mode, sint32 maxprims, uint32 flags)
{
    FUNCTION_MARKER(0x800F05F4, "NFS4.EXE");
    maxot = 16;
    maxprim = maxprims;
    if (otbuf != 0)
        purgememadr(otbuf);
    uint32 bytes = ((uint32)maxprim * 13 + (uint32)maxot) * 8;
    char *storage = ((char *)(reservememadr("Draw Lists", (sint32)bytes, 16)));
    sint32 otCount = maxot;
    otbuf = storage;
    otbl = ((uint32 *)(storage));
    char *primitives = storage + ((uint32)otCount << 3);
    alternateOT = ((uint32 *)(storage + ((uint32)otCount << 2)));
    primbuf = primitives;
    primbase = primitives;
    alternatePrimitives = primitives + ((uint32)maxprims * 13 << 2);
    ClearOTagR(((uint32 *)(storage)), otCount);
    void *result = otbuf;
    char *firstPrimitives = ((char *)(primbuf));
    uint32 *firstOT = otbl;
    linkmodeflag = flags;
    oti = 0;
    otp = 0;
    drawpending = 0;
    primptr = firstPrimitives;
    nextprim = firstOT;
    return result;
}

void waitdraw(void)
{
    FUNCTION_MARKER(0x800F06E0, "NFS4.EXE");
    if (drawpending != 0)
    {
        DrawSync(0);
        drawpending = 0;
    }
}

sint32 settrans(sint32 mode)
{
    FUNCTION_MARKER(0x800F070C, "NFS4.EXE");
    if (mode >= 0)
        semitrans = mode == 0 ? 1U : 3U;
    return (sint32)semitrans >> 1;
}
