#include <stddef.h>
#include "xport.h"
#include "psx.h"
#include "frontend/psx/mmeffect.h"
#include "game/psx/rendercache.h"

void FeDraw_SetABRMode(sint32 abr)
{
    FUNCTION_MARKER(0x8004D6D8, "FRONT.BIN");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    DR_MODE *dr_mode = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(dr_mode, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(dr_mode + 1);
    setaddr(ot, dr_mode);
    uint16 tpage = GetTPage(2, abr, 0, 256);
    SetDrawMode(dr_mode, 0, 0, tpage, 0);
}
