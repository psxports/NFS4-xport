#include <stddef.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "game/common/gmesetup.h"
#include "game/psx/draw.h"
#include "game/psx/drawc.h"
#include "game/psx/platform.h"
#include "game/psx/texture.h"

struct shapetbl;

sint32 gFlip;
sint32 gLoop;
sint32 gTotalMem;
dflip gEnviro[2];
sint32 Draw::gNumView;
sint32 Draw::gViewOtSize;
sint32 Draw::gMidGroundOtz;
sint32 Draw::gDoVSync;
char *Draw::gMaxPrim;
Draw::tView Draw::gView[10];

sint32 Draw::SetView(sint32 x0, sint32 y0, sint32 x1, sint32 y1, sint32 w, sint32 h, sint32 dtd, sint32 isbg, sint32 otsize)
{
    FUNCTION_MARKER(0x800BDAC0, "NFS4.EXE");

    tView *newview = &gView[gNumView];
    DRAWENV *e00 = &newview->drawenv[0];
    DRAWENV *e10 = &newview->drawenv[1];
    SetDefDrawEnv(e00, x0, y0, w, h);
    SetDefDrawEnv(e10, x1, y1, w, h);
    e00->r0 = 0;
    e00->g0 = 0;
    e00->b0 = 0;
    e10->r0 = 0;
    e10->g0 = 0;
    e10->b0 = 0;
    e10->isbg = isbg;
    e00->isbg = isbg;
    e10->dtd = dtd;
    e00->dtd = dtd;
    sint32 viewid = gNumView;
    newview->otsize = otsize;
    newview->membudget = 0;
    gNumView = (sint32)((uint32)viewid + 1);
    return viewid;
}

void Draw::InitViews(void)
{
    FUNCTION_MARKER(0x800BDBD4, "NFS4.EXE");

    gNumView = 0;
}

void Draw::InitViewOT(void)
{
    FUNCTION_MARKER(0x800BDBE0, "NFS4.EXE");

    for (sint32 i = 0; i < gNumView; ++i)
    {
        gView[i].ot[0] = (uint32 *)reservememadr("ot0", (sint32)((uint32)gView[i].otsize * sizeof(*gView[i].ot[0])), 16);
        gView[i].ot[1] = (uint32 *)reservememadr("ot1", (sint32)((uint32)gView[i].otsize * sizeof(*gView[i].ot[1])), 16);
    }
}

void Draw::InitViewOTInGame(void)
{
    FUNCTION_MARKER(0x800BDC64, "NFS4.EXE");

    static char ot0[] = "ot0";
    static char ot1[] = "ot1";
    for (sint32 i = 0; i < gNumView; ++i)
    {
        gView[i].ot[0] = (uint32 *)Platform::ReserveMemory((sint32)((uint32)gView[i].otsize * sizeof(*gView[i].ot[0])), ot0);
        gView[i].ot[1] = (uint32 *)Platform::ReserveMemory((sint32)((uint32)gView[i].otsize * sizeof(*gView[i].ot[1])), ot1);
    }
}

void AllocatePrimitivesBuffer(void)
{
    FUNCTION_MARKER(0x800BDE60, "NFS4.EXE");

    if (GameSetup::gData.commMode == 1)
        Draw::InitViewOT();
    else
        Draw::InitViewOTInGame();
    gTotalMem = GameSetup::gData.commMode == 1 ? 0x22500 : 0x1f600;
    static char ps0[] = "ps0";
    static char ps1[] = "ps1";
    gEnviro[0].server = Platform::ReserveMemory(gTotalMem, ps0);
    gEnviro[1].server = Platform::ReserveMemory(gTotalMem, ps1);
    if (GameSetup::gData.commMode == 1)
    {
        Draw::tView *view0 = &Draw::gView[Draw::gPlayer1View];
        view0->membudget = (sint32)((uint32)(gTotalMem >> 1) - 0x1a00);
        Draw::tView *view1 = &Draw::gView[Draw::gPlayer2View];
        view1->membudget = (sint32)((uint32)(gTotalMem >> 1) - 0x1a00);
    }
    else
    {
        Draw::tView *view = &Draw::gView[Draw::gPlayer1View];
        view->membudget = (sint32)((uint32)gTotalMem - 0x1a00);
    }
}

void Draw::DeInitViews(void)
{
    FUNCTION_MARKER(0x800BDCE0, "NFS4.EXE");

    for (sint32 i = 0; i < gNumView; ++i)
    {
        if (gView[i].ot[0] != 0)
            purgememadr(gView[i].ot[0]);
        if (gView[i].ot[1] != 0)
            purgememadr(gView[i].ot[1]);
        gView[i].ot[0] = 0;
        gView[i].ot[1] = 0;
    }
}

void ClearPrimitivesBuffer(void)
{
    FUNCTION_MARKER(0x800BDF9C, "NFS4.EXE");

    DrawSync(0);
    if (gEnviro[0].server != 0)
        purgememadr(gEnviro[0].server);
    if (gEnviro[1].server != 0)
        purgememadr(gEnviro[1].server);
    gEnviro[1].server = 0;
    gEnviro[0].server = 0;
    Draw::DeInitViews();
}

void Draw::DeInitViewsInGame(void)
{
    FUNCTION_MARKER(0x800BDD68, "NFS4.EXE");

    for (sint32 i = 0; i < gNumView; ++i)
    {
        gView[i].ot[0] = 0;
        gView[i].ot[1] = 0;
    }
}

void ClearPlatformPrimitivesBuffer(void)
{
    FUNCTION_MARKER(0x800BE004, "NFS4.EXE");

    DrawSync(0);
    gEnviro[1].server = 0;
    gEnviro[0].server = 0;
    if (GameSetup::gData.commMode == 1)
        Draw::DeInitViews();
    else
        Draw::DeInitViewsInGame();
}

DRAWENV * Draw::GetDRAWENV(sint32 viewid, sint32 page)
{
    FUNCTION_MARKER(0x800BDD9C, "NFS4.EXE");

    return &gView[viewid].drawenv[page];
}

void Draw::SetViewMemBudget(sint32 viewid, sint32 totalmem)
{
    FUNCTION_MARKER(0x800BDDD8, "NFS4.EXE");

    gView[viewid].membudget = totalmem;
}

void Draw::SetViewColor(sint32 viewid, sint32 r, sint32 g, sint32 b)
{
    FUNCTION_MARKER(0x800BDE00, "NFS4.EXE");

    tView *view = &gView[viewid];
    if (view->drawenv[0].isbg != 0)
    {
        view->drawenv[0].r0 = r;
        view->drawenv[0].g0 = g;
        view->drawenv[0].b0 = b;
    }
    if (view->drawenv[1].isbg != 0)
    {
        view->drawenv[1].r0 = r;
        view->drawenv[1].g0 = g;
        view->drawenv[1].b0 = b;
    }
}

void Draw::StartRenderingView(sint32 viewid)
{
    FUNCTION_MARKER(0x800BE060, "NFS4.EXE");
    Draw::tCacheHeader *head = &DrawC::carCache.head;
    Draw::tView *view = &gView[viewid];
    gViewOtSize = view->otsize;
    sint32 midGround = (sint32)((uint32)view->otsize * 7U);
    if (midGround < 0)
        midGround = (sint32)((uint32)midGround + 7U);
    head->clipW = view->drawenv[0].clip.w;
    head->clipH = view->drawenv[0].clip.h;
    gMidGroundOtz = midGround >> 3;
    head->cprim.LastPrim = (uint32 *)view->ot[gFlip];
    if (viewid == gPlayer1View || viewid == gPlayer2View)
        head->cprim.MPrimPtr = head->cprim.PrimPtr + view->membudget;
    else
        head->cprim.MPrimPtr = gMaxPrim;
    head->mirror = 0;
}

void Draw::StopRenderingView(sint32 viewid)
{
    FUNCTION_MARKER(0x800BE118, "NFS4.EXE");
    Draw::tView *view = &gView[viewid];
    DRAWENV LEnv = view->drawenv[gFlip];
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    uint32 *last = cprim->LastPrim;
    DR_ENV *pEnv = (DR_ENV *)cprim->PrimPtr;
    setaddr(pEnv, getaddr(last + view->otsize - 1));
    uint32 *ot = last + view->otsize - 1;
    cprim->PrimPtr = (char *)pEnv + 64;
    setaddr(ot, pEnv);
    SetDrawEnv(pEnv, &LEnv);
}

void Draw::CheckFirstFrameRender(void)
{
    FUNCTION_MARKER(0x800BE230, "NFS4.EXE");

    if (gFlip == -1)
    {
        DirectSetEnvironment(320, 256, 320, 240, 1, 0, 1, 0, 0, 0);
        DirectSetEnvironment(0, 256, 320, 240, 1, 1, 1, 0, 0, 0);
        gFlip = 1;
        AllocatePrimitivesBuffer();
    }
}

void Draw::StartFrameRender(void)
{
    FUNCTION_MARKER(0x800BE2C0, "NFS4.EXE");
    for (sint32 i = 0; i < gNumView; ++i)
        ClearOTagR(gView[i].ot[gFlip], gView[i].otsize);
    DrawC::carCache.head.cprim.PrimPtr = gEnviro[gFlip].server;
    gMaxPrim = gEnviro[gFlip].server + gTotalMem;
}

void Draw::SetDrawSyncCallback(void (*p)(void))
{
    FUNCTION_MARKER(0x800BE360, "NFS4.EXE");

    gSyncCallback = p;
}

void Draw::StopFrameRender(void)
{
    FUNCTION_MARKER(0x800BE36C, "NFS4.EXE");

    DrawSync(0);
    gLoop = (sint32)((uint32)gLoop + 1);
    if (gSyncCallback != 0)
        gSyncCallback();
    if (gDoVSync != 0)
        VSync(0);
    PutDispEnv(&gEnviro[gFlip].disp);
    for (sint32 i = 0; i < gNumView; ++i)
        DrawOTag(gView[i].ot[gFlip] + gView[i].otsize - 1);
    gFlip = (sint32)(1 - (uint32)gFlip);
}

void Draw::DrawDirectScreen(shapetbl *tile, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x800BE450, "NFS4.EXE");

    Texture::Vramcf(tile, x, (sint32)((uint32)y + 256), 0, 0);
}

void Draw::DirectSetEnvironment(sint32 x, sint32 y, sint32 w, sint32 h, sint32 edraw, sint32 edisplay, sint32 erase, sint32 r, sint32 g, sint32 b)
{
    FUNCTION_MARKER(0x800BE478, "NFS4.EXE");

    if (edraw != 0)
    {
        DRAWENV e;
        SetDefDrawEnv(&e, x, y, w, h);
        if (erase != 0)
        {
            e.r0 = r;
            e.g0 = g;
            e.b0 = b;
            e.isbg = 1;
        }
        else
            e.isbg = 0;
        PutDrawEnv(&e);
    }
    if (edisplay != 0)
    {
        DISPENV e;
        SetDefDispEnv(&e, x, y, w, h);
        SetDispMask(0);
        PutDispEnv(&e);
        timedwait(timerhz >> 1);
        SetDispMask(1);
    }
}

void Draw::SetEnvironment(sint32 w, sint32 h, sint32 edraw, sint32 edisplay, sint32 erase, sint32 r, sint32 g, sint32 b)
{
    FUNCTION_MARKER(0x800BE57C, "NFS4.EXE");

    DirectSetEnvironment(0, 256, w, h, edraw, edisplay, erase, r, g, b);
}

void Draw::InitRenderEngine(sint32 x0, sint32 y0, sint32 x1, sint32 y1, sint32 w, sint32 h)
{
    FUNCTION_MARKER(0x800BE5D8, "NFS4.EXE");

    gFlip = -1;
    gEnviro[0].server = 0;
    gEnviro[1].server = 0;
    SetDefDispEnv(&gEnviro[0].disp, x0, y0, w, h);
    SetDefDispEnv(&gEnviro[1].disp, x1, y1, w, h);
    SetDrawSyncCallback(0);
}

void Draw::RestartRenderEngine(void)
{
    FUNCTION_MARKER(0x800BE67C, "NFS4.EXE");
}

void Draw::DeInitRenderEngine(void)
{
    FUNCTION_MARKER(0x800BE684, "NFS4.EXE");

    ClearPlatformPrimitivesBuffer();
    gFlip = -1;
}

void Draw::InitLibRender(void)
{
    FUNCTION_MARKER(0x800BE6A8, "NFS4.EXE");

    gLoop = 1;
    InitGeom();
}

void (*Draw::gSyncCallback)(void);
