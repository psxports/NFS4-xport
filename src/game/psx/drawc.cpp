#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "psx_gte.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/bworld.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/color.h"
#include "game/common/genericpmx.h"
#include "game/common/matrixtdef.h"
#include "game/common/paths.h"
#include "game/common/r3dcar.h"
#include "game/common/render.h"
#include "game/common/renderview.h"
#include "game/common/track.h"
#include "game/common/transformer_obj.h"
#include "game/psx/cario.h"
#include "game/psx/draw.h"
#include "game/psx/drawc.h"
#include "game/psx/fe3dmenu.h"
#include "game/psx/flare.h"
#include "game/psx/night.h"
#include "game/psx/rendercache.h"
#include "game/psx/texture.h"
#include "game/psx/trsproj.h"

struct PCOORD16
{
    sint16 x;
    sint16 y;
    sint16 z;
    sint16 p;
};

struct Draw::tVertex
{
    PCOORD16 sv;
};

NativeRenderCache gNativeRenderCache;
static uint32 hilight_colors[5] = { 0x00700A00UL, 0x00400A00UL, 0x00300300UL, 0x00180300UL, 0x00180300UL };
Draw::CarCache &DrawC::carCache = gNativeRenderCache.car;
sint32 DrawC::gEnvMapMax;
sint32 DrawC::gShadowMax;
sint32 DrawC::gMenuLights;
sint32 DrawC::gMenuLightsDirection;
sint32 DrawC::gWetRoad;
sint16 DrawC::gReflectOffset;
DrawC::tEnvMap *DrawC::gEnvMap;
DrawC::tEnvMap *DrawC::gShadow;
MATRIX DrawC::gScreenMat;
MATRIX DrawC::gMatA;
sint16 DrawC::gOverlay[51];
sint16 DrawC::SirenFlash[16] = { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1 };
sint16 DrawC::HeadLightFlash[16] = { 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 };
sint16 DrawC::TailLightFlash[16] = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 };
sint32 DrawC::gMenuColor[2];
sint16 DrawC::gEnvMapOffset[4] = { 128, 64, 0, 192 };

void ChangeTPage(uint16 *tpage, sint32 nabr)
{
    FUNCTION_MARKER(0x800BE6D0, "NFS4.EXE");
    *tpage = (uint16)((*tpage & 0xFF9FU) | ((uint32)nabr << 5));
}

void DrawC::SetEnviroment(void)
{
    FUNCTION_MARKER(0x800BE6E8, "NFS4.EXE");
}

void DrawC::ReadLightingData(void)
{
    FUNCTION_MARKER(0x800BE6F0, "NFS4.EXE");
    char name[256];
    sprintf(name, "%sTr%02d.env", Paths_Paths[6], GameSetup::gData.track);
    char *RenderingFileData = (char *)loadfileadr(name, 0x10);
    char *ScaneData = RenderingFileData;
    sint32 count = Risk_ReadNextValue(&ScaneData);
    gEnvMapMax = count;
    gEnvMap = (tEnvMap *)reservememadr("envmap", (sint32)((uint32)count * 6U), 0);
    for (sint32 i = 0; i < gEnvMapMax; ++i)
    {
        sint32 value = Risk_ReadNextValue(&ScaneData);
        gEnvMap[i].slice = (sint16)value;
        value = Risk_ReadNextValue(&ScaneData);
        gEnvMap[i].tex = (sint16)value;
        value = Risk_ReadNextValue(&ScaneData);
        gEnvMap[i].extra = (sint16)((uint32)value << 8);
        value = Risk_ReadNextValue(&ScaneData);
        tEnvMap *entry = &gEnvMap[i];
        entry->extra = (sint16)((uint32)(uint16)entry->extra + (uint32)value);
        if (entry->slice < 0)
        {
            entry->slice = 0x7FFF;
            break;
        }
    }
    count = Risk_ReadNextValue(&ScaneData);
    gShadowMax = count;
    gShadow = (tEnvMap *)reservememadr("Shadow", (sint32)((uint32)count * 6U), 0);
    for (sint32 i = 0; i < gShadowMax; ++i)
    {
        sint32 value = Risk_ReadNextValue(&ScaneData);
        gShadow[i].slice = (sint16)value;
        value = Risk_ReadNextValue(&ScaneData);
        gShadow[i].tex = (sint16)value;
        value = Risk_ReadNextValue(&ScaneData);
        gShadow[i].extra = (sint16)((uint32)value << 8);
        value = Risk_ReadNextValue(&ScaneData);
        tEnvMap *entry = &gShadow[i];
        entry->extra = (sint16)((uint32)(uint16)entry->extra + (uint32)value);
        if (entry->slice < 0)
        {
            entry->slice = 0x7FFF;
            break;
        }
    }
    purgememadr(RenderingFileData);
}

void DrawC::ReadeMapData(void)
{
    FUNCTION_MARKER(0x800BE8F8, "NFS4.EXE");

    R3DCcar::ReadeMapData();
}

void DrawC::BuildRenderingData(void)
{
    FUNCTION_MARKER(0x800BE918, "NFS4.EXE");
    ReadLightingData();
    R3DCcar::ReadeMapData();
    R3DCcar::ReadTrackShadow();
}

void DrawC::KillRenderingData(void)
{
    FUNCTION_MARKER(0x800BE948, "NFS4.EXE");
    purgememadr(gEnvMap);
    purgememadr(gShadow);
}

void DrawC::NightHeadlight(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800BE978, "NFS4.EXE");
    sint32 *light = &carObj->render.light;
    if ((Cars::gList[gCView.player]->control.lights & 6) != 0)
    {
        coorddef tmp;
        tmp.x = (sint32)((uint32)carObj->N.position.x - (uint32)Cars::gHumanRaceCarList[gCView.player]->N.position.x);
        tmp.y = (sint32)((uint32)carObj->N.position.y - (uint32)Cars::gHumanRaceCarList[gCView.player]->N.position.y);
        tmp.z = (sint32)((uint32)carObj->N.position.z - (uint32)Cars::gHumanRaceCarList[gCView.player]->N.position.z);
        coorddef tmp2;
        transform(&tmp, &gNightMat, &tmp2);
        MATRIX nightMat;
        DrawW::WorldSetUpTranslation(&tmp2, &nightMat);
        DrawW::WorldSetUpMatrix(&gNightMat, &nightMat);
        PCOORD16 zero;
        zero.z = 0;
        zero.y = 0;
        zero.x = 0;
        gte_ldv0(&zero);
        gte_rt();
        VECTOR nightV;
        gte_stlvnl(&nightV);
        Night::AdditiveNightCalc(&nightV, (CVECTOR *)light);
    }
    if (Night::gDrawLightning != 0)
    {
        uint8 *pointerBytes = (uint8 *)&light;
        const uint8 *weather = (const uint8 *)&Night::gWeatherColor[Night::gLightningType];
        sint32 newR = pointerBytes[0] + weather[0];
        sint32 newG = pointerBytes[1] + weather[1];
        sint32 newB = pointerBytes[2] + weather[2];
        if (newR > 255) newR = 255;
        if (newG > 255) newG = 255;
        if (newB > 255) newB = 255;
        pointerBytes[0] = (uint8)newR;
        pointerBytes[1] = (uint8)newG;
        pointerBytes[2] = (uint8)newB;
    }
}

void DrawC::MenuColorData(sint32 color, Car::tObj *carObj, sint32 player)
{
    FUNCTION_MARKER(0x800BEB24, "NFS4.EXE");
    if (carObj->async_handle == 0)
    {
        GameSetup::tCarData *carInfo = carObj->carInfo;
        if (carObj->render.currentCarType != carInfo->carType)
            return;
        carObj->render.upgradeFlags = (char)((uint8)carInfo->EngineMods +
            (uint8)carInfo->WeightTransfer * 2U + (uint8)carInfo->GroundEffects * 4U);
        if ((uint32)(uint16)carObj->render.currentCarType - 16U < 3U)
            carObj->render.upgradeFlags = 7;
        uint8 upgrade = (uint8)carObj->render.upgradeFlags;
        sint32 menuColor = (sint32)((uint32)color + (uint32)(upgrade & 1) * 256U + (uint32)(upgrade & 2) * 256U);
        if (gMenuColor[player] == menuColor)
            return;
        gMenuColor[player] = menuColor;
        DrawSync(0);
        sint32 carType = carObj->render.currentCarType;
        if (carType < 28)
        {
            if ((color & 8) != (carObj->render.colorIndex & 8))
            {
                char filename[10];
                char infilename[100];
                R3DCar::GetCarName(filename, carType, carObj->carInfo->Country);
                strcat(filename, color < 8 ? "l" : "d");
                strcpy(infilename, Paths_Paths[24]);
                strcat(infilename, filename);
                strcat(infilename, ".psh");
                char *shpfile = (char *)loadfileadr(infilename, 0x10);
                CarIO::UpdateCarTextureData(shpfile, carObj, player);
                purgememadr(shpfile);
                DrawSync(0);
            }
            sint32 bodyColor = color & 7;
            Texture::pal8bit *palCopy = (Texture::pal8bit *)carObj->render.palCopy;
            Texture::CarColor = bodyColor + ((uint8)carObj->render.upgradeFlags & 2) * 4;
            Texture::ProcessPaletteCopy(palCopy, 0, 1);
            palCopy = (Texture::pal8bit *)carObj->render.palCopy;
            sint32 palNum = carObj->render.palNum;
            Texture::CarColor = bodyColor + ((uint8)carObj->render.upgradeFlags & 1) * 8;
            Texture::ProcessPaletteCopy(palCopy, 1, palNum);
        }
        else
        {
            Texture::pal8bit *palCopy = (Texture::pal8bit *)carObj->render.palCopy;
            sint32 palNum = carObj->render.palNum;
            Texture::CarColor = color & 7;
            Texture::ProcessPaletteCopy(palCopy, 0, palNum);
        }
    }
    carObj->render.colorIndex = (sint16)color;
}

sint32 DrawC::PrimStart(Draw::tVertex *center, Car::tObj *carObj, sint32 lightAvg, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800BED44, "NFS4.EXE");
    sint32 carType = carObj->render.currentCarType;
    bool cop_flag = (uint32)carType - 22U < 6U;
    sint32 half = 0;
    gte_SetRotMatrix(&gScreenMat);
    gte_SetTransMatrix(&gScreenMat);
    gte_ldv0(center);
    xport_gte_execute(0x180001U);
    uint32 depth;
    gte_stsz(&depth);
    sint32 sub_otz = (sint32)depth >> 2;
    sd->sub_otz = sub_otz;
    if (R3DCar::InMenu == 0)
    {
        sd->sub_otz = sub_otz >> 1;
        if (sd->sub_otz < 0 || sd->sub_otz > (sint32)((uint32)Draw::gViewOtSize - 3U))
            return -1;
    }
    else
        sd->sub_otz = 0;
    Car::tObj *sort_carObj = (Car::tObj *)carObj->render.sort_carObj;
    carObj->render.world_otz = sd->sub_otz;
    sint32 sub_otOffset;
    if (sort_carObj != 0)
    {
        sd->sub_ot = sort_carObj->render.sub_ot;
        sd->sub_otSize = sort_carObj->render.sub_otSize;
        sd->sub_otz = (sint32)(((uint32)sd->sub_otz - (uint32)sort_carObj->render.world_otz) << 1);
        sub_otOffset = sort_carObj->render.sub_otOffset;
    }
    else
    {
        sd->sub_ot = carObj->render.sub_ot;
        sd->sub_otSize = carObj->render.sub_otSize;
        ClearOTagR((uint32 *)sd->sub_ot, carObj->render.sub_otSize);
        sd->sub_otz = 0;
        sub_otOffset = carObj->render.sub_otOffset;
    }
    sd->sub_otz = (sint32)((uint32)sd->sub_otz << 2);
    carObj->render.sub_otz = (sint32)((uint32)sd->sub_otz + (uint32)sub_otOffset - ((uint32)sub_otz << 2));
    sd->sub_otSize = (sint32)((uint32)sd->sub_otSize - 1U);
    sd->pmxStart = CarIO::carPixMap + carObj->render.textureStartIndex;
    sd->offsetU0 = (uint8)carObj->render.textureOffsetU;
    sd->offsetV0 = (uint8)carObj->render.textureOffsetV;
    sd->offsetU1 = carObj->render.licenseOffsetU[0];
    sd->offsetV1 = carObj->render.licenseOffsetV[0];
    sd->offsetU2 = carObj->render.licenseOffsetU[1];
    sd->offsetV2 = carObj->render.licenseOffsetV[1];
    for (sint32 i = 50; i >= 0; --i)
        gOverlay[i] = 0;
    if (R3DCar::InMenu == 0)
    {
        sint32 damage = carObj->N.damage[0];
        sint32 flag = damage > 0x1E0000 ? 1 : (damage > 0xA0000 ? 2 : 0);
        if (flag != 0)
        {
            gOverlay[0] = (sint16)(gOverlay[0] | flag);
            gOverlay[2] = (sint16)(gOverlay[2] | flag);
        }
        damage = carObj->N.damage[2];
        flag = damage > 0x1E0000 ? 0x100 : (damage > 0xA0000 ? 0x200 : 0);
        if (flag != 0)
        {
            gOverlay[0] = (sint16)(gOverlay[0] | flag);
            gOverlay[2] = (sint16)(gOverlay[2] | flag);
        }
        damage = carObj->N.damage[3];
        flag = damage > 0x1E0000 ? 0x400 : (damage > 0xA0000 ? 0x800 : 0);
        if (flag != 0) gOverlay[2] = (sint16)(gOverlay[2] | flag);
        damage = carObj->N.damage[4];
        if (damage > 0xA0000)
        {
            gOverlay[1] = (sint16)(gOverlay[1] | (damage > 0x1E0000 ? 0x100 : 0x200));
            gOverlay[2] = (sint16)(gOverlay[2] | (damage > 0x1E0000 ? 0x1000 : 0x2000));
        }
        damage = carObj->N.damage[6];
        if (damage > 0xA0000)
        {
            gOverlay[1] = (sint16)(gOverlay[1] | (damage > 0x1E0000 ? 1 : 2));
            gOverlay[2] = (sint16)(gOverlay[2] | (damage > 0x1E0000 ? 0x10 : 0x20));
        }
        damage = carObj->N.damage[7];
        flag = damage > 0x1E0000 ? 4 : (damage > 0xA0000 ? 8 : 0);
        if (flag != 0) gOverlay[2] = (sint16)(gOverlay[2] | flag);
        if (carObj->N.damage[8] > 0x30000)
            gOverlay[4] = (sint16)(gOverlay[4] | 0x101);
        if ((carObj->render.brakeLight & 1) != 0)
        {
            if ((gOverlay[1] & 1) == 0)
            {
                gOverlay[24] = (sint16)(gOverlay[24] | 0x81);
                gOverlay[25] = (sint16)(gOverlay[25] | 0x81);
            }
            if ((gOverlay[1] & 0x100) == 0)
            {
                gOverlay[24] = (sint16)(gOverlay[24] | 0x8100);
                gOverlay[25] = (sint16)(gOverlay[25] | 0x8100);
            }
        }
        if (carObj->control.gear == 0)
        {
            if ((gOverlay[1] & 1) == 0) gOverlay[26] = (sint16)(gOverlay[26] | 0x80);
            if ((gOverlay[1] & 0x100) == 0) gOverlay[26] = (sint16)(gOverlay[26] | 0x8000);
        }
        if ((carObj->render.headLight & 0x11) != 0)
        {
            if ((carObj->render.headLight & 0x40) != 0 || (gOverlay[0] & 1) == 0)
                gOverlay[29] = (sint16)(gOverlay[29] | 0x81);
            if ((carObj->render.headLight & 4) != 0 || (gOverlay[0] & 0x100) == 0)
                gOverlay[29] = (sint16)(gOverlay[29] | 0x8100);
        }
        if ((carObj->render.brakeLight & 2) != 0)
        {
            if ((gOverlay[1] & 1) == 0)
            {
                gOverlay[24] = (sint16)(gOverlay[24] | 0x80);
                gOverlay[25] = (sint16)(gOverlay[25] | 0x80);
            }
            if ((gOverlay[1] & 0x100) == 0)
            {
                gOverlay[24] = (sint16)(gOverlay[24] | 0x8000);
                gOverlay[25] = (sint16)(gOverlay[25] | 0x8000);
            }
        }
        if (!cop_flag)
        {
            sint32 mirror = sd->head.mirror;
            if ((carObj->render.signalLight[mirror] & 0x80) != 0) gOverlay[28] = (sint16)(gOverlay[28] | 0x40);
            if ((carObj->render.signalLight[mirror] & 8) != 0)
            {
                if ((gOverlay[0] & 1) == 0) gOverlay[27] = (sint16)(gOverlay[27] | 0x80);
                if ((gOverlay[1] & 1) == 0) gOverlay[28] = (sint16)(gOverlay[28] | 0x81);
            }
            if ((carObj->render.signalLight[mirror ^ 1] & 0x80) != 0) gOverlay[28] = (sint16)(gOverlay[28] | 0x4000);
            if ((carObj->render.signalLight[mirror ^ 1] & 8) != 0)
            {
                if ((gOverlay[0] & 0x100) == 0) gOverlay[27] = (sint16)(gOverlay[27] | 0x8000);
                if ((gOverlay[1] & 0x100) == 0) gOverlay[28] = (sint16)(gOverlay[28] | 0x8100);
            }
            uint8 signalBrakeFlare = carType < 28 ? (uint8)R3DCar::SignalBrakeFlare[carType] : (uint8)R3DCar::RecessedLight[carType - 28];
            if (signalBrakeFlare > 1)
            {
                if ((gOverlay[28] & 0x40) == 0) gOverlay[28] = (sint16)(gOverlay[28] | (gOverlay[24] & 0xFF));
                if ((gOverlay[28] & 0x4000) == 0) gOverlay[28] = (sint16)(gOverlay[28] | (gOverlay[24] & 0xFF00));
            }
            if (signalBrakeFlare == 3)
            {
                sint16 overlay = gOverlay[24];
                gOverlay[24] = gOverlay[28];
                gOverlay[28] = overlay;
            }
        }
    }
    if (cop_flag)
    {
        gOverlay[27] = (sint16)(gOverlay[27] | 2);
        gOverlay[28] = (sint16)(gOverlay[28] | 0x200);
        if (gOverlay[4] == 0)
        {
            if (SirenFlash[(uint16)carObj->render.signalLight[0] & 15] != 0)
            {
                gOverlay[27] = (sint16)(gOverlay[27] | 0x81);
                gOverlay[28] = (sint16)(gOverlay[28] | 0x81);
            }
            if (SirenFlash[(uint16)carObj->render.signalLight[1] & 15] != 0)
            {
                gOverlay[27] = (sint16)(gOverlay[27] | 0x8100);
                gOverlay[28] = (sint16)(gOverlay[28] | 0x8100);
            }
        }
        if (R3DCar::InMenu != 0 || (carObj->AIFlags & 2) != 0)
        {
            gOverlay[29] = (sint16)(gOverlay[29] & 0x7E7E);
            if ((carObj->render.brakeLight & 1) == 0)
            {
                gOverlay[24] = (sint16)(gOverlay[24] & 0x7F7F);
                gOverlay[25] = (sint16)(gOverlay[25] & 0x7F7F);
            }
            if (HeadLightFlash[(uint16)carObj->render.signalLight[0] & 15] != 0 && ((carObj->render.headLight & 0x40) != 0 || (gOverlay[0] & 1) == 0))
                gOverlay[29] = (sint16)(gOverlay[29] | 0x81);
            if (HeadLightFlash[(uint16)carObj->render.signalLight[1] & 15] != 0 && ((carObj->render.headLight & 4) != 0 || (gOverlay[0] & 0x100) == 0))
                gOverlay[29] = (sint16)(gOverlay[29] | 0x8100);
            if ((gOverlay[1] & 1) == 0)
            {
                if (TailLightFlash[(uint16)carObj->render.signalLight[0] & 15] != 0)
                {
                    gOverlay[24] = (sint16)(gOverlay[24] | 0x80);
                    gOverlay[25] = (sint16)(gOverlay[25] | 0x80);
                }
                if (HeadLightFlash[(uint16)carObj->render.signalLight[0] & 15] != 0) gOverlay[26] = (sint16)(gOverlay[26] | 0x80);
            }
            if ((gOverlay[1] & 0x100) == 0)
            {
                if (TailLightFlash[(uint16)carObj->render.signalLight[1] & 15] != 0)
                {
                    gOverlay[24] = (sint16)(gOverlay[24] | 0x8000);
                    gOverlay[25] = (sint16)(gOverlay[25] | 0x8000);
                }
                if (HeadLightFlash[(uint16)carObj->render.signalLight[1] & 15] != 0) gOverlay[26] = (sint16)(gOverlay[26] | 0x8000);
            }
        }
    }
    SVECTOR r;
    r.vx = 0;
    r.vz = 0;
    r.vy = R3DCar::yawCam;
    RotMatrix(&r, &gMatA);
    gMatA.t[2] = 0;
    gMatA.t[1] = 0;
    gMatA.t[0] = 0;
    gWetRoad = 0;
    if (R3DCar::InMenu == 0)
    {
        sint32 eSpeed = 3;
        DRAWENV *LEnv = Draw::GetDRAWENV(gCView.id, gFlip);
        sint32 quad = (sint8)carObj->N.simRoadInfo.quad;
        uint32 envMap = (uint16)gEnvMap[carObj->N.eIndexEnvMap].tex;
        uint32 extraEnvMap = (uint16)gEnvMap[carObj->N.eIndexEnvMap].extra;
        uint32 shadow = (uint16)gShadow[carObj->N.eIndexShadow].tex;
        uint32 extraShadow = (uint16)gShadow[carObj->N.eIndexShadow].extra;
        sint32 tpageShadow = 2;
        if (quad < (sint32)(extraEnvMap & 255U)) envMap = (sint8)(extraEnvMap >> 8);
        if (quad < (sint32)(extraShadow & 255U)) shadow = (sint8)(extraShadow >> 8);
        if ((sint16)envMap > 10)
        {
            envMap -= 10U;
            half = 1;
        }
        if ((sint16)shadow < 0)
        {
            eSpeed = 0;
            shadow = 0U - shadow;
        }
        if ((sint16)shadow > 10)
        {
            shadow -= 10U;
            tpageShadow = 1;
        }
        sint32 envIndex = (sint16)(envMap - 1U);
        if (envIndex < 0)
        {
            sd->ePmx0.u0 = 0;
            sd->ePmx0.v0 = 0;
            sd->ePmx0.clut = 0;
        }
        else
        {
            Draw::tPixMap map = Track::gReflectionMaps[envIndex];
            sd->ePmx0 = map;
        }
        sint32 shadowIndex = (sint16)(shadow - 1U);
        if (shadowIndex < 0)
        {
            sd->ePmx1.u0 = 0;
            sd->ePmx1.v0 = 0;
            sd->ePmx1.clut = 0;
        }
        else
        {
            Draw::tPixMap *map = &Track::gReflectionMaps[shadowIndex];
            PSX_RECT tw;
            tw.x = map->u0;
            tw.y = map->v0;
            tw.w = 128;
            tw.h = 64;
            SetDrawMode(&sd->drawModeOn, LEnv->dfe, 0, map->tpage, &tw);
            tw.h = 0;
            tw.w = 0;
            tw.y = 0;
            tw.x = 0;
            SetDrawMode(&sd->drawModeOff, LEnv->dfe, 0, LEnv->tpage, &tw);
            Draw::tPixMap copy = *map;
            sd->ePmx1 = copy;
            ChangeTPage(&sd->ePmx1.tpage, tpageShadow);
        }
        sd->eAddZ = ((carObj->N.positionXZ >> (eSpeed + 3)) & 63) + gEnvMapOffset[sd->ePmx1.v0 >> 6];
        if (GameSetup::gData.Weather != 0 && BWorldSm::TunnelFlagSm(&carObj->N.simRoadInfo) != 1 && Cars::kSkidMarkSurface[carObj->N.driveSurfaceType] == 1)
            gWetRoad = 1;
        sint32 height = (sint32)((uint32)carObj->N.dimension.y * 3U) >> 1;
        gReflectOffset = (sint16)((sint32)((uint32)height + (uint32)carObj->N.objAltitude) >> 8);
    }
    else
    {
        DRAWENV *LEnv = Draw::GetDRAWENV(gCView.id, gFlip);
        SetDrawMode(&sd->drawModeOn, LEnv->dfe, 1, LEnv->tpage, 0);
        SetDrawMode(&sd->drawModeOff, LEnv->dfe, 0, LEnv->tpage, 0);
        Draw::tPixMap map = *gMenuPixmap[0];
        sd->ePmx0 = map;
    }
    ChangeTPage(&sd->ePmx0.tpage, 1);
    sint32 eColor;
    if (R3DCar::InMenu == 0)
    {
        eColor = (sint32)((uint32)(lightAvg >> 2) * R3DCar::eMapColour.r) >> 7;
        if (half != 0)
            eColor = (sint32)((uint32)eColor << 1) / 3;
        sint32 eColor2 = (sint32)((uint32)eColor * 3U) >> 2;
        sd->eColor2 = (sint32)((uint32)eColor2 * 0x10101U);
    }
    else
        eColor = lightAvg >> 1;
    sd->eColor0 = (sint32)((uint32)eColor * 0x10101U);
    if (carObj->render.currentCarType == 1)
        eColor >>= 2;
    else
        eColor = (sint32)((uint32)eColor * 3U) >> 2;
    sd->eColor1 = (sint32)((uint32)eColor * 0x10101U);
    return carObj->render.world_otz;
}

void DrawC::PrimStop(Car::tObj *carObj, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800BFC84, "NFS4.EXE");
    if (carObj->render.sort_flag != 0)
        return;
    Car::tObj *sort_carObj = (Car::tObj *)carObj->render.sort_carObj;
    sint32 worldZ;
    sint32 sub_otSize;
    if (sort_carObj != 0)
    {
        if (sort_carObj->render.sort_flag != 0)
        {
            sort_carObj->render.sort_flag = (sint16)((uint16)sort_carObj->render.sort_flag - 1U);
            if (sort_carObj->render.sort_flag != 0)
                return;
        }
        sub_otSize = sort_carObj->render.sub_otSize;
        worldZ = sort_carObj->render.world_otz;
    }
    else
    {
        sub_otSize = carObj->render.sub_otSize;
        worldZ = carObj->render.world_otz;
    }
    sint32 tailIndex = (sint32)((uint32)sub_otSize - 1U);
    uint32 *ot = sd->head.cprim.LastPrim + worldZ;
    setaddr(sd->sub_ot, getaddr(ot));
    ot = sd->head.cprim.LastPrim + worldZ;
    setaddr(ot, sd->sub_ot + tailIndex);
}

void DrawC::Prim(matrixtdef *m, coorddef *t, Transformer::zObj *obj, Transformer::zOverlay *overlay, sint32 envmap, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800BFD44, "NFS4.EXE");
    uint32 environment0;
    memcpy(&environment0, &sd->ePmx0, sizeof(environment0));
    COORD16 *Nvertice = obj->Nvertex;
    if (environment0 == 0)
    {
        uint32 environment1;
        memcpy(&environment1, &sd->ePmx1, sizeof(environment1));
        if (environment1 == 0)
            envmap &= 0xBE;
    }
    if ((envmap & 1) != 0)
    {
        gte_SetRotMatrix(&gMatA);
        gte_SetTransMatrix(&gMatA);
        sint32 count = obj->numVertex;
        for (sint32 i = 0; i < count; ++i)
        {
            sd->vt0 = Nvertice[i];
            gte_ldv0(&sd->vt0);
            gte_rt();
            gte_stlvnl(&sd->tv);
            uint32 v = sd->tv.vz;
            if (sd->tv.vz < 0)
                v = 0U - v;
            sd->tV[i].u = (uint8)sd->tv.vx;
            sd->tV[i].v = (uint8)v;
        }
    }
    TrsProj::SetTransPrecision(8);
    for (sint32 row = 0; row < 3; ++row)
    {
        sint32 r0 = m->m[row];
        sint32 r1 = m->m[row + 3];
        sint32 r2 = m->m[row + 6];
        sd->matB.m[row][0] = (sint16)(r0 >> 4);
        sd->matB.m[row][1] = (sint16)(r1 >> 4);
        sd->matB.m[row][2] = (sint16)(r2 >> 4);
    }
    for (sint32 column = 0; column < 3; ++column)
        sd->matB.m[1][column] = (sint16)-sd->matB.m[1][column];
    sint32 shift = TrsProj::precision & 31;
    sd->matB.t[0] = t->x >> shift;
    sd->matB.t[1] = (sint32)(0U - (uint32)(t->y >> shift));
    sd->matB.t[2] = t->z >> shift;
    TrsProj::ResetTransPrecision();
    gte_SetRotMatrix(&sd->matB);
    gte_SetTransMatrix(&sd->matB);
    COORD16 *vertice = obj->vertex;
    sint32 count = obj->numVertex;
    for (sint32 i = 0; i < count; ++i)
        sd->tV[i].vt = vertice[i];
    sint32 renderMode = envmap & 9;
    for (sint32 i = (sint32)obj->numFacet - 1; i >= 0; --i)
    {
        Transformer::zFacet *facet = obj->facet + i;
        sint32 id0 = facet->vertexId0;
        sint32 id1 = facet->vertexId1;
        sint32 id2 = facet->vertexId2;
        if (sd->head.cprim.PrimPtr >= sd->head.cprim.MPrimPtr)
            continue;
        gte_ldv3(&sd->tV[id0].vt, &sd->tV[id1].vt, &sd->tV[id2].vt);
        xport_gte_execute(0x280030);
        gte_nclip();
        gte_swc2(24, &sd->bfct);
        sint32 bfct = sd->bfct;
        if (sd->head.mirror != 0)
            bfct = (sint32)(0U - (uint32)bfct);
        if (bfct <= 0)
            continue;
        if ((renderMode & 1) != 0)
            gte_stsxy3(&sd->dvx0, &sd->dvx1, &sd->dvx2);
        gte_avsz3();
        gte_stotz(&sd->otz);
        sint32 overlayFlag = 0;
        uint16 facetFlag = 0;
        if ((renderMode & 8) != 0)
        {
            sint32 overlayWord = gOverlay[facet->textureIndex];
            if (overlayWord != 0)
            {
                sint16 facet_flag = facet->flag;
                overlayFlag = facet_flag < 0 ? overlayWord >> 8 : overlayWord & 0xFF;
                sint32 sd_otz = (sint32)((uint32)sd->otz + (uint32)sd->sub_otz);
                if (renderMode == 8 && (overlayFlag & 0x40) != 0)
                    sd_otz = (sint32)((uint32)sd_otz - 2U);
                if (sd_otz < 0 || sd_otz > sd->sub_otSize)
                    continue;
                facetFlag = (uint16)facet_flag & 0xFFF;
                if ((facet_flag & 0x3F0) != 0)
                {
                    overlayFlag &= facetFlag >> 4;
                    if (overlayFlag != 0)
                        while ((overlayFlag & 3) == 0)
                            overlayFlag >>= 2;
                }
                sd->otz = sd_otz;
            }
            else
            {
                sd->otz = (sint32)((uint32)sd->otz + (uint32)sd->sub_otz);
                if (sd->otz < 0 || sd->otz > sd->sub_otSize)
                    continue;
                facetFlag = (uint16)facet->flag & 0xFFF;
            }
        }
        else
        {
            sd->otz = (sint32)((uint32)sd->otz + (uint32)sd->sub_otz);
            if (sd->otz < 0 || sd->otz > sd->sub_otSize)
                continue;
        }
        if ((renderMode & 1) != 0)
        {
            uint16 reflectionFlag = renderMode == 1 ? (uint16)facet->flag : facetFlag;
            uint32 reflectionTexture = 0;
            if ((reflectionFlag & 0x3F3) != 0)
                memcpy(&reflectionTexture, &sd->ePmx1, sizeof(reflectionTexture));
            if (reflectionTexture != 0)
            {
                sd->vt0 = Nvertice[facet->vertexId0];
                sd->vt1 = Nvertice[facet->vertexId1];
                sd->vt2 = Nvertice[facet->vertexId2];
                DR_MODE *mode = (DR_MODE *)sd->head.cprim.PrimPtr;
                uint32 *ot = sd->sub_ot + sd->otz;
                sd->head.cprim.PrimPtr = (char *)(mode + 1);
                DR_MODE modeCopy = sd->drawModeOff;
                *mode = modeCopy;
                setaddr(mode, getaddr(ot));
                setaddr(ot, mode);
                POLY_FT3 *prim = (POLY_FT3 *)sd->head.cprim.PrimPtr;
                ot = sd->sub_ot + sd->otz;
                sd->head.cprim.PrimPtr = (char *)(prim + 1);
                prim->tag = 0x07000000 | getaddr(ot);
                setaddr(ot, prim);
                mode = (DR_MODE *)sd->head.cprim.PrimPtr;
                ot = sd->sub_ot + sd->otz;
                sd->head.cprim.PrimPtr = (char *)(mode + 1);
                modeCopy = sd->drawModeOn;
                *mode = modeCopy;
                setaddr(mode, getaddr(ot));
                setaddr(ot, mode);
                uint32 xy0, xy1, xy2;
                memcpy(&xy0, &sd->dvx0, sizeof(xy0));
                memcpy(&xy1, &sd->dvx1, sizeof(xy1));
                memcpy(&xy2, &sd->dvx2, sizeof(xy2));
                prim->p[0].xy.xy = xy0;
                prim->p[1].xy.xy = xy1;
                prim->p[2].xy.xy = xy2;
                prim->rgbc.rgbc = sd->eColor0;
                setcode(prim, 0x26);
                uint16 clut = sd->ePmx1.clut;
                uint16 tpage = sd->ePmx1.tpage;
                prim->p[0].attr = clut;
                prim->p[1].attr = tpage;
                sint32 u = sd->ePmx1.u0 + 64;
                sint32 v = sd->ePmx1.v0 + (uint8)sd->eAddZ;
                prim->p[0].uv.u = (uint8)(sd->vt0.y + u);
                prim->p[0].uv.v = (uint8)(sd->vt0.z + v);
                prim->p[1].uv.u = (uint8)(sd->vt1.y + u);
                prim->p[1].uv.v = (uint8)(sd->vt1.z + v);
                prim->p[2].uv.u = (uint8)(sd->vt2.y + u);
                prim->p[2].uv.v = (uint8)(sd->vt2.z + v);
            }
            uint32 environmentTexture;
            memcpy(&environmentTexture, &sd->ePmx0, sizeof(environmentTexture));
            if (environmentTexture != 0)
            {
                POLY_FT3 *prim = (POLY_FT3 *)sd->head.cprim.PrimPtr;
                uint32 *ot = sd->sub_ot + sd->otz;
                sd->head.cprim.PrimPtr = (char *)(prim + 1);
                prim->tag = 0x07000000 | getaddr(ot);
                setaddr(ot, prim);
                uint32 xy0, xy1, xy2;
                memcpy(&xy0, &sd->dvx0, sizeof(xy0));
                memcpy(&xy1, &sd->dvx1, sizeof(xy1));
                memcpy(&xy2, &sd->dvx2, sizeof(xy2));
                prim->p[0].xy.xy = xy0;
                prim->p[1].xy.xy = xy1;
                prim->p[2].xy.xy = xy2;
                if (renderMode == 1)
                    prim->rgbc.rgbc = sd->eColor0;
                else if ((overlayFlag & 1) != 0)
                    prim->rgbc.rgbc = sd->eColor2;
                else
                    prim->rgbc.rgbc = (facetFlag & 4) != 0 ? sd->eColor1 : sd->eColor0;
                setcode(prim, 0x26);
                uint16 clut = sd->ePmx0.clut;
                uint16 tpage = sd->ePmx0.tpage;
                prim->p[0].attr = clut;
                prim->p[1].attr = tpage;
                sint32 u = sd->ePmx0.u0 + 64;
                sint32 v = sd->ePmx0.v0;
                prim->p[0].uv.u = (uint8)(sd->tV[id0].u + u);
                prim->p[0].uv.v = (uint8)(sd->tV[id0].v + v);
                prim->p[1].uv.u = (uint8)(sd->tV[id1].u + u);
                prim->p[1].uv.v = (uint8)(sd->tV[id1].v + v);
                prim->p[2].uv.u = (uint8)(sd->tV[id2].u + u);
                prim->p[2].uv.v = (uint8)(sd->tV[id2].v + v);
            }
        }
        sint32 which = overlayFlag & 3;
        Transformer::zOverlay *facetOverlay = which != 0 ? overlay + facet->textureIndex * 3 + which - 1 : 0;
        POLY_FT3 *prim = (POLY_FT3 *)sd->head.cprim.PrimPtr;
        uint32 *ot = sd->sub_ot + sd->otz;
        sd->head.cprim.PrimPtr = (char *)(prim + 1);
        prim->tag = 0x07000000 | getaddr(ot);
        setaddr(ot, prim);
        if ((renderMode & 1) != 0)
        {
            uint32 xy0, xy1, xy2;
            memcpy(&xy0, &sd->dvx0, sizeof(xy0));
            memcpy(&xy1, &sd->dvx1, sizeof(xy1));
            memcpy(&xy2, &sd->dvx2, sizeof(xy2));
            prim->p[0].xy.xy = xy0;
            prim->p[1].xy.xy = xy1;
            prim->p[2].xy.xy = xy2;
        }
        else
            gte_stsxy3(&prim->p[0].xy, &prim->p[1].xy, &prim->p[2].xy);
        uint32 color = renderMode == 8 && (overlayFlag & 0x80) != 0 ? 0x808080 : sd->color;
        uint8 code = 0x24;
        if (renderMode == 0 && (envmap & 2) != 0)
            code = (facet->flag & 1) != 0 ? 0x26 : 0x24;
        else if ((renderMode & 8) != 0)
            code = (facetFlag & 1) != 0 ? 0x26 : 0x24;
        prim->rgbc.rgbc = color;
        setcode(prim, code);
        if (which != 0)
        {
            sint32 index = facet->textureIndex + facetOverlay->offset;
            sint32 u = facetOverlay->u;
            sint32 v = facetOverlay->v;
            Draw::tPixMap *pmx = sd->pmxStart + index;
            uint16 clut = pmx->clut;
            uint16 tpage = pmx->tpage;
            prim->p[0].attr = clut;
            prim->p[1].attr = tpage;
            prim->p[0].uv.u = (uint8)(facet->uv0.u + u);
            prim->p[1].uv.u = (uint8)(facet->uv1.u + u);
            prim->p[2].uv.u = (uint8)(facet->uv2.u + u);
            prim->p[0].uv.v = (uint8)(facet->uv0.v + v);
            prim->p[1].uv.v = (uint8)(facet->uv1.v + v);
            prim->p[2].uv.v = (uint8)(facet->uv2.v + v);
        }
        else
        {
            Draw::tPixMap *pmx = sd->pmxStart + facet->textureIndex;
            sint32 offsetU = 0;
            sint32 offsetV = 0;
            bool offsetUV = false;
            if (renderMode == 0)
            {
                if ((envmap & 2) != 0)
                {
                    offsetUV = (facet->flag & 1) == 0;
                    if (offsetUV)
                    {
                        offsetU = sd->offsetU0;
                        offsetV = sd->offsetV0;
                    }
                }
                else if ((pmx->flag & 0x7F) != 0)
                {
                    offsetUV = true;
                    offsetU = sd->offsetU1;
                    offsetV = sd->offsetV1;
                    if ((pmx->flag & 2) != 0)
                    {
                        offsetU = sd->offsetU2;
                        offsetV = sd->offsetV2;
                    }
                }
            }
            uint16 clut = pmx->clut;
            uint16 tpage = pmx->tpage;
            prim->p[0].attr = clut;
            prim->p[1].attr = tpage;
            if (offsetUV)
            {
                prim->p[0].uv.u = (uint8)(facet->uv0.u + offsetU);
                prim->p[1].uv.u = (uint8)(facet->uv1.u + offsetU);
                prim->p[2].uv.u = (uint8)(facet->uv2.u + offsetU);
                prim->p[0].uv.v = (uint8)(facet->uv0.v + offsetV);
                prim->p[1].uv.v = (uint8)(facet->uv1.v + offsetV);
                prim->p[2].uv.v = (uint8)(facet->uv2.v + offsetV);
            }
            else
            {
                uint16 uv0, uv1, uv2;
                memcpy(&uv0, &facet->uv0, sizeof(uv0));
                memcpy(&uv1, &facet->uv1, sizeof(uv1));
                memcpy(&uv2, &facet->uv2, sizeof(uv2));
                prim->p[0].uv.uv = uv0;
                prim->p[1].uv.uv = uv1;
                prim->p[2].uv.uv = uv2;
            }
        }
    }
}

void DrawC::DividePrim(COORD16 *vt0, COORD16 *vt1, COORD16 *vt2, uint16 *u0, uint16 *u1, uint16 *u2, Draw::tPixMap *pmx, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800C12F8, "NFS4.EXE");
    if (sd->head.cprim.PrimPtr >= sd->head.cprim.MPrimPtr)
        return;
    gte_ldv3(vt0, vt1, vt2);
    xport_gte_execute(0x280030);
    gte_nclip();
    gte_swc2(24, &sd->bfct);
    sint32 bfct = sd->bfct;
    if (sd->head.mirror != 0)
        bfct = (sint32)(0U - (uint32)bfct);
    if (bfct <= 0)
        return;
    gte_stsxy3(&sd->dvx0, &sd->dvx1, &sd->dvx2);
    sint32 clipW = sd->head.clipW;
    sint32 clipH = sd->head.clipH;
    if ((sd->dvx0 > clipW && sd->dvx1 > clipW && sd->dvx2 > clipW) ||
        (sd->dvx0 < 0 && sd->dvx1 < 0 && sd->dvx2 < 0) ||
        (sd->dvy0 > clipH && sd->dvy1 > clipH && sd->dvy2 > clipH) ||
        (sd->dvy0 < 0 && sd->dvy1 < 0 && sd->dvy2 < 0) ||
        (vt0->z < 0 && vt1->z < 0 && vt2->z < 0))
        return;
    gte_avsz3();
    gte_stotz(&sd->otz);
    sd->otz = (sint32)((uint32)sd->otz + (uint32)sd->sub_otz);
    if (sd->otz < 0 || sd->otz > sd->sub_otSize)
        return;
    POLY_FT3 *prim = (POLY_FT3 *)sd->head.cprim.PrimPtr;
    uint32 *ot = sd->sub_ot + sd->otz;
    sd->head.cprim.PrimPtr = (char *)(prim + 1);
    prim->tag = 0x07000000 | getaddr(ot);
    setaddr(ot, prim);
    uint32 xy0, xy1, xy2;
    memcpy(&xy0, &sd->dvx0, sizeof(xy0));
    memcpy(&xy1, &sd->dvx1, sizeof(xy1));
    memcpy(&xy2, &sd->dvx2, sizeof(xy2));
    prim->p[0].xy.xy = xy0;
    prim->p[1].xy.xy = xy1;
    prim->p[2].xy.xy = xy2;
    prim->rgbc.rgbc = sd->color;
    setcode(prim, 0x24);
    uint16 clut = pmx->clut;
    uint16 tpage = pmx->tpage;
    prim->p[0].attr = clut;
    prim->p[1].attr = tpage;
    uint16 uv0, uv1, uv2;
    memcpy(&uv1, u1, sizeof(uv1));
    memcpy(&uv0, u0, sizeof(uv0));
    memcpy(&uv2, u2, sizeof(uv2));
    prim->p[0].uv.uv = uv0;
    prim->p[1].uv.uv = uv1;
    prim->p[2].uv.uv = uv2;
}

void DrawC::PrimClip(matrixtdef *m, coorddef *t, Transformer::zObj *obj, Transformer::zOverlay *overlay, sint32 envmap, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800C155C, "NFS4.EXE");
    uint32 environment0;
    memcpy(&environment0, &sd->ePmx0, sizeof(environment0));
    COORD16 *Nvertice = obj->Nvertex;
    if (environment0 == 0)
    {
        uint32 environment1;
        memcpy(&environment1, &sd->ePmx1, sizeof(environment1));
        if (environment1 == 0)
            envmap &= 0xBE;
    }
    if ((envmap & 1) != 0)
    {
        gte_SetRotMatrix(&gMatA);
        gte_SetTransMatrix(&gMatA);
        sint32 count = obj->numVertex;
        for (sint32 i = 0; i < count; ++i)
        {
            sd->vt0 = Nvertice[i];
            gte_ldv0(&sd->vt0);
            gte_rt();
            gte_stlvnl(&sd->tv);
            uint32 v = sd->tv.vz;
            if (sd->tv.vz < 0)
                v = 0U - v;
            sd->tV[i].u = (uint8)sd->tv.vx;
            sd->tV[i].v = (uint8)v;
        }
    }
    TrsProj::SetTransPrecision(8);
    for (sint32 row = 0; row < 3; ++row)
    {
        sint32 r0 = m->m[row];
        sint32 r1 = m->m[row + 3];
        sint32 r2 = m->m[row + 6];
        sd->matB.m[row][0] = (sint16)(r0 >> 4);
        sd->matB.m[row][1] = (sint16)(r1 >> 4);
        sd->matB.m[row][2] = (sint16)(r2 >> 4);
    }
    sint32 shift = TrsProj::precision & 31;
    sd->matB.t[0] = t->x >> shift;
    sd->matB.t[1] = t->y >> shift;
    sd->matB.t[2] = t->z >> shift;
    TrsProj::ResetTransPrecision();
    gte_SetRotMatrix(&sd->matB);
    gte_SetTransMatrix(&sd->matB);
    COORD16 *vertice = obj->vertex;
    sint32 count = obj->numVertex;
    for (sint32 i = 0; i < count; ++i)
    {
        sd->tV[i].vt = vertice[i];
        gte_ldv0(&sd->tV[i].vt);
        gte_rt();
        gte_stlvnl(&sd->tv);
        sint32 x = sd->tv.vx;
        sint32 y = sd->tv.vy;
        sint32 z = sd->tv.vz;
        sd->tV[i].vt.x = (sint16)x;
        sd->tV[i].vt.y = (sint16)y;
        sd->tV[i].vt.z = (sint16)z;
    }
    gte_SetRotMatrix(&gScreenMat);
    gte_SetTransMatrix(&gScreenMat);
    sint32 renderMode = envmap & 9;
    if (renderMode == 0 && (envmap & 0x20) != 0)
    {
        for (sint32 i = (sint32)obj->numFacet - 1; i >= 0; --i)
        {
            Transformer::zFacet *facet = obj->facet + i;
            sint32 id0 = facet->vertexId0;
            sint32 id1 = facet->vertexId1;
            sint32 id2 = facet->vertexId2;
            sint32 textureIndex = facet->textureIndex;
            sd->vt0 = sd->tV[id0].vt;
            sd->vt1 = sd->tV[id1].vt;
            sd->vt2 = sd->tV[id2].vt;
            sd->vt3.x = (sint16)((sd->vt0.x + sd->vt1.x + 1) >> 1);
            sd->vt3.y = (sint16)((sd->vt0.y + sd->vt1.y + 1) >> 1);
            sd->vt3.z = (sint16)((sd->vt0.z + sd->vt1.z + 1) >> 1);
            Draw::tPixMap *pmxStart = sd->pmxStart;
            sd->vt4.x = (sint16)((sd->vt1.x + sd->vt2.x + 1) >> 1);
            sd->vt4.y = (sint16)((sd->vt1.y + sd->vt2.y + 1) >> 1);
            sd->vt4.z = (sint16)((sd->vt1.z + sd->vt2.z + 1) >> 1);
            sd->vt5.x = (sint16)((sd->vt2.x + sd->vt0.x + 1) >> 1);
            sd->vt5.y = (sint16)((sd->vt2.y + sd->vt0.y + 1) >> 1);
            sd->vt5.z = (sint16)((sd->vt2.z + sd->vt0.z + 1) >> 1);
            uint16 uv0, uv1, uv2;
            memcpy(&uv0, &facet->uv0, sizeof(uv0));
            memcpy(&uv1, &facet->uv1, sizeof(uv1));
            memcpy(&uv2, &facet->uv2, sizeof(uv2));
            memcpy(&sd->u0, &uv0, sizeof(uv0));
            memcpy(&sd->u1, &uv1, sizeof(uv1));
            memcpy(&sd->u2, &uv2, sizeof(uv2));
            Draw::tPixMap *pmx = pmxStart + textureIndex;
            if ((envmap & 2) != 0)
            {
                sint32 u = sd->offsetU0;
                sint32 v = sd->offsetV0;
                sd->u0 = (uint8)(sd->u0 + u);
                sd->v0 = (uint8)(sd->v0 + v);
                sd->u1 = (uint8)(sd->u1 + u);
                sd->v1 = (uint8)(sd->v1 + v);
                sd->u2 = (uint8)(sd->u2 + u);
                sd->v2 = (uint8)(sd->v2 + v);
            }
            sd->u3 = (uint8)((sd->u0 + sd->u1 + 1) >> 1);
            sd->v3 = (uint8)((sd->v0 + sd->v1 + 1) >> 1);
            sd->u4 = (uint8)((sd->u1 + sd->u2 + 1) >> 1);
            sd->v4 = (uint8)((sd->v1 + sd->v2 + 1) >> 1);
            sd->u5 = (uint8)((sd->u2 + sd->u0 + 1) >> 1);
            sd->v5 = (uint8)((sd->v2 + sd->v0 + 1) >> 1);
            DividePrim(&sd->vt0, &sd->vt3, &sd->vt5, (uint16 *)&sd->u0, (uint16 *)&sd->u3, (uint16 *)&sd->u5, pmx, sd);
            DividePrim(&sd->vt3, &sd->vt1, &sd->vt4, (uint16 *)&sd->u3, (uint16 *)&sd->u1, (uint16 *)&sd->u4, pmx, sd);
            DividePrim(&sd->vt5, &sd->vt4, &sd->vt2, (uint16 *)&sd->u5, (uint16 *)&sd->u4, (uint16 *)&sd->u2, pmx, sd);
            DividePrim(&sd->vt5, &sd->vt3, &sd->vt4, (uint16 *)&sd->u5, (uint16 *)&sd->u3, (uint16 *)&sd->u4, pmx, sd);
        }
        return;
    }
    for (sint32 i = (sint32)obj->numFacet - 1; i >= 0; --i)
    {
        Transformer::zFacet *facet = obj->facet + i;
        sint32 id0 = facet->vertexId0;
        sint32 id1 = facet->vertexId1;
        sint32 id2 = facet->vertexId2;
        if (sd->head.cprim.PrimPtr >= sd->head.cprim.MPrimPtr)
            continue;
        gte_ldv3(&sd->tV[id0].vt, &sd->tV[id1].vt, &sd->tV[id2].vt);
        xport_gte_execute(0x280030);
        gte_nclip();
        gte_swc2(24, &sd->bfct);
        sint32 bfct = sd->bfct;
        if (sd->head.mirror != 0)
            bfct = (sint32)(0U - (uint32)bfct);
        if (bfct <= 0)
            continue;
        gte_stsxy3(&sd->dvx0, &sd->dvx1, &sd->dvx2);
        sint16 z0 = sd->tV[id0].vt.z;
        sint16 z1 = sd->tV[id1].vt.z;
        sint16 z2 = sd->tV[id2].vt.z;
        sd->vt0.z = z0;
        sd->vt1.z = z1;
        sd->vt2.z = z2;
        sint32 clipW = sd->head.clipW;
        sint32 clipH = sd->head.clipH;
        if ((sd->dvx0 > clipW && sd->dvx1 > clipW && sd->dvx2 > clipW) ||
            (sd->dvx0 < 0 && sd->dvx1 < 0 && sd->dvx2 < 0) ||
            (sd->dvy0 > clipH && sd->dvy1 > clipH && sd->dvy2 > clipH) ||
            (sd->dvy0 < 0 && sd->dvy1 < 0 && sd->dvy2 < 0) ||
            (sd->vt0.z < 0 && sd->vt1.z < 0 && sd->vt2.z < 0))
            continue;
        gte_avsz3();
        gte_stotz(&sd->otz);
        sint32 overlayFlag = 0;
        uint16 facetFlag = 0;
        if ((renderMode & 8) != 0)
        {
            sint32 overlayWord = gOverlay[facet->textureIndex];
            if (overlayWord != 0)
            {
                sint16 facet_flag = facet->flag;
                overlayFlag = facet_flag < 0 ? overlayWord >> 8 : overlayWord & 0xFF;
                sint32 sd_otz = (sint32)((uint32)sd->otz + (uint32)sd->sub_otz);
                if (renderMode == 8 && (overlayFlag & 0x40) != 0)
                    sd_otz = (sint32)((uint32)sd_otz - 2U);
                if (sd_otz < 0 || sd_otz > sd->sub_otSize)
                    continue;
                facetFlag = (uint16)facet_flag & 0xFFF;
                if ((facet_flag & 0x3F0) != 0)
                {
                    overlayFlag &= facetFlag >> 4;
                    if (overlayFlag != 0)
                        while ((overlayFlag & 3) == 0)
                            overlayFlag >>= 2;
                }
                sd->otz = sd_otz;
            }
            else
            {
                sd->otz = (sint32)((uint32)sd->otz + (uint32)sd->sub_otz);
                if (sd->otz < 0 || sd->otz > sd->sub_otSize)
                    continue;
                facetFlag = (uint16)facet->flag & 0xFFF;
            }
        }
        else
        {
            sd->otz = (sint32)((uint32)sd->otz + (uint32)sd->sub_otz);
            if (sd->otz < 0 || sd->otz > sd->sub_otSize)
                continue;
        }
        if (renderMode == 9 && (envmap & 0x20) != 0 && (facetFlag & 1) != 0)
            continue;
        if ((renderMode & 1) != 0)
        {
            uint16 reflectionFlag = renderMode == 1 ? (uint16)facet->flag : facetFlag;
            uint32 reflectionTexture = 0;
            if ((reflectionFlag & 0x3F3) != 0)
                memcpy(&reflectionTexture, &sd->ePmx1, sizeof(reflectionTexture));
            if (reflectionTexture != 0)
            {
                sd->vt0 = Nvertice[facet->vertexId0];
                sd->vt1 = Nvertice[facet->vertexId1];
                sd->vt2 = Nvertice[facet->vertexId2];
                DR_MODE *mode = (DR_MODE *)sd->head.cprim.PrimPtr;
                uint32 *ot = sd->sub_ot + sd->otz;
                sd->head.cprim.PrimPtr = (char *)(mode + 1);
                DR_MODE modeCopy = sd->drawModeOff;
                *mode = modeCopy;
                setaddr(mode, getaddr(ot));
                setaddr(ot, mode);
                POLY_FT3 *prim = (POLY_FT3 *)sd->head.cprim.PrimPtr;
                ot = sd->sub_ot + sd->otz;
                sd->head.cprim.PrimPtr = (char *)(prim + 1);
                prim->tag = 0x07000000 | getaddr(ot);
                setaddr(ot, prim);
                mode = (DR_MODE *)sd->head.cprim.PrimPtr;
                ot = sd->sub_ot + sd->otz;
                sd->head.cprim.PrimPtr = (char *)(mode + 1);
                modeCopy = sd->drawModeOn;
                *mode = modeCopy;
                setaddr(mode, getaddr(ot));
                setaddr(ot, mode);
                uint32 xy0, xy1, xy2;
                memcpy(&xy0, &sd->dvx0, sizeof(xy0));
                memcpy(&xy1, &sd->dvx1, sizeof(xy1));
                memcpy(&xy2, &sd->dvx2, sizeof(xy2));
                prim->p[0].xy.xy = xy0;
                prim->p[1].xy.xy = xy1;
                prim->p[2].xy.xy = xy2;
                prim->rgbc.rgbc = sd->eColor0;
                setcode(prim, 0x26);
                uint16 clut = sd->ePmx1.clut;
                uint16 tpage = sd->ePmx1.tpage;
                prim->p[0].attr = clut;
                prim->p[1].attr = tpage;
                sint32 u = sd->ePmx1.u0 + 64;
                sint32 v = sd->ePmx1.v0 + (uint8)sd->eAddZ;
                prim->p[0].uv.u = (uint8)(sd->vt0.y + u);
                prim->p[0].uv.v = (uint8)(sd->vt0.z + v);
                prim->p[1].uv.u = (uint8)(sd->vt1.y + u);
                prim->p[1].uv.v = (uint8)(sd->vt1.z + v);
                prim->p[2].uv.u = (uint8)(sd->vt2.y + u);
                prim->p[2].uv.v = (uint8)(sd->vt2.z + v);
            }
            uint32 environmentTexture;
            memcpy(&environmentTexture, &sd->ePmx0, sizeof(environmentTexture));
            if (environmentTexture != 0)
            {
                POLY_FT3 *prim = (POLY_FT3 *)sd->head.cprim.PrimPtr;
                uint32 *ot = sd->sub_ot + sd->otz;
                sd->head.cprim.PrimPtr = (char *)(prim + 1);
                prim->tag = 0x07000000 | getaddr(ot);
                setaddr(ot, prim);
                uint32 xy0, xy1, xy2;
                memcpy(&xy0, &sd->dvx0, sizeof(xy0));
                memcpy(&xy1, &sd->dvx1, sizeof(xy1));
                memcpy(&xy2, &sd->dvx2, sizeof(xy2));
                prim->p[0].xy.xy = xy0;
                prim->p[1].xy.xy = xy1;
                prim->p[2].xy.xy = xy2;
                if (renderMode == 1)
                    prim->rgbc.rgbc = sd->eColor0;
                else if ((overlayFlag & 1) != 0)
                    prim->rgbc.rgbc = sd->eColor2;
                else
                    prim->rgbc.rgbc = (facetFlag & 4) != 0 ? sd->eColor1 : sd->eColor0;
                setcode(prim, 0x26);
                uint16 clut = sd->ePmx0.clut;
                uint16 tpage = sd->ePmx0.tpage;
                prim->p[0].attr = clut;
                prim->p[1].attr = tpage;
                sint32 u = sd->ePmx0.u0 + 64;
                sint32 v = sd->ePmx0.v0;
                prim->p[0].uv.u = (uint8)(sd->tV[id0].u + u);
                prim->p[0].uv.v = (uint8)(sd->tV[id0].v + v);
                prim->p[1].uv.u = (uint8)(sd->tV[id1].u + u);
                prim->p[1].uv.v = (uint8)(sd->tV[id1].v + v);
                prim->p[2].uv.u = (uint8)(sd->tV[id2].u + u);
                prim->p[2].uv.v = (uint8)(sd->tV[id2].v + v);
            }
        }
        sint32 which = overlayFlag & 3;
        Transformer::zOverlay *facetOverlay = which != 0 ? overlay + facet->textureIndex * 3 + which - 1 : 0;
        POLY_FT3 *prim = (POLY_FT3 *)sd->head.cprim.PrimPtr;
        uint32 *ot = sd->sub_ot + sd->otz;
        sd->head.cprim.PrimPtr = (char *)(prim + 1);
        prim->tag = 0x07000000 | getaddr(ot);
        setaddr(ot, prim);
        uint32 xy0, xy1, xy2;
        memcpy(&xy0, &sd->dvx0, sizeof(xy0));
        memcpy(&xy1, &sd->dvx1, sizeof(xy1));
        memcpy(&xy2, &sd->dvx2, sizeof(xy2));
        if (renderMode == 0)
            prim->p[2].xy.xy = xy2;
        prim->p[0].xy.xy = xy0;
        prim->p[1].xy.xy = xy1;
        if (renderMode != 0)
            prim->p[2].xy.xy = xy2;
        uint32 color = renderMode == 8 && (overlayFlag & 0x80) != 0 ? 0x808080 : sd->color;
        uint8 code = 0x24;
        if (renderMode == 0)
            code = (facet->flag & 1) != 0 ? 0x26 : 0x24;
        else if ((renderMode & 8) != 0)
            code = (facetFlag & 1) != 0 ? 0x26 : 0x24;
        prim->rgbc.rgbc = color;
        setcode(prim, code);
        if (renderMode == 9 && which != 0 && (facetFlag & 1) != 0)
            setcode(prim, code | 2);
        if (which != 0)
        {
            sint32 index = facet->textureIndex + facetOverlay->offset;
            sint32 u = facetOverlay->u;
            sint32 v = facetOverlay->v;
            Draw::tPixMap *pmx = sd->pmxStart + index;
            uint16 clut = pmx->clut;
            uint16 tpage = pmx->tpage;
            prim->p[0].attr = clut;
            prim->p[1].attr = tpage;
            prim->p[0].uv.u = (uint8)(facet->uv0.u + u);
            prim->p[1].uv.u = (uint8)(facet->uv1.u + u);
            prim->p[2].uv.u = (uint8)(facet->uv2.u + u);
            prim->p[0].uv.v = (uint8)(facet->uv0.v + v);
            prim->p[1].uv.v = (uint8)(facet->uv1.v + v);
            prim->p[2].uv.v = (uint8)(facet->uv2.v + v);
        }
        else
        {
            Draw::tPixMap *pmx = sd->pmxStart + facet->textureIndex;
            sint32 offsetU = 0;
            sint32 offsetV = 0;
            bool offsetUV = false;
            if (renderMode == 0)
            {
                if ((envmap & 2) != 0 && (facet->flag & 1) == 0)
                {
                    offsetUV = true;
                    offsetU = sd->offsetU0;
                    offsetV = sd->offsetV0;
                }
                else if ((pmx->flag & 0x7F) != 0)
                {
                    offsetUV = true;
                    offsetU = sd->offsetU1;
                    offsetV = sd->offsetV1;
                    if ((pmx->flag & 2) != 0)
                    {
                        offsetU = sd->offsetU2;
                        offsetV = sd->offsetV2;
                    }
                }
            }
            uint16 clut = pmx->clut;
            uint16 tpage = pmx->tpage;
            prim->p[0].attr = clut;
            prim->p[1].attr = tpage;
            if (offsetUV)
            {
                prim->p[0].uv.u = (uint8)(facet->uv0.u + offsetU);
                prim->p[1].uv.u = (uint8)(facet->uv1.u + offsetU);
                prim->p[2].uv.u = (uint8)(facet->uv2.u + offsetU);
                prim->p[0].uv.v = (uint8)(facet->uv0.v + offsetV);
                prim->p[1].uv.v = (uint8)(facet->uv1.v + offsetV);
                prim->p[2].uv.v = (uint8)(facet->uv2.v + offsetV);
            }
            else
            {
                uint16 uv0, uv1, uv2;
                memcpy(&uv0, &facet->uv0, sizeof(uv0));
                memcpy(&uv1, &facet->uv1, sizeof(uv1));
                memcpy(&uv2, &facet->uv2, sizeof(uv2));
                prim->p[0].uv.uv = uv0;
                prim->p[1].uv.uv = uv1;
                prim->p[2].uv.uv = uv2;
            }
        }
    }
}

void DrawC::PrimMenu(matrixtdef *m, coorddef *t, Transformer::zObj *obj, Transformer::zOverlay *overlay, sint32 envmap, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800C32B0, "NFS4.EXE");
    COORD16 *Nvertice = obj->Nvertex;
    if ((envmap & 1) != 0)
    {
        gte_SetRotMatrix(&gMatA);
        gte_SetTransMatrix(&gMatA);
        sint32 count = obj->numVertex;
        for (sint32 i = 0; i < count; ++i)
        {
            sd->vt0 = Nvertice[i];
            gte_ldv0(&sd->vt0);
            gte_rt();
            gte_stlvnl(&sd->tv);
            uint32 v = sd->tv.vz;
            if (sd->tv.vz < 0)
                v = 0U - v;
            sd->tV[i].u = (uint8)sd->tv.vx;
            sd->tV[i].v = (uint8)v;
        }
    }
    TrsProj::SetTransPrecision(8);
    for (sint32 row = 0; row < 3; ++row)
    {
        sint32 r0 = m->m[row];
        sint32 r1 = m->m[row + 3];
        sint32 r2 = m->m[row + 6];
        sd->matB.m[row][0] = (sint16)(r0 >> 4);
        sd->matB.m[row][1] = (sint16)(r1 >> 4);
        sd->matB.m[row][2] = (sint16)(r2 >> 4);
    }
    for (sint32 column = 0; column < 3; ++column)
        sd->matB.m[1][column] = (sint16)-sd->matB.m[1][column];
    sint32 shift = TrsProj::precision & 31;
    sd->matB.t[0] = t->x >> shift;
    sd->matB.t[1] = (sint32)(0U - (uint32)(t->y >> shift));
    sd->matB.t[2] = t->z >> shift;
    TrsProj::ResetTransPrecision();
    gte_SetRotMatrix(&sd->matB);
    gte_SetTransMatrix(&sd->matB);
    COORD16 *vertice = obj->vertex;
    sint32 count = obj->numVertex;
    for (sint32 i = 0; i < count; ++i)
        sd->tV[i].vt = vertice[i];
    for (sint32 i = (sint32)obj->numFacet - 1; i >= 0; --i)
    {
        Transformer::zFacet *facet = obj->facet + i;
        sint32 id0 = facet->vertexId0;
        sint32 id1 = facet->vertexId1;
        sint32 id2 = facet->vertexId2;
        if (sd->head.cprim.PrimPtr >= sd->head.cprim.MPrimPtr)
            continue;
        gte_ldv3(&sd->tV[id0].vt, &sd->tV[id1].vt, &sd->tV[id2].vt);
        xport_gte_execute(0x280030);
        gte_nclip();
        gte_swc2(24, &sd->bfct);
        sint32 bfct = sd->bfct;
        if (sd->head.mirror != 0)
            bfct = (sint32)(0U - (uint32)bfct);
        if (bfct <= 0)
            continue;
        gte_stsxy3(&sd->dvx0, &sd->dvx1, &sd->dvx2);
        gte_avsz3();
        gte_stotz(&sd->otz);
        sd->otz = (sint32)((uint32)sd->otz + (uint32)sd->sub_otz);
        if (sd->otz < 0 || sd->otz > sd->sub_otSize)
            continue;
        sint32 textureIndex = facet->textureIndex;
        uint16 facetFlag = (uint16)facet->flag;
        sint32 overlayWord = gOverlay[textureIndex];
        sint32 overlayFlag = 0;
        if (overlayWord != 0)
        {
            overlayFlag = (facetFlag & 0x8000) != 0 ? overlayWord >> 8 : overlayWord & 0x3F;
            if ((facetFlag & 0x3F0) != 0)
            {
                overlayFlag &= (facetFlag & 0xFFF) >> 4;
                if (overlayFlag != 0)
                    while ((overlayFlag & 3) == 0)
                        overlayFlag >>= 2;
            }
        }
        if ((envmap & 1) != 0)
        {
            DR_MODE *mode = (DR_MODE *)sd->head.cprim.PrimPtr;
            uint32 *ot = sd->sub_ot + sd->otz;
            sd->head.cprim.PrimPtr = (char *)(mode + 1);
            DR_MODE modeCopy = sd->drawModeOff;
            *mode = modeCopy;
            setaddr(mode, getaddr(ot));
            setaddr(ot, mode);
            POLY_FT3 *environmentPrim = (POLY_FT3 *)sd->head.cprim.PrimPtr;
            ot = sd->sub_ot + sd->otz;
            sd->head.cprim.PrimPtr = (char *)(environmentPrim + 1);
            environmentPrim->tag = 0x07000000 | getaddr(ot);
            setaddr(ot, environmentPrim);
            mode = (DR_MODE *)sd->head.cprim.PrimPtr;
            ot = sd->sub_ot + sd->otz;
            sd->head.cprim.PrimPtr = (char *)(mode + 1);
            modeCopy = sd->drawModeOn;
            *mode = modeCopy;
            setaddr(mode, getaddr(ot));
            setaddr(ot, mode);
            uint32 xy0, xy1, xy2;
            memcpy(&xy0, &sd->dvx0, sizeof(xy0));
            memcpy(&xy1, &sd->dvx1, sizeof(xy1));
            memcpy(&xy2, &sd->dvx2, sizeof(xy2));
            environmentPrim->p[0].xy.xy = xy0;
            environmentPrim->p[1].xy.xy = xy1;
            environmentPrim->p[2].xy.xy = xy2;
            environmentPrim->rgbc.rgbc = (facetFlag & 4) != 0 ? sd->eColor1 : sd->eColor0;
            setcode(environmentPrim, 0x26);
            uint16 clut = sd->ePmx0.clut;
            uint16 tpage = sd->ePmx0.tpage;
            environmentPrim->p[0].attr = clut;
            environmentPrim->p[1].attr = tpage;
            sint32 u = sd->ePmx0.u0 + 64;
            sint32 v = sd->ePmx0.v0;
            environmentPrim->p[0].uv.u = (uint8)(sd->tV[id0].u + u);
            environmentPrim->p[0].uv.v = (uint8)(sd->tV[id0].v + v);
            environmentPrim->p[1].uv.u = (uint8)(sd->tV[id1].u + u);
            environmentPrim->p[1].uv.v = (uint8)(sd->tV[id1].v + v);
            environmentPrim->p[2].uv.u = (uint8)(sd->tV[id2].u + u);
            environmentPrim->p[2].uv.v = (uint8)(sd->tV[id2].v + v);
        }
        sint32 which = overlayFlag & 3;
        Transformer::zOverlay *facetOverlay = which != 0 ? overlay + textureIndex * 3 + which - 1 : 0;
        POLY_FT3 *prim = (POLY_FT3 *)sd->head.cprim.PrimPtr;
        uint32 *ot = sd->sub_ot + sd->otz;
        sd->head.cprim.PrimPtr = (char *)(prim + 1);
        prim->tag = 0x07000000 | getaddr(ot);
        setaddr(ot, prim);
        uint32 xy0, xy1, xy2;
        memcpy(&xy0, &sd->dvx0, sizeof(xy0));
        memcpy(&xy1, &sd->dvx1, sizeof(xy1));
        memcpy(&xy2, &sd->dvx2, sizeof(xy2));
        if (which == 0)
        {
            prim->p[1].xy.xy = xy1;
            prim->p[0].xy.xy = xy0;
        }
        else
        {
            prim->p[0].xy.xy = xy0;
            prim->p[1].xy.xy = xy1;
        }
        prim->p[2].xy.xy = xy2;
        prim->rgbc.rgbc = sd->color;
        setcode(prim, (facetFlag & 1) != 0 ? 0x26 : 0x24);
        if (which != 0)
        {
            sint32 index = facet->textureIndex + facetOverlay->offset;
            sint32 u = facetOverlay->u;
            sint32 v = facetOverlay->v;
            Draw::tPixMap *pmx = sd->pmxStart + index;
            uint16 clut = pmx->clut;
            uint16 tpage = pmx->tpage;
            prim->p[0].attr = clut;
            prim->p[1].attr = tpage;
            prim->p[0].uv.u = (uint8)(facet->uv0.u + u);
            prim->p[1].uv.u = (uint8)(facet->uv1.u + u);
            prim->p[2].uv.u = (uint8)(facet->uv2.u + u);
            prim->p[0].uv.v = (uint8)(facet->uv0.v + v);
            prim->p[1].uv.v = (uint8)(facet->uv1.v + v);
            prim->p[2].uv.v = (uint8)(facet->uv2.v + v);
        }
        else
        {
            Draw::tPixMap *pmx = sd->pmxStart + facet->textureIndex;
            uint16 clut = pmx->clut;
            uint16 tpage = pmx->tpage;
            prim->p[0].attr = clut;
            prim->p[1].attr = tpage;
            if ((envmap & 2) != 0 && (facetFlag & 1) == 0)
            {
                sint32 u = sd->offsetU0;
                sint32 v = sd->offsetV0;
                prim->p[0].uv.u = (uint8)(facet->uv0.u + u);
                prim->p[1].uv.u = (uint8)(facet->uv1.u + u);
                prim->p[2].uv.u = (uint8)(facet->uv2.u + u);
                prim->p[0].uv.v = (uint8)(facet->uv0.v + v);
                prim->p[1].uv.v = (uint8)(facet->uv1.v + v);
                prim->p[2].uv.v = (uint8)(facet->uv2.v + v);
            }
            else
            {
                uint16 uv0, uv1, uv2;
                memcpy(&uv0, &facet->uv0, sizeof(uv0));
                memcpy(&uv1, &facet->uv1, sizeof(uv1));
                memcpy(&uv2, &facet->uv2, sizeof(uv2));
                prim->p[0].uv.uv = uv0;
                prim->p[1].uv.uv = uv1;
                prim->p[2].uv.uv = uv2;
            }
        }
    }
}

void DrawC::PrimHalo(matrixtdef *m, coorddef *t, Transformer::zObj *obj, sint32 type, sint32 index, sint32 reflect, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800C3A30, "NFS4.EXE");
    COORD16 *vertice = obj->vertex;
    TrsProj::SetTransPrecision(8);
    for (sint32 row = 0; row < 3; ++row)
    {
        sint32 r0 = m->m[row];
        sint32 r1 = m->m[row + 3];
        sint32 r2 = m->m[row + 6];
        sd->matB.m[row][0] = (sint16)(r0 >> 4);
        sd->matB.m[row][1] = (sint16)(r1 >> 4);
        sd->matB.m[row][2] = (sint16)(r2 >> 4);
    }
    for (sint32 column = 0; column < 3; ++column)
        sd->matB.m[1][column] = (sint16)-sd->matB.m[1][column];
    sint32 shift = TrsProj::precision & 31;
    sd->matB.t[0] = t->x >> shift;
    sd->matB.t[1] = (sint32)(0U - (uint32)(t->y >> shift));
    sd->matB.t[2] = t->z >> shift;
    TrsProj::ResetTransPrecision();
    for (sint32 i = (sint32)obj->numFacet - 1; i >= 0; --i)
    {
        Transformer::zFacet *facet = obj->facet + i;
        sint32 id0 = facet->vertexId0;
        sint32 id1 = facet->vertexId1;
        sint32 id2 = facet->vertexId2;
        gte_SetRotMatrix(&sd->matB);
        gte_SetTransMatrix(&sd->matB);
        sd->vt0 = vertice[id0];
        sd->vt1 = vertice[id1];
        sd->vt2 = vertice[id2];
        gte_ldv3(&sd->vt0, &sd->vt1, &sd->vt2);
        xport_gte_execute(0x280030);
        gte_nclip();
        gte_swc2(24, &sd->bfct);
        sint32 bfct = sd->bfct;
        if (sd->head.mirror != 0)
            bfct = (sint32)(0U - (uint32)bfct);
        if (bfct <= 0)
            continue;
        gte_avsz3();
        gte_stotz(&sd->otz);
        sd->otz = (sint32)((uint32)sd->otz + (uint32)sd->sub_otz);
        if (sd->otz < 0 || sd->otz > sd->sub_otSize)
            continue;
        uint32 selectedType = (uint32)type & 0xFFBF;
        if (index >= 0)
        {
            sint32 overlayFlag = facet->flag < 0 ? (sint8)((uint16)gOverlay[index] >> 8) : (uint8)gOverlay[index];
            if ((type & 0x40) != 0 && (overlayFlag & 0x40) == 0)
                overlayFlag = facet->flag < 0 ? (sint8)((uint16)gOverlay[24] >> 8) : (uint8)gOverlay[24];
            if ((overlayFlag & 0x81) == 0)
                continue;
            if ((overlayFlag & 3) != 1 && (type & 0x7F00) != 0)
                selectedType >>= 8;
        }
        sint32 real_type = selectedType & 255;
        uint32 *copyLastPrim = sd->head.cprim.LastPrim;
        sd->head.cprim.LastPrim = sd->sub_ot;
        Flare::CarShapedHalo(real_type, &sd->vt0, &sd->vt1, &sd->vt2, facet->flag, sd->otz, &sd->head);
        if (reflect > 0 || (reflect == -1 && real_type == 5) || (reflect == -2 && real_type != 5))
        {
            gte_SetRotMatrix(&sd->matB);
            gte_SetTransMatrix(&sd->matB);
            Flare::CarShapedHalo((selectedType & 255) | 0x100, &sd->vt0, &sd->vt1, &sd->vt2, facet->flag, sd->otz, &sd->head);
        }
        sd->head.cprim.LastPrim = copyLastPrim;
    }
}

void DrawC::ShadowPrim(Draw::tVertex *shadowVT, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800C3ED8, "NFS4.EXE");
    Draw::tPixMap *shadowPmx = gShadowPixmap[0];
    if (R3DCar::InMenu != 0)
        shadowPmx = gMenuPixmap[1];
    ChangeTPage(&shadowPmx->tpage, 2);
    gte_SetRotMatrix(&gScreenMat);
    gte_SetTransMatrix(&gScreenMat);
    if (sd->head.cprim.PrimPtr >= sd->head.cprim.MPrimPtr)
        return;
    gte_ldv0(shadowVT);
    xport_gte_execute(0x180001);
    POLY_FT4 *projected = (POLY_FT4 *)carCache.head.cprim.PrimPtr;
    gte_stsxy(&projected->p[0].xy);
    gte_ldv3(shadowVT + 1, shadowVT + 3, shadowVT + 2);
    xport_gte_execute(0x280030);
    gte_stsxy3(&projected->p[1].xy, &projected->p[3].xy, &projected->p[2].xy);
    gte_avsz4();
    gte_stotz(&sd->otz);
    sd->otz = (sint32)((uint32)(sd->otz >> 1) + 40U);
    if (sd->otz < 0 || sd->otz > (sint32)((uint32)Draw::gViewOtSize - 3U))
        return;
    POLY_FT4 *prim = (POLY_FT4 *)sd->head.cprim.PrimPtr;
    uint32 *ot = sd->head.cprim.LastPrim + sd->otz;
    sd->head.cprim.PrimPtr = (char *)(prim + 1);
    addPrim(ot, prim);
    uint32 color = sd->color;
    setlen(prim, 9);
    prim->rgbc.rgbc = color;
    setcode(prim, 0x2E);
    Draw::tPixMap map = *shadowPmx;
    prim->p[0].uv.u = map.u0;
    prim->p[0].uv.v = map.v0;
    prim->p[0].attr = map.clut;
    prim->p[1].uv.u = map.u1;
    prim->p[1].uv.v = map.v1;
    prim->p[1].attr = map.tpage;
    prim->p[2].uv.u = map.u2;
    prim->p[2].uv.v = map.v2;
    prim->p[2].attr = map.pad2;
    prim->p[3].uv.u = map.u3;
    prim->p[3].uv.v = map.v3;
    prim->p[3].attr = map.flag;
}

void DrawC::DivideShadowPrim(COORD16 *vt0, COORD16 *vt1, COORD16 *vt2, COORD16 *vt3, uint16 *u0, uint16 *u1, uint16 *u2, uint16 *u3, Draw::tPixMap *pmx, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800C40DC, "NFS4.EXE");
    if (sd->head.cprim.PrimPtr >= sd->head.cprim.MPrimPtr)
        return;
    gte_ldv0(vt0);
    xport_gte_execute(0x180001);
    POLY_FT4 *projected = (POLY_FT4 *)carCache.head.cprim.PrimPtr;
    gte_stsxy(&projected->p[0].xy);
    gte_ldv3(vt1, vt2, vt3);
    xport_gte_execute(0x280030);
    gte_stsxy3(&projected->p[1].xy, &projected->p[3].xy, &projected->p[2].xy);
    if (R3DCar::InMenu == 0)
    {
        gte_avsz4();
        gte_stotz(&sd->otz);
        sd->otz = (sint32)((uint32)(sd->otz >> 3) + 40U);
        if (sd->otz < 0 || sd->otz > (sint32)((uint32)Draw::gViewOtSize - 3U))
            return;
    }
    else
        sd->otz = 0;
    if (vt0->z < 0 && vt1->z < 0 && vt2->z < 0 && vt3->z < 0)
        return;
    POLY_FT4 *prim = (POLY_FT4 *)sd->head.cprim.PrimPtr;
    uint32 *last = sd->head.cprim.LastPrim;
    sd->head.cprim.PrimPtr = (char *)(prim + 1);
    uint32 *ot = last + sd->otz;
    addPrim(ot, prim);
    uint32 color = sd->color;
    setlen(prim, 9);
    prim->rgbc.rgbc = color;
    setcode(prim, 0x2E);
    uint16 clut = pmx->clut;
    uint16 tpage = pmx->tpage;
    prim->p[0].attr = clut;
    prim->p[1].attr = tpage;
    uint16 uv0, uv1, uv2, uv3;
    memcpy(&uv0, u0, sizeof(uv0));
    memcpy(&uv1, u1, sizeof(uv1));
    memcpy(&uv3, u3, sizeof(uv3));
    memcpy(&uv2, u2, sizeof(uv2));
    prim->p[0].uv.uv = uv0;
    prim->p[1].uv.uv = uv1;
    prim->p[2].uv.uv = uv3;
    prim->p[3].uv.uv = uv2;
}

void DrawC::ShadowPrimClip(Draw::tVertex *shadowVT, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800C42C4, "NFS4.EXE");
    Draw::tPixMap *shadowPmx = gShadowPixmap[0];
    if (R3DCar::InMenu != 0)
        shadowPmx = gMenuPixmap[1];
    ChangeTPage(&shadowPmx->tpage, 2);
    gte_SetRotMatrix(&gScreenMat);
    gte_SetTransMatrix(&gScreenMat);
    COORD16 *corner[4] = { &sd->vt0, &sd->vt1, &sd->vt2, &sd->vt3 };
    const sint32 order[4] = { 0, 1, 3, 2 };
    for (sint32 i = 0; i < 4; ++i)
    {
        corner[i]->x = shadowVT[order[i]].sv.x;
        corner[i]->y = shadowVT[order[i]].sv.y;
        corner[i]->z = shadowVT[order[i]].sv.z;
    }
    for (sint32 i = 0; i < 4; ++i)
    {
        corner[i]->x = (sint16)((uint16)corner[i]->x * 4);
        corner[i]->y = (sint16)((uint16)corner[i]->y * 4);
        corner[i]->z = (sint16)((uint16)corner[i]->z * 4);
    }
    COORD16 *middle[5] = { &sd->vt4, &sd->vt5, &sd->vt6, &sd->vt7, &sd->vt8 };
    const sint32 left[5] = { 0, 1, 2, 3, 0 };
    const sint32 right[5] = { 1, 2, 3, 0, 2 };
    for (sint32 i = 0; i < 5; ++i)
    {
        middle[i]->x = (sint16)((corner[left[i]]->x + corner[right[i]]->x + 1) >> 1);
        middle[i]->y = (sint16)((corner[left[i]]->y + corner[right[i]]->y + 1) >> 1);
        middle[i]->z = (sint16)((corner[left[i]]->z + corner[right[i]]->z + 1) >> 1);
    }
    uint16 uv0 = (uint16)(shadowPmx->u0 | (shadowPmx->v0 << 8));
    uint16 uv1 = (uint16)(shadowPmx->u1 | (shadowPmx->v1 << 8));
    uint16 uv2 = (uint16)(shadowPmx->u3 | (shadowPmx->v3 << 8));
    uint16 uv3 = (uint16)(shadowPmx->u2 | (shadowPmx->v2 << 8));
    sd->u0 = (uint8)uv0;
    sd->v0 = (uint8)(uv0 >> 8);
    sd->u1 = (uint8)uv1;
    sd->v1 = (uint8)(uv1 >> 8);
    sd->u2 = (uint8)uv2;
    sd->v2 = (uint8)(uv2 >> 8);
    sd->u3 = (uint8)uv3;
    sd->v3 = (uint8)(uv3 >> 8);
    sd->u4 = (uint8)((sd->u0 + sd->u1 + 1) >> 1);
    sd->v4 = (uint8)((sd->v0 + sd->v1 + 1) >> 1);
    sd->u5 = (uint8)((sd->u1 + sd->u2 + 1) >> 1);
    sd->v5 = (uint8)((sd->v1 + sd->v2 + 1) >> 1);
    sd->offsetU0 = (uint8)((sd->u2 + sd->u3 + 1) >> 1);
    sd->offsetV0 = (uint8)((sd->v2 + sd->v3 + 1) >> 1);
    sd->offsetU1 = (uint8)((sd->u3 + sd->u0 + 1) >> 1);
    sd->offsetV1 = (uint8)((sd->v3 + sd->v0 + 1) >> 1);
    sd->offsetU2 = (uint8)((sd->u0 + sd->u2 + 1) >> 1);
    sd->offsetV2 = (uint8)((sd->v0 + sd->v2 + 1) >> 1);
    DivideShadowPrim(&sd->vt0, &sd->vt4, &sd->vt8, &sd->vt7, (uint16 *)&sd->u0, (uint16 *)&sd->u4, (uint16 *)&sd->offsetU2, (uint16 *)&sd->offsetU1, shadowPmx, sd);
    DivideShadowPrim(&sd->vt4, &sd->vt1, &sd->vt5, &sd->vt8, (uint16 *)&sd->u4, (uint16 *)&sd->u1, (uint16 *)&sd->u5, (uint16 *)&sd->offsetU2, shadowPmx, sd);
    DivideShadowPrim(&sd->vt7, &sd->vt8, &sd->vt6, &sd->vt3, (uint16 *)&sd->offsetU1, (uint16 *)&sd->offsetU2, (uint16 *)&sd->offsetU0, (uint16 *)&sd->u3, shadowPmx, sd);
    DivideShadowPrim(&sd->vt8, &sd->vt5, &sd->vt2, &sd->vt6, (uint16 *)&sd->offsetU2, (uint16 *)&sd->u5, (uint16 *)&sd->u2, (uint16 *)&sd->offsetU0, shadowPmx, sd);
}

void DrawC::SpotPrims(matrixtdef *m, coorddef *t, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800C4800, "NFS4.EXE");
    TrsProj::SetTransPrecision(8);
    for (sint32 row = 0; row < 3; ++row)
    {
        sint32 r0 = m->m[row];
        sint32 r1 = m->m[row + 3];
        sint32 r2 = m->m[row + 6];
        sd->matB.m[row][0] = (sint16)(r0 >> 4);
        sd->matB.m[row][1] = (sint16)(r1 >> 4);
        sd->matB.m[row][2] = (sint16)(r2 >> 4);
    }
    for (sint32 column = 0; column < 3; ++column)
        sd->matB.m[1][column] = (sint16)-sd->matB.m[1][column];
    sint32 shift = TrsProj::precision & 31;
    sd->matB.t[0] = t->x >> shift;
    sd->matB.t[1] = (sint32)(0U - (uint32)(t->y >> shift));
    sd->matB.t[2] = t->z >> shift;
    TrsProj::ResetTransPrecision();
    gte_SetRotMatrix(&sd->matB);
    gte_SetTransMatrix(&sd->matB);
    DR_MODE *mode = (DR_MODE *)sd->head.cprim.PrimPtr;
    uint32 *last = sd->head.cprim.LastPrim;
    sd->otz = 0;
    sd->head.cprim.PrimPtr = (char *)(mode + 1);
    addPrim(last + sd->otz, mode);
    SetDrawMode(mode, 0, 0, 0x120, 0);
    sd->vt0 = Fe3D::spotVertex[32];
    for (sint32 i = 0; i < 32; ++i)
    {
        sint32 iPlus = i + 1;
        if (iPlus == 32)
            iPlus = 0;
        if (sd->head.cprim.PrimPtr < sd->head.cprim.MPrimPtr)
        {
            sd->vt1 = Fe3D::spotVertex[i];
            sd->vt2 = Fe3D::spotVertex[iPlus];
            gte_ldv3(&sd->vt0, &sd->vt1, &sd->vt2);
            xport_gte_execute(0x280030);
            POLY_G3 *prim = (POLY_G3 *)sd->head.cprim.PrimPtr;
            last = sd->head.cprim.LastPrim;
            sd->head.cprim.PrimPtr = (char *)(prim + 1);
            addPrim(last + sd->otz, prim);
            gte_stsxy3(&prim->p[0].xy, &prim->p[1].xy, &prim->p[2].xy);
            uint32 color = sd->color;
            setlen(prim, 6);
            prim->p[1].rgbc.rgbc = 0;
            prim->p[2].rgbc.rgbc = 0;
            prim->p[0].rgbc.rgbc = color;
            setcode(prim, 0x32);
        }
    }
    mode = (DR_MODE *)sd->head.cprim.PrimPtr;
    last = sd->head.cprim.LastPrim;
    sd->head.cprim.PrimPtr = (char *)(mode + 1);
    addPrim(last + sd->otz, mode);
    SetDrawMode(mode, 0, 1, 0x120, 0);
}

void DrawC::ShowroomPrims(matrixtdef *m, coorddef *t, Draw::CarCache *sd)
{
    FUNCTION_MARKER(0x800C4B84, "NFS4.EXE");
    Draw::tPixMap *lightPmx = gMenuPixmap[3];
    if (gShowroomLights == 0)
        return;
    sint32 tick = gettick();
    sint32 quotient = tick >> 8;
    if (tick < 0)
        quotient = (sint32)((uint32)tick + 255U) >> 8;
    sint32 hilight[2];
    sint32 hilight_direction[2];
    hilight[0] = (sint32)((uint32)tick - ((uint32)quotient << 8)) >> 3;
    hilight_direction[0] = -1;
    if (gMenuLightsDirection == 0)
    {
        hilight[1] = (hilight[0] + 16) & 31;
        hilight_direction[1] = -1;
    }
    else
    {
        hilight[1] = 32 - hilight[0];
        hilight_direction[1] = 1;
    }
    sint8 hilight_state[32];
    for (sint32 i = 31; i >= 0; --i)
        hilight_state[i] = -1;
    for (sint32 i = 0; i < 2; ++i)
        for (sint32 j = 0; j < 5; ++j)
        {
            sint32 index = (hilight[i] + j * hilight_direction[i]) & 31;
            if (hilight_state[index] < j)
                hilight_state[index] = (sint8)j;
        }
    ChangeTPage(&lightPmx->tpage, 1);
    TrsProj::SetTransPrecision(8);
    for (sint32 row = 0; row < 3; ++row)
    {
        sint32 r0 = m->m[row];
        sint32 r1 = m->m[row + 3];
        sint32 r2 = m->m[row + 6];
        sd->matB.m[row][0] = (sint16)(r0 >> 4);
        sd->matB.m[row][1] = (sint16)(r1 >> 4);
        sd->matB.m[row][2] = (sint16)(r2 >> 4);
    }
    for (sint32 column = 0; column < 3; ++column)
        sd->matB.m[1][column] = (sint16)-sd->matB.m[1][column];
    sint32 shift = TrsProj::precision & 31;
    sd->matB.t[0] = t->x >> shift;
    sd->matB.t[1] = (sint32)(0U - (uint32)(t->y >> shift));
    sd->matB.t[2] = t->z >> shift;
    TrsProj::ResetTransPrecision();
    gte_SetRotMatrix(&sd->matB);
    gte_SetTransMatrix(&sd->matB);
    sd->otz = 0;
    for (sint32 i = 0; i < 32; ++i)
    {
        sint32 iPlus = i * 2 + 2;
        if (iPlus == 64)
            iPlus = 0;
        if (sd->head.cprim.PrimPtr >= sd->head.cprim.MPrimPtr)
            return;
        sd->vt0 = Fe3D::lightsVertex[i * 2];
        sd->vt1 = Fe3D::lightsVertex[iPlus];
        sd->vt2 = Fe3D::lightsVertex[i * 2 + 1];
        sd->vt3 = Fe3D::lightsVertex[iPlus + 1];
        for (sint32 j = 0; j < 2; ++j)
        {
            if (j != 0 && hilight_state[i] < 0)
                continue;
            gte_ldv0(&sd->vt0);
            xport_gte_execute(0x180001);
            POLY_FT4 *prim = (POLY_FT4 *)sd->head.cprim.PrimPtr;
            sd->head.cprim.PrimPtr = (char *)(prim + 1);
            addPrim(sd->head.cprim.LastPrim + sd->otz, prim);
            gte_stsxy(&prim->p[0].xy);
            gte_ldv3(&sd->vt1, &sd->vt3, &sd->vt2);
            xport_gte_execute(0x280030);
            gte_stsxy3(&prim->p[1].xy, &prim->p[3].xy, &prim->p[2].xy);
            uint32 color = j == 0 ? 0x300A00 : hilight_colors[hilight_state[i]];
            prim->rgbc.rgbc = color;
            setcode(prim, 0x2E);
            setlen(prim, 9);
            Draw::tPixMap map = *lightPmx;
            prim->p[0].uv.u = map.u0;
            prim->p[0].uv.v = map.v0;
            prim->p[0].attr = map.clut;
            prim->p[1].uv.u = map.u1;
            prim->p[1].uv.v = map.v1;
            prim->p[1].attr = map.tpage;
            prim->p[2].uv.u = map.u2;
            prim->p[2].uv.v = map.v2;
            prim->p[2].attr = map.pad2;
            prim->p[3].uv.u = map.u3;
            prim->p[3].uv.v = map.v3;
            prim->p[3].attr = map.flag;
        }
    }
}
