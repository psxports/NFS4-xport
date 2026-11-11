#ifndef DRAW_H
#define DRAW_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/renderview.h"

struct shapetbl;

struct dflip
{
    DISPENV disp;
    char *server;
};

extern sint32 gFlip;

class Draw
{
public:
    struct tCtrlSkidmark;

    struct FlareCache;

    struct HorizonCache;

    struct SkyCache;

    struct SVertex;

    struct SubdivStruct;

    struct tGiveShelbyMoreCache;

    struct tVertex;

    struct CarVertex;

    struct CarCache;

    struct DCache;

    struct tPixMap
    {
        uint8 u0;
        uint8 v0;
        uint16 clut;
        uint8 u1;
        uint8 v1;
        uint16 tpage;
        uint8 u2;
        uint8 v2;
        uint16 pad2;
        uint8 u3;
        uint8 v3;
        uint16 flag;
    };

    struct PrimStruct
    {
        uint32 *LastPrim;
        char *PrimPtr;
        char *MPrimPtr;
    };

    struct tCacheHeader
    {
        PrimStruct cprim;
        sint32 mirror;
        sint16 clipW;
        sint16 clipH;
    };

    static sint32 gPlayer1View;
    static sint32 gPlayer2View;
    static sint32 gRearView;

    struct tView
    {
        sint32 otsize;
        sint32 membudget;
        DRAWENV drawenv[2];
        uint32 *ot[2];
    };

    static sint32 gNumView;
    static sint32 gViewOtSize;
    static sint32 gMidGroundOtz;
    static sint32 gDoVSync;
    static char *gMaxPrim;
    static tView gView[10];
    static void (*gSyncCallback)(void);

    static sint32 SetView(sint32 x0, sint32 y0, sint32 x1, sint32 y1, sint32 w, sint32 h, sint32 dtd, sint32 isbg, sint32 otsize);
    static void InitViews(void);
    static void InitViewOT(void);
    static void InitViewOTInGame(void);
    static void DeInitViews(void);
    static void DeInitViewsInGame(void);
    static DRAWENV * GetDRAWENV(sint32 viewid, sint32 page);
    static void SetViewMemBudget(sint32 viewid, sint32 totalmem);
    static void SetViewColor(sint32 viewid, sint32 r, sint32 g, sint32 b);
    static void StartRenderingView(sint32 viewid);
    static void StopRenderingView(sint32 viewid);
    static void CheckFirstFrameRender(void);
    static void StartFrameRender(void);
    static void SetDrawSyncCallback(void (*p)(void));
    static void StopFrameRender(void);
    static void DrawDirectScreen(shapetbl *tile, sint32 x, sint32 y);
    static void DirectSetEnvironment(sint32 x, sint32 y, sint32 w, sint32 h, sint32 edraw, sint32 edisplay, sint32 erase, sint32 r, sint32 g, sint32 b);
    static void SetEnvironment(sint32 w, sint32 h, sint32 edraw, sint32 edisplay, sint32 erase, sint32 r, sint32 g, sint32 b);
    static void InitRenderEngine(sint32 x0, sint32 y0, sint32 x1, sint32 y1, sint32 w, sint32 h);
    static void RestartRenderEngine(void);
    static void DeInitRenderEngine(void);
    static void InitLibRender(void);
    static sint32 CircleClip(coorddef *pt1, coorddef *pt2, sint32 r);
    static void kCtrlSkidmark(tCtrlSkidmark *fskid);
    static void MenuRenderingView(Car::tObj *carObj, DRender::tView *Vi, sint32 posX, sint32 posY, sint32 player, uint32 rotation, sint32 camRot, float camY, float camZ, sint32 light, sint32 reflection);
};

extern dflip gEnviro[2];

void ClearPrimitivesBuffer(void);

#endif
