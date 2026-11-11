#ifndef NIGHT_H
#define NIGHT_H

#include "xport.h"
#include "psx.h"
#include "game/common/gmesetup.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"

struct tCompRGB
{
    char r;
    char g;
    char b;
};

struct tNightInitCache
{
    tCompRGB table[257];
};

class Night
{
public:
    typedef uint8 tLightingTable[256][16];

    typedef uint8 tWeatherLightingTable[256];

    typedef uint8 tCopLightingTable[256][8];

    static tLightingTable *gCurrentNightColor;
    static tLightingTable *gPlayerLightingTable;
    static tCopLightingTable *gCopLightingTableRed;
    static tCopLightingTable *gCopLightingTableBlue;
    static tWeatherLightingTable *gWeatherLightingTable[2];
    static tCopLightingTable *gCopColor[2];

    static char *gNightTbl;

    static CVECTOR gNightAmbientColor;
    static CVECTOR gAdditiveHeadlightColor[16];
    static CVECTOR gColor[2];

    static sint32 gXDist;
    static sint32 gZNear;
    static sint32 gZDist;
    static sint32 gXDistShift;
    static sint32 gZDistShift;
    static sint32 gLightningType;
    static sint32 gTotalLights;
    static sint32 gLightning;
    static sint32 gNextLightning;
    static sint32 gEndNextLightning;
    static sint32 gNextFlicker;
    static sint32 gFlashAzimuth;
    static char gShowForks;
    static sint32 gFlashIntensity;
    static char gDrawLightning;
    static char gCopCountryLightTbl[2][5][2];
    static sint32 gLightningPauseAreas[16][2];
    static sint32 gPlayerHeadLightColor[2];
    static sint32 gWeatherColor[2];

    static sint32 FindClosestColor(CVECTOR colorMatch, sint32 *bestIndex);
    static void CreateNightTableElement(sint32 colorIndex, sint32 colorH, sint32 bright, uint8 *colorval);
    static void CreateNightTable(sint32 colorIndex, sint32 colorH, sint32 bright, uint8 (*tbl)[256][16]);
    static void GenerateNextLightningEvent(void);
    static void PauseLightningEffect(sint32 player);
    static void DoLightningEffect(DRender::tView *Vi);
    static void SetCopColor(GameSetup::tCarData *carinfo);
    static void InitPlayerHeadLightColor(sint32 player);
    static void SetPlayerHeadLightColor(sint32 player, sint32 colorIndex, sint32 bright);
    static void InitCopLightColors(void);
    static void SetCopLightColors(sint32 colorIndex, sint32 brighten);
    static void InitWeatherTables(void);
    static void SetWeatherColors(sint32 colorIndex);
    static void GenerateAllLightTables(void);
    static void InitNightDriving(void);
    static void KillNightDriving(void);
    static void RestartNightDriving(void);
    static void SetEnviroment(DRender::tView *Vi);
    static void AdditiveNightCalc(VECTOR *v, CVECTOR *color);
    static void NightCopCalc(VECTOR *v, sint16 *idx);
    static void NightCalc(VECTOR *v, sint16 *idx, Draw::tGiveShelbyMoreCache *sd);
};

extern sint32 gNight_renderNight;

#endif
