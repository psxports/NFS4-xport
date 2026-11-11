#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "psx_gpu.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/pad.h"
#include "game/common/audiocmn.h"
#include "game/common/audiomus.h"
#include "game/common/bo.h"
#include "game/common/cameraobj.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/copspeak.h"
#include "game/common/dashhud.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/mathnfs.h"
#include "game/common/render.h"
#include "game/common/renderview.h"
#include "game/common/replay.h"
#include "game/common/sim.h"
#include "game/common/stats.h"
#include "game/common/textsys.h"
#include "game/psx/draw.h"
#include "game/psx/flare.h"
#include "game/psx/font.h"
#include "game/psx/hud.h"
#include "game/psx/rendercache.h"
#include "game/psx/textpix.h"
#include "lib/psx/fixdmult.h"

class BTC
{
public:
    static sint32 Countdown;
    static sint32 BonusTime;
    static sint32 BonusTimeTick;
    static sint32 playedsoundalready;
    static sint32 UserHasControl;
    static char CurrentPerpName[10];
};

struct tSmallCoordXY
{
    sint16 x;
    sint16 y;
};

static char currentSpriteTransparent;
static sint32 currentSpriteColor;
static tSmallCoordXY *g1Player;
static SPRT *gSprite0;
static SPRT *gSprite1;
static sint32 PerpOverlayOn[2];
static sint32 PerpOverlayMessage[2];
static uint32 day_needle[30] = { 0x000A09BAUL, 0x00F4F4F4UL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x00039AE4UL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x00080808UL, 0x000A09BAUL, 0x00F4F4F4UL, 0x0000A8B0UL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x0000A8B0UL, 0x000A09BAUL, 0x00000000UL, 0x00000000UL, 0x00000000UL };
static uint32 night_needle[30] = { 0x000A09BAUL, 0x001180AFUL, 0x003F9AE4UL, 0x000A09BAUL, 0x000A09BAUL, 0x00039AE4UL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x00080808UL, 0x003EB8F5UL, 0x00286828UL, 0x0000A8B0UL, 0x000A09BAUL, 0x000A09BAUL, 0x003EB8F5UL, 0x003F9AE4UL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x000A09BAUL, 0x003EB8F5UL, 0x0000A8B0UL, 0x000A09BAUL, 0x00000000UL, 0x00000000UL, 0x00000000UL };
static POLY_F4 gHudF4[14];
static POLY_FT4 gHudFT4[10];
static DR_MODE gTPage0[2][4];
static DR_MODE gTPage1[2][4];
static POLY_G4 gHudG4[8];
static sint32 HudSplitTimeDiff1[2];
static sint32 HudSplitTimeDiff2[2];
sint32 FinalBTC_Countdown;
sint16 HudBustedOverlayPlayer;
sint32 mapMarkerMCos;
sint32 mapMarkerMSin;
sint32 HudMapOffsetY;
sint32 gMapRotate;
sint32 gMapScaleX;
sint32 gMapScaleY;
sint32 gMapOffX;
sint32 gMapOffY;
sint32 HudBustedOverlay;
float fMapScaleX[11] = { 39.5f, 50.0f, 44.0f, 43.5f, 39.5f, 52.5999985f, 43.4000015f, 33.5f, 35.5999985f, 29.7999992f, 38.0f };
float fMapScaleY[11] = { 40.7999992f, 50.0f, 46.0999985f, 45.2999992f, 36.7000008f, 51.2000008f, 42.4000015f, 32.5f, 37.2999992f, 27.3999996f, 37.9000015f };
sint16 fMapOffX[11] = { 7, 0, 16, 3, -88, 19, 2, -5, -3, 45, 4 };
sint16 fMapOffY[11] = { 7, 0, -3, 2, -12, -7, 7, 4, 0, -32, -2 };
sint16 fMapRotate[11] = { 4032, 0, 454, 2476, 3694, 3689, 3758, 3822, 1370, 3808, 32 };
char HudminChar[6] = { 58, 58, 39, 39, 39, 46 };
char HudsecChar[6] = { 46, 58, 34, 34, 34, 44 };
sint32 Hud::gCdActive;
sint32 Hud::ActivateCDPlayer;
sint32 BTC::Countdown;
sint32 Hud::gStatsView;
sint32 Hud::gCdLastTick;
sint32 Hud::gCdScrollTitle;
sint32 Hud::gDebugInfo = 1;
sint32 BTC::BonusTime;
sint32 BTC::BonusTimeTick;
sint32 BTC::playedsoundalready;
sint32 BTC::UserHasControl;
sint32 Hud::gShowedCDPlayer;
sint32 Hud::BeTheCop;
sint32 Hud::kTurnSongOffNext;
uint32 Hud::Character[5] = { 4347332U, 4337928U, 15776284U, 7617052U, 0U };
sint32 Hud::gHudView[2];
sint32 Hud::gMapView[2];
sint32 Hud::gTacView[2];
char Hud::gWingmanInterface[2];
char Hud::gWingmanFlashIcon[2];
sint32 Hud::gWingmanFlashTicks[2];
CVECTOR Hud::gMarkerColor[12];
CVECTOR Hud::gCopMarkerColor[12];

tSmallCoordXY Hud::gElementPositions[2][19] =
{
    {
        {22, 54},
        {159, 19},
        {8, 19},
        {18, 35},
        {8, 204},
        {221, 24},
        {23, -2},
        {21, -2},
        {23, -3},
        {2, -4},
        {7, 50},
        {4, 18},
        {-41, 0},
        {90, 212},
        {253, 162},
        {8, 196},
        {218, 20},
        {160, 97},
        {160, 97},
    },
    {
        {22, 56},
        {252, 17},
        {8, 17},
        {26, 30},
        {8, 101},
        {128, 18},
        {23, -2},
        {21, -2},
        {23, -3},
        {2, -1},
        {7, 52},
        {4, 18},
        {-42, 0},
        {90, 105},
        {259, 60},
        {8, 72},
        {215, 22},
        {160, 99},
        {160, 42},
    },
};

char BTC::CurrentPerpName[10];

void Hud::CreateHudViews(void)
{
    FUNCTION_MARKER(0x800D1A80, "NFS4.EXE");
    InitTables();
    sint32 numCars = GameSetup::gData.numCars;
    BeTheCop = 0;
    for (sint32 i = 0; i < numCars; ++i)
        if ((GameSetup::gData.carInfo[i].carClass & 0x40) != 0)
            BeTheCop = 1;
    if (GameSetup::gData.numPlayerRaceCars >= 2)
        HudMapOffsetY = -6;
    else if ((uint32)GameSetup::gData.numPlayerRaceCars +
        (uint32)GameSetup::gData.numOpponentRaceCars == 1U || BeTheCop != 0)
        HudMapOffsetY = 8;
    else
        HudMapOffsetY = 0;
    if (GameSetup::gData.commMode == 1)
    {
        sint32 y = (sint32)((uint32)HudMapOffsetY + 318U);
        gMapView[0] = Draw::SetView(261, y, 581, y, 45, 48, 0, 0, 1);
        y = (sint32)((uint32)HudMapOffsetY + 425U);
        gMapView[1] = Draw::SetView(261, y, 581, y, 45, 48, 0, 0, 1);
        gHudView[0] = Draw::SetView(0, 256, 320, 256, 320, 120, 0, 0, 1);
        gHudView[1] = Draw::SetView(0, 376, 320, 376, 320, 120, 0, 0, 1);
        gTacView[0] = Draw::SetView(277, 275, 597, 275, 28, 28, 0, 0, 1);
        gTacView[1] = Draw::SetView(277, 380, 597, 380, 28, 28, 0, 0, 1);
    }
    else
    {
        sint32 y = (sint32)((uint32)HudMapOffsetY + 420U);
        gMapView[0] = Draw::SetView(255, y, 575, y, 45, 48, 0, 0, 1);
        gHudView[0] = Draw::SetView(0, 256, 320, 256, 320, 240, 0, 0, 1);
        gTacView[0] = Draw::SetView(184, 277, 504, 277, 28, 28, 0, 0, 1);
    }
    gStatsView = Draw::SetView(0, 256, 320, 256, 320, 240, 0, 0, 1);
}

void Hud::GoTpage(sint32 page)
{
    FUNCTION_MARKER(0x800D1D6C, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    DR_MODE *prim = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    uint32 tpage = ((((uint32)page << 6) + 128U) & 1023U) >> 6;
    SetDrawMode(prim, 0, 0, tpage, 0);
}

void Hud::DebugInfo(void)
{
    FUNCTION_MARKER(0x800D1DFC, "NFS4.EXE");
    char string[20];
    sprintf(string, "SLC %d", Cars::gList[0]->N.simRoadInfo.slice);
    Font::TextXY(string, 230, 45);
    sint32 width = textpixels(string);
    FBuildF4(1, 230, 48, (sint32)((uint32)width + 5U), 10, 0x50505, 0, 0);
    sint32 b = AudioMus::Buffered();
    sint32 t = AudioMus::Threshold();
    sprintf(string, "AUD %d.%d/%d.%d", b / 1000, (b % 1000) / 100,
        t / 1000, (t % 1000) / 100);
    Font::TextXY(string, 230, 35);
    width = textpixels(string);
    FBuildF4(1, 230, 38, (sint32)((uint32)width + 5U), 10, 0x50505, 0, 0);
#if 0
    CopSpeak::ShowQueue();
#endif
}

static void uppercase(char *string)
{
    FUNCTION_MARKER(0x800D2A44, "NFS4.EXE");
    sint32 len = (sint32)(uint32)strlen(string);
    for (sint32 n = 0; n < len; ++n)
    {
        uint32 ch = (uint8)string[n];
        if (ch - 0x61U < 26U)
            string[n] = (char)(ch + 0xE0U);
    }
}

void Hud::DebugCrap(void)
{
    FUNCTION_MARKER(0x800D1F90, "NFS4.EXE");
}

void Hud::BuildSprite(SPRT *sprt, sint32 shapeIdx, sint32 x, sint32 y, uint32 color, sint32 trans)
{
    FUNCTION_MARKER(0x800D1F98, "NFS4.EXE");
    HudPmx::tShape *shape = &HudPmx::gShapes[shapeIdx];
    SetSemiTrans(sprt, trans);
    setlen(sprt, 4);
    uint32 rgbc = (uint32)color | 0x66000000U;
    uint32 xy = ((uint32)y << 16) | (uint32)x;
    memcpy(&sprt->rgbc, &rgbc, sizeof(rgbc));
    memcpy(&sprt->p[0].xy, &xy, sizeof(xy));
    uint32 uv;
    memcpy(&uv, &shape->pixmap.u0, sizeof(uv));
    memcpy(&sprt->p[0].uv, &uv, sizeof(uv));
    uint32 wh = ((uint32)(sint32)shape->height << 16) | (uint32)(sint32)shape->width;
    memcpy(&sprt->w, &wh, sizeof(wh));
}

void Hud::BuildSprite2(SPRT *sprt, sint32 shapeIdx, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x800D2044, "NFS4.EXE");
    uint32 color = currentSpriteColor;
    sint32 trans = (uint8)currentSpriteTransparent;
    BuildSprite(sprt, shapeIdx, x, y, color, trans);
}

void Hud::FBuildSprite(sint32 shapeIdx, sint32 x, sint32 y, uint32 color, sint32 trans)
{
    FUNCTION_MARKER(0x800D2070, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    SPRT *prim = reinterpret_cast<SPRT *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    BuildSprite(prim, shapeIdx, x, y, color, trans);
}

void Hud::BuildSpriteFromFont(SPRT *sprt, char ch, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x800D2104, "NFS4.EXE");
    sint32 u, v, w, h, yo;
    Font::GetUVWH(ch, &u, &v, &w, &h, &yo);
    setlen(sprt, 4);
    uint32 rgbc = (uint32)currentSpriteColor | 0x66000000U;
    memcpy(&sprt->rgbc, &rgbc, sizeof(rgbc));
    uint32 xy = (((uint32)y + (uint32)yo) << 16) | (uint32)x;
    memcpy(&sprt->p[0].xy, &xy, sizeof(xy));
    sprt->p[0].uv.u = (uint8)u;
    sprt->p[0].uv.v = (uint8)v;
    uint16 clut = gFontClut;
    sprt->p[0].attr = clut;
    sprt->w = (sint16)(uint16)w;
    sprt->h = (sint16)(uint16)h;
}

void Hud::BuildF3(POLY_F3 *prim, HudPmx::tShape *shape, sint32 x, sint32 y, uint32 color)
{
    FUNCTION_MARKER(0x800D21C4, "NFS4.EXE");
    uint32 rgbc = (uint32)color;
    memcpy(&prim->rgbc, &rgbc, sizeof(rgbc));
    SetPolyF3(prim);
    uint32 xy = ((uint32)y << 16) | (uint32)x;
    memcpy(&prim->p[0], &xy, sizeof(xy));
    sint32 halfHeight = (sint32)shape->height / 2;
    uint32 top = ((uint32)y - (uint32)halfHeight) << 16;
    uint32 right = (uint32)x + (uint32)(sint32)shape->width;
    xy = top | right;
    memcpy(&prim->p[1], &xy, sizeof(xy));
    halfHeight = (sint32)shape->height / 2;
    uint32 bottom = ((uint32)y + (uint32)halfHeight) << 16;
    right = (uint32)x + (uint32)(sint32)shape->width;
    xy = bottom | right;
    memcpy(&prim->p[2], &xy, sizeof(xy));
}

void Hud::BuildGT4(POLY_GT4 *prim, HudPmx::tShape *shape, sint32 x, sint32 y, uint32 color)
{
    FUNCTION_MARKER(0x800D2288, "NFS4.EXE");
    setlen(prim, 12);
    uint32 top = (uint32)y << 16;
    uint32 xy = top | (uint32)x;
    memcpy(&prim->p[0].xy, &xy, sizeof(xy));
    uint32 rgbc = (uint32)color | 0x3E000000U;
    memcpy(&prim->p[0].rgbc, &rgbc, sizeof(rgbc));
    uint32 rgb = (uint32)color;
    memcpy(&prim->p[1].rgbc, &rgb, sizeof(rgb));
    memcpy(&prim->p[2].rgbc, &rgb, sizeof(rgb));
    memcpy(&prim->p[3].rgbc, &rgb, sizeof(rgb));
    uint32 uv;
    memcpy(&uv, &shape->pixmap.u0, sizeof(uv));
    memcpy(&prim->p[0].uv, &uv, sizeof(uv));
    uint32 right = (uint32)x + (uint32)(sint32)shape->width;
    xy = top | right;
    memcpy(&prim->p[1].xy, &xy, sizeof(xy));
    memcpy(&uv, &shape->pixmap.u1, sizeof(uv));
    memcpy(&prim->p[1].uv, &uv, sizeof(uv));
    uint32 bottom = ((uint32)y + (uint32)(sint32)shape->height) << 16;
    xy = bottom | (uint32)x;
    memcpy(&prim->p[2].xy, &xy, sizeof(xy));
    memcpy(&uv, &shape->pixmap.u2, sizeof(uv));
    memcpy(&prim->p[2].uv, &uv, sizeof(uv));
    bottom = ((uint32)y + (uint32)(sint32)shape->height) << 16;
    right = (uint32)x + (uint32)(sint32)shape->width;
    xy = bottom | right;
    memcpy(&prim->p[3].xy, &xy, sizeof(xy));
    memcpy(&uv, &shape->pixmap.u3, sizeof(uv));
    memcpy(&prim->p[3].uv, &uv, sizeof(uv));
}

void Hud::BuildFT4(POLY_FT4 *prim, HudPmx::tShape *shape, sint32 x, sint32 y, uint32 color, sint32 trans)
{
    FUNCTION_MARKER(0x800D2334, "NFS4.EXE");
    uint32 rgbc = (uint32)color;
    memcpy(&prim->rgbc, &rgbc, sizeof(rgbc));
    SetPolyFT4(prim);
    SetSemiTrans(prim, trans);
    uint32 top = (uint32)y << 16;
    uint32 xy = top | (uint32)x;
    memcpy(&prim->p[0].xy, &xy, sizeof(xy));
    uint32 uv;
    memcpy(&uv, &shape->pixmap.u0, sizeof(uv));
    memcpy(&prim->p[0].uv, &uv, sizeof(uv));
    uint32 right = (uint32)x + (uint32)(sint32)shape->width;
    xy = top | right;
    memcpy(&prim->p[1].xy, &xy, sizeof(xy));
    memcpy(&uv, &shape->pixmap.u1, sizeof(uv));
    memcpy(&prim->p[1].uv, &uv, sizeof(uv));
    uint32 bottom = ((uint32)y + (uint32)(sint32)shape->height) << 16;
    xy = bottom | (uint32)x;
    memcpy(&prim->p[2].xy, &xy, sizeof(xy));
    memcpy(&uv, &shape->pixmap.u2, sizeof(uv));
    memcpy(&prim->p[2].uv, &uv, sizeof(uv));
    bottom = ((uint32)y + (uint32)(sint32)shape->height) << 16;
    right = (uint32)x + (uint32)(sint32)shape->width;
    xy = bottom | right;
    memcpy(&prim->p[3].xy, &xy, sizeof(xy));
    memcpy(&uv, &shape->pixmap.u3, sizeof(uv));
    memcpy(&prim->p[3].uv, &uv, sizeof(uv));
}

void Hud::BuildMirrorFT4(POLY_FT4 *prim, HudPmx::tShape *shape, sint32 x, sint32 y, uint32 color, sint32 trans)
{
    FUNCTION_MARKER(0x800D2424, "NFS4.EXE");
    uint32 rgbc = (uint32)color;
    memcpy(&prim->rgbc, &rgbc, sizeof(rgbc));
    SetPolyFT4(prim);
    SetSemiTrans(prim, trans);
    uint32 top = (uint32)y << 16;
    uint32 right = (uint32)x + (uint32)(sint32)shape->width;
    uint32 xy = top | right;
    memcpy(&prim->p[0].xy, &xy, sizeof(xy));
    uint32 uv;
    memcpy(&uv, &shape->pixmap.u0, sizeof(uv));
    xy = top | (uint32)x;
    memcpy(&prim->p[1].xy, &xy, sizeof(xy));
    memcpy(&prim->p[0].uv, &uv, sizeof(uv));
    memcpy(&uv, &shape->pixmap.u1, sizeof(uv));
    memcpy(&prim->p[1].uv, &uv, sizeof(uv));
    prim->p[1].uv.u = (uint8)(prim->p[1].uv.u - 1U);
    uint32 bottom = ((uint32)y + (uint32)(sint32)shape->height) << 16;
    right = (uint32)x + (uint32)(sint32)shape->width;
    xy = bottom | right;
    memcpy(&prim->p[2].xy, &xy, sizeof(xy));
    memcpy(&uv, &shape->pixmap.u2, sizeof(uv));
    memcpy(&prim->p[2].uv, &uv, sizeof(uv));
    bottom = ((uint32)y + (uint32)(sint32)shape->height) << 16;
    xy = bottom | (uint32)x;
    memcpy(&prim->p[3].xy, &xy, sizeof(xy));
    memcpy(&uv, &shape->pixmap.u3, sizeof(uv));
    memcpy(&prim->p[3].uv, &uv, sizeof(uv));
    prim->p[3].uv.u = (uint8)(prim->p[3].uv.u - 1U);
}

void Hud::BuildMapMirrorFT4(POLY_FT4 *prim, HudPmx::tShape *shape, sint32 x, sint32 y, uint32 color, sint32 trans)
{
    FUNCTION_MARKER(0x800D2528, "NFS4.EXE");
    uint32 rgbc = (uint32)color;
    memcpy(&prim->rgbc, &rgbc, sizeof(rgbc));
    SetPolyFT4(prim);
    SetSemiTrans(prim, trans);
    uint32 top = (uint32)y << 16;
    uint32 right = (uint32)x + (uint32)(sint32)shape->width;
    uint32 xy = top | right;
    memcpy(&prim->p[0].xy, &xy, sizeof(xy));
    uint32 uv;
    memcpy(&uv, &shape->pixmap.u0, sizeof(uv));
    xy = top | (uint32)x;
    memcpy(&prim->p[1].xy, &xy, sizeof(xy));
    memcpy(&prim->p[0].uv, &uv, sizeof(uv));
    memcpy(&uv, &shape->pixmap.u1, sizeof(uv));
    prim->p[1].xy.x = (sint16)((uint16)prim->p[1].xy.x - 3U);
    prim->p[0].xy.x = (sint16)((uint16)prim->p[0].xy.x - 3U);
    memcpy(&prim->p[1].uv, &uv, sizeof(uv));
    uint32 bottom = ((uint32)y + (uint32)(sint32)shape->height) << 16;
    right = (uint32)x + (uint32)(sint32)shape->width;
    xy = bottom | right;
    memcpy(&prim->p[2].xy, &xy, sizeof(xy));
    memcpy(&uv, &shape->pixmap.u2, sizeof(uv));
    memcpy(&prim->p[2].uv, &uv, sizeof(uv));
    bottom = ((uint32)y + (uint32)(sint32)shape->height) << 16;
    xy = bottom | (uint32)x;
    memcpy(&prim->p[3].xy, &xy, sizeof(xy));
    memcpy(&uv, &shape->pixmap.u3, sizeof(uv));
    prim->p[2].xy.x = (sint16)((uint16)prim->p[2].xy.x - 3U);
    memcpy(&prim->p[3].uv, &uv, sizeof(uv));
    prim->p[3].xy.x = (sint16)((uint16)prim->p[3].xy.x - 3U);
}

void Hud::BuildF4(POLY_F4 *prim, sint32 trans, sint32 x, sint32 y, sint32 w, sint32 h, uint32 color)
{
    FUNCTION_MARKER(0x800D2634, "NFS4.EXE");
    uint32 rgbc = (uint32)color;
    memcpy(&prim->rgbc, &rgbc, sizeof(rgbc));
    SetPolyF4(prim);
    SetSemiTrans(prim, trans);
    uint32 top = (uint32)y << 16;
    uint32 right = (uint32)x + (uint32)w;
    uint32 bottom = ((uint32)y + (uint32)h) << 16;
    uint32 xy0 = top | (uint32)x;
    uint32 xy1 = top | right;
    uint32 xy2 = bottom | (uint32)x;
    uint32 xy3 = bottom | right;
    memcpy(&prim->p[0], &xy0, sizeof(xy0));
    memcpy(&prim->p[1], &xy1, sizeof(xy1));
    memcpy(&prim->p[2], &xy2, sizeof(xy2));
    memcpy(&prim->p[3], &xy3, sizeof(xy3));
}

void Hud::BuildG4(POLY_G4 *prim, sint32 trans, sint32 x, sint32 y, sint32 w, sint32 h, uint32 col1, uint32 col2, uint32 col3, uint32 col4)
{
    FUNCTION_MARKER(0x800D26D8, "NFS4.EXE");
    uint32 rgbc;
    rgbc = (uint32)col1;
    memcpy(&prim->p[0].rgbc, &rgbc, sizeof(rgbc));
    rgbc = (uint32)col2;
    memcpy(&prim->p[1].rgbc, &rgbc, sizeof(rgbc));
    rgbc = (uint32)col3;
    memcpy(&prim->p[2].rgbc, &rgbc, sizeof(rgbc));
    rgbc = (uint32)col4;
    memcpy(&prim->p[3].rgbc, &rgbc, sizeof(rgbc));
    SetPolyG4(prim);
    SetSemiTrans(prim, trans);
    uint32 top = (uint32)y << 16;
    uint32 right = (uint32)x + (uint32)w;
    uint32 bottom = ((uint32)y + (uint32)h) << 16;
    uint32 xy0 = top | (uint32)x;
    uint32 xy1 = top | right;
    uint32 xy2 = bottom | (uint32)x;
    uint32 xy3 = bottom | right;
    memcpy(&prim->p[0].xy, &xy0, sizeof(xy0));
    memcpy(&prim->p[1].xy, &xy1, sizeof(xy1));
    memcpy(&prim->p[2].xy, &xy2, sizeof(xy2));
    memcpy(&prim->p[3].xy, &xy3, sizeof(xy3));
}

void Hud::BuildF4o(POLY_F4 *prim, sint32 trans, sint32 x, sint32 y, sint32 w, sint32 h, uint32 color, sint8 x0off, sint8 x1off)
{
    FUNCTION_MARKER(0x800D2794, "NFS4.EXE");
    uint32 rgbc = (uint32)color;
    memcpy(&prim->rgbc, &rgbc, sizeof(rgbc));
    SetPolyF4(prim);
    SetSemiTrans(prim, trans);
    uint32 top = (uint32)y << 16;
    uint32 right = (uint32)x + (uint32)w;
    uint32 xy0 = top | ((uint32)x + (uint32)(sint32)x0off);
    uint32 xy1 = top | (right + (uint32)(sint32)x1off);
    uint32 bottom = ((uint32)y + (uint32)h) << 16;
    uint32 xy2 = bottom | (uint32)x;
    uint32 xy3 = bottom | right;
    memcpy(&prim->p[0], &xy0, sizeof(xy0));
    memcpy(&prim->p[1], &xy1, sizeof(xy1));
    memcpy(&prim->p[2], &xy2, sizeof(xy2));
    memcpy(&prim->p[3], &xy3, sizeof(xy3));
}

void Hud::FBuildGT4(HudPmx::tShape *shape, sint32 x, sint32 y, uint32 col1)
{
    FUNCTION_MARKER(0x800D2868, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_GT4 *prim = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    BuildGT4(prim, shape, x, y, col1);
}

static void BuildSpeedDigit(sint32 digit, sint32 x, sint32 y)
{
    POLY_GT4 *prim = reinterpret_cast<POLY_GT4 *>(DrawC::carCache.head.cprim.PrimPtr);
    Hud::FBuildGT4(&HudPmx::gShapes[44 + digit], x, y, 0xC8C8C8);
    const uint32 color2 = 0x505050;
    memcpy(&prim->p[3].rgbc, &color2, sizeof(color2));
    memcpy(&prim->p[2].rgbc, &color2, sizeof(color2));
}

void Hud::FBuildFT4(HudPmx::tShape *shape, sint32 x, sint32 y, uint32 col1)
{
    FUNCTION_MARKER(0x800D28F4, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_FT4 *prim = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    BuildFT4(prim, shape, x, y, col1, 0);
}

void Hud::FBuildF4(sint32 transparent, sint32 x, sint32 y, sint32 w, sint32 h, uint32 col1, char x0off, char x1off)
{
    FUNCTION_MARKER(0x800D2984, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_F4 *prim = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    BuildF4o(prim, transparent, x, y, w, h, col1, (sint8)x0off, (sint8)x1off);
}

void Hud::Kill(void)
{
    FUNCTION_MARKER(0x800D2AA4, "NFS4.EXE");
    HudPmx::ReleaseFont();
    if (gSprite0 != 0)
        purgememadr(gSprite0);
    SPRT *sprite1 = gSprite1;
    gSprite0 = 0;
    if (sprite1 != 0)
        purgememadr(sprite1);
    gSprite1 = 0;
}

void Hud::Init0(void)
{
    FUNCTION_MARKER(0x800D2AF4, "NFS4.EXE");
    gSprite0 = static_cast<SPRT *>(reservememadr("HUD1", 0x80C, 0));
    if (GameSetup::gData.commMode == 1)
        gSprite1 = static_cast<SPRT *>(reservememadr("HUD2", 0x80C, 0));
}

void Hud::InitMapFrame(sint32 i, sint32 mode)
{
    FUNCTION_MARKER(0x800D2B4C, "NFS4.EXE");
    SPRT *gSprt1 = i == 0 ? gSprite0 : gSprite1;
    POLY_F4 *HudF4 = i == 0 ? gHudF4 : gHudF4 + 7;
    POLY_FT4 *HudFT4 = i == 0 ? gHudFT4 : gHudFT4 + 5;
    sint32 splitY = i == 0 ? 0 : -13;
    sint32 h1 = HudPmx::gShapes[14].height;
    sint32 w1 = HudPmx::gShapes[14].width;
    sint32 h3 = h1 - HudPmx::gShapes[20].height;
    sint32 w3 = w1 - HudPmx::gShapes[24].width;
    sint32 lx = g1Player[14].x;
    sint32 h2 = HudPmx::gShapes[24].height;
    sint32 w2 = HudPmx::gShapes[20].width;
    if (mode == 1)
        lx -= 30;
    currentSpriteColor = 0xFF5721;
    currentSpriteTransparent = 0;
    uint32 y = (uint32)g1Player[14].y + (uint32)HudMapOffsetY;
    BuildSprite2(gSprt1 + 63, 14, lx, (sint32)(y + (uint32)splitY));
    y = (uint32)g1Player[14].y + (uint32)HudMapOffsetY;
    BuildSprite2(gSprt1 + 64, 15, g1Player[14].x + w1 + w2, (sint32)(y + (uint32)splitY));
    y = (uint32)g1Player[14].y + (uint32)HudMapOffsetY;
    BuildSprite2(gSprt1 + 65, 16, lx, (sint32)(y + (uint32)h1 + (uint32)h2 + (uint32)splitY));
    y = (uint32)g1Player[14].y + (uint32)HudMapOffsetY;
    BuildSprite2(gSprt1 + 66, 17, g1Player[14].x + w1 + w2,
        (sint32)(y + (uint32)h1 + (uint32)h2 + (uint32)splitY));
    sint32 shapeIdx = (sint32)((uint32)mode + 20U);
    sint32 middleX = lx + w1;
    y = (uint32)g1Player[14].y + (uint32)HudMapOffsetY;
    BuildSprite2(gSprt1 + 67, shapeIdx, middleX, (sint32)(y + (uint32)splitY));
    y = (uint32)g1Player[14].y + (uint32)HudMapOffsetY;
    BuildSprite2(gSprt1 + 68, shapeIdx, middleX,
        (sint32)(y + (uint32)h1 + (uint32)h2 + (uint32)h3 + (uint32)splitY));
    y = (uint32)g1Player[14].y + (uint32)HudMapOffsetY;
    BuildSprite2(gSprt1 + 69, 24, lx, (sint32)(y + (uint32)h1 + (uint32)splitY));
    y = (uint32)g1Player[14].y + (uint32)HudMapOffsetY;
    BuildSprite2(gSprt1 + 70, 24, g1Player[14].x + w1 + w2 + w3,
        (sint32)(y + (uint32)h1 + (uint32)splitY));
    BuildF4(HudF4 + 4, 1, 0, 0, 45, 48, 0);
    if (GameSetup::gData.mirrorTrack == 0)
        BuildFT4(HudFT4, &HudPmx::gShapes[120], 0, 0, 0x808080, 1);
    else
        BuildMapMirrorFT4(HudFT4, &HudPmx::gShapes[120], 0, 0, 0x808080, 1);
    BuildFT4(HudFT4 + 1, &HudPmx::gShapes[112], 0, 0, 0x808080, 1);
    BuildMirrorFT4(HudFT4 + 2, &HudPmx::gShapes[112], HudPmx::gShapes[112].width, 0, 0x808080, 1);
    BuildFT4(HudFT4 + 3, &HudPmx::gShapes[113], 0, 0, 0x808080, 1);
    BuildMirrorFT4(HudFT4 + 4, &HudPmx::gShapes[113], HudPmx::gShapes[113].width, 0, 0x808080, 1);
}

void Hud::BuildTimeSprites(SPRT *sprt, const char *str, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x800D2EF8, "NFS4.EXE");
    char minSep[6] = { 58, 58, 39, 39, 39, 46 };
    char secSep[6] = { 46, 58, 34, 34, 34, 44 };
    sint32 language = GameSetup::gData.userSetting.language;
    uint8 langMin = (uint8)minSep[language];
    uint8 langSec = (uint8)secSep[language];
    for (sint32 c = (uint8)*str; c != 0; c = (uint8)*str)
    {
        if (c == 'M')
            c = langMin;
        if (c == 'S')
            c = langSec;
        charactertbl *ch = Font::Getcharacter(c);
        sint32 w = (sint8)ch->advance + 1;
        BuildSpriteFromFont(sprt, (char)c, x, y);
        ++sprt;
        ++str;
        x = (sint32)((uint32)x + (uint32)w);
    }
}

void Hud::Init(void)
{
    FUNCTION_MARKER(0x800D302C, "NFS4.EXE");
    if (!gpu_register_packet_range(gTPage0, sizeof(gTPage0)) ||
        !gpu_register_packet_range(gTPage1, sizeof(gTPage1)) ||
        !gpu_register_packet_range(gHudG4, sizeof(gHudG4)) ||
        !gpu_register_packet_range(gHudF4, sizeof(gHudF4)) ||
        !gpu_register_packet_range(gHudFT4, sizeof(gHudFT4)))
        abort();
    for (sint32 i = 0; i < 2; ++i)
    {
        for (sint32 j = 0; j < 4; ++j)
        {
            SetDrawMode(&gTPage0[i][j], 0, 0, 2, 0);
            SetDrawMode(&gTPage1[i][j], 0, 0, 3, 0);
        }
    }
    for (sint32 i = 0; i <= DashHUD::gInfo.splitscreen; ++i)
    {
        SPRT *gSprt1 = i == 0 ? gSprite0 : gSprite1;
        POLY_F4 *HudF4 = i == 0 ? gHudF4 : gHudF4 + 7;
        POLY_G4 *HudG4 = i == 0 ? gHudG4 : gHudG4 + 4;
        sint32 splitY = i == 0 ? 0 : -15;
        GameSetup::tCarData *carInfo = &GameSetup::gData.carInfo[i];
        sint32 timelapshift = carInfo->HudTime == 0 ? 16 : 0;
        if (carInfo->HudLapnum == 0)
            timelapshift += 16;
        BuildSprite(gSprt1, 104, g1Player[0].x, g1Player[0].y + splitY - timelapshift, 0xBEBE, 0);
        currentSpriteColor = 0x808080;
        BuildSprite2(gSprt1 + 1, 128, g1Player[1].x, g1Player[1].y + splitY);
        BuildSprite2(gSprt1 + 2, i == 0 ? 129 : 131, 0, 0);
        gSprt1[2].w = 28;
        gSprt1[2].h = 28;
        sint32 w1 = HudPmx::gShapes[107].width;
        sint32 w2 = BeTheCop == 0 ? 70 : 46;
        sint32 x = g1Player[2].x;
        sint32 y = g1Player[2].y + splitY;
        BuildSprite2(gSprt1 + 4, 105, x, y);
        x += w1;
        BuildG4(HudG4, 1, x, y, w2, 10, 0, 0x707070, 0, 0x707070);
        x += w2;
        BuildSprite2(gSprt1 + 5, 125, x, y);
        BuildF4(HudF4, 1, x, y + 7, 7, 3, 0x707070);
        w1 = HudPmx::gShapes[107].width;
        x = g1Player[3].x;
        timelapshift = carInfo->HudTime == 0 ? 16 : 0;
        y = g1Player[3].y + splitY - timelapshift;
        BuildSprite2(gSprt1 + 6, 107, x, y);
        x += w1;
        BuildG4(HudG4 + 1, 1, x, y, 29, 10, 0, 0x707070, 0, 0x707070);
        x += 29;
        BuildF4(HudF4 + 1, 1, x, y + 7, 7, 3, 0x707070);
        BuildSprite2(gSprt1 + 7, 125, x, y);
        w1 = HudPmx::gShapes[107].width;
        x = g1Player[4].x;
        y = g1Player[4].y + splitY;
        BuildSprite2(gSprt1 + 8, 105, x, y);
        x += w1;
        BuildG4(HudG4 + 3, 1, x, y, 60, 10, 0, 0x707070, 0, 0x707070);
        x += 60;
        BuildF4(HudF4 + 2, 1, x, y + 7, 7, 3, 0x707070);
        BuildSprite2(gSprt1 + 9, 125, x, y);
        w1 = HudPmx::gShapes[118].width;
        w2 = 38;
        if (BeTheCop == 0)
            w2 = GameSetup::gData.checkpointHUD[i] == 0 ? 61 : 50;
        if (BeTheCop == 0)
            x = g1Player[5].x;
        else
            x = g1Player[2].x + 14;
        if (BeTheCop == 0)
            y = g1Player[5].y + splitY;
        else
            y = g1Player[2].y + splitY + 14;
        BuildSprite2(gSprt1 + 10, 118, x, y);
        x += w1;
        BuildG4(HudG4 + 2, 1, x, y, w2, 10, 0, 0x707070, 0, 0x707070);
        x += w2;
        BuildF4(HudF4 + 3, 1, x, y + 7, 7, 3, 0x707070);
        BuildSprite(gSprt1 + 11, 125, x, y, 0x808080, 0);
        currentSpriteColor = 0xA0A0A0;
        BuildTimeSprites(gSprt1 + 12, "00M00S00", g1Player[2].x + g1Player[6].x,
            g1Player[2].y + splitY + g1Player[6].y);
        BuildTimeSprites(gSprt1 + 20, "0/0", g1Player[3].x + g1Player[7].x,
            g1Player[3].y + g1Player[7].y + splitY - timelapshift);
        currentSpriteColor = 0xA0A0A0;
        BuildTimeSprites(gSprt1 + 23, "0M00S00", g1Player[4].x + g1Player[8].x,
            g1Player[4].y + g1Player[8].y + splitY + 1);
        if (BeTheCop == 0)
            x = g1Player[5].x + g1Player[9].x;
        else
            x = g1Player[2].x + 16;
        if (BeTheCop == 0)
            y = g1Player[5].y + splitY + 1;
        else
            y = g1Player[2].y + splitY + 12;
        const char *str = GameSetup::gData.checkpointHUD[i] == 0 ? "0M00S00" : "0.000";
        x += HudPmx::gShapes[118].width;
        BuildTimeSprites(gSprt1 + 30, str, x, y);
        HudSplitTimeDiff1[i] = (sint32)gSprt1[31].p[0].xy.y - (sint32)gSprt1[30].p[0].xy.y;
        currentSpriteColor = 0xA0A0A0;
        HudSplitTimeDiff2[i] = (sint32)gSprt1[34].p[0].xy.y - (sint32)gSprt1[30].p[0].xy.y;
        w1 = HudPmx::gShapes[44].width;
        w2 = HudPmx::gShapes[71].width;
        x = g1Player[14].x + g1Player[10].x;
        y = (sint32)((uint32)g1Player[14].y + (uint32)HudMapOffsetY +
            (uint32)g1Player[10].y + (uint32)splitY);
        if (i == 0 && DashHUD::gInfo.splitscreen != 0)
            y = (sint32)((uint32)y - 2U);
        BuildSprite(gSprt1 + 40, 62, g1Player[14].x + 19, (sint32)((uint32)y - 1U), 0x808080, 0);
        BuildSprite2(gSprt1 + 37, 44, x, y);
        x += w1;
        BuildSprite2(gSprt1 + 38, 71, x, y);
        BuildSprite2(gSprt1 + 39, 54, x + w2, y);
        currentSpriteColor = 0x808080;
        x = g1Player[1].x + g1Player[11].x;
        y = g1Player[1].y + g1Player[11].y + splitY;
        BuildSprite2(gSprt1 + 49, 26, x - 1, y);
        BuildSprite2(gSprt1 + 50, 27, x - 2, y - 1);
        for (sint32 j = 0; j < 8; ++j)
            BuildSprite2(gSprt1 + 41 + j, 28 + j, x + 2, y + 6);
        InitMapFrame(i, 0);
    }
    currentSpriteColor = 0x808080;
    currentSpriteTransparent = 1;
    SPRT *spriteReplay = gSprite0;
    sint32 baseX = g1Player[13].x;
    sint32 baseY = g1Player[13].y;
    BuildSprite2(spriteReplay + 55, 111, baseX, baseY);
    BuildSprite2(spriteReplay + 51, 108, baseX + 18, baseY);
    BuildSprite2(spriteReplay + 52, 110, baseX + 37, baseY);
    BuildSprite2(spriteReplay + 53, 63, baseX + 58, baseY);
    BuildSprite2(spriteReplay + 56, 114, baseX + 74, baseY);
    BuildSprite2(spriteReplay + 54, 0, baseX + 109, baseY - 7);
    BuildSprite2(spriteReplay + 57, 3, 0, baseY + 4);
    currentSpriteTransparent = 0;
    InitCdPlayer();
    Reset();
    BTC::BonusTime = 0;
    BTC::Countdown = 0;
    FinalBTC_Countdown = 0;
    BTC::UserHasControl = 0;
    HudBustedOverlay = 0;
    for (sint32 i = 0; i < 2; ++i)
    {
        PerpOverlayOn[i] = 0;
        PerpOverlayMessage[i] = 0;
    }
    kTurnSongOffNext = 0;
}

void Hud::InitTables(void)
{
    FUNCTION_MARKER(0x800D39EC, "NFS4.EXE");
    g1Player = gElementPositions[GameSetup::gData.numPlayerRaceCars < 2 ? 0 : 1];
}

void Hud::BuildETimeString(SPRT *sprt, sint32 time)
{
    FUNCTION_MARKER(0x800D3A18, "NFS4.EXE");
    if (time < 0)
        time = 0;
    sint32 temp1 = time / 64;
    sint32 min = (temp1 / 60) % 60;
    sint32 sec = temp1 % 60;
    sint32 hun = ((time % 64) * 100) / 64;
    memcpy(&sprt[0].p[0].uv, &HudPmx::gHudNumberUV[min / 10], 4);
    memcpy(&sprt[1].p[0].uv, &HudPmx::gHudNumberUV[min % 10], 4);
    memcpy(&sprt[3].p[0].uv, &HudPmx::gHudNumberUV[sec / 10], 4);
    memcpy(&sprt[4].p[0].uv, &HudPmx::gHudNumberUV[sec % 10], 4);
    memcpy(&sprt[6].p[0].uv, &HudPmx::gHudNumberUV[hun / 10], 4);
    memcpy(&sprt[7].p[0].uv, &HudPmx::gHudNumberUV[hun % 10], 4);
}

bool Hud::BuildDistanceString(SPRT *sprt, sint32 player)
{
    FUNCTION_MARKER(0x800D3BA4, "NFS4.EXE");
    sint32 slices = Cars::gHumanRaceCarList[player]->stats.checkpointUpdate;
    sint32 dist = (sint32)((uint32)slices * 6U);
    if (dist < 0)
        dist = (sint32)(0U - (uint32)dist);
    if (GameSetup::gData.checkpointHUD[player] == 2)
        dist = (sint32)((uint32)dist * 1000U) / 1607;
    if (dist >= 10000)
        return false;
    memcpy(&sprt[0].p[0].uv, &HudPmx::gHudNumberUV[dist / 1000], 4);
    memcpy(&sprt[2].p[0].uv, &HudPmx::gHudNumberUV[(dist % 1000) / 100], 4);
    memcpy(&sprt[3].p[0].uv, &HudPmx::gHudNumberUV[(dist % 100) / 10], 4);
    memcpy(&sprt[4].p[0].uv, &HudPmx::gHudNumberUV[dist % 10], 4);
    return true;
}

void Hud::BuildTimeString(SPRT *sprt, sint32 time)
{
    FUNCTION_MARKER(0x800D3D44, "NFS4.EXE");
    if (time < 0)
        time = (sint32)(0U - (uint32)time);
    sint32 temp1 = time / 64;
    sint32 min = temp1 / 60;
    sint32 sec = temp1 % 60;
    sint32 hun = ((time % 64) * 100) / 64;
    memcpy(&sprt[0].p[0].uv, &HudPmx::gHudNumberUV[min % 10], 4);
    memcpy(&sprt[2].p[0].uv, &HudPmx::gHudNumberUV[sec / 10], 4);
    memcpy(&sprt[3].p[0].uv, &HudPmx::gHudNumberUV[sec % 10], 4);
    memcpy(&sprt[5].p[0].uv, &HudPmx::gHudNumberUV[hun / 10], 4);
    memcpy(&sprt[6].p[0].uv, &HudPmx::gHudNumberUV[hun % 10], 4);
}

void Hud::BuildTach(sint32 player)
{
    FUNCTION_MARKER(0x800D3E94, "NFS4.EXE");
    SPRT *gSprt1 = player == 0 ? gSprite0 : gSprite1;
    sint32 carType = GameSetup::gData.carInfo[player].carType;
    if (carType >= 30)
        carType = 29;
    uint32 color = GameSetup::gData.Time == 0 ? day_needle[carType] : night_needle[carType];
    sint32 rpm = (sint32)((uint32)DashHUD::gInfo.rpm << 16);
    sint32 fangle = rpm / 10800 + 0x5999;
    if (fangle < 0x5999)
        fangle = 0x5999;
    if (fangle > 0x13334)
        fangle = 0x13334;
    sint32 sin, cos;
    fixedsincos(fangle, &sin, &cos);
    uint32 clut;
    memcpy(&clut, &HudPmx::gShapes[player == 0 ? 129 : 131].pixmap.u0, 4);
    sint32 x = fixedmult(cos, 29);
    sint32 y = fixedmult(sin, 29);
    uint32 uv = (clut & 0xFFFF0000U) | ((uint32)(y + 157) << 8) |
        (uint32)(x + (player == 0 ? 29 : 117));
    memcpy(&gSprt1[2].p[0].uv, &uv, 4);
    sint32 cos1 = fixedmult(cos, 10) + 14;
    sint32 sin1 = fixedmult(sin, 10) + 14;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_F3 *prim = reinterpret_cast<POLY_F3 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    LINE_F2 *line = reinterpret_cast<LINE_F2 *>(cprim->PrimPtr);
    setaddr(line, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(line + 1);
    setaddr(ot, line);
    setlen(line, 3);
    uint32 lineColor = ((uint32)color + 0x484848U) | 0x42000000U;
    line->p[0].x = (sint16)(14 - x);
    memcpy(&line->rgbc, &lineColor, 4);
    line->p[0].y = (sint16)(14 - y);
    line->p[1].y = (sint16)sin1;
    line->p[1].x = (sint16)cos1;
    POLY_F3 *prim2 = reinterpret_cast<POLY_F3 *>(cprim->PrimPtr);
    ot = cprim->LastPrim;
    setaddr(prim2, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim2 + 1);
    setaddr(ot, prim2);
    BuildF3(prim, &HudPmx::gShapes[130], cos1, sin1, color);
    BuildF3(prim2, &HudPmx::gShapes[130], cos1, sin1, 0);
    SetSemiTrans(prim, 0);
    fixedsincos(fangle - 0x200, &sin, &cos);
    sint16 coord = (sint16)(14 - fixedmult(cos, 32));
    prim->p[1].x = coord;
    prim2->p[1].x = coord;
    coord = (sint16)(14 - fixedmult(sin, 32));
    prim->p[1].y = coord;
    prim2->p[1].y = coord;
    fixedsincos(fangle + 0x200, &sin, &cos);
    coord = (sint16)(14 - fixedmult(cos, 32));
    prim->p[2].x = coord;
    prim2->p[2].x = coord;
    coord = (sint16)(14 - fixedmult(sin, 32));
    prim->p[2].y = coord;
    prim2->p[2].y = coord;
    ot = cprim->LastPrim;
    prim2->p[0].y = (sint16)(prim2->p[0].y + 2);
    prim2->p[2].y = (sint16)(coord + 2);
    prim2->p[1].y = (sint16)(prim2->p[1].y + 2);
    addPrim(ot, &gSprt1[2]);
}

sint32 Hud::BuildString(char *str, sint32 x, sint32 y, sint32 color, sint32 player, bool justwidth)
{
    FUNCTION_MARKER(0x800D42C8, "NFS4.EXE");
    GoTpage(1);
    sint32 numch = (sint32)(uint32)strlen(str);
    sint32 ox = x;
    uint32 ix = (uint32)x;
    for (sint32 i = 0; i < numch; ++i, ++str)
    {
        uint8 ch = (uint8)*str;
        if (ch == ' ')
            ix += 3U;
        else if (ch == '*')
        {
            uint8 alphShape = gPadinfo.buf[0].ID == 0x23 ? 173 : 170;
            ix += 2U;
            if (!justwidth)
                FBuildSprite(alphShape, (sint32)ix, y, color, 0);
            ix += 3U + (uint32)(sint32)HudPmx::gShapes[alphShape].width;
            if (GameSetup::gData.commMode == 1)
            {
                bool second = false;
                if (gPadinfo.buf[4].ID == 0x23)
                {
                    if (gPadinfo.buf[0].ID != 0x23)
                        second = true;
                }
                else if (gPadinfo.buf[0].ID == 0x23)
                    second = true;
                if (second)
                {
                    alphShape = gPadinfo.buf[4].ID == 0x23 ? 173 : 170;
                    if (!justwidth)
                        FBuildSprite(alphShape, (sint32)ix, y, color, 0);
                    ix += 3U + (uint32)(sint32)HudPmx::gShapes[alphShape].width;
                }
            }
        }
        else
        {
            uint8 alphShape;
            sint32 offy = 0;
            if (ch == '^')
                alphShape = gPadinfo.buf[player * 4].ID == 0x23 ? 173 : 170;
            else if (ch == '(')
                alphShape = gPadinfo.buf[player * 4].ID == 0x23 ? 171 : 169;
            else if (ch == ')')
                alphShape = gPadinfo.buf[player * 4].ID == 0x23 ? 172 : 168;
            else if (ch == '&')
                alphShape = 174;
            else if ((uint8)(ch - '0') < 10)
                alphShape = (uint8)(ch + 110);
            else if (ch == '-')
                alphShape = 72;
            else if (ch == ':')
                alphShape = 73;
            else if ((uint8)(ch + 64) < 29)
            {
                alphShape = (uint8)(ch - 118);
                offy = -1;
            }
            else
            {
                ch = (uint8)*str;
                alphShape = (uint8)(ch + 67);
                if (ch == 0xE5)
                {
                    alphShape = 103;
                    offy = -1;
                }
            }
            if (!justwidth)
                FBuildSprite(alphShape, (sint32)ix, (sint32)((uint32)y + (uint32)offy), color, 0);
            ix += 1U + (uint32)(sint32)HudPmx::gShapes[alphShape].width;
        }
    }
    GoTpage(0);
    return (sint32)(ix - (uint32)ox);
}

void Hud::BuildNumbers0(sint32 player)
{
    FUNCTION_MARKER(0x800D4624, "NFS4.EXE");
    SPRT *pSprt = player == 0 ? gSprite0 : gSprite1;
    POLY_F4 *HudF4 = player == 0 ? gHudF4 : gHudF4 + 7;
    POLY_G4 *HudG4 = player == 0 ? gHudG4 : gHudG4 + 4;
    sint32 splitY = player == 0 ? 0 : -15;
    if (GameSetup::gData.carInfo[player].HudTime != 0)
    {
        if (DashHUD::gInfo.flashtime == 0 || (simGlobal.gameTicks & 16) == 0)
        {
            sint32 time = BeTheCop != 0 ? BTC::Countdown : DashHUD::gInfo.laptime;
            BuildETimeString(pSprt + 12, time);
        }
        sint32 num = BeTheCop != 0 ? 5 : 8;
        uint32 *ot = DrawC::carCache.head.cprim.LastPrim;
        for (sint32 j = 12; j < 12 + num; ++j)
            addPrim(ot, &pSprt[j]);
        ot = DrawC::carCache.head.cprim.LastPrim;
        for (sint32 j = 4; j < 6; ++j)
            addPrim(ot, &pSprt[j]);
        ot = DrawC::carCache.head.cprim.LastPrim;
        addPrim(ot, &HudG4[0]);
        addPrim(ot, &HudF4[0]);
    }
    if (BeTheCop == 0)
    {
        sint32 y = g1Player[5].y + splitY + g1Player[9].y;
        pSprt[10].p[0].xy.y = (sint16)y;
        HudG4[2].p[0].xy.y = (sint16)y;
        HudG4[2].p[1].xy.y = (sint16)y;
        HudG4[2].p[2].xy.y = (sint16)(y + 10);
        HudG4[2].p[3].xy.y = (sint16)(y + 10);
        pSprt[11].p[0].xy.y = (sint16)y;
        HudF4[3].p[0].y = (sint16)(y + 7);
        HudF4[3].p[1].y = (sint16)(y + 7);
        HudF4[3].p[2].y = (sint16)(y + 10);
        HudF4[3].p[3].y = (sint16)(y + 10);
        ++y;
        pSprt[30].p[0].xy.y = (sint16)y;
        sint32 diff = (uint16)HudSplitTimeDiff1[player];
        pSprt[32].p[0].xy.y = (sint16)y;
        pSprt[33].p[0].xy.y = (sint16)y;
        pSprt[31].p[0].xy.y = (sint16)(diff + y);
        diff = (uint16)HudSplitTimeDiff2[player];
        pSprt[35].p[0].xy.y = (sint16)y;
        pSprt[36].p[0].xy.y = (sint16)y;
        pSprt[34].p[0].xy.y = (sint16)(diff + y);
    }
    sint32 bonusTime = BTC::BonusTime;
    sint32 num;
    if (bonusTime != 0 && BeTheCop != 0)
    {
        if (GameSetup::gData.carInfo[player].HudTime == 0)
            return;
        memcpy(&pSprt[10].p[0].uv, &HudPmx::gShapes[118].pixmap.u0, 4);
        BuildTimeString(pSprt + 30, bonusTime < 0 ? 0 : bonusTime);
        num = 4;
    }
    else
    {
        if (GameSetup::gData.checkpointType == 0)
            return;
        if (GameSetup::gData.checkpointHUD[player] == 0)
        {
            Car::tObj *carObj = Cars::gHumanRaceCarList[player];
            if (carObj->stats.checkpointDisplay < 1)
                return;
            sint32 time = carObj->stats.checkpointDifference;
            if (time < -0x95FF || time > 0x95FF || BeTheCop != 0 || DashHUD::gInfo.wrongway[player] != 0)
                return;
            sint32 shape = time < 0 ? 119 : 118;
            memcpy(&pSprt[10].p[0].uv, &HudPmx::gShapes[shape].pixmap.u0, 4);
            BuildTimeString(pSprt + 30, Cars::gHumanRaceCarList[player]->stats.checkpointDifference);
            num = 7;
        }
        else
        {
            if (!BuildDistanceString(pSprt + 30, player))
                return;
            sint32 shape = Cars::gHumanRaceCarList[player]->stats.checkpointUpdate < 0 ? 119 : 118;
            memcpy(&pSprt[10].p[0].uv, &HudPmx::gShapes[shape].pixmap.u0, 4);
            num = 5;
        }
    }
    uint32 *ot = DrawC::carCache.head.cprim.LastPrim;
    for (sint32 j = 30; j < 30 + num; ++j)
        addPrim(ot, &pSprt[j]);
    GoTpage(0);
    ot = DrawC::carCache.head.cprim.LastPrim;
    addPrim(ot, &pSprt[10]);
    GoTpage(1);
    ot = DrawC::carCache.head.cprim.LastPrim;
    addPrim(ot, &pSprt[11]);
    ot = DrawC::carCache.head.cprim.LastPrim;
    addPrim(ot, &HudG4[2]);
    addPrim(ot, &HudF4[3]);
}

void Hud::BuildNumbers(sint32 player)
{
    FUNCTION_MARKER(0x800D4E70, "NFS4.EXE");
    SPRT *pSprt = player == 0 ? gSprite0 : gSprite1;
    POLY_F4 *HudF4 = player == 0 ? gHudF4 : gHudF4 + 7;
    POLY_G4 *HudG4 = player == 0 ? gHudG4 : gHudG4 + 4;
    sint32 splitY = player == 0 ? 0 : -15;
    const GameSetup::tCarData &carInfo = GameSetup::gData.carInfo[player];
    const DashHUD::tInfo &info = DashHUD::gInfo;
    Draw::PrimStruct &cprim = DrawC::carCache.head.cprim;

    if (carInfo.HudLapnum != 0 && BeTheCop == 0 && info.maxlaps != 1)
    {
        memcpy(&pSprt[20].p[0].uv, &HudPmx::gHudNumberUV[info.lap], 4);
        memcpy(&pSprt[22].p[0].uv, &HudPmx::gHudNumberUV[info.maxlaps], 4);
        uint32 *ot = cprim.LastPrim;
        for (sint32 j = 20; j < 23; ++j)
            addPrim(ot, &pSprt[j]);
        for (sint32 j = 6; j < 8; ++j)
            addPrim(ot, &pSprt[j]);
        addPrim(ot, &HudG4[1]);
        addPrim(ot, &HudF4[1]);
    }

    if (carInfo.HudTime != 0 && info.record != 0 && info.record < 0x9600 &&
        BeTheCop == 0 && gShowedCDPlayer == 0 && info.maxlaps != 1)
    {
        if (info.flashtime == 0 || (simGlobal.gameTicks & 16) == 0)
            BuildTimeString(pSprt + 23, info.record);
        uint32 *ot = cprim.LastPrim;
        for (sint32 j = 23; j < 30; ++j)
            addPrim(ot, &pSprt[j]);
        for (sint32 j = 8; j < 10; ++j)
            addPrim(ot, &pSprt[j]);
        addPrim(ot, &HudG4[3]);
        addPrim(ot, &HudF4[2]);
    }

    if (carInfo.HudPosition != 0 && BeTheCop == 0 && info.opponents > 1)
    {
        const HudPmx::tShape &position = HudPmx::gShapes[44 + info.position];
        const HudPmx::tShape &opponents = HudPmx::gShapes[53 + info.opponents];
        pSprt[37].p[0].xy.x = (sint16)(g1Player[14].x + g1Player[10].x +
            HudPmx::gShapes[44].width - 2 - position.width);
        memcpy(&pSprt[37].p[0].uv, &position.pixmap.u0, 4);
        pSprt[37].w = position.width;
        memcpy(&pSprt[39].p[0].uv, &opponents.pixmap.u0, 4);
        pSprt[39].w = opponents.width;
        uint32 *ot = cprim.LastPrim;
        for (sint32 j = 37; j < 40; ++j)
            addPrim(ot, &pSprt[j]);
        if (carInfo.HudMap != 0)
        {
            GoTpage(0);
            ot = cprim.LastPrim;
            addPrim(ot, &pSprt[40]);
            GoTpage(1);
        }
    }

    GoTpage(0);
    if (carInfo.HudTach != 0)
    {
        uint32 *ot = cprim.LastPrim;
        sint32 gearSprite = info.gear == 0 ? 48 : (info.gear == 1 ? 47 : 39 + info.gear);
        addPrim(ot, &pSprt[gearSprite]);
        addPrim(ot, &pSprt[carInfo.HudSpeed == 0 ? 50 : 49]);
    }
    GoTpage(1);
    if (carInfo.HudTach != 0)
    {
        sint32 speed = fixedmult(carInfo.HudSpeedMult, info.speed) / 65536;
        sint32 hun = speed / 100;
        sint32 ten = speed / 10 - hun * 10;
        sint32 w1 = HudPmx::gShapes[44].width + 1;
        sint32 w2 = (w1 + HudPmx::gShapes[45].width) >> 1;
        sint32 w7 = (w1 + HudPmx::gShapes[51].width) >> 1;
        sint32 x = g1Player[1].x + g1Player[12].x + 4 + 2 * w1;
        sint32 y = g1Player[1].y + g1Player[12].y + splitY;
        BuildSpeedDigit(speed % 10, x, y);
        x -= ten == 1 ? w2 : (ten == 7 ? w7 : w1);
        if (hun != 0 || ten != 0)
            BuildSpeedDigit(ten, x, y);
        if (ten == 1)
            x -= w1 - w2;
        if (hun != 0)
            BuildSpeedDigit(hun, x - 1 - HudPmx::gShapes[44 + hun].width, y);
    }

    if (info.wrongway[player] != 0 && (simGlobal.gameTicks & 32) != 0)
    {
        uint32 *ot = cprim.LastPrim;
        addPrim(ot, &pSprt[0]);
    }
}

void Hud::InitMap(void)
{
    FUNCTION_MARKER(0x800D5A48, "NFS4.EXE");
    sint32 count = Cars::gNumRaceCars;
    for (sint32 i = 0; i < count; ++i)
        memcpy(&gMarkerColor[i], &Cars::gRaceCarList[i]->carInfo->HudColour, 4);
    count = Cars::gNumCopCars;
    for (sint32 i = 0; i < count; ++i)
        memcpy(&gCopMarkerColor[i], &Cars::gCopCarList[i]->carInfo->HudColour, 4);
}

void Hud::BuildMapMarkers(sint32 player)
{
    FUNCTION_MARKER(0x800D5AE8, "NFS4.EXE");
    (void)player;
    sint32 cenZ = (sint32)((uint32)gMapOffY + 2U);
    sint32 cenX = gMapOffX;
    for (sint32 group = 0; group < 2; ++group)
    {
        Car::tObj **cars = group == 0 ? Cars::gCopCarList : Cars::gRaceCarList;
        for (sint32 i = 0; i < (group == 0 ? Cars::gNumCopCars : Cars::gNumRaceCars); ++i)
        {
            Car::tObj *car = cars[i];
            if (car->N.active == 0)
                continue;
            sint32 slice = car->N.simRoadInfo.slice;
            Trk::NewSlice *road = &BWorldSm::slices[slice];
            sint32 center = road->center[0];
            sint32 scale = gMapScaleX;
            if (scale == 0 || (scale == -1 && center == INT_MIN))
                abort();
            sint32 x = center / scale;
            center = road->center[2];
            scale = gMapScaleY;
            if (scale == 0 || (scale == -1 && center == INT_MIN))
                abort();
            sint32 z = center / scale;
            sint32 rx = fixedmult(mapMarkerMCos, x);
            sint32 rz = fixedmult(mapMarkerMSin, z);
            rx = (sint32)((uint32)cenX + (uint32)rx - (uint32)rz);
            sint32 rotatedX = fixedmult(mapMarkerMSin, x);
            sint32 rotatedZ = fixedmult(mapMarkerMCos, z);
            rz = (sint32)((uint32)cenZ + (uint32)rotatedX + (uint32)rotatedZ);
            if (GameSetup::gData.mirrorTrack != 0)
                rx = (sint32)(0U - (uint32)rx);
            Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
            SPRT *sprt = reinterpret_cast<SPRT *>(cprim->PrimPtr);
            uint32 *ot = cprim->LastPrim;
            setaddr(sprt, getaddr(ot));
            setaddr(ot, sprt);
            car = cars[i];
            sint32 flags = group == 0 ? car->AIFlags : car->carFlags;
            cprim->PrimPtr = reinterpret_cast<char *>(sprt + 1);
            uint32 color;
            sint32 shape;
            sint32 markerX;
            if (group == 0)
            {
                if ((flags & 2) == 0)
                    memcpy(&color, &gCopMarkerColor[i], sizeof(color));
                else
                    color = gFlip == 0 && simVar.quickPauseSim == 0 ? 0xFF0000 : 0xFF;
                currentSpriteColor = (sint32)color;
                shape = 122;
                markerX = 20;
            }
            else if ((flags & 0x200) != 0)
            {
                if ((car->AIFlags & 2) == 0)
                    memcpy(&color, &gMarkerColor[i], sizeof(color));
                else
                    color = gFlip == 0 && simVar.quickPauseSim == 0 ? 0xFF0000 : 0xFF;
                currentSpriteColor = (sint32)color;
                shape = 121;
                markerX = 19;
            }
            else
            {
                shape = (flags & 4) != 0 ? 121 : 122;
                markerX = (flags & 4) != 0 ? 19 : 20;
                memcpy(&color, &gMarkerColor[i], sizeof(color));
            }
            BuildSprite(sprt, shape, (sint32)(((uint32)rx + (uint32)markerX) & 0xFFFFU),
                (sint32)((24U - (uint32)rz) & 0xFFFFU), color, 0);
        }
    }
}

void Hud::WingmanFlash(sint32 player, sint32 index)
{
    FUNCTION_MARKER(0x800D5FB8, "NFS4.EXE");
    if (Replay::ReplayMode < 2 && HudBustedOverlay == 0)
    {
        if ((uint8)gWingmanInterface[player] != 1)
        {
            InitMapFrame(player, 1);
            gWingmanInterface[player] = 1;
        }
        gWingmanFlashIcon[player] = (char)index;
        gWingmanFlashTicks[player] = (sint32)((uint32)ticks + 100U);
    }
}

void Hud::BuildWingmanInterface(sint32 player)
{
    FUNCTION_MARKER(0x800D606C, "NFS4.EXE");
    sint32 splitY = player == 0 ? 0 : -15;
    uint32 currentTick = (uint32)ticks;
    sint32 flashTicks = (sint32)((uint32)gWingmanFlashTicks[player] - currentTick);
    sint32 x = g1Player[14].x;
    sint32 y = (sint32)((uint32)(sint32)g1Player[14].y + (uint32)HudMapOffsetY +
        (uint32)splitY + 2U);
    for (sint32 i = 0; i < 5; ++i)
    {
        char *text = TextSys::Word(0x29 + i);
        BuildString(text, x - 27, (sint32)((uint32)y + 3U + (uint32)i * 9U),
            0x808080, i == 0 ? 0 : player, false);
    }
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    if (flashTicks > 0)
    {
        POLY_F4 *poly = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(poly, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(poly + 1);
        setaddr(ot, poly);
        sint32 flashY = (sint32)((uint32)y + ((uint8)gWingmanFlashIcon[player] + 1U) * 9U + 2U);
        BuildF4(poly, 0, x - 16, flashY, 63, 8, (flashTicks % 20) * 10);
    }
    for (sint32 i = 0; i < 5; ++i)
    {
        POLY_F4 *poly = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(poly, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(poly + 1);
        setaddr(ot, poly);
        BuildF4(poly, 0, x - 28, (sint32)((uint32)y + 2U + (uint32)i * 9U), 75, 7, 0);
    }
    POLY_F4 *poly = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(poly, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(poly + 1);
    setaddr(ot, poly);
    BuildF4(poly, 1, x - 28, y, 75, 48, 0);
}

void Hud::InitCdPlayer(void)
{
    FUNCTION_MARKER(0x800D63B8, "NFS4.EXE");
    sint32 replayMode = Replay::ReplayMode;
    gCdLastTick = 0;
    gCdScrollTitle = 0;
    gCdActive = 0;
    ActivateCDPlayer = replayMode < 2;
}

void Hud::BuildCdPlayer(sint32 type, sint32)
{
    FUNCTION_MARKER(0x800D63DC, "NFS4.EXE");
    static sint32 keepup = 0;

    sint32 x = g1Player[15].x + 2;
    sint32 y = g1Player[15].y + 2;
    if (type == 0)
        keepup = 1;
    const AudioMus::tCurrentSong *currentSong = AudioMus::GetCurrentSong();
    if (!currentSong)
        return;

    if (!gCdActive &&
        (simGlobal.gameTicks < 576 ||
         ((uint8)countdown < 4 && !BeTheCop) ||
         ((PAD_state(4) & 0x400) && DashHUD::gInfo.splitscreen) ||
         ((PAD_state(0) & 0x400) && (!BeTheCop || DashHUD::gInfo.splitscreen)) ||
         (gPadinfo.buf[0].ID == 0x23 && gPadinfo.buf[0].data.analog.lefty >= 192 &&
             (!BeTheCop || DashHUD::gInfo.splitscreen)) ||
         (gPadinfo.buf[4].ID == 0x23 && gPadinfo.buf[4].data.analog.lefty >= 192 &&
             DashHUD::gInfo.splitscreen))) {
        gCdActive = 1;
        ActivateCDPlayer = 1;
    }
    if (ActivateCDPlayer) {
        if (BeTheCop)
            gCdActive = 1;
        sint32 now = ticks;
        ActivateCDPlayer = 0;
        gCdScrollTitle = 1;
        gCdLastTick = now;
        keepup = type == 0;
    }
    if (keepup)
        type = 0;

    sint32 time = currentSong->remaining;
    sint32 index = currentSong->index;
    char strtitle[64];
    char strartist[32];
    char *title = 0;
    char *artist = 0;
    if (currentSong->info.title) {
        sprintf(strtitle, "%.31s", currentSong->info.title);
        title = strtitle;
        uppercase(title);
    }
    if (currentSong->info.artist) {
        sprintf(strartist, "%.31s", currentSong->info.artist);
        artist = strartist;
        if (type == 0)
            uppercase(artist);
    }
    if (!gCdActive) {
        kTurnSongOffNext = 1;
        return;
    }

    char strindex[12];
    if (index > 0) {
        sprintf(strindex, "%02d", index);
        if (!title) {
            sint32 now = ticks;
            gCdScrollTitle = 1;
            gCdLastTick = now;
        }
    } else {
        strcpy(strindex, "- -");
        artist = 0;
        if (index == 0 || index == -2 || !title) {
            const char *status = TextSys::Word(index == 0 ? 68 : index == -2 ? 69 : 70);
            if (status) {
                sprintf(strtitle, "%.63s", status);
                title = strtitle;
                uppercase(title);
            } else {
                title = 0;
            }
        }
    }

    sint32 dx = 0;
    char strscrolltitle[64];
    if (title) {
        const sint32 glyphCount = sizeof(HudPmx::gShapes) / sizeof(HudPmx::gShapes[0]);
        for (char *s = title; *s; ++s) {
            uint8 ch = (uint8)*s;
            sint32 glyph = (ch >= '0' && ch <= '9') ? ch + 110 : ch + 67;
            if (glyph >= glyphCount && !(ch >= 0xC0 && ch <= 0xDC) && ch != 0xE5)
                *s = ' ';
        }

        sint32 tx = BuildString(title, 0, 0, 0, 0, true);
        if (gCdScrollTitle < (sint32)((uint32)tx + 76U)) {
            while (gCdLastTick < ticks) {
                gCdLastTick = (sint32)((uint32)gCdLastTick + 4U);
                gCdScrollTitle = (sint32)((uint32)gCdScrollTitle + 1U);
            }
        } else if ((sint32)((uint32)gCdLastTick + 128U) < ticks) {
            gCdActive = 0;
        }
        sint32 offset = (sint32)(76U - (uint32)gCdScrollTitle);
        char *out = strscrolltitle;
        for (const char *s = title; *s; ++s) {
            uint8 ch = (uint8)*s;
            sint32 w = 3;
            if (ch != ' ') {
                sint32 glyph = (ch >= '0' && ch <= '9') ? ch + 110 : ch + 67;
                if (glyph >= glyphCount)
                    glyph = ch == 0xE5 ? 103 : ch - 118;
                w = HudPmx::gShapes[glyph].width + 1;
            }
            sint32 next = (sint32)((uint32)offset + (uint32)w);
            if (next >= 76)
                break;
            if (offset < 0)
                dx = (sint32)((uint32)dx + (uint32)w);
            else
                *out++ = (char)ch;
            offset = next;
        }
        *out = 0;
    } else {
        sint32 now = ticks;
        keepup = 0;
        gCdLastTick = now;
    }

    if (type == 0) {
        if (!title)
            return;
        gShowedCDPlayer = 1;
        BuildString(strscrolltitle, (sint32)((uint32)x + (uint32)dx + 86U - (uint32)gCdScrollTitle),
            y + 10, 0xBEBE, 0, false);
        if (artist)
            BuildString(artist, x + 10, y + 19, 0x808080, 0, false);
        GoTpage(0);
        BlackThinBox(g1Player[15].x + 10, g1Player[15].y + 10, 80, 18);
        FBuildF4(0, g1Player[15].x + 10, g1Player[15].y + 10, 80, 18, 0, 0, 0);
    } else {
        gShowedCDPlayer = 1;
        Font::TextColor(4);
        Font::TextXY(strindex, x, y - 2);
        if (title) {
            Font::TextColor(3);
            BuildString(strscrolltitle, (sint32)((uint32)x + (uint32)dx + 98U - (uint32)gCdScrollTitle),
                y + 3, 0xBEBE, 0, false);
            GoTpage(0);
            if (index != 0) {
                char strtime[18];
                sprintf(strtime, "%1d%c%02d", time / 60000,
                    (uint8)HudminChar[GameSetup::gData.userSetting.language], (time % 60000) / 1000);
                Font::TextColor(4);
                sint32 w = textpixels(strtime);
                Font::TextXY(strtime, (sint32)((uint32)x - (uint32)w + 92U), y + 12);
            }
        }
        BlackThinBox(g1Player[15].x, g1Player[15].y, 102, 28);
        FBuildF4(0, g1Player[15].x, g1Player[15].y, 102, 14, 0, 0, 0);
        FBuildF4(0, g1Player[15].x, g1Player[15].y + 27, 102, 1, 0, 0, 0);
        FBuildF4(1, g1Player[15].x, g1Player[15].y, 102, 28, 0, 0, 0);
    }
}

sint32 Hud::BuildRadar(sint32 player)
{
    FUNCTION_MARKER(0x800D6B48, "NFS4.EXE");
    const BO::tNewtonObj *car = Camera::gInfo[player].anchor;
    sint32 m00 = car->orientMat.m[0] >> 8;
    sint32 m01 = car->orientMat.m[6] >> 8;
    sint32 m10 = car->orientMat.m[2] >> 8;
    sint32 m11 = car->orientMat.m[8] >> 8;
    sint32 cenX = car->position.x >> 8;
    sint32 cenZ = (sint32)(0U - (uint32)car->position.z) >> 8;
    coorddef raceScreen[sizeof(Cars::gRaceCarList) / sizeof(Cars::gRaceCarList[0])];
    coorddef copScreen[sizeof(Cars::gCopCarList) / sizeof(Cars::gCopCarList[0])];
    sint32 visible = 0;
    for (sint32 group = 0; group < 2; ++group)
    {
        Car::tObj **cars = group == 0 ? Cars::gRaceCarList : Cars::gCopCarList;
        coorddef *scr = group == 0 ? raceScreen : copScreen;
        for (sint32 i = 0; i < (group == 0 ? Cars::gNumRaceCars : Cars::gNumCopCars); ++i)
        {
            const Car::tObj *candidate = cars[i];
            if (group != 0 && candidate->N.active == 0)
                continue;
            sint32 x = (candidate->N.position.x >> 8) - cenX;
            sint32 z = ((sint32)(0U - (uint32)candidate->N.position.z) >> 8) - cenZ;
            sint32 mapx = (sint32)((uint32)m00 * (uint32)x + (uint32)m01 * (uint32)z) >> 16;
            sint32 mapz = (sint32)((uint32)m10 * (uint32)x + (uint32)m11 * (uint32)z) >> 17;
            if (GameSetup::gData.mirrorTrack != 0)
                mapx = -mapx;
            scr[i].x = mapx;
            scr[i].z = mapz;
            if (candidate->N.active != 0 && (group != 0 || candidate->carIndex != player) &&
                mapx > -22 && mapx < 22 && mapz > -24 && mapz < 24)
                visible = 1;
        }
    }

    if (visible == 0)
    {
        for (sint32 group = 0; group < 2; ++group)
        {
            Car::tObj **cars = group == 0 ? Cars::gRaceCarList : Cars::gCopCarList;
            coorddef *scr = group == 0 ? raceScreen : copScreen;
            for (sint32 i = 0; i < (group == 0 ? Cars::gNumRaceCars : Cars::gNumCopCars); ++i)
            {
                if (cars[i]->N.active == 0)
                    continue;
                sint32 x = scr[i].x >> 2;
                sint32 z = scr[i].z >> 2;
                scr[i].x = x;
                scr[i].z = z;
                if ((group != 0 || cars[i]->carIndex != player) && x > -22 && x < 22 && z > -24 && z < 24)
                    visible = 2;
            }
        }
    }

    for (sint32 i = 0; i < Cars::gNumCopCars; ++i)
    {
        if (Cars::gCopCarList[i]->N.active == 0)
            continue;
        currentSpriteColor = gFlip == 0 && simVar.quickPauseSim == 0 ? 0xFF0000 : 0xFF;
        FBuildSprite(122, (uint16)(copScreen[i].x + 20),
            (uint16)(copScreen[i].z + 24), currentSpriteColor, 0);
    }
    for (sint32 i = 0; i < Cars::gNumRaceCars; ++i)
    {
        if (i == player || Cars::gRaceCarList[i]->N.active == 0)
            continue;
        bool human = (Cars::gRaceCarList[i]->carFlags & 4) != 0;
        uint32 color;
        memcpy(&color, &gMarkerColor[i], sizeof(color));
        FBuildSprite(human ? 121 : 122, (uint16)(raceScreen[i].x + (human ? 19 : 20)),
            (uint16)(raceScreen[i].z + 24), color, 0);
    }
    return visible;
}

void Hud::BuildReplay(void)
{
    FUNCTION_MARKER(0x800D7250, "NFS4.EXE");
    char hilite[5] = { 4, 0, 1, 2, 3 };
    SPRT *sprites = 0;
    uint32 color = 0x66808080U;
    for (sint32 i = 51; i < 63; ++i)
    {
        sprites = gSprite0;
        memcpy(&sprites[i].rgbc, &color, 4);
    }
    color = 0x6600BEBEU;
    sint32 spr = (uint8)hilite[Replay::ReplayInterface.selection] + 51;
    memcpy(&sprites[spr].rgbc, &color, 4);
    if (Replay::ReplayInterface.selection == 3)
        memcpy(&sprites[56].rgbc, &color, 4);
    HudPmx::tShape *shape = &HudPmx::gShapes[110 - Replay::ReplayInterface.pause];
    memcpy(&gSprite0[52].p[0].uv, &shape->pixmap, 4);
    sint32 speed = Replay::ReplayInterface.speed;
    if (speed == 0)
        spr = 114;
    else if (speed == 1)
        spr = 115;
    else if (speed == 2)
        spr = 117;
    else
        spr = 116;
    gSprite0[56].p[0].uv.u = HudPmx::gShapes[spr].pixmap.u0;
    gSprite0[56].p[0].uv.v = HudPmx::gShapes[spr].pixmap.v0;
    sprites = gSprite0;
    shape = &HudPmx::gShapes[Replay::ReplayInterface.camera + 2];
    memcpy(&sprites[57].p[0].uv, &shape->pixmap, 4);
    sprites[57].p[0].xy.x = (sint16)((uint16)g1Player[13].x + 117U);
    color = Replay::ReplayInterface.selection == 4 ? 0x6600BEBEU : 0x66808080U;
    memcpy(&sprites[57].rgbc, &color, 4);
    uint32 *ot = DrawC::carCache.head.cprim.LastPrim;
    sprites = gSprite0;
    addPrim(ot, &sprites[57]);
    for (sint32 i = 51; i < 56; ++i)
        addPrim(ot, &sprites[i]);
    sprites = gSprite0;
    ot = DrawC::carCache.head.cprim.LastPrim;
    addPrim(ot, &sprites[56]);
    addPrim(ot, &gTPage1[0][3]);
    addPrim(ot, &gTPage0[0][3]);
}

sint32 Hud::NextPlayer(sint32 player)
{
    FUNCTION_MARKER(0x800D754C, "NFS4.EXE");
    sint32 direction = (Input::gLookBehind[player] > 0) ^ (DashHUD::gInfo.wrongway[player] > 0);
    Car::tObj *carObj = Cars::gHumanRaceCarList[player];
    if (Cars::gNumRaceCars >= 2)
    {
        sint32 j = Stats::GetPosition(carObj);
        if (j != 1 || direction != 0)
        {
            sint32 index = carObj->sortIndex;
            direction ^= GameSetup::gData.reverseTrack;
            sint32 numCars = Cars::gNumCars;
            sint32 limit = (sint32)((uint32)numCars - 1U);
            for (sint32 i = 0; i < limit; ++i)
            {
                if (direction == 0)
                    index = (sint32)((uint32)index + 1U);
                else
                    index = (sint32)((uint32)index - 1U);
                if (index < 0)
                    index = (sint32)((uint32)index + (uint32)numCars);
                if (index >= numCars)
                    index = 0;
                Car::tObj *next = Cars::gSortedList[index];
                sint32 flags = next->carFlags;
                if ((flags & 4) != 0)
                    return player == 0 ? 8 : 7;
                if ((flags & 8) != 0)
                    return next->personalityIndex;
            }
        }
    }
    return -1;
}

char * Hud::NextPlayerNameOrCarOrTime(sint32 player)
{
    FUNCTION_MARKER(0x800D76B0, "NFS4.EXE");
    static char emptyLabel[1] = { 0 };
    sint32 direction = (Input::gLookBehind[player] > 0) ^ (DashHUD::gInfo.wrongway[player] > 0);
    Car::tObj *carObj = Cars::gHumanRaceCarList[player];
    if (Cars::gNumRaceCars >= 2)
    {
        sint32 j = Stats::GetPosition(carObj);
        if (j != 1 || direction != 0)
        {
            sint32 index = carObj->sortIndex;
            direction ^= GameSetup::gData.reverseTrack;
            sint32 numCars = Cars::gNumCars;
            sint32 limit = (sint32)((uint32)numCars - 1U);
            for (sint32 i = 0; i < limit; ++i)
            {
                if (direction == 0)
                    index = (sint32)((uint32)index + 1U);
                else
                    index = (sint32)((uint32)index - 1U);
                if (index < 0)
                    index = (sint32)((uint32)index + (uint32)numCars);
                if (index >= numCars)
                    index = 0;
                Car::tObj *next = Cars::gSortedList[index];
                if ((next->carFlags & 12) != 0)
                {
                    if (GameSetup::gData.carInfo[player].HudOpponentID == 2)
                        return next->carNameLocalized;
                    return next->carInfo->driver;
                }
            }
        }
    }
    return emptyLabel;
}

void Hud::RenderMapView(void)
{
    FUNCTION_MARKER(0x800D7838, "NFS4.EXE");
    for (sint32 player = 0; player <= DashHUD::gInfo.splitscreen; ++player)
    {
        GameSetup::tCarData *carInfo = &GameSetup::gData.carInfo[player];
        if (carInfo->HudMap != 0 && DashHUD::gInfo.showhud[player] != 0 &&
            (uint8)gWingmanInterface[player] == 0)
        {
            POLY_FT4 *HudFT4 = player == 0 ? gHudFT4 : gHudFT4 + 5;
            Draw::StartRenderingView(gMapView[player]);
            if (carInfo->HudMap == 1)
            {
                BuildMapMarkers(player);
                uint32 *ot = DrawC::carCache.head.cprim.LastPrim;
                addPrim(ot, HudFT4);
            }
            else
            {
                sint32 radar = BuildRadar(player);
                uint32 *ot = DrawC::carCache.head.cprim.LastPrim;
                if (radar == 1)
                {
                    addPrim(ot, HudFT4 + 1);
                    addPrim(ot, HudFT4 + 2);
                }
                else
                {
                    addPrim(ot, HudFT4 + 3);
                    addPrim(ot, HudFT4 + 4);
                }
            }
            uint32 *ot = DrawC::carCache.head.cprim.LastPrim;
            addPrim(ot, &gTPage1[player][1]);
            Draw::StopRenderingView(gMapView[player]);
        }
    }
}

void Hud::BlackThinBox(sint32 x, sint32 y, sint32 w, sint32 h)
{
    FUNCTION_MARKER(0x800D7ABC, "NFS4.EXE");
    sint32 cornerWidth = (sint16)HudPmx::gShapes[17].width;
    sint16 CORNERHEIGHT = (sint16)HudPmx::gShapes[17].height;
    sint32 top = (sint32)((uint32)y - 2U);
    sint32 topX = (sint32)((uint32)x + (uint32)cornerWidth - 2U);
    sint32 innerWidth = (sint32)((uint32)w - (uint32)(cornerWidth * 2) + 2U);
    FBuildF4(0, topX, top, innerWidth, 2, 0, 0, 0);
    sint32 left = (sint32)((uint32)x - 2U);
    sint32 sideY = (sint32)((uint32)y + (uint32)CORNERHEIGHT - 2U);
    sint32 innerHeight = (sint32)((uint32)h - (uint32)(CORNERHEIGHT * 2) + 4U);
    FBuildF4(0, left, sideY, 2, innerHeight, 0, 0, 0);
    sint32 right = (sint32)((uint32)x + (uint32)w);
    FBuildF4(0, (sint32)((uint32)right - 2U), sideY, 2, innerHeight, 0, 0, 0);
    sint32 bottom = (sint32)((uint32)y + (uint32)h);
    FBuildF4(0, topX, bottom, innerWidth, 2, 0, 0, 0);
    FBuildGT4(&HudPmx::gShapes[14], left, top, 0);
    right = (sint32)((uint32)right - (uint32)cornerWidth);
    FBuildGT4(&HudPmx::gShapes[15], right, top, 0);
    bottom = (sint32)((uint32)bottom - (uint32)(CORNERHEIGHT - 2));
    FBuildGT4(&HudPmx::gShapes[16], left, bottom, 0);
    FBuildGT4(&HudPmx::gShapes[17], right, bottom, 0);
}

void BigBTCTime(sint32 secs)
{
    FUNCTION_MARKER(0x800D7FB0, "NFS4.EXE");
    static sint32 lastsec = 50;
    static sint32 lastsectick = 0;
    if (BTC::BonusTime != 0 || HudBustedOverlay != 0 || secs < 0)
        return;
    if (secs != lastsec)
    {
        sint32 currentTick = ticks;
        lastsec = secs;
        lastsectick = currentTick;
    }
    sint32 x = g1Player[15].x + 2;
    sint32 y = g1Player[15].y;
    sint32 xx = x;
    sint32 yy = y;
    sint32 w1 = HudPmx::gShapes[44].width + 1;
    sint32 w2 = HudPmx::gShapes[45].width + 1;
    uint32 Col;
    uint32 Col2;
    if (secs < 11)
    {
        uint32 currentTick = (uint32)ticks;
        sint32 diff = (sint32)(currentTick - (uint32)lastsectick);
        if (diff < 64)
        {
            Hud::BlackThinBox(xx, yy, w1 * 2, 14);
            Hud::FBuildF4(0, xx, yy, w1 * 2, 14, 0, 0, 0);
            BTC::playedsoundalready = 0;
            return;
        }
        Col = 200;
        if (BTC::playedsoundalready == 0)
        {
            AudioCmn::PlayWrongWaySFX();
            BTC::playedsoundalready = 1;
        }
        Col2 = 100;
    }
    else
    {
        Col = 0xC800;
        BTC::playedsoundalready = 0;
        Col2 = 0x6400;
    }
    x += w1;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_GT4 *prim = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    sint32 ten = secs / 10;
    Hud::BuildGT4(prim, &HudPmx::gShapes[secs % 10 + 44], x - 1, y, Col);
    memcpy(&prim->p[3].rgbc, &Col2, sizeof(Col2));
    memcpy(&prim->p[2].rgbc, &Col2, sizeof(Col2));
    if (ten != 0)
    {
        x -= ten == 1 ? w2 : w1;
        prim = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
        ot = cprim->LastPrim;
        setaddr(prim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
        setaddr(ot, prim);
        Hud::BuildGT4(prim, &HudPmx::gShapes[ten + 44], x, y, Col);
        memcpy(&prim->p[3].rgbc, &Col2, sizeof(Col2));
        memcpy(&prim->p[2].rgbc, &Col2, sizeof(Col2));
    }
    Hud::BlackThinBox(xx, yy, w1 * 2, 14);
    Hud::FBuildF4(0, xx, yy, w1 * 2, 14, 0, 0, 0);
}

void Hud::Draw321Num(sint32 x, sint32 y, sint32 num, sint32 flare_intensity, sint32, sint32)
{
    FUNCTION_MARKER(0x800D7CA8, "NFS4.EXE");
    if (flare_intensity != 0)
    {
        sint32 k = 0;
        uint32 by = (uint32)y;
        for (sint32 j = 0; j < 5; ++j)
        {
            uint32 bx = (uint32)x;
            for (sint32 i = 0; i < 5; ++i)
            {
                if ((Character[num] & (1UL << k)) != 0)
                    Flare::_2DHalo((sint32)(bx + 4U), (sint32)(by + 4U),
                        flare_intensity, flare_intensity, 6);
                bx += 10U;
                ++k;
            }
            by += 9U;
        }
    }
    BlackThinBox((sint32)((uint32)x - 3U), (sint32)((uint32)y - 2U), 56, 49);
    sint32 k = 0;
    uint32 by = (uint32)y;
    for (sint32 j = 0; j < 5; ++j)
    {
        uint32 bx = (uint32)x;
        for (sint32 i = 0; i < 5; ++i)
        {
            sint32 index = (Character[num] & (1UL << k)) != 0 ? 61 : 60;
            ++k;
            FBuildSprite(index, (sint32)(bx + 1U), (sint32)(by + 1U), 0x808080UL, 0);
            bx += 10U;
        }
        by += 9U;
    }
    FBuildF4(0, (sint32)((uint32)x - 3U), (sint32)((uint32)y - 2U), 56, 49, 0, 0, 0);
    GoTpage(1);
}

void Hud::Render321Go(void)
{
    FUNCTION_MARKER(0x800D7E64, "NFS4.EXE");
    static char oldCountdown = (char)0xFF;
    static uint32 countdownTick = 0;
    sint32 view = gStatsView;
    gCView.id = view;
    Draw::StartRenderingView(view);
    sint32 y = g1Player[17].y;
    uint32 currentCountdown = (uint8)countdown;
    if ((sint32)(sint8)oldCountdown != (sint32)currentCountdown)
    {
        uint32 currentTick = (uint32)ticks;
        oldCountdown = (char)currentCountdown;
        countdownTick = currentTick;
    }
    uint32 currentTick = (uint32)ticks - countdownTick;
    sint32 flare_intensity = 0;
    if (currentTick < 100)
    {
        if (currentCountdown == 4)
            flare_intensity = 8000 - (sint32)currentTick * 80;
        else
            flare_intensity = 6000 - (sint32)currentTick * 60;
    }
    currentCountdown = (uint8)countdown;
    sint32 num = 4 - (sint32)currentCountdown;
    if (currentCountdown != 4)
    {
        Draw321Num(72, y, num, flare_intensity, 1, 0);
        Draw321Num(135, y, num, flare_intensity, 0, 0);
        Draw321Num(198, y, num, flare_intensity, 0, 1);
    }
    Draw::StopRenderingView(gStatsView);
}

void Hud::RenderHudView(void)
{
    FUNCTION_MARKER(0x800D82D0, "NFS4.EXE");
    char sBuildOutput[64];
    for (sint32 j = 0; j <= DashHUD::gInfo.splitscreen; ++j)
    {
        sint32 splitY = j == 0 ? 0 : -15;
        Draw::StartRenderingView(gHudView[j]);
        DebugInfo();
        DebugCrap();
        if (GameSetup::gData.raceType == 1 && PerpOverlayOn[j] != 0)
        {
            char *text = TextSys::Word((sint32)((uint32)PerpOverlayMessage[j] + 0x41U));
            uint32 ww = (uint32)textpixels(text);
            sint32 y = g1Player[18].y + splitY;
            Font::TextColor(4);
            text = TextSys::Word((sint32)((uint32)PerpOverlayMessage[j] + 0x41U));
            uint32 ww2 = ww >> 1;
            Font::TextXY(text, (sint32)(160U - (ww2 + 1U)), y);
            sint32 gameTick = simGlobal.gameTicks;
            uint32 color = (gameTick & 16) == 0 ? 0x800000 : 0x80;
            sint32 flare_type = (gameTick & 16) == 0 ? 10 : 8;
            sint32 flare_intensity = 4000 - (gameTick % 15) * 250;
            uint32 x = 160U - ww2;
            Flare::_2DHalo((sint32)(x - 35U), y + 8, flare_intensity, flare_intensity, flare_type);
            Flare::_2DHalo((sint32)(x - 45U), y + 8, flare_intensity, flare_intensity, flare_type);
            FBuildGT4(&HudPmx::gShapes[61], (sint32)(x - 38U), y + 5, color);
            FBuildGT4(&HudPmx::gShapes[61], (sint32)(x - 48U), y + 5, color);
            gameTick = simGlobal.gameTicks;
            color = (gameTick & 16) != 0 ? 0x800000 : 0x80;
            flare_type = (gameTick & 16) != 0 ? 10 : 8;
            Flare::_2DHalo((sint32)(ww2 + 192U), y + 8, flare_intensity, flare_intensity, flare_type);
            Flare::_2DHalo((sint32)(ww2 + 202U), y + 8, flare_intensity, flare_intensity, flare_type);
            FBuildGT4(&HudPmx::gShapes[61], (sint32)(ww2 + 189U), y + 5, color);
            FBuildGT4(&HudPmx::gShapes[61], (sint32)(ww2 + 199U), y + 5, color);
            BlackThinBox((sint32)(x - 49U), y, (sint32)(ww + 98U), 17);
            FBuildF4(0, (sint32)(x - 49U), y, 23, 17, 0, 0, 0);
            FBuildF4(0, (sint32)(ww2 + 186U), y, 21, 17, 0, 0, 0);
            FBuildF4(0, (sint32)(ww2 + 186U), y + 3, 22, 11, 0, 0, 0);
            FBuildF4(1, (sint32)(x - 26U), y, (sint32)(ww + 54U), 17, 0x461414, 0, 0);
        }
        if (j == DashHUD::gInfo.splitscreen)
        {
            sint32 secs = BTC::Countdown >> 6;
            if ((uint32)secs - 1U < 30U)
                BigBTCTime(secs);
            else
            {
                sint32 type = 0;
                if (simGlobal.gameTicks >= 576)
                {
                    type = 1;
                    if ((uint8)countdown < 4 && BeTheCop == 0)
                        type = 0;
                }
                BuildCdPlayer(type, j);
            }
        }
        if (DashHUD::gInfo.showhud[j] != 0)
        {
            SPRT *gSprt1 = j == 0 ? gSprite0 : gSprite1;
            DashHUD::CheckWrongWay(j);
            DashHUD::HUDCalc(j);
            BuildNumbers0(j);
            BuildNumbers(j);
            uint32 *ot = DrawC::carCache.head.cprim.LastPrim;
            addPrim(ot, &gTPage0[j][0]);
            GameSetup::tCarData *carInfo = &GameSetup::gData.carInfo[j];
            if (carInfo->HudTach != 0)
                addPrim(ot, &gSprt1[1]);
            if (carInfo->HudMap != 0)
            {
                if (carInfo->HudOpponentID != 0 && BeTheCop == 0 && GameSetup::gData.commMode != 1)
                {
                    sint32 nextplayer = NextPlayer(j);
                    if (nextplayer >= 0 && nextplayer < 9 && BeTheCop == 0)
                    {
                        sint32 totalwidth = HudPmx::gShapes[120].width;
                        sint32 w1 = HudPmx::gShapes[14].width;
                        sint32 w2 = HudPmx::gShapes[20].width;
                        char *name = NextPlayerNameOrCarOrTime(j);
                        sprintf(sBuildOutput, "%s", name);
                        GoTpage(1);
                        uppercase(sBuildOutput);
                        sint32 width = BuildString(sBuildOutput, 0, 0, 0, 0, true);
                        sint32 centerOffset = (sint32)((uint32)(totalwidth + 2) - (uint32)width) >> 1;
                        BuildString(sBuildOutput, (sint32)((uint32)(sint32)g1Player[14].x + (uint32)centerOffset),
                            (sint32)((uint32)(sint32)g1Player[14].y + (uint32)HudMapOffsetY +
                                (uint32)splitY - (uint32)(sint32)HudPmx::gShapes[14].height + 1U),
                            0x808080, 0, false);
                        GoTpage(0);
                        FBuildGT4(&HudPmx::gShapes[14], g1Player[14].x,
                            (sint32)((uint32)(sint32)g1Player[14].y + (uint32)HudMapOffsetY +
                                (uint32)splitY - (uint32)(sint32)HudPmx::gShapes[14].height), 0x808080);
                        FBuildGT4(&HudPmx::gShapes[15], g1Player[14].x + w1 + w2,
                            (sint32)((uint32)(sint32)g1Player[14].y + (uint32)HudMapOffsetY +
                                (uint32)splitY - (uint32)(sint32)HudPmx::gShapes[15].height), 0x808080);
                        sint32 h = HudPmx::gShapes[14].height;
                        FBuildF4(0, g1Player[14].x + 2,
                            (sint32)((uint32)(sint32)g1Player[14].y + (uint32)HudMapOffsetY +
                                (uint32)splitY - (uint32)h), w1 + w2 + 3, h, 0, 0, 0);
                        h = (uint8)gWingmanInterface[j] == 0 ? 3 : 2;
                        FBuildF4(0, g1Player[14].x,
                            (sint32)((uint32)(sint32)g1Player[14].y + (uint32)HudMapOffsetY + (uint32)splitY),
                            3, h, 0, 0, 0);
                        FBuildF4(0, g1Player[14].x + w1 + w2 + HudPmx::gShapes[15].width - 3,
                            (sint32)((uint32)(sint32)g1Player[14].y + (uint32)HudMapOffsetY + (uint32)splitY),
                            3, 3, 0, 0, 0);
                    }
                }
                ot = DrawC::carCache.head.cprim.LastPrim;
                for (sint32 i = 63; i < 71; ++i)
                    addPrim(ot, &gSprt1[i]);
            }
        }
        if (j == 0 && Replay::ReplayMode >= 2 && Replay::ReplayInterface.statsScreen == 0)
            BuildReplay();
        uint32 *ot = DrawC::carCache.head.cprim.LastPrim;
        addPrim(ot, &gTPage1[j][0]);
        if (DashHUD::gInfo.showhud[j] != 0 && (uint8)gWingmanInterface[j] != 0 && Replay::ReplayMode < 2)
            BuildWingmanInterface(j);
        ot = DrawC::carCache.head.cprim.LastPrim;
        addPrim(ot, &gTPage0[j][1]);
        Draw::StopRenderingView(gHudView[j]);
    }
}

void Hud::RenderTacView(void)
{
    FUNCTION_MARKER(0x800D8C48, "NFS4.EXE");
    for (sint32 player = 0; player <= DashHUD::gInfo.splitscreen; ++player)
    {
        if (GameSetup::gData.carInfo[player].HudTach != 0 && DashHUD::gInfo.showhud[player] != 0)
        {
            Draw::StartRenderingView(gTacView[player]);
            DashHUD::HUDCalc(player);
            BuildTach(player);
            uint32 *ot = DrawC::carCache.head.cprim.LastPrim;
            addPrim(ot, &gTPage1[player][2]);
            Draw::StopRenderingView(gTacView[player]);
        }
    }
}

void Hud::ParseTime(sint32 nTime, char *sLapTime)
{
    FUNCTION_MARKER(0x800D8D64, "NFS4.EXE");
    if (nTime < 0)
        nTime = 0;
    sint32 scaled = (sint32)((uint32)nTime * 0x6400U) / 0x4000;
    sint32 min = (sint16)(scaled / 6000);
    sint32 remainder = scaled - min * 6000;
    sint32 sec = (sint16)(remainder / 100);
    sint32 hun = (sint16)(remainder - sec * 100);
    if (min < 60 && nTime != 0)
        sprintf(sLapTime, "%01d%c%02d%c%02d", min,
            (uint8)HudminChar[GameSetup::gData.userSetting.language], sec,
            (uint8)HudsecChar[GameSetup::gData.userSetting.language], hun);
    else
        sprintf(sLapTime, " - %c - - %c - -",
            (uint8)HudminChar[GameSetup::gData.userSetting.language],
            (uint8)HudsecChar[GameSetup::gData.userSetting.language]);
}

void Hud::RenderPauseBox(sint32 x, sint32 y, sint32 w, sint32 h)
{
    FUNCTION_MARKER(0x800D8ED0, "NFS4.EXE");
    sint16 CORNERWIDTH = (sint16)HudPmx::gShapes[17].width;
    sint16 CORNERHEIGHT = (sint16)HudPmx::gShapes[17].height;
    sint32 left = (sint32)((uint32)x - 2U);
    sint32 top = (sint32)((uint32)y + 10U);
    FBuildFT4(&HudPmx::gShapes[14], left, top, 0);
    sint32 right = (sint32)((uint32)x + (uint32)w - (uint32)CORNERWIDTH);
    FBuildFT4(&HudPmx::gShapes[15], right, top, 0);
    sint32 bottom = (sint32)((uint32)y + (uint32)h);
    sint32 cornerY = (sint32)((uint32)bottom - (uint32)CORNERHEIGHT);
    FBuildFT4(&HudPmx::gShapes[16], left, cornerY, 0);
    FBuildFT4(&HudPmx::gShapes[17], right, cornerY, 0);
    FBuildF4(0, x, (sint32)((uint32)bottom - 2U), w, 2, 0, 0, 0);
    BlackThinBox(x, y, w, h);
    FBuildF4(0, x, y, w, 12, 0, 0, 0);
    FBuildF4(1, x, (sint32)((uint32)y + 12U), w,
        (sint32)((uint32)h - 12U), 0x461414UL, 0, 0);
}

void Hud::Render(void)
{
    FUNCTION_MARKER(0x800D9058, "NFS4.EXE");
    if (Replay::ReplayInterface.statsScreen != 0)
    {
        if (simGlobal.gameTicks < 576)
        {
            DashHUD::gInfo.showhud[0] = 0;
            DashHUD::gInfo.showhud[1] = 0;
        }
        gCView.id = gStatsView;
        Draw::StartRenderingView(gStatsView);
        DebugCrap();
        RenderStatsView();
        Draw::StopRenderingView(gStatsView);
        return;
    }
    if (HudBustedOverlay == 0 && BTC::BonusTime != 0 &&
        (sint32)((uint32)ticks - (uint32)BTC::BonusTimeTick) >= 251)
    {
        sint32 countamount = 250;
        if (DashHUD::gInfo.splitscreen == 0 ||
            ((Cars::gRaceCarList[0]->carFlags & 0x200) != 0 &&
             (Cars::gRaceCarList[1]->carFlags & 0x200) != 0))
            countamount = 50;
        sint32 bonus = BTC::BonusTime;
        sint32 count = bonus < countamount ? bonus : countamount;
        sint32 remaining = (sint32)((uint32)bonus - (uint32)countamount);
        BTC::Countdown = (sint32)((uint32)BTC::Countdown + (uint32)count);
        if (remaining < 0)
            remaining = 0;
        BTC::BonusTime = remaining;
    }
    if (BeTheCop != 0 && BTC::UserHasControl == 0)
    {
        for (sint32 i = 0; i <= DashHUD::gInfo.splitscreen; ++i)
        {
            Draw::StartRenderingView(gHudView[i]);
            GoTpage(1);
            if (i == 0 && Replay::ReplayMode >= 2 && Replay::ReplayInterface.statsScreen == 0)
                BuildReplay();
            if (i == DashHUD::gInfo.splitscreen)
            {
                sint32 type = 1;
                if (simGlobal.gameTicks < 576 && (uint8)countdown == 0)
                    type = 0;
                BuildCdPlayer(type, i);
            }
            if (DashHUD::gInfo.showhud[i] != 0)
            {
                DebugInfo();
                BuildNumbers0(i);
            }
            GoTpage(0);
            Draw::StopRenderingView(gHudView[i]);
        }
        return;
    }
    if (HudBustedOverlay != 0 && Replay::ReplayMode < 2)
    {
        sint32 view = gStatsView;
        sint16 player = HudBustedOverlayPlayer;
        gCView.id = view;
        Draw::StartRenderingView(view);
        StatsTimer[player] = (sint32)((uint32)StatsTimer[player] + 1U);
        BTCStats(player, false);
        Draw::StopRenderingView(gStatsView);
    }
    for (sint32 i = 0; i <= DashHUD::gInfo.splitscreen; ++i)
    {
        sint32 wingmode = Input::WingCommandMode(i);
        if ((uint8)gWingmanInterface[i] == (uint32)wingmode && HudBustedOverlay != 0)
        {
            gWingmanInterface[i] = 0;
            InitMapFrame(i, 0);
            continue;
        }
        if (gWingmanFlashTicks[i] < ticks)
        {
            if ((uint8)gWingmanInterface[i] != (uint32)wingmode)
            {
                if (Replay::ReplayMode >= 2)
                {
                    gWingmanInterface[i] = 0;
                    InitMapFrame(i, 0);
                    continue;
                }
                if (HudBustedOverlay == 0)
                {
                    InitMapFrame(i, wingmode);
                    gWingmanInterface[i] = (char)wingmode;
                }
            }
            if (Replay::ReplayMode >= 2)
            {
                gWingmanInterface[i] = 0;
                InitMapFrame(i, 0);
            }
        }
    }
    gShowedCDPlayer = 0;
    if (simGlobal.gameTicks < 576 && (uint8)countdown != 0 && BeTheCop == 0)
        Render321Go();
    RenderHudView();
    RenderMapView();
    RenderTacView();
}

void Hud::PositionMap(void)
{
    FUNCTION_MARKER(0x800D94C0, "NFS4.EXE");
    sint32 track = GameSetup::gData.track;
    gMapScaleX = (sint32)(fMapScaleX[track] * 65536.0f);
    gMapScaleY = (sint32)(fMapScaleY[track] * 65536.0f);
    sint32 x = fMapOffX[track];
    gMapOffX = x;
    if (GameSetup::gData.mirrorTrack != 0)
        gMapOffX = x - 2;
    sint32 rotation = fMapRotate[track];
    gMapOffY = fMapOffY[track];
    gMapRotate = rotation;
    sint32 cos = ccos(gMapRotate);
    rotation = gMapRotate;
    mapMarkerMCos = (sint32)((uint32)cos << 4);
    mapMarkerMSin = (sint32)((uint32)csin(rotation) << 4);
}

void Hud::BTC_Update(char *perpname, sint32 timeleft, sint32 userHasControl)
{
    FUNCTION_MARKER(0x800D95C8, "NFS4.EXE");
    snprintf(BTC::CurrentPerpName, sizeof(BTC::CurrentPerpName), "%s", perpname ? perpname : "");
    sint32 bonusTime = BTC::BonusTime;
    BTC::UserHasControl = userHasControl;
    if (bonusTime == 0 && HudBustedOverlay == 0)
        BTC::Countdown = (sint32)((uint32)timeleft << 1);
}

void Hud::BustedOverlayOn(sint32 time, char *name, sint32 caught, sint16 player)
{
    FUNCTION_MARKER(0x800D962C, "NFS4.EXE");
    StatsTimer[player] = 0;
    if (Replay::ReplayMode >= 2)
        return;
    FinalBTC_Countdown = BTC::Countdown;
    sprintf(BTCPerpInfo[player][NextPerp[player]].name, name);
    if (caught != 0)
    {
        BTCPerpInfo[player][NextPerp[player]].caught = 1;
        BTCPerpInfo[player][NextPerp[player]].time = time;
        NextPerp[player] = (sint16)((uint16)NextPerp[player] + 1U);
    }
    else
    {
        for (sint32 i = 0; i < 2; ++i)
        {
            sint32 next = NextPerp[i];
            if (next == 0 || BTCPerpInfo[i][next - 1].caught != 0)
            {
                BTCPerpInfo[i][next].caught = 0;
                BTCPerpInfo[i][NextPerp[i]].time = 0;
                sprintf(BTCPerpInfo[i][NextPerp[i]].name, BTC::CurrentPerpName);
                NextPerp[i] = (sint16)((uint16)NextPerp[i] + 1U);
            }
        }
    }
    HudBustedOverlay = 1;
    HudBustedOverlayPlayer = player;
}

void Hud::BustedOverlayOff(void)
{
    FUNCTION_MARKER(0x800D97E4, "NFS4.EXE");
    HudBustedOverlay = 0;
}

void Hud::Reset(void)
{
    FUNCTION_MARKER(0x800D97F0, "NFS4.EXE");
    if (Replay::ReplayMode < 2)
    {
        for (sint32 i = 0; i < 2; ++i)
        {
            NextPerp[i] = 0;
            BTC::CurrentPerpName[0] = '\0';
        }
    }
    BTC::BonusTime = 0;
    HudBustedOverlay = 0;
    PerpOverlayOn[0] = 0;
    PerpOverlayOn[1] = 0;
    BTC::UserHasControl = 0;
}

void Hud::BTC_QuitOut(void)
{
    FUNCTION_MARKER(0x800D9848, "NFS4.EXE");
    if (HudBustedOverlay == 0)
    {
        for (sint32 i = 0; i < 2; ++i)
        {
            sprintf(BTCPerpInfo[i][NextPerp[i]].name, BTC::CurrentPerpName);
            BTCPerpInfo[i][NextPerp[i]].caught = 0;
            BTCPerpInfo[i][NextPerp[i]].time = 0;
            NextPerp[i] = (sint16)((uint16)NextPerp[i] + 1U);
        }
    }
}

void Hud::BTC_BonusTime(sint32 extratime)
{
    FUNCTION_MARKER(0x800D990C, "NFS4.EXE");
    sint32 currentTick = ticks;
    BTC::BonusTime = extratime;
    BTC::BonusTimeTick = currentTick;
}

void Hud::Perp_OverlayOn(sint32 player, sint32 level)
{
    FUNCTION_MARKER(0x800D9924, "NFS4.EXE");
    PerpOverlayOn[player] = 1;
    PerpOverlayMessage[player] = level;
}

void Hud::Perp_OverlayOff(sint32 player)
{
    FUNCTION_MARKER(0x800D9950, "NFS4.EXE");
    PerpOverlayOn[player] = 0;
}
