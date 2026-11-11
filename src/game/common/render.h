#ifndef RENDER_H
#define RENDER_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/renderview.h"

class Render
{
public:
    static sint32 gDebugView;
    static sint32 gPauseMenuView;
    static sint32 gBlurEffectDepth1;
    static sint32 gBlurEffectDepth2;
    static sint32 gBlurEffectMode;

    static void CreatePlayerViews(void);
    static void CreatePauseMenuViews(void);
    static void CreateViews(void);
    static void StartRenderingWorldView(sint32 viewid);
    static void StopRenderingWorldView(sint32 viewid);
    static void RenderWorld(DRender::tView *Vi);
    static void RenderPlayerView(sint32 ViewID, sint32 Player);
    static void RenderDebugView(void);
    static void StartFrameRender(void);
    static void StopFrameRender(void);
    static void RenderPauseMenuView(void);
    static void InitTrackRender(void);
    static void RestartTrackRender(void);
    static void InitTrackRenderPostSim(void);
    static void KillTrackRender(void);
    static void InitLibRender(void);
    static void InitBlurMode(void);
    static void InsertDepthOfField(void);
};

void Render_Render(sint32 pause);

extern DRender::tView gCView;
extern PSX_RECT gPauseMenuRect;

#endif
