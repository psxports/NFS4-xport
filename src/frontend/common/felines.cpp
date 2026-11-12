#include "xport.h"
#include "frontend/common/fefades.h"
#include "frontend/psx/psxfront.h"

void PSXDrawBrightEndLine(sint32 col, sint32 x, sint32 y, sint32 w, sint32 h, sint32 side, sint32 fade, sint32 darksides)
{
    FUNCTION_MARKER(0x8004BB38, "FRONT.BIN");
    PSXTransDrawBrightEndLine(col, x, y, w, h, side, fade, darksides, 0);
}

void PSXTransDrawBrightEndLine(sint32 col, sint32 x, sint32 y, sint32 w, sint32 h, sint32 side, sint32 fade, sint32 darksides, sint32 opacity)
{
    FUNCTION_MARKER(0x8004BB78, "FRONT.BIN");
    if (fade == 128)
        return;
    sint32 BrightCol = CalcFadeVal(col, 0xFFFFFF, 64);
    sint32 ww = w;
    sint32 hh = h;
    if (fade != 0)
    {
        if ((uint32)side < 2)
        {
            sint32 scaled = (h * (128 - fade));
            if (scaled < 0)
                scaled += 127;
            hh = scaled >> 7;
            sint32 endbit = scaled >> 10;
            if (endbit > 8)
                endbit = 8;
            if (side == 1)
                PSXDrawGouraudSquare(x, (((y + hh) - endbit)), ww, endbit, col, col, BrightCol, BrightCol);
            else
            {
                y = ((y + h) - hh);
                PSXDrawGouraudSquare(x, y, ww, endbit, BrightCol, BrightCol, col, col);
            }
        }
        else
        {
            sint32 scaled = (w * (128 - fade));
            if (scaled < 0)
                scaled += 127;
            ww = scaled >> 7;
            sint32 endbit = scaled >> 10;
            if (endbit > 8)
                endbit = 8;
            if (side == 2)
            {
                x = ((x + w) - ww);
                PSXDrawGouraudSquare(x, y, endbit, hh, BrightCol, col, BrightCol, col);
            }
            else
                PSXDrawGouraudSquare((((x + ww) - endbit)), y, endbit, hh, col, BrightCol, col, BrightCol);
        }
    }
    if (darksides != 0)
    {
        if (side == 2 || fade == 0)
            PSXDrawGouraudSquare((((x + ww) - darksides)), y, darksides, hh, col, 0, col, 0);
        if (side == 3 || fade == 0)
            PSXDrawGouraudSquare(x, y, darksides, hh, 0, col, 0, col);
    }
    if (opacity == 0)
        PSXDrawSquare(col, x, y, ww, hh);
    else
        PSXDrawTransSquare(col, x, y, ww, hh, 1);
}
