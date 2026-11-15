#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "psx_gpu.h"
#include "hd.h"
#include "hd_ui.h"
#include "game/common/bworld.h"
#include "game/common/camera.h"
#include "game/common/dashhud.h"
#include "game/common/frame.h"
#include "game/common/genericpmx.h"
#include "game/common/gmesetup.h"
#include "game/common/mpause.h"
#include "game/common/object.h"
#include "game/common/r3dcar.h"
#include "game/common/render.h"
#include "game/common/sim.h"
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

struct OriginalFlareObservation
{
    PSX_RECT source;
    DVECTOR position[2];
    bool active[2];
    bool pending;
};

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
static bool preparedViewsValid;
static DRender::tView preparedViews[2];
static float preparedFogStart[2];
static HdWorldViewSettings preparedWorldSettings[2];
static OriginalFlareObservation originalObservation;
static DRender::tView originalViews[2];
static uint32 originalWorldViews;
static sint32 originalWorldFrame;
static bool restoringWorld;
static bool originalPacketsRegistered;
static uint32 originalWorldPackets[0x22500 / 4];
uint32 og_world_builds;
uint32 og_feedback_resets;
uint32 hd_world_skipped_views;

static void capture_original_observation(sint32 frame)
{
    originalObservation.source = Draw::gView[Draw::gPlayer1View].drawenv[frame].clip;
    originalObservation.source.w = gEnviro[frame].disp.disp.w;
    originalObservation.source.h = gEnviro[frame].disp.disp.h;
    for (sint32 player = 0; player < 2; ++player)
    {
        originalObservation.active[player] = gFlare_LensFlare.isDrawn[player] != 0;
        originalObservation.position[player] = gFlare_LensFlare.pos[player];
        gFlare_LensFlare.isDrawn[player] = 0;
    }
    originalObservation.pending = true;
}

uint32 Render::DeferredWorldViews(void)
{
    return originalWorldViews;
}

bool Render::RestoreWorld(void)
{
    if (originalWorldViews == 0)
        return true;
    GpuRasterState savedRaster;
    gpu_export_raster(&savedRaster);
    Draw::PrimStruct savedPrimitives = DrawC::carCache.head.cprim;
    DRender::tView savedView = gCView;
    char *savedLimit = Draw::gMaxPrim;
    sint32 savedFrame = gFlip;
    gFlip = originalWorldFrame;
    DrawC::carCache.head.cprim.PrimPtr = reinterpret_cast<char *>(originalWorldPackets);
    Draw::gMaxPrim = reinterpret_cast<char *>(originalWorldPackets) + sizeof(originalWorldPackets);
    restoringWorld = true;
    for (sint32 player = 0; player < 2; ++player)
    {
        const DRender::tView *view = originalViews + player;
        if (view->player != player || (originalWorldViews & (1 << view->id)) == 0)
            continue;
        ClearOTagR(Draw::gView[view->id].ot[gFlip], Draw::gView[view->id].otsize);
        gCView = *view;
        StartRenderingWorldView(view->id);
        RenderWorld(&gCView);
        Draw::StopRenderingView(view->id);
        DrawOTag(Draw::gView[view->id].ot[gFlip] + Draw::gView[view->id].otsize - 1);
    }
    capture_original_observation(gFlip);
    restoringWorld = false;
    originalWorldViews = 0;
    gFlip = savedFrame;
    Draw::gMaxPrim = savedLimit;
    DrawC::carCache.head.cprim = savedPrimitives;
    gCView = savedView;
    ++og_world_builds;
    if ((gBlurEffectMode & 1) != 0)
        ++og_feedback_resets;
    return gpu_restore_raster(&savedRaster) != 0;
}

bool Render::RestoreOriginalObservation(void)
{
    if (!originalObservation.pending)
        return true;
    memset(gFlare_LensFlare.screenData, 0, sizeof(gFlare_LensFlare.screenData));
    for (sint32 player = 0; player < 2; ++player)
    {
        if (!originalObservation.active[player])
            continue;
        sint32 x = originalObservation.position[player].vx - 2;
        sint32 y = originalObservation.position[player].vy - 2;
        sint32 left = x < 0 ? 0 : x;
        sint32 top = y < 0 ? 0 : y;
        sint32 right = x + 5 > originalObservation.source.w ? originalObservation.source.w : x + 5;
        sint32 bottom = y + 5 > originalObservation.source.h ? originalObservation.source.h : y + 5;
        if (left >= right || top >= bottom)
            continue;
        PSX_RECT region = {(sint16)(originalObservation.source.x + left), (sint16)(originalObservation.source.y + top), (sint16)(right - left), (sint16)(bottom - top)};
        uint16 pixels[26];
        if (StoreImage(&region, reinterpret_cast<uint32 *>(pixels)) != 0)
            return false;
        for (sint32 row = top; row < bottom; ++row)
            for (sint32 column = left; column < right; ++column)
                gFlare_LensFlare.screenData[player][row - y][column - x] = pixels[(row - top) * region.w + column - left];
    }
    originalObservation.pending = false;
    return true;
}

void Render::ResetState(void)
{
    FramePresentation::RestoreOriginal();
    originalWorldViews = 0;
    memset(&originalObservation, 0, sizeof(originalObservation));
    preparedViewsValid = false;
    BWorld::ResetRenderState();
    Hrz::ResetRenderState();
    hd_clear_history();
}

void Render::UpdateState(bool advanceEffects)
{
    if (advanceEffects)
    {
        BWorld::StartLoop();
        Track::AnimateTextures();
        Souffle::DoSouffle();
    }
    sint32 count = GameSetup::gData.commMode == 1 ? 2 : 1;
    for (sint32 player = 0; player < count; ++player)
    {
        preparedViews[player].id = player == 0 ? Draw::gPlayer1View : Draw::gPlayer2View;
        preparedViews[player].player = player;
        Camera::GetViewInfo(player, &preparedViews[player].cview, preparedViews[player].id);
        if (advanceEffects)
        {
            Night::UpdateState(&preparedViews[player]);
        }
        DRender::tView savedView = gCView;
        gCView = preparedViews[player];
        Fog::Update(player);
        preparedFogStart[player] = (float)TrackSpec::gSpec.fogspec.start;
        gCView = savedView;
        BWorld::PrepareView(&preparedViews[player], advanceEffects);
        hd_capture_skids(player, &preparedViews[player]);
        hd_capture_night(player, &preparedViews[player]);
        preparedWorldSettings[player].chunk = gCurrContext->currentChunk;
        preparedWorldSettings[player].track = GameSetup::gData.track;
        preparedWorldSettings[player].chunkFarSquared = gCurrContext->chunkFarZClipSq * 1024.0f;
        preparedWorldSettings[player].polygonFarSquared = gCurrContext->polyFarZClipSq * 1024.0f;
        preparedWorldSettings[player].lowDetailSquared = gCurrContext->lowDetailDistSq * 1024.0f;
        preparedWorldSettings[player].lineFarSquared = gCurrContext->lineFarZClipSq * 1024.0f;
        preparedWorldSettings[player].skipRoadLines = GameSetup::gData.track == 0 && GameSetup::gData.Weather == 1;
        if (advanceEffects)
            BWorld::PrepareObjectState(&preparedViews[player]);
        if (advanceEffects)
            Weather::UpdateState(&preparedViews[player]);
    }
    Draw::DCache objectCache = {0};
    Object::PrepareRemaining(&preparedViews[0], &objectCache);
    R3DCar::UpdateState(preparedViews, count, advanceEffects);
    Hrz::UpdateState(advanceEffects);
    preparedViewsValid = true;
    float fogScale = 16.0f / (float)((uint32)1 << (TrackSpec::gSpec.fogspec.dist2base & 31));
    hd_capture(preparedViews, count, preparedFogStart, fogScale, TrackSpec::gSpec.fogstate != 0, preparedWorldSettings);
    for (sint32 player = 0; player < count; ++player)
        hd_capture_track_halos(player);
    hd_capture_feedback(gBlurEffectMode, gBlurEffectDepth1, gBlurEffectDepth2, advanceEffects);
    hd_capture_objects();
    hd_capture_sky();
    hd_capture_road();
    hd_capture_cars();
    for (sint32 player = 0; player < count; ++player)
    {
        const Car::tObj *car = Cars::gHumanRaceCarList[player];
        hd_ui_tach_sample(player, (float)car->flywheelRpm, car, (uint32)simGlobal.gameTicks);
    }
    hd_capture_weather();
    hd_capture_souffle();
}

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
    if (preparedViewsValid)
    {
        gCView.cview = preparedViews[gCView.player].cview;
        TrsProj::SetViewTrsProjEnviro(&gCView);
    }
    else
        Camera::SetCamera(&gCView);
    Track::AnimateTrackLighting();
    Draw::StartRenderingView(viewid);
    sint32 mirror = GameSetup::gData.mirrorTrack;
    sint32 player = gCView.player;
    DrawC::carCache.head.mirror = mirror;
    if (preparedViewsValid)
        TrackSpec::gSpec.fogspec.start = (sint32)preparedFogStart[player];
    else
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
    bool feedbackSupported = (gBlurEffectMode & 1) == 0 || ((gBlurEffectMode == 0xB || gBlurEffectMode == 0x1B || gBlurEffectMode == 0x1F) && gBlurEffectDepth1 == 0 && gBlurEffectDepth2 == 0);
    if (!restoringWorld && preparedViewsValid && FramePresentation::WantsHd() && feedbackSupported && gTotalMem <= (sint32)sizeof(originalWorldPackets))
    {
        if (!originalPacketsRegistered)
            originalPacketsRegistered = gpu_register_packet_range(originalWorldPackets, sizeof(originalWorldPackets)) != 0;
        if (originalPacketsRegistered)
        {
            originalViews[Vi->player] = *Vi;
            originalWorldViews |= 1 << Vi->id;
            originalWorldFrame = gFlip;
            ++hd_world_skipped_views;
            return;
        }
    }
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
    hd_ui_world_view(Player);
    gCView.id = ViewID;
    gCView.player = Player;
    StartRenderingWorldView(ViewID);
    Weather::PrepareRender(&gCView);
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
    originalWorldViews = 0;
    Draw::CheckFirstFrameRender();
    if (FramePresentation::WantsHd())
        memset(gFlare_LensFlare.screenData, 0, sizeof(gFlare_LensFlare.screenData));
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
    hd_ui_begin_frame(0);
    hd_ui_pause_frame();
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
        if (!FramePresentation::RestoreOriginal())
            return;
        gPauseMenuRect.w = 320;
        gPauseRender = 1;
        gPauseMenuRect.x = 0;
        gPauseMenuRect.y = 0;
        gPauseMenuRect.h = 240;
        DrawSync(0);
        hd_capture_pause();
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
    if (update != 0 || MPause::NeedsRedraw())
    {
        Draw::StartFrameRender();
        Render::RenderPauseMenuView();
        RPause::StopPauseMenu();
    }
}

void FlareThing(void)
{
    FUNCTION_MARKER(0x800B35E8, "NFS4.EXE");
    if (FramePresentation::WantsHd())
    {
        capture_original_observation(gFlip);
        return;
    }
    originalObservation.pending = false;
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
        frame = (1 - gFlip);
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
        sint32 right = (xo + 64);
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
    ResetState();
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
    ResetState();
    hd_release_level();
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
    if (!restoringWorld && (gBlurEffectMode & 8) != 0)
        StampImage(1, gBlurEffectDepth1);
    if (!restoringWorld && (gBlurEffectMode & 0x10) != 0)
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
