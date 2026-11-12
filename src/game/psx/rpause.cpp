#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/render.h"
#include "game/psx/draw.h"
#include "game/psx/rpause.h"

void RPause::CopyBackToFrontBuffer(void)
{
    FUNCTION_MARKER(0x800DCEC4, "NFS4.EXE");
    DRAWENV *drawenv = Draw::GetDRAWENV(Draw::gPlayer1View, 1);
    PSX_RECT rscreen;
    memcpy(&rscreen, &drawenv->clip, sizeof(rscreen));
    sint32 x = rscreen.x;
    sint32 offsetX = gPauseMenuRect.x;
    sint32 viewid = Draw::gPlayer1View;
    sint16 width = gPauseMenuRect.w;
    sint16 height = gPauseMenuRect.h;
    rscreen.x = (sint16)(x + offsetX);
    sint32 y = rscreen.y;
    sint32 offsetY = gPauseMenuRect.y;
    rscreen.w = width;
    rscreen.h = height;
    rscreen.y = (sint16)(y + offsetY);
    drawenv = Draw::GetDRAWENV(viewid, 0);
    sint32 destinationX = drawenv->clip.x + gPauseMenuRect.x;
    sint32 destinationY = drawenv->clip.y + gPauseMenuRect.y;
    MoveImage(&rscreen, destinationX, destinationY);
}

void RPause::StartPauseMenu(void)
{
    FUNCTION_MARKER(0x800DCF84, "NFS4.EXE");
    Draw::StartFrameRender();
}

void RPause::StopPauseMenu(void)
{
    FUNCTION_MARKER(0x800DCFA4, "NFS4.EXE");
    PutDispEnv(&gEnviro[1].disp);
    DrawSync(0);
    VSync(0);
    CopyBackToFrontBuffer();
    sint32 frame = gFlip;
    sint32 viewid = Render::gPauseMenuView;
    Draw::tView *view = &Draw::gView[viewid];
    sint32 otsize = view->otsize;
    uint32 *ot = view->ot[frame];
    DrawOTag(ot + otsize - 1);
    systemtask(0);
    gFlip = (1 - gFlip);
}
