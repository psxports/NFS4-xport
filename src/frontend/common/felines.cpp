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
            sint32 scaled = (sint32)((uint32)h * (128U - (uint32)fade));
            if (scaled < 0)
                scaled += 127;
            hh = scaled >> 7;
            sint32 endbit = scaled >> 10;
            if (endbit > 8)
                endbit = 8;
            if (side == 1)
                PSXDrawGouraudSquare(x, (sint32)((uint32)y + (uint32)hh - (uint32)endbit), ww, endbit, col, col, BrightCol, BrightCol);
            else
            {
                y = (sint32)((uint32)y + (uint32)h - (uint32)hh);
                PSXDrawGouraudSquare(x, y, ww, endbit, BrightCol, BrightCol, col, col);
            }
        }
        else
        {
            sint32 scaled = (sint32)((uint32)w * (128U - (uint32)fade));
            if (scaled < 0)
                scaled += 127;
            ww = scaled >> 7;
            sint32 endbit = scaled >> 10;
            if (endbit > 8)
                endbit = 8;
            if (side == 2)
            {
                x = (sint32)((uint32)x + (uint32)w - (uint32)ww);
                PSXDrawGouraudSquare(x, y, endbit, hh, BrightCol, col, BrightCol, col);
            }
            else
                PSXDrawGouraudSquare((sint32)((uint32)x + (uint32)ww - (uint32)endbit), y, endbit, hh, col, BrightCol, col, BrightCol);
        }
    }
    if (darksides != 0)
    {
        if (side == 2 || fade == 0)
            PSXDrawGouraudSquare((sint32)((uint32)x + (uint32)ww - (uint32)darksides), y, darksides, hh, col, 0, col, 0);
        if (side == 3 || fade == 0)
            PSXDrawGouraudSquare(x, y, darksides, hh, 0, col, 0, col);
    }
    if (opacity == 0)
        PSXDrawSquare(col, x, y, ww, hh);
    else
        PSXDrawTransSquare(col, x, y, ww, hh, 1);
}
