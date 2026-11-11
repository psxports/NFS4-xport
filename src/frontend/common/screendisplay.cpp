#include "xport.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/screendisplay.h"
#include "frontend/psx/psxfront.h"

tScreenDisplay *screenDisplay;

void tScreenDisplay::DrawBackground(void)
{
    FUNCTION_MARKER(0x80045950, "FRONT.BIN");
    sint16 fade = (sint16)(menuDefs->menuDisplayOptions.fScreenFade >> 1);
    if (fade > 128)
        fade = 128;
    for (sint32 i = 0; i < 32; ++i)
        DrawShapeExtended(i + 11, 0, 0, 0, fade, 0, 0);
}

void tScreenDisplay::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x800459DC, "FRONT.BIN");
    *swapFileName = 0;
    numSwapShapes = 0;
    numPermShapes = 43;
    *permFileName = const_cast<char *>("zDsply");
}
