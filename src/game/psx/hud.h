#ifndef HUD_H
#define HUD_H

#include "xport.h"
#include "psx.h"
#include "game/common/hudpmx.h"

struct tBTCPerpInfo
{
    char name[8];
    sint32 time;
    sint32 caught;
};

struct tSmallCoordXY;

extern sint32 HudBustedOverlay;
extern tBTCPerpInfo BTCPerpInfo[2][10];
extern sint32 StatsTimer[2];

class Hud
{
public:
    static sint16 NextPerp[2];

    static tSmallCoordXY gElementPositions[2][19];

    static CVECTOR gMarkerColor[12];
    static CVECTOR gCopMarkerColor[12];
    static sint32 gShowedCDPlayer;
    static sint32 BeTheCop;
    static sint32 kTurnSongOffNext;

    static sint32 gCdActive;
    static sint32 ActivateCDPlayer;
    static sint32 gStatsView;
    static sint32 gCdLastTick;
    static sint32 gCdScrollTitle;
    static sint32 gDebugInfo;
    static uint32 Character[5];
    static sint32 gHudView[2];
    static sint32 gMapView[2];
    static sint32 gTacView[2];
    static char gWingmanInterface[2];
    static char gWingmanFlashIcon[2];
    static sint32 gWingmanFlashTicks[2];

    static void CreateHudViews(void);
    static void GoTpage(sint32 page);
    static void DebugInfo(void);
    static void DebugCrap(void);
    static void BuildSprite(SPRT *sprt, sint32 shapeIdx, sint32 x, sint32 y, uint32 color, sint32 trans);
    static void BuildSprite2(SPRT *sprt, sint32 shapeIdx, sint32 x, sint32 y);
    static void FBuildSprite(sint32 shapeIdx, sint32 x, sint32 y, uint32 color, sint32 trans);
    static void BuildSpriteFromFont(SPRT *sprt, char ch, sint32 x, sint32 y);
    static void BuildF3(POLY_F3 *prim, HudPmx::tShape *shape, sint32 x, sint32 y, uint32 color);
    static void BuildGT4(POLY_GT4 *prim, HudPmx::tShape *shape, sint32 x, sint32 y, uint32 color);
    static void BuildFT4(POLY_FT4 *prim, HudPmx::tShape *shape, sint32 x, sint32 y, uint32 color, sint32 trans);
    static void BuildMirrorFT4(POLY_FT4 *prim, HudPmx::tShape *shape, sint32 x, sint32 y, uint32 color, sint32 trans);
    static void BuildMapMirrorFT4(POLY_FT4 *prim, HudPmx::tShape *shape, sint32 x, sint32 y, uint32 color, sint32 trans);
    static void BuildF4(POLY_F4 *prim, sint32 trans, sint32 x, sint32 y, sint32 w, sint32 h, uint32 color);
    static void BuildG4(POLY_G4 *prim, sint32 trans, sint32 x, sint32 y, sint32 w, sint32 h, uint32 col1, uint32 col2, uint32 col3, uint32 col4);
    static void BuildF4o(POLY_F4 *prim, sint32 trans, sint32 x, sint32 y, sint32 w, sint32 h, uint32 color, sint8 x0off, sint8 x1off);
    static void FBuildGT4(HudPmx::tShape *shape, sint32 x, sint32 y, uint32 col1);
    static void FBuildFT4(HudPmx::tShape *shape, sint32 x, sint32 y, uint32 col1);
    static void FBuildF4(sint32 transparent, sint32 x, sint32 y, sint32 w, sint32 h, uint32 col1, char x0off, char x1off);
    static void Kill(void);
    static void Init0(void);
    static void InitMapFrame(sint32 i, sint32 mode);
    static void BuildTimeSprites(SPRT *sprt, const char *str, sint32 x, sint32 y);
    static void Init(void);
    static void InitTables(void);
    static void BuildETimeString(SPRT *sprt, sint32 time);
    static bool BuildDistanceString(SPRT *sprt, sint32 player);
    static void BuildTimeString(SPRT *sprt, sint32 time);
    static void BuildTach(sint32 player);
    static sint32 BuildString(char *str, sint32 x, sint32 y, sint32 color, sint32 player, bool justwidth);
    static void BuildNumbers0(sint32 player);
    static void BuildNumbers(sint32 player);
    static void InitMap(void);
    static void BuildMapMarkers(sint32 player);
    static void WingmanFlash(sint32 player, sint32 index);
    static void BuildWingmanInterface(sint32 player);
    static void InitCdPlayer(void);
    static void BuildCdPlayer(sint32 type, sint32);
    static sint32 BuildRadar(sint32 player);
    static void BuildReplay(void);
    static sint32 NextPlayer(sint32 player);
    static char * NextPlayerNameOrCarOrTime(sint32 player);
    static void RenderMapView(void);
    static void BlackThinBox(sint32 x, sint32 y, sint32 w, sint32 h);
    static void Draw321Num(sint32 x, sint32 y, sint32 num, sint32 flare_intensity, sint32, sint32);
    static void Render321Go(void);
    static void RenderHudView(void);
    static void RenderTacView(void);
    static void ParseTime(sint32 nTime, char *sLapTime);
    static void RenderPauseBox(sint32 x, sint32 y, sint32 w, sint32 h);
    static void Render(void);
    static void PositionMap(void);
    static void BTC_Update(char *perpname, sint32 timeleft, sint32 userHasControl);
    static void BustedOverlayOn(sint32 time, char *name, sint32 caught, sint16 player);
    static void BustedOverlayOff(void);
    static void Reset(void);
    static void BTC_QuitOut(void);
    static void BTC_BonusTime(sint32 extratime);
    static void Perp_OverlayOn(sint32 player, sint32 level);
    static void Perp_OverlayOff(sint32 player);
    static void BTCStats(sint16 player, bool postgame);
    static void RenderStatsView(void);
};

extern sint32 FinalBTC_Countdown;

#endif
