#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/audiocmn.h"
#include "game/common/cameraobj.h"
#include "game/common/chunk.h"
#include "game/common/gmesetup.h"
#include "game/common/paths.h"
#include "game/common/renderview.h"
#include "game/common/sim.h"
#include "game/psx/hrzsku.h"
#include "game/psx/night.h"
#include "game/psx/rendercache.h"
#include "game/psx/trackspec.h"
#include "game/psx/weather.h"
#include "lib/psx/blkmov.h"
#include "lib/psx/random.h"

sint32 gNight_renderNight;
tNightInitCache *gNightInitCache;
tCompRGB *gTableCache;
char *nightfile;
char CopCarTypeLights[6] = { 0, 0, 1, 0, 1, 1 };
sint32 Night::gXDist;
sint32 Night::gZNear;
sint32 Night::gZDist;
sint32 Night::gXDistShift;
sint32 Night::gZDistShift;
sint32 Night::gLightningType;
sint32 Night::gTotalLights;
sint32 Night::gLightning;
sint32 Night::gNextLightning;
sint32 Night::gEndNextLightning;
sint32 Night::gNextFlicker;
sint32 Night::gFlashAzimuth;
char Night::gShowForks;
sint32 Night::gFlashIntensity;
char Night::gDrawLightning;
char *Night::gNightTbl;
CVECTOR Night::gNightAmbientColor;
Night::tLightingTable *Night::gCurrentNightColor;
Night::tLightingTable *Night::gPlayerLightingTable;
Night::tCopLightingTable *Night::gCopLightingTableRed;
Night::tCopLightingTable *Night::gCopLightingTableBlue;
char Night::gCopCountryLightTbl[2][5][2] = { { { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 } }, { { 1, 1 }, { 0, 0 }, { 1, 1 }, { 0, 1 }, { 0, 1 } } };
sint32 Night::gLightningPauseAreas[16][2];
sint32 Night::gPlayerHeadLightColor[2];
sint32 Night::gWeatherColor[2];
CVECTOR Night::gAdditiveHeadlightColor[16];
CVECTOR Night::gColor[2];
Night::tWeatherLightingTable *Night::gWeatherLightingTable[2];
Night::tCopLightingTable *Night::gCopColor[2];

sint32 Night::FindClosestColor(CVECTOR colorMatch, sint32 *bestIndex)
{
    FUNCTION_MARKER(0x800DB318, "NFS4.EXE");
    sint32 maxLights = (gTotalLights + 1);
    tCompRGB searchColor;
    memcpy(&searchColor, gTableCache, sizeof(searchColor));
    sint32 bestDiff = 0x2FA03;
    for (sint32 search = 1; search < maxLights; ++search)
    {
        sint32 diffR = colorMatch.r - (uint8)searchColor.r;
        sint32 diffG = colorMatch.g - (uint8)searchColor.g;
        sint32 diffB = colorMatch.b - (uint8)searchColor.b;
        memcpy(&searchColor, gTableCache + search, sizeof(searchColor));
        sint32 diffSum = diffR * diffR + diffG * diffG + diffB * diffB;
        if (diffSum < bestDiff)
        {
            bestDiff = diffSum;
            *bestIndex = search - 1;
            if (bestDiff < 64)
                return bestDiff;
        }
    }
    return bestDiff;
}

void Night::CreateNightTableElement(sint32 colorIndex, sint32 colorH, sint32 bright, uint8 *colorval)
{
    FUNCTION_MARKER(0x800DB3E0, "NFS4.EXE");
    uint32 b15 = bright * 273U;
    tCompRGB *source = gTableCache + colorIndex;
    uint32 channels = colorH;
    sint32 newR = (uint8)source->r + ((sint32)((channels & 255U) * b15) >> 12);
    sint32 newG = (uint8)source->g + ((sint32)(((channels >> 8) & 255U) * b15) >> 12);
    sint32 newB = (uint8)source->b + ((sint32)(((channels >> 16) & 255U) * b15) >> 12);
    if (newR > 255)
        newR = 255;
    if (newG > 255)
        newG = 255;
    if (newB > 255)
        newB = 255;
    CVECTOR newColor;
    newColor.r = (uint8)newR & 0xF8;
    newColor.g = (uint8)newG & 0xF8;
    newColor.b = (uint8)newB & 0xF8;

    newColor.cd = 0;
    sint32 bestIndex;
    sint32 diff = FindClosestColor(newColor, &bestIndex);
    if (diff < 513 || (Chunk::numLight + 4) <= colorIndex || gTotalLights > 255) *colorval = (uint8)bestIndex;
    else
    {
        gTableCache[gTotalLights].r = (char)newColor.r;
        gTableCache[gTotalLights].g = (char)newColor.g;
        gTableCache[gTotalLights].b = (char)newColor.b;
        *colorval = (uint8)gTotalLights;
        gTotalLights = (gTotalLights + 1);
    }
}

void Night::CreateNightTable(sint32 colorIndex, sint32 colorH, sint32 bright, uint8 (*tbl)[256][16])
{
    FUNCTION_MARKER(0x800DB5A4, "NFS4.EXE");
    CreateNightTableElement(colorIndex, colorH, bright, (*tbl)[colorIndex] + bright);
}

void Night::GenerateNextLightningEvent(void)
{
    FUNCTION_MARKER(0x800DB5CC, "NFS4.EXE");
    uint32 value = random();
    gNextLightning = (simGlobal.gameTicks + (value & 0x7FFU) + 31U);
    value = random();
    sint32 next = gNextLightning;
    gEndNextLightning = (next + (value & 15U) + 15U);
    gNextFlicker = next;
    value = random();
    gFlashAzimuth = value & 0xFFFF;
    value = random();
    gShowForks = (char)(value & 1U);
}

void Night::PauseLightningEffect(sint32 player)
{
    FUNCTION_MARKER(0x800DB640, "NFS4.EXE");
    sint32 track = GameSetup::gData.track;
    sint32 slice = Camera::gInfo[player].slicePos.slice;
    sint32 startp = gLightningPauseAreas[track][0];
    sint32 endp = gLightningPauseAreas[track][1];
    sint32 pause_flag = startp < slice && slice < endp;
    if (Weather::gType == Weather::kSnow)
        pause_flag = 1;
    if (pause_flag != 0)
    {
        sint32 currentTick = simGlobal.gameTicks;
        gNextLightning = currentTick;
        gEndNextLightning = currentTick;
    }
}

void Night::DoLightningEffect(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800DB6C4, "NFS4.EXE");
    static char lightningInit = 1;
    if (gLightning != 0)
    {
        AudioCmn::PlayThunder(gFlashIntensity, gFlashAzimuth);
        Hrz::LightningFlicker(0);
        gLightning = 0;
    }
    sint32 currentTick = simGlobal.gameTicks;
    if (gNextLightning < currentTick && currentTick < gEndNextLightning && gNextFlicker < currentTick)
    {
        uint32 value = random();
        gLightningType = value & 1;
        Hrz::LightningFlicker(1);
        gDrawLightning = BWorldSm::TunnelFlagSm(&Camera::gInfo[Vi->player].slicePos) == 0;
        gLightning = 1;
        value = random();
        gNextFlicker = (simGlobal.gameTicks + (value & 3U));
        value = random();
        gFlashIntensity = ((gLightningType + 1U) * (value & 31U) + 64U);
        if (lightningInit != 0)
        {
            if (gShowForks != 0)
                Hrz::SetLightingPosInSky(Vi);
            lightningInit = 0;
        }
    }
    currentTick = simGlobal.gameTicks;
    if (gEndNextLightning < currentTick)
    {
        GenerateNextLightningEvent();
        Hrz::CalculateLightning();
        lightningInit = 1;
    }
}

void Night::SetCopColor(GameSetup::tCarData *carinfo)
{
    FUNCTION_MARKER(0x800DB848, "NFS4.EXE");
    sint32 cartype = (carinfo->carType - 22);
    sint32 country = carinfo->Country;
    sint32 carTable = (uint8)CopCarTypeLights[cartype];
    tCopLightingTable *copColors[2] = { gCopLightingTableRed, gCopLightingTableBlue };
    sint32 col1 = (uint8)gCopCountryLightTbl[carTable][country][0];
    sint32 col2 = (uint8)gCopCountryLightTbl[carTable][country][1];
    tCopLightingTable *first = copColors[col1];
    gCopColor[0] = first;
    gCopColor[1] = copColors[col2];
}

void Night::InitPlayerHeadLightColor(sint32 player)
{
    FUNCTION_MARKER(0x800DB8DC, "NFS4.EXE");
    if (gPlayerLightingTable == 0)
        gPlayerLightingTable = static_cast<tLightingTable *>(reservememadr("plnight", 4096, 0));
    memcpy(&gPlayerHeadLightColor[player], &TrackSpec::gSpec.nightspec.nightcolor, 4);
}

void Night::SetPlayerHeadLightColor(sint32 player, sint32 colorIndex, sint32 bright)
{
    FUNCTION_MARKER(0x800DB938, "NFS4.EXE");
    CreateNightTable(colorIndex, gPlayerHeadLightColor[player], bright, gPlayerLightingTable);
}

void Night::InitCopLightColors(void)
{
    FUNCTION_MARKER(0x800DB974, "NFS4.EXE");
    if (gCopLightingTableRed == 0)
        gCopLightingTableRed = static_cast<tCopLightingTable *>(reservememadr("CopRed", 2048, 0));
    if (gCopLightingTableBlue == 0)
        gCopLightingTableBlue = static_cast<tCopLightingTable *>(reservememadr("CopBlue", 2048, 0));
}

void Night::SetCopLightColors(sint32 colorIndex, sint32 brighten)
{
    FUNCTION_MARKER(0x800DB9D0, "NFS4.EXE");
    sint32 brightnessIndex = brighten >> 1;
    CreateNightTableElement(colorIndex, 0x602020, brighten, (*gCopLightingTableBlue)[colorIndex] + brightnessIndex);
    CreateNightTableElement(colorIndex, 0x101054, brighten, (*gCopLightingTableRed)[colorIndex] + brightnessIndex);
}

void Night::InitWeatherTables(void)
{
    FUNCTION_MARKER(0x800DBA50, "NFS4.EXE");
    for (sint32 i = 0; i < 2; ++i)
    {
        if (gWeatherLightingTable[i] == 0)
            gWeatherLightingTable[i] = static_cast<tWeatherLightingTable *>(reservememadr("wtnight", 256, 0));
    }
    gWeatherColor[0] = 0x574054;
    gWeatherColor[1] = 0x6C4040;
}

void Night::SetWeatherColors(sint32 colorIndex)
{
    FUNCTION_MARKER(0x800DBAD4, "NFS4.EXE");
    for (sint32 i = 0; i < 2; ++i)
        CreateNightTableElement(colorIndex, gWeatherColor[i], 15, *gWeatherLightingTable[i] + colorIndex);
}

void Night::GenerateAllLightTables(void)
{
    FUNCTION_MARKER(0x800DBB50, "NFS4.EXE");
    static char colorCreationTable[16] = { 0, 15, 7, 5, 11, 13, 3, 9, 1, 14, 10, 4, 2, 12, 6, 8 };
    sint32 numLights = Chunk::numLight;
    gNightInitCache = &gNativeRenderCache.night;
    gTableCache = gNightInitCache->table;
    gTotalLights = numLights;
    for (sint32 i = 0; i < 256; ++i)
    {
        memcpy(&gTableCache[i].r, &Chunk::lightTable[i].r, 1);
        memcpy(&gTableCache[i].g, &Chunk::lightTable[i].g, 1);
        memcpy(&gTableCache[i].b, &Chunk::lightTable[i].b, 1);
    }
    for (sint32 colorIndex = 0; colorIndex < gTotalLights; ++colorIndex)
    {
        if (GameSetup::gData.Weather == 1)
            SetWeatherColors(colorIndex);
        for (sint32 i = 0; i < 16; ++i)
        {
            sint32 bright = (uint8)colorCreationTable[i];
            SetPlayerHeadLightColor(0, colorIndex, bright);
            if (GameSetup::gData.cops != 0 && i < 8)
                SetCopLightColors(colorIndex, bright);
        }
    }
    for (sint32 i = 0; i < 256; ++i)
    {
        memcpy(&Chunk::lightTable[i].r, &gTableCache[i].r, 1);
        memcpy(&Chunk::lightTable[i].g, &gTableCache[i].g, 1);
        memcpy(&Chunk::lightTable[i].b, &gTableCache[i].b, 1);
    }
    for (sint32 i = 0; i < 16; ++i)
    {
        gAdditiveHeadlightColor[i].r = (uint8)((gPlayerHeadLightColor[0] & 255U) * i / 15U);
        gAdditiveHeadlightColor[i].g = (uint8)((((uint32)gPlayerHeadLightColor[0] >> 8) & 255U) * i / 15U);
        gAdditiveHeadlightColor[i].b = (uint8)((((uint32)gPlayerHeadLightColor[0] >> 16) & 255U) * i / 15U);
    }
}

void Night::InitNightDriving(void)
{
    FUNCTION_MARKER(0x800DBDE4, "NFS4.EXE");
    sint32 time = GameSetup::gData.Time;
    sint32 renderNight = 0;
    if (time != 0)
        renderNight = GameSetup::gData.commMode != 1;
    gNight_renderNight = renderNight;
    if (time != 0 && renderNight == 0)
    {
        TrackSpec::gSpec.depthcuespec.distance = 255;
        uint32 black = 0;
        memcpy(&TrackSpec::gSpec.depthcuespec.color, &black, 4);
    }
    if (gNight_renderNight == 0)
        return;
    char name[256];
    sprintf(name, "%snight.psh", Paths_Paths[25]);
    sint32 size = filesize(name);
    nightfile = static_cast<char *>(reservememadr("night.psh", size, 0));
    void *mem = loadshapeadr(name, 0);
    sint32 length = filesize(name);
    blockmove(mem, nightfile, length);
    purgememadr(mem);
    gNightTbl = static_cast<char *>(locateshape(nightfile, "nght")) + 16;
    InitPlayerHeadLightColor(0);
    if (GameSetup::gData.cops != 0)
        InitCopLightColors();
    if (GameSetup::gData.Weather == 1)
        InitWeatherTables();
    GenerateAllLightTables();
    if (GameSetup::gData.Weather == 1)
    {
        gLightning = 0;
        uint32 value = random();
        gNextLightning = (simGlobal.gameTicks + (value & 0x1FFU));
        value = random();
        sint32 next = gNextLightning;
        gEndNextLightning = (next + (value & 0x31U));
        gNextFlicker = next;
        Hrz::CalculateLightning();
    }
}

void Night::KillNightDriving(void)
{
    FUNCTION_MARKER(0x800DBF80, "NFS4.EXE");
    char *file = nightfile;
    if (file != 0)
        purgememadr(file);
    tLightingTable *playerTable = gPlayerLightingTable;
    nightfile = 0;
    if (playerTable != 0)
        purgememadr(playerTable);
    tCopLightingTable *redTable = gCopLightingTableRed;
    gPlayerLightingTable = 0;
    if (redTable != 0)
        purgememadr(redTable);
    tCopLightingTable *blueTable = gCopLightingTableBlue;
    gCopLightingTableRed = 0;
    if (blueTable != 0)
        purgememadr(blueTable);
    tWeatherLightingTable *weatherTable = gWeatherLightingTable[0];
    gCopLightingTableBlue = 0;
    if (weatherTable != 0)
        purgememadr(weatherTable);
    weatherTable = gWeatherLightingTable[1];
    gWeatherLightingTable[0] = 0;
    if (weatherTable != 0)
        purgememadr(weatherTable);
    gWeatherLightingTable[1] = 0;
}

void Night::RestartNightDriving(void)
{
    FUNCTION_MARKER(0x800DC020, "NFS4.EXE");
    if (GameSetup::gData.Weather == 1 && GameSetup::gData.Time != 0)
    {
        gLightning = 0;
        uint32 value = random();
        gNextLightning = (simGlobal.gameTicks + (value & 0x1FFU));
        value = random();
        sint32 next = gNextLightning;
        gEndNextLightning = (next + (value & 0x31U));
        gNextFlicker = next;
        Hrz::LightningFlicker(0);
    }
}

void Night::SetEnviroment(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800DC0A8, "NFS4.EXE");
    if (GameSetup::gData.Time == 0)
        return;
    tLightingTable *playerTable = gPlayerLightingTable;
    sint32 weather = GameSetup::gData.Weather;
    gDrawLightning = 0;
    gCurrentNightColor = playerTable;
    if (weather == 1)
    {
        PauseLightningEffect(Vi->player);
        if (Vi->player == 0)
            DoLightningEffect(Vi);
    }
    gXDistShift = 10;
    gZDistShift = 12;
    Car::tObj *target = reinterpret_cast<Car::tObj *>(Camera::gInfo[Vi->player].target);
    gZNear = 128;
    if (((uint8)target->control.lights & 4) != 0)
    {
        gZDistShift = 13;
        gXDistShift = 11;
    }
    if (Camera::GetMode(Vi->player) == 0)
        gZNear = -512;
    sint32 xShift = gXDistShift;
    sint32 zShift = gZDistShift;
    gXDist = (1U << (xShift & 31U));
    gZDist = (1U << (zShift & 31U));
    gZDistShift = (zShift - 6);
    gXDistShift = (xShift - 5);
}

void Night::AdditiveNightCalc(VECTOR *v, CVECTOR *color)
{
    FUNCTION_MARKER(0x800DC1B8, "NFS4.EXE");
    sint32 z = v->vz;
    sint32 znear = gZNear;
    uint32 zShift = gZDistShift & 31U;
    sint32 zfar = (znear + (1U << (((uint32)gZDistShift + 6U) & 31U)));
    if (z <= znear || z >= zfar)
        return;
    uint32 xShift = gXDistShift & 31U;
    sint32 x = v->vx;
    sint32 xdist = (1U << ((gXDistShift + 5U) & 31U));
    sint32 negativeXdist = (-xdist);
    if (x <= negativeXdist || x >= xdist)
        return;
    sint32 row = (z - znear) >> zShift;
    sint32 column = (x + xdist) >> xShift;
    sint32 index = (((uint32)row << 6) + column);
    sint32 lookup = (uint8)gNightTbl[index];
    CVECTOR addColor = gAdditiveHeadlightColor[lookup];
    sint32 newR = color->r + addColor.r;
    sint32 newG = color->g + addColor.g;
    sint32 newB = color->b + addColor.b;
    if (newR > 255) newR = 255;
    if (newG > 255) newG = 255;
    if (newB > 255) newB = 255;
    color->r = (uint8)newR;
    color->g = (uint8)newG;
    color->b = (uint8)newB;
}
