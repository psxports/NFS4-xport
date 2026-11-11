#include <stddef.h>
#include "xport.h"
#include "psx.h"
#include "game/common/bworld.h"
#include "game/common/camera.h"
#include "game/common/dashhud.h"
#include "game/common/genericpmx.h"
#include "game/common/gmesetup.h"
#include "game/common/mpause.h"
#include "game/common/r3dcar.h"
#include "game/common/render.h"
#include "game/common/renderview.h"
#include "game/common/souffle.h"
#include "game/common/track.h"
#include "game/common/trgsfx.h"
#include "game/psx/draw.h"
#include "game/psx/drawc.h"
#include "game/psx/draww.h"
#include "game/psx/flare.h"
#include "game/psx/hrzsku.h"
#include "game/psx/hud.h"
#include "game/psx/loading.h"
#include "game/psx/night.h"
#include "game/psx/rpause.h"
#include "game/psx/skidmark.h"
#include "game/psx/texture.h"
#include "game/psx/textureprocess.h"
#include "game/psx/trackspec.h"
#include "game/psx/trsproj.h"
#include "game/psx/weather.h"

sint32 gPauseRender;
DRender::tView gCView;
PSX_RECT gPauseMenuRect;
sint32 Draw::gPlayer1View;
sint32 Draw::gPlayer2View;
sint32 Draw::gRearView;
sint32 Render::gDebugView;
sint32 Render::gPauseMenuView;
sint32 Render::gBlurEffectDepth1;
sint32 Render::gBlurEffectDepth2;
sint32 Render::gBlurEffectMode;

void Render::CreatePlayerViews(void)
{
    FUNCTION_MARKER(0x800B30AC, "NFS4.EXE");
    if (GameSetup::gData.commMode == 1)
    {
        Draw::gPlayer1View = Draw::SetView(0, 256, 320, 256, 320, 120, 0, 1, 2000);
        Draw::gPlayer2View = Draw::SetView(0, 376, 320, 376, 320, 120, 0, 1, 2000);
    }
    else
    {
        Draw::gPlayer1View = Draw::SetView(0, 256, 320, 256, 320, 240, 0, 1, 3000);
        Draw::gPlayer2View = -1;
    }
}

void Render::CreatePauseMenuViews(void)
{
    FUNCTION_MARKER(0x800B3198, "NFS4.EXE");
    gPauseMenuView = Draw::SetView(0, 256, 320, 256, 320, 240, 0, 0, 1);
}

void Render::CreateViews(void)
{
    FUNCTION_MARKER(0x800B31E4, "NFS4.EXE");
    Draw::InitViews();
    CreatePlayerViews();
    Hud::CreateHudViews();
    CreatePauseMenuViews();
}

void Render::StartRenderingWorldView(sint32 viewid)
{
    FUNCTION_MARKER(0x800B321C, "NFS4.EXE");
    Camera::SetCamera(&gCView);
    Track::AnimateTrackLighting();
    Draw::StartRenderingView(viewid);
    sint32 mirror = GameSetup::gData.mirrorTrack;
    sint32 player = gCView.player;
    DrawC::carCache.head.mirror = mirror;
    Fog::Update(player);
    DrawC::SetEnviroment();
    TrsProj::ResetTransPrecision();
    Night::SetEnviroment(&gCView);
}

void Render::StopRenderingWorldView(sint32 viewid)
{
    FUNCTION_MARKER(0x800B3298, "NFS4.EXE");
    Draw::StopRenderingView(viewid);
}

void Render::RenderWorld(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800B32B8, "NFS4.EXE");
    BWorld::OnyxBuildFacets(Vi);
    Skidmark::OnyxBuildFacets(Vi);
    Souffle::InsertFacet(Vi);
    R3DCar::InsertAllListFacet(Vi);
    Hrz::BuildHorizon(Vi);
    Weather::BuildWeather(Vi);
    InsertDepthOfField();
}

void Render::RenderPlayerView(sint32 ViewID, sint32 Player)
{
    FUNCTION_MARKER(0x800B335C, "NFS4.EXE");
    gCView.id = ViewID;
    gCView.player = Player;
    StartRenderingWorldView(ViewID);
    RenderWorld(&gCView);
    Draw::StopRenderingView(ViewID);
}

void Render::RenderDebugView(void)
{
    FUNCTION_MARKER(0x800B33A8, "NFS4.EXE");
}

void Render::StartFrameRender(void)
{
    FUNCTION_MARKER(0x800B33B0, "NFS4.EXE");
    BWorld::StartLoop();
    Track::AnimateTextures();
    Souffle::DoSouffle();
    Draw::CheckFirstFrameRender();
    Draw::StartFrameRender();
}

void Render::StopFrameRender(void)
{
    FUNCTION_MARKER(0x800B33F0, "NFS4.EXE");
    Draw::StopFrameRender();
}

void Render::RenderPauseMenuView(void)
{
    FUNCTION_MARKER(0x800B3430, "NFS4.EXE");
    sint32 viewid = gPauseMenuView;
    gCView.id = viewid;
    Draw::StartRenderingView(viewid);
    MPause::Render();
    viewid = gPauseMenuView;
    sint32 ogFlip = gFlip;
    gFlip = 0;
    Draw::StopRenderingView(viewid);
    gFlip = ogFlip;
}

void Render_Render(sint32 pause)
{
    FUNCTION_MARKER(0x800B34A8, "NFS4.EXE");
    if (pause == 0)
    {
        if (gPauseRender != 0)
        {
            gPauseRender = 0;
            return;
        }
        if (GameSetup::gData.commMode == 1)
        {
            Render::StartFrameRender();
            Render::RenderPlayerView(Draw::gPlayer1View, 0);
            Render::RenderPlayerView(Draw::gPlayer2View, 1);
        }
        else
        {
            Render::StartFrameRender();
            Render::RenderPlayerView(Draw::gPlayer1View, 0);
        }
        Hud::Render();
        Render::RenderDebugView();
        Draw::StopFrameRender();
        return;
    }
    if (gPauseRender == 0)
    {
        gPauseMenuRect.w = 320;
        gPauseRender = 1;
        gPauseMenuRect.x = 0;
        gPauseMenuRect.y = 0;
        gPauseMenuRect.h = 240;
        DrawSync(0);
        RPause::CopyBackToFrontBuffer();
        MPause::StartPauseMenu();
        gMPauseUpdate = 1;
        gMPauseUpdateNextTime = 0;
    }
    sint32 update = gMPauseUpdate;
    gPauseMenuRect.x = 78;
    gPauseMenuRect.y = 98;
    gPauseMenuRect.w = 164;
    gPauseMenuRect.h = 139;
    if (update != 0)
    {
        Draw::StartFrameRender();
        Render::RenderPauseMenuView();
        RPause::StopPauseMenu();
    }
}

void FlareThing(void)
{
    FUNCTION_MARKER(0x800B35E8, "NFS4.EXE");
    sint32 players = GameSetup::gData.commMode == 1 ? 2 : 1;
    for (sint32 i = 0; i < players; ++i)
    {
        if (gFlare_LensFlare.isDrawn[i] == 0)
            continue;
        sint32 frame = gFlip;
        sint32 oldx = (uint16)gFlare_LensFlare.oldpos[i].vx;
        sint32 screenx = (uint16)gEnviro[frame].disp.disp.x;
        PSX_RECT rec;
        rec.x = (sint16)(screenx + oldx - 2);
        sint32 screeny = (uint16)gEnviro[frame].disp.disp.y;
        sint32 oldy = (uint16)gFlare_LensFlare.oldpos[i].vy;
        rec.w = 5;
        rec.h = 5;
        rec.y = (sint16)(screeny + oldy - 2);
        StoreImage(&rec, reinterpret_cast<uint32 *>(gFlare_LensFlare.screenData[i]));
        gFlare_LensFlare.isDrawn[i] = 0;
    }
}

void StampImage(sint32 xo, sint32 depth)
{
    FUNCTION_MARKER(0x800B38A8, "NFS4.EXE");
    sint32 frame;
    if ((Render::gBlurEffectMode & 2) != 0)
        frame = gFlip;
    else
        frame = (sint32)(1U - (uint32)gFlip);
    if ((Render::gBlurEffectMode & 4) == 0)
        xo = 0;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    for (sint32 i = 0; i < 5; ++i)
    {
        POLY_FT4 *ft4_p = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim + depth;
        setaddr(ft4_p, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(ft4_p) + 40;
        setaddr(ot, ft4_p);
        setlen(ft4_p, 9);
        ft4_p->rgbc.code = 0x2E;
        sint32 right = (sint32)((uint32)xo + 64U);
        ft4_p->p[1].xy.x = (sint16)right;
        ft4_p->p[3].xy.x = (sint16)right;
        ft4_p->p[0].uv.u = 0;
        ft4_p->p[0].uv.v = 0;
        ft4_p->p[1].uv.u = 64;
        ft4_p->p[1].uv.v = 0;
        ft4_p->p[2].uv.u = 0;
        ft4_p->p[2].uv.v = 240;
        ft4_p->p[3].uv.u = 64;
        ft4_p->p[3].uv.v = 240;
        ft4_p->p[0].xy.x = (sint16)xo;
        ft4_p->p[0].xy.y = 0;
        ft4_p->p[1].xy.y = 0;
        ft4_p->p[2].xy.x = (sint16)xo;
        ft4_p->p[2].xy.y = 240;
        ft4_p->p[3].xy.y = 240;
        ft4_p->rgbc.r = 128;
        ft4_p->rgbc.g = 128;
        ft4_p->rgbc.b = 128;
        sint32 pagex = frame == 0 ? (320 + i * 64) & 0x3FF : (i * 64) & 0x3FF;
        ft4_p->p[1].attr = (uint16)((pagex >> 6) | 0x110);
        xo = right;
    }
}

void Render::InitTrackRender(void)
{
    FUNCTION_MARKER(0x800B36F4, "NFS4.EXE");
    TrackSpec::Load(GameSetup::gData.Weather, GameSetup::gData.Time);
    CreateViews();
    Draw::InitRenderEngine(320, 256, 0, 256, 320, 240);
    DrawC::BuildRenderingData();
    Texture::InitTrackTexture();
    GenericPMX::LoadTexture();
    Souffle::InitTrackSouffle();
    Skidmark::InitSkidmark();
    TrgSfx::InitTrgSfx();
    DashHUD::InitHUD();
    Loading::UpdateLoadingScreen(3);
    Fog::InitFogTriggers();
    Draw::SetDrawSyncCallback(FlareThing);
    InitBlurMode();
}

void Render::RestartTrackRender(void)
{
    FUNCTION_MARKER(0x800B37AC, "NFS4.EXE");
    Draw::RestartRenderEngine();
    DepthCue_Init();
    Night::RestartNightDriving();
    Skidmark::RestartSkidmark();
    Souffle::RestartTrackSouffle();
    TrgSfx::RestartTrgSfx();
}

void Render::InitTrackRenderPostSim(void)
{
    FUNCTION_MARKER(0x800B37F4, "NFS4.EXE");
    gPauseRender = 0;
    Texture::DeInitPaletteSharing();
    CV::InitColorVertices();
}

void Render::KillTrackRender(void)
{
    FUNCTION_MARKER(0x800B3820, "NFS4.EXE");
    Draw::DeInitRenderEngine();
    DrawC::KillRenderingData();
    Night::KillNightDriving();
    Texture::KillTrackTexture();
    Souffle::KillTrackSouffle();
    Skidmark::KillSkidmark();
    TrgSfx::KillTrgSfx();
    Hud::Kill();
    Draw::SetDrawSyncCallback(0);
    Fog::ReleasePositions();
}

void Render::InitLibRender(void)
{
    FUNCTION_MARKER(0x800B3888, "NFS4.EXE");
    Draw::InitLibRender();
}

void Render::InitBlurMode(void)
{
    FUNCTION_MARKER(0x800B3A04, "NFS4.EXE");
    switch (GameSetup::gData.sgge)
    {
    case 0x10:
        gBlurEffectMode = 0xB;
        break;
    case 0x20:
        gBlurEffectMode = 0x1B;
        break;
    case 0x40:
        gBlurEffectMode = 0x1F;
        break;
    }
}

void Render::InsertDepthOfField(void)
{
    FUNCTION_MARKER(0x800B3A6C, "NFS4.EXE");
    if ((gBlurEffectMode & 1) == 0)
        return;
    if ((gBlurEffectMode & 8) != 0)
        StampImage(1, gBlurEffectDepth1);
    if ((gBlurEffectMode & 0x10) != 0)
        StampImage(2, gBlurEffectDepth2);
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_F4 *prim = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim + gBlurEffectDepth1;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim) + 24;
    setaddr(ot, prim);
    setlen(prim, 5);
    prim->rgbc.code = 0x2A;
    prim->p[0].x = 0;
    prim->p[0].y = 0;
    prim->p[1].x = 320;
    prim->p[1].y = 0;
    prim->p[2].x = 0;
    prim->rgbc.r = 0;
    prim->rgbc.g = 0;
    prim->rgbc.b = 0;
    DR_STP *stp_prim = reinterpret_cast<DR_STP *>(cprim->PrimPtr);
    prim->p[2].y = 240;
    prim->p[3].x = 320;
    prim->p[3].y = 240;
    ot = cprim->LastPrim + gBlurEffectDepth1;
    setaddr(stp_prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(stp_prim) + 12;
    setaddr(ot, stp_prim);
    SetDrawStp(stp_prim, 1);
    DR_MODE *dr_mode = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
    ot = cprim->LastPrim + gBlurEffectDepth1;
    setaddr(dr_mode, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(dr_mode) + 12;
    setaddr(ot, dr_mode);
    uint16 tpage = GetTPage(2, 1, 0, 256);
    SetDrawMode(dr_mode, 0, 0, tpage, 0);
}
