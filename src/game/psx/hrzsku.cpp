#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "psx_gte.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/camera.h"
#include "game/common/genericpmx.h"
#include "game/common/gmesetup.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"
#include "game/psx/flare.h"
#include "game/psx/hrzsku.h"
#include "game/psx/night.h"
#include "game/psx/rendercache.h"
#include "game/psx/texture.h"
#include "game/psx/trackspec.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/random.h"

struct tHrz_LightningFork;

struct tHrz_LightningFork;

struct tHrz_LightningFork
{
    DVECTOR pos;
    char pmxIndex;
};

struct tHrz_Lightning
{
    tHrz_LightningFork forks[16];
    sint32 brightness;
    char numForks;
};

static SVECTOR sunPosInSky;
static SVECTOR moonPosInSky;
static SVECTOR *starPosInSky;
static uint32 *starColors;
tHrz_Lightning gHrz_Lightning;
SVECTOR *gRngCoordTop;
char gSkyPixmapIndex[64];
sint32 gfxPmxHeightPercentage[16];
Draw::tPixMap *gHorizonPixmap[16];
Draw::tPixMap *gpPmx[16];
SVECTOR gSkyMesh[85];
CVECTOR gSkyColor[85];
Draw::tPixMap gHorizonExtraSkyPixmaps[4];
CVECTOR gHrzRingColor[2][17];
CHorizonSpec *Hrz::gTrackSpec;
CSkySpec *Sky::gTrackSpec;
SVECTOR Hrz::gLightningPosInSky;
CVECTOR Hrz::gSaveCol[4];

void Horizon_InterpolateLineSCoords(DVECTOR *sc, DVECTOR *s0, DVECTOR *s1, sint32 *percentage, sint32 n, sint32 bPercentageArray)
{
    FUNCTION_MARKER(0x800CF574, "NFS4.EXE");
    sint32 p = 0;
    if (bPercentageArray == 0)
        p = *percentage;
    for (sint32 i = 0; i < n; ++i)
    {
        if (bPercentageArray != 0)
            p = *percentage;
        uint32 product = (uint32)p * (uint32)((sint32)s1->vx - (sint32)s0->vx);
        if ((sint32)product < 0)
            product += 0xFFFFU;
        sc->vx = (sint16)((uint16)s0->vx + ((sint32)product >> 16));
        if (bPercentageArray != 0)
            p = *percentage;
        product = (uint32)p * (uint32)((sint32)s1->vy - (sint32)s0->vy);
        if ((sint32)product < 0)
            product += 0xFFFFU;
        sc->vy = (sint16)((uint16)s0->vy + ((sint32)product >> 16));
        if (bPercentageArray != 0)
            ++percentage;
        ++s0;
        ++s1;
        ++sc;
    }
}

void HrzSetPsxMatrix(matrixtdef *m)
{
    FUNCTION_MARKER(0x800D083C, "NFS4.EXE");
    MATRIX mpsx;
    for (sint32 column = 0; column < 3; ++column)
    {
        sint32 t1 = m->m[column * 3];
        sint32 t2 = m->m[column * 3 + 1];
        sint32 t3 = m->m[column * 3 + 2];
        mpsx.m[0][column] = (sint16)(t1 >> 4);
        mpsx.m[1][column] = (sint16)((sint32)(0U - (uint32)t2) >> 4);
        mpsx.m[2][column] = (sint16)(t3 >> 4);
    }
    gte_SetRotMatrix(&mpsx);
}

void Sky::InitStars(void)
{
    FUNCTION_MARKER(0x800CF6B4, "NFS4.EXE");
    if (starPosInSky != 0)
        return;
    uint32 oldSeed = random();
    seedrandom(gTrackSpec->starRandomSeed);
    starPosInSky = static_cast<SVECTOR *>(reservememadr("stars", (sint32)((uint32)gTrackSpec->numStars << 3), 0));
    starColors = static_cast<uint32 *>(reservememadr("starCols", (sint32)((uint32)gTrackSpec->numStars << 2), 0));
    for (sint32 i = 0; i < gTrackSpec->numStars; ++i) {
        sint32 latAngle = random() & 0xffff;
        uint32 sample = random();
        uint32 angleRange = (uint32)gTrackSpec->starAngleHigh - (uint32)gTrackSpec->starAngleLow;
        if (angleRange == 0)
            abort();
        sint32 heightAngle = (sint32)((uint32)gTrackSpec->starAngleLow + sample % angleRange);
        sint32 height = fixedmult(fixedsin(heightAngle), 1000);
        sint32 radius = fixedmult(fixedcos(heightAngle), 1000);
        starPosInSky[i].vx = (sint16)fixedmult(fixedsin(latAngle), radius);
        starPosInSky[i].vy = (sint16)height;
        starPosInSky[i].vz = (sint16)fixedmult(fixedcos(latAngle), radius);
        sample = random();
        uint32 brightnessRange = (uint32)gTrackSpec->starBrightMax - (uint32)gTrackSpec->starBrightMin;
        if (brightnessRange == 0)
            abort();
        uint32 starBright = (uint32)gTrackSpec->starBrightMin + sample % brightnessRange;
        starColors[i] = (starBright << 16) | (starBright << 8) | starBright;
    }
    seedrandom(oldSeed);
}

void Sky::KillStars(void)
{
    FUNCTION_MARKER(0x800CF89C, "NFS4.EXE");
    if (starPosInSky != 0)
        purgememadr(starPosInSky);
    if (starColors != 0)
        purgememadr(starColors);
    starPosInSky = 0;
    starColors = 0;
}

void Hrz::InitSkyColor(void)
{
    FUNCTION_MARKER(0x800CF8E0, "NFS4.EXE");
    sint32 k = 0;
    for (sint32 i = 0; i < 5; ++i) {
        CVECTOR cur_bk = Sky::gTrackSpec->backcolors[i];
        CVECTOR cur_fr = Sky::gTrackSpec->frontcolors[i];
        CVECTOR rounddiff;
        rounddiff.r = (cur_bk.r - cur_fr.r) / 8;
        rounddiff.g = (cur_bk.g - cur_fr.g) / 8;
        rounddiff.b = (cur_bk.b - cur_fr.b) / 8;
        for (sint32 j = 0; j < 17; ++j, ++k) {
            sint32 weight = j < 9 ? j : 16 - j;
            gSkyColor[k].r = cur_fr.r + rounddiff.r * weight;
            gSkyColor[k].g = cur_fr.g + rounddiff.g * weight;
            gSkyColor[k].b = cur_fr.b + rounddiff.b * weight;
        }
    }
}

void Hrz::InitSky(void)
{
    FUNCTION_MARKER(0x800CFA3C, "NFS4.EXE");
    sunPosInSky.vx = (sint16)fixedmult(fixedcos(Sky::gTrackSpec->sunAngleInSky), 1000);
    sunPosInSky.vz = (sint16)fixedmult(fixedsin(Sky::gTrackSpec->sunAngleInSky), 1000);
    sunPosInSky.vy = (sint16)((uint16)TrackSpec::gSpec.skyspec.sunHeightInSky +
        (uint16)TrackSpec::gSpec.skyspec.yoffset);
    moonPosInSky.vx = (sint16)fixedmult(fixedcos(Sky::gTrackSpec->moonAngleInSky), 1000);
    moonPosInSky.vz = (sint16)fixedmult(fixedsin(Sky::gTrackSpec->moonAngleInSky), 1000);
    moonPosInSky.vy = (sint16)((uint16)TrackSpec::gSpec.skyspec.moonHeightInSky +
        (uint16)TrackSpec::gSpec.skyspec.yoffset);
    if (GameSetup::gData.commMode == 1) {
        Draw::SetViewColor(Draw::gPlayer1View, Sky::gTrackSpec->clearcolor.r,
            Sky::gTrackSpec->clearcolor.g, Sky::gTrackSpec->clearcolor.b);
        Draw::SetViewColor(Draw::gPlayer2View, Sky::gTrackSpec->clearcolor.r,
            Sky::gTrackSpec->clearcolor.g, Sky::gTrackSpec->clearcolor.b);
    } else {
        Draw::SetViewColor(Draw::gPlayer1View, Sky::gTrackSpec->clearcolor.r,
            Sky::gTrackSpec->clearcolor.g, Sky::gTrackSpec->clearcolor.b);
    }
    sint32 k = 0;
    for (sint32 i = 0; i < 5; ++i) {
        sint32 angle, radius;
        if (Sky::gTrackSpec->flags & 0x80) {
            radius = fixedmult(fixedcos(Sky::gTrackSpec->ringAngles[i]), 1000);
            angle = Sky::gTrackSpec->ringAngles[i];
        } else {
            angle = ((Sky::gTrackSpec->flags & 2) && i < 4) ? i << 11 : i << 12;
            radius = fixedmult(fixedcos(angle), 1000);
        }
        sint32 height = fixedmult(fixedsin(angle), 1000);
        for (sint32 j = 0; j < 17; ++j, ++k) {
            sint32 longitude = (sint32)(((uint32)j << 12) - (uint32)Sky::gTrackSpec->sunAngleInSky + 0x4000);
            gSkyMesh[k].vx = (sint16)fixedmult(fixedsin(longitude), radius);
            gSkyMesh[k].vy = (sint16)((uint16)Sky::gTrackSpec->yoffset + height);
            gSkyMesh[k].vz = (sint16)fixedmult(fixedcos(longitude), radius);
        }
    }
    if (Sky::gTrackSpec->type == 1)
        for (sint32 i = 0; i < 4; ++i)
            for (sint32 j = 0; j < 16; ++j)
                gSkyPixmapIndex[i * 16 + j] = (char)((uint8)Sky::gTrackSpec->cloudIndices[i][j % 4] + 8);
    if (Sky::gTrackSpec->type != 1 || (Sky::gTrackSpec->flags & 0x20))
        InitSkyColor();
    Flare::InitLensFlare();
}

void Hrz::Init2DRing(void)
{
    FUNCTION_MARKER(0x800CFD80, "NFS4.EXE");
    for (sint32 i = 0; i < 17; ++i) {
        sint32 angle = (sint32)(((uint32)i << 12) + (uint32)gTrackSpec->angle);
        gRngCoordTop[i].vx = (sint16)fixedmult(fixedsin(angle), 32000);
        gRngCoordTop[i].vy = 0;
        gRngCoordTop[i].vz = (sint16)fixedmult(fixedcos(angle), 32000);
    }
    sint32 nMaxPmxHeight_InPixels = 1;
    sint32 pnPmxHeight_InPixels[16];
    CHorizonSpec *spec = gTrackSpec;
    for (sint32 i = 0; i < 16; ++i) {
        Draw::tPixMap *pmx = gHorizonPixmap[(uint8)spec->ringPMX[i] & 7];
        gpPmx[i] = pmx;
        sint32 height = pmx->v2 - pmx->v1;
        pnPmxHeight_InPixels[i] = height;
        if (nMaxPmxHeight_InPixels < height)
            nMaxPmxHeight_InPixels = height;
    }
    for (sint32 i = 0; i < 16; ++i)
        gfxPmxHeightPercentage[i] = (pnPmxHeight_InPixels[i] * 65536) / nMaxPmxHeight_InPixels;
    for (sint32 level = 0; level < 2; ++level) {
        CVECTOR cur_bk = gTrackSpec->backColor[level];
        CVECTOR cur_fr = gTrackSpec->frontColor[level];
        CVECTOR rounddiff;
        rounddiff.r = (cur_bk.r - cur_fr.r) / 8;
        rounddiff.g = (cur_bk.g - cur_fr.g) / 8;
        rounddiff.b = (cur_bk.b - cur_fr.b) / 8;
        for (sint32 j = 0; j < 16; ++j) {
            sint32 weight = j < 9 ? j : 16 - j;
            gHrzRingColor[level][j].r = cur_fr.r + rounddiff.r * weight;
            gHrzRingColor[level][j].g = cur_fr.g + rounddiff.g * weight;
            gHrzRingColor[level][j].b = cur_fr.b + rounddiff.b * weight;
        }
        gHrzRingColor[level][16] = gHrzRingColor[level][0];
    }
    InitSky();
    gSaveCol[1] = Sky::gTrackSpec->frontcolors[0];
}

void Hrz::GetHorizonPixMap(Draw::tPixMap *p)
{
    FUNCTION_MARKER(0x800D0064, "NFS4.EXE");
    for (sint32 i = 11; i >= 0; --i)
        gHorizonPixmap[i] = &p[i];
    for (sint32 i = 0; i < 4; ++i) {
        Texture::CloneUVPmx(gHorizonPixmap[8 + i], 0, &gHorizonExtraSkyPixmaps[i]);
        gHorizonPixmap[12 + i] = &gHorizonExtraSkyPixmaps[i];
    }
}

void Hrz::InitHorizon(void)
{
    FUNCTION_MARKER(0x800D0118, "NFS4.EXE");
    CHorizonSpec *spec = &TrackSpec::gSpec.horizonspec;
    gTrackSpec = spec;
    Sky::gTrackSpec = &TrackSpec::gSpec.skyspec;
    gRngCoordTop = static_cast<SVECTOR *>(reservememadr("gRngCoordTop", 0x88, 0));
    Init2DRing();
    if (TrackSpec::gSpec.skyspec.flags & 0x10)
        Sky::InitStars();
}

void Hrz::KillHorizon(void)
{
    FUNCTION_MARKER(0x800D0184, "NFS4.EXE");
    purgememadr(gRngCoordTop);
    Sky::KillStars();
}

void Hrz::LightningAddFork(sint8 direction, sint8 column, sint8 row)
{
    FUNCTION_MARKER(0x800D01B0, "NFS4.EXE");
    uint32 type;
    if (row == 0)
        type = 3;
    else if (row < 3)
        type = random() % 3;
    else
        type = 2;
    sint32 index = (uint8)gHrz_Lightning.numForks;
    gHrz_Lightning.numForks = (char)(index + 1);
    tHrz_LightningFork *fork = &gHrz_Lightning.forks[index];
    fork->pos.vx = column * 15;
    fork->pos.vy = row * 15;
    if (type == 0) {
        LightningAddFork(direction, (sint8)(column + direction), (sint8)(row + 1));
        fork->pmxIndex = (random() & 1) + 2;
    } else if (type == 1) {
        LightningAddFork(-1, (sint8)(column - 1), (sint8)(row + 1));
        LightningAddFork(1, (sint8)(column + 1), (sint8)(row + 1));
        fork->pmxIndex = 6;
    } else if (type == 2) {
        fork->pmxIndex = (random() & 1) + 4;
    } else {
        direction = (random() & 1) ? -1 : 1;
        LightningAddFork(direction, (sint8)(column + direction), (sint8)(row + 1));
        fork->pmxIndex = random() & 1;
    }
    if (direction < 0)
        fork->pmxIndex += 8;
}

void Hrz::CalculateLightning(void)
{
    FUNCTION_MARKER(0x800D03C8, "NFS4.EXE");
    gHrz_Lightning.numForks = 0;
    gHrz_Lightning.brightness = 200;
    LightningAddFork(1, 0, 0);
}

void Hrz::TextureQuad(DVECTOR *pt, char type, char bright, Draw::DCache *sd)
{
    FUNCTION_MARKER(0x800D0404, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    uint32 *otBase = cprim->LastPrim;
    POLY_FT4 *prim = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
    uint32 *ot = otBase + sd->otz;
    setaddr(prim, getaddr(ot));
    ot = otBase + sd->otz;
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    setlen(prim, 9);
    prim->rgbc.code = 0x2E;
    prim->rgbc.b = (uint8)bright;
    prim->rgbc.g = (uint8)bright;
    prim->rgbc.r = (uint8)bright;
    prim->p[0].xy.x = pt->vx;
    prim->p[0].xy.y = pt->vy;
    prim->p[2].xy.x = pt->vx;
    prim->p[2].xy.y = (sint16)((uint16)pt->vy + 16);
    prim->p[1].xy.x = (sint16)((uint16)pt->vx + 16);
    prim->p[1].xy.y = pt->vy;
    prim->p[3].xy.x = (sint16)((uint16)pt->vx + 16);
    prim->p[3].xy.y = (sint16)((uint16)pt->vy + 16);
    Draw::tPixMap *pixmap = gLightningPixmap[(uint8)type];
    uint32 l0, l1, l2, l3;
    memcpy(&l0, &pixmap->u0, 4);
    memcpy(&l1, &pixmap->u1, 4);
    memcpy(&l2, &pixmap->u2, 4);
    memcpy(&l3, &pixmap->u3, 4);
    memcpy(&prim->p[0].uv, &l0, 4);
    memcpy(&prim->p[1].uv, &l1, 4);
    memcpy(&prim->p[2].uv, &l2, 4);
    memcpy(&prim->p[3].uv, &l3, 4);
}

void Hrz::SetLightingPosInSky(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800D0534, "NFS4.EXE");
    coorddef forwardVec = { Vi->cview.mrotation.m[6], Vi->cview.mrotation.m[7], Vi->cview.mrotation.m[8] };
    sint32 dx = random() % 8000;
    gLightningPosInSky.vx = (sint16)((forwardVec.x >> 2) + dx - 4000);
    sint32 dy = random() % 3000;
    gLightningPosInSky.vy = (sint16)((forwardVec.y >> 2) + 8000 + dy);
    sint32 dz = random() % 8000;
    gLightningPosInSky.vz = (sint16)((forwardVec.z >> 2) + dz - 4000);
}

void Hrz::BuildForkLightning(Draw::DCache *sd)
{
    FUNCTION_MARKER(0x800D0644, "NFS4.EXE");
    if (gHrz_Lightning.brightness <= 0)
        return;
    sd->otz = (sint32)((uint32)Draw::gViewOtSize - 2U);
    coorddef trans;
    memset(&trans, 0, sizeof(trans));
    HrzSetPsxTranslation(&trans);
    DVECTOR screenPos;
    gte_ldv0(&gLightningPosInSky);
    gte_rtps();
    gte_stsxy(&screenPos);
    uint8 i = 0;
    if ((uint8)gHrz_Lightning.numForks != 0)
    {
        do
        {
            tHrz_LightningFork *fork = &gHrz_Lightning.forks[i];
            i = (uint8)(i + 1);
            DVECTOR pos;
            pos.vx = (sint16)((uint16)fork->pos.vx + (uint16)screenPos.vx);
            sint32 y = (uint16)fork->pos.vy + (uint16)screenPos.vy;
            char bright = (char)gHrz_Lightning.brightness;
            pos.vy = (sint16)y;
            TextureQuad(&pos, fork->pmxIndex, bright, sd);
        } while (i < (uint8)gHrz_Lightning.numForks);
    }
    gHrz_Lightning.brightness = (sint32)((uint32)gHrz_Lightning.brightness - 16U);
}

void HrzSetPsxTranslation(coorddef *t)
{
    FUNCTION_MARKER(0x800D091C, "NFS4.EXE");
    MATRIX mpsx;
    mpsx.t[0] = t->x >> 10;
    mpsx.t[1] = t->y >> 10;
    mpsx.t[2] = t->z >> 10;
    gte_SetTransMatrix(&mpsx);
}

void Hrz::LightningFlicker(sint32 on)
{
    FUNCTION_MARKER(0x800D0760, "NFS4.EXE");
    if (on == 0)
    {
        Sky::gTrackSpec->frontcolors[0] = gSaveCol[1];
        InitSkyColor();
        return;
    }
    if (Night::gShowForks != 0)
        return;
    uint32 color = on == 1 ? 0x00FFC0C0U : 0x00C02828U;
    memcpy(&Sky::gTrackSpec->frontcolors[0], &color, 4);
    for (sint32 i = 84; i >= 0; --i)
        memcpy(&gSkyColor[i], &color, 4);
}

void Hrz::RotProj16(sint32 n, SVECTOR *s, sint32 *z, DVECTOR *p)
{
    FUNCTION_MARKER(0x800D096C, "NFS4.EXE");
    uint32 remaining = (uint32)n;
    while (remaining != 0)
    {
        gte_ldv0(s);
        gte_rtps();
        ++s;
        gte_stsxy(p);
        ++p;
        gte_swc2(27, z);
        --remaining;
        ++z;
    }
}

void Hrz::SetDitheringPrim(sint32 dither, sint32 otz)
{
    FUNCTION_MARKER(0x800D09B0, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    uint32 *ot = cprim->LastPrim + otz;
    DR_MODE *prim = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    SetDrawMode(prim, 0, dither, 0x100, 0);
}

void Hrz::BuildSky(void)
{
    FUNCTION_MARKER(0x800D0A38, "NFS4.EXE");
    sint32 pshift = GameSetup::gData.commMode == 1 ? 60 : 120;
    Draw::SkyCache *sky = &gNativeRenderCache.sky;
    Draw::DCache *sd = &gNativeRenderCache.horizonDraw;
    DVECTOR *pSkyMesh = sky->pSkyMesh;
    sint32 *pSkyZ = sky->pSkyZ;
    sint32 perspective = (Sky::gTrackSpec->flags & 1) != 0;
    coorddef trans;
    memset(&trans, 0, sizeof(trans));
    HrzSetPsxTranslation(&trans);
    if (perspective != 0)
    {
        for (sint32 i = 0; i < 84; i += 3)
        {
            gte_ldv3(&gSkyMesh[i], &gSkyMesh[i + 1], &gSkyMesh[i + 2]);
            gte_rtpt();
            gte_stsxy3(&pSkyMesh[i], &pSkyMesh[i + 1], &pSkyMesh[i + 2]);
            gte_swc2(17, &pSkyZ[i]);
            gte_swc2(18, &pSkyZ[i + 1]);
            gte_swc2(19, &pSkyZ[i + 2]);
        }
        gte_ldv0(&gSkyMesh[84]);
        gte_rtps();
        gte_stsxy(&pSkyMesh[84]);
        gte_stsz(&pSkyZ[84]);
    }
    else
    {
        for (sint32 i = 0; i < 85; ++i)
        {
            VECTOR transformed;
            gte_ldv0(&gSkyMesh[i]);
            gte_rtps();
            gte_stlvnl(&transformed);
            gte_stsxy(&pSkyMesh[i]);
            pSkyMesh[i].vy = (sint16)((transformed.vy >> 2) + pshift);
            gte_stsz(&pSkyZ[i]);
        }
    }
    if ((Sky::gTrackSpec->flags & 0x40) != 0)
        SetDitheringPrim(0, (sint32)((uint32)Draw::gViewOtSize - 2U));
    if ((TrackSpec::gSpec.skyspec.flags & 4) != 0)
    {
        sint32 otz_old;
        memcpy(&otz_old, &sd->otz, 4);
        Flare::Sun(&sunPosInSky, reinterpret_cast<Draw::FlareCache *>(sd));
        memcpy(&sd->otz, &otz_old, 4);
    }
    for (sint32 i = 0; i < 64; ++i)
    {
        sint32 temp = (i * 17) / 16;
        if (pSkyZ[temp + 17] == 0 && pSkyZ[temp + 18] == 0 &&
            pSkyZ[temp + 1] == 0 && pSkyZ[temp] == 0)
            continue;
        sint16 clipW = sd->head.clipW;
        if (pSkyMesh[temp + 17].vx > clipW && pSkyMesh[temp + 18].vx > clipW &&
            pSkyMesh[temp + 1].vx > clipW && pSkyMesh[temp].vx > clipW)
            continue;
        if (pSkyMesh[temp + 17].vx < 0 && pSkyMesh[temp + 18].vx < 0 &&
            pSkyMesh[temp + 1].vx < 0 && pSkyMesh[temp].vx < 0)
            continue;
        sint16 clipH = sd->head.clipH;
        if (pSkyMesh[temp + 17].vy > clipH && pSkyMesh[temp + 18].vy > clipH &&
            pSkyMesh[temp + 1].vy > clipH && pSkyMesh[temp].vy > clipH)
            continue;
        if (pSkyMesh[temp + 17].vy < 0 && pSkyMesh[temp + 18].vy < 0 &&
            pSkyMesh[temp + 1].vy < 0 && pSkyMesh[temp].vy < 0)
            continue;
        Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
        if (Sky::gTrackSpec->type == 1)
        {
            if ((Sky::gTrackSpec->flags & 0x20) != 0)
            {
                POLY_GT4 *prim = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
                Draw::tPixMap *pmx = gHorizonPixmap[(uint8)gSkyPixmapIndex[i]];
                uint32 *ot = cprim->LastPrim + Draw::gViewOtSize - 2;
                setaddr(prim, getaddr(ot));
                setaddr(ot, prim);
                cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
                memcpy(&prim->p[0].rgbc, &gSkyColor[temp + 17], 4);
                memcpy(&prim->p[1].rgbc, &gSkyColor[temp + 18], 4);
                memcpy(&prim->p[2].rgbc, &gSkyColor[temp], 4);
                setlen(prim, 12);
                prim->p[0].rgbc.code = 0x3C;
                memcpy(&prim->p[3].rgbc, &gSkyColor[temp + 1], 4);
                memcpy(&prim->p[0].uv, &pmx->u0, 4);
                memcpy(&prim->p[1].uv, &pmx->u1, 4);
                memcpy(&prim->p[2].uv, &pmx->u2, 4);
                memcpy(&prim->p[3].uv, &pmx->u3, 4);
                memcpy(&prim->p[0].xy, &pSkyMesh[temp + 17], 4);
                memcpy(&prim->p[1].xy, &pSkyMesh[temp + 18], 4);
                memcpy(&prim->p[2].xy, &pSkyMesh[temp], 4);
                memcpy(&prim->p[3].xy, &pSkyMesh[temp + 1], 4);
            }
            else
            {
                POLY_FT4 *prim = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
                Draw::tPixMap *pmx = gHorizonPixmap[(uint8)gSkyPixmapIndex[i]];
                uint32 *ot = cprim->LastPrim + Draw::gViewOtSize - 2;
                setaddr(prim, getaddr(ot));
                cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
                setaddr(ot, prim);
                setlen(prim, 9);
                memcpy(&prim->rgbc, &Sky::gTrackSpec->frontcolors[0], 4);
                prim->rgbc.code = 0x2C;
                memcpy(&prim->p[0].uv, &pmx->u0, 4);
                memcpy(&prim->p[1].uv, &pmx->u1, 4);
                memcpy(&prim->p[2].uv, &pmx->u2, 4);
                memcpy(&prim->p[3].uv, &pmx->u3, 4);
                memcpy(&prim->p[0].xy, &pSkyMesh[temp + 17], 4);
                memcpy(&prim->p[1].xy, &pSkyMesh[temp + 18], 4);
                memcpy(&prim->p[2].xy, &pSkyMesh[temp], 4);
                memcpy(&prim->p[3].xy, &pSkyMesh[temp + 1], 4);
            }
        }
        else
        {
            POLY_G4 *prim = reinterpret_cast<POLY_G4 *>(cprim->PrimPtr);
            uint32 *ot = cprim->LastPrim + Draw::gViewOtSize - 2;
            setaddr(prim, getaddr(ot));
            setaddr(ot, prim);
            cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
            memcpy(&prim->p[0].rgbc, &gSkyColor[temp + 17], 4);
            memcpy(&prim->p[1].rgbc, &gSkyColor[temp + 18], 4);
            memcpy(&prim->p[2].rgbc, &gSkyColor[temp], 4);
            setlen(prim, 8);
            prim->p[0].rgbc.code = 0x38;
            memcpy(&prim->p[3].rgbc, &gSkyColor[temp + 1], 4);
            memcpy(&prim->p[0].xy, &pSkyMesh[temp + 17], 4);
            memcpy(&prim->p[1].xy, &pSkyMesh[temp + 18], 4);
            memcpy(&prim->p[2].xy, &pSkyMesh[temp], 4);
            memcpy(&prim->p[3].xy, &pSkyMesh[temp + 1], 4);
        }
    }
    if ((Sky::gTrackSpec->flags & 0x40) != 0)
        SetDitheringPrim(1, (sint32)((uint32)Draw::gViewOtSize - 2U));
}

void Sky::RenderStars(Draw::SkyCache *sd, sint32 otz)
{
    FUNCTION_MARKER(0x800D1160, "NFS4.EXE");
    sint32 pshift = GameSetup::gData.commMode == 1 ? 60 : 120;
    SVECTOR *pcnt = starPosInSky;
    coorddef trans;
    memset(&trans, 0, sizeof(trans));
    HrzSetPsxTranslation(&trans);
    for (sint32 n = 0; n < gTrackSpec->numStars; ++n)
    {
        gte_ldv0(pcnt);
        gte_rtps();
        ++pcnt;
        sint32 zcnt;
        gte_stsz(&zcnt);
        if (zcnt > 0)
        {
            DVECTOR scnt;
            VECTOR transformed;
            gte_stsxy(&scnt);
            gte_stlvnl(&transformed);
            scnt.vy = (sint16)((transformed.vy >> 2) + pshift);
            if (scnt.vx <= sd->head.clipW && scnt.vx >= 0 &&
                scnt.vy <= sd->head.clipH && scnt.vy >= 0)
            {
                Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
                uint32 *ot = cprim->LastPrim + otz;
                TILE_1 *prim = reinterpret_cast<TILE_1 *>(cprim->PrimPtr);
                setaddr(prim, getaddr(ot));
                cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
                setaddr(ot, prim);
                uint32 color = starColors[n];
                setlen(prim, 2);
                memcpy(&prim->rgbc, &color, 4);
                prim->rgbc.code = 0x68;
                memcpy(&prim->p[0], &scnt, 4);
            }
        }
    }
}

void Hrz::BuildHorizon(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800D131C, "NFS4.EXE");
    sint32 fxOverlapPercentage = 0x107AE;
    coorddef trans;
    memset(&trans, 0, sizeof(trans));
    Draw::HorizonCache *hsd = &gNativeRenderCache.horizon;
    sint32 hrz_projchange = 0;
    if (Camera::gGeomScreen > 700)
    {
        hrz_projchange = Camera::gGeomScreen;
        SetGeomScreen(700);
    }
    HrzSetPsxMatrix(&Vi->cview.mrotationInv);
    HrzSetPsxTranslation(&trans);
    RotProj16(17, gRngCoordTop, hsd->vertexTop, hsd->scptMidBot);
    sint32 farI = 0;
    sint32 Zmax = 0;
    for (sint32 i = 0; i < 16; ++i)
    {
        if (Zmax < hsd->vertexTop[i])
        {
            Zmax = hsd->vertexTop[i];
            farI = i;
        }
    }
    SVECTOR updown[2];
    updown[0].vx = gRngCoordTop[farI].vx;
    updown[0].vy = (sint16)((uint16)gTrackSpec->yoffset + (uint16)gTrackSpec->height);
    updown[0].vz = gRngCoordTop[farI].vz;
    updown[1].vx = gRngCoordTop[farI].vx;
    updown[1].vy = (sint16)gTrackSpec->yoffset;
    updown[1].vz = gRngCoordTop[farI].vz;
    DVECTOR temp2d[2];
    gte_ldv0(&updown[1]);
    gte_rtps();
    gte_stsxy(&temp2d[1]);
    gte_ldv0(&updown[0]);
    gte_rtps();
    gte_stsxy(&temp2d[0]);
    sint32 dx = (sint32)temp2d[0].vx - (sint32)hsd->scptMidBot[farI].vx;
    sint32 dy = (sint32)temp2d[0].vy - (sint32)hsd->scptMidBot[farI].vy;
    for (sint32 i = 0; i < 17; ++i)
    {
        if (hsd->vertexTop[i] > 0)
        {
            hsd->scptTop[i].vx = (sint16)((uint16)hsd->scptMidBot[i].vx + dx);
            hsd->scptTop[i].vy = (sint16)((uint16)hsd->scptMidBot[i].vy + dy);
        }
    }
    dx = (sint32)temp2d[1].vx - (sint32)hsd->scptMidBot[farI].vx;
    dy = (sint32)temp2d[1].vy - (sint32)hsd->scptMidBot[farI].vy;
    for (sint32 i = 0; i < 17; ++i)
    {
        if (hsd->vertexTop[i] > 0)
        {
            hsd->scptMidBot[i].vx = (sint16)((uint16)hsd->scptMidBot[i].vx + dx);
            hsd->scptMidBot[i].vy = (sint16)((uint16)hsd->scptMidBot[i].vy + dy);
        }
    }
    Horizon_InterpolateLineSCoords(hsd->scptPmxLeft, hsd->scptMidBot, hsd->scptTop, gfxPmxHeightPercentage, 16, 1);
    Horizon_InterpolateLineSCoords(hsd->scptPmxRight, hsd->scptMidBot + 1, hsd->scptTop + 1, gfxPmxHeightPercentage, 16, 1);
    if (TrackSpec::gSpec.horizonstate != 0)
    {
        for (sint32 i = 0; i < 16; ++i)
        {
            if (hsd->vertexTop[i] < 16000 && hsd->vertexTop[i + 1] < 16000)
                continue;
            DVECTOR mpts[4];
            memcpy(&mpts[0], &hsd->scptPmxLeft[i], 4);
            memcpy(&mpts[1], &hsd->scptPmxRight[i], 4);
            memcpy(&mpts[2], &hsd->scptMidBot[i + 1], 4);
            memcpy(&mpts[3], &hsd->scptMidBot[i], 4);
            if (mpts[0].vx < 0 && mpts[1].vx < 0 && mpts[2].vx < 0 && mpts[3].vx < 0)
                continue;
            sint16 clipW = hsd->head.clipW;
            if (mpts[0].vx > clipW && mpts[1].vx > clipW && mpts[2].vx > clipW && mpts[3].vx > clipW)
                continue;
            if (mpts[0].vy < 0 && mpts[1].vy < 0 && mpts[2].vy < 0 && mpts[3].vy < 0)
                continue;
            sint16 clipH = hsd->head.clipH;
            if (mpts[0].vy > clipH && mpts[1].vy > clipH && mpts[2].vy > clipH && mpts[3].vy > clipH)
                continue;
            DVECTOR right;
            Horizon_InterpolateLineSCoords(&right, &hsd->scptPmxLeft[i], &hsd->scptPmxRight[i], &fxOverlapPercentage, 1, 0);
            Draw::tPixMap *pmx = gpPmx[i];
            if ((uint8)gTrackSpec->ringPMX[i] == 16)
                continue;
            Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
            uint32 *ot = cprim->LastPrim + Draw::gViewOtSize - 2;
            POLY_GT4 *prim = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
            setaddr(prim, getaddr(ot));
            setaddr(ot, prim);
            cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
            memcpy(&prim->p[0].rgbc, &gHrzRingColor[1][i], 4);
            memcpy(&prim->p[1].rgbc, &gHrzRingColor[1][i + 1], 4);
            memcpy(&prim->p[2].rgbc, &gHrzRingColor[0][i], 4);
            memcpy(&prim->p[3].rgbc, &gHrzRingColor[0][i + 1], 4);
            SetPolyGT4(prim);
            memcpy(&prim->p[0].uv, &pmx->u0, 4);
            memcpy(&prim->p[1].uv, &pmx->u1, 4);
            memcpy(&prim->p[2].uv, &pmx->u2, 4);
            memcpy(&prim->p[3].uv, &pmx->u3, 4);
            if ((uint8)gTrackSpec->ringPMX[i] < 8)
            {
                memcpy(&prim->p[0].xy, &hsd->scptPmxLeft[i], 4);
                memcpy(&prim->p[1].xy, &right, 4);
                memcpy(&prim->p[2].xy, &hsd->scptMidBot[i], 4);
                memcpy(&prim->p[3].xy, &hsd->scptMidBot[i + 1], 4);
            }
            else
            {
                memcpy(&prim->p[0].xy, &right, 4);
                memcpy(&prim->p[1].xy, &hsd->scptPmxLeft[i], 4);
                memcpy(&prim->p[2].xy, &hsd->scptMidBot[i + 1], 4);
                memcpy(&prim->p[3].xy, &hsd->scptMidBot[i], 4);
            }
        }
    }
    if (Night::gLightning != 0 && Night::gShowForks != 0)
        BuildForkLightning(&gNativeRenderCache.horizonDraw);
    if ((TrackSpec::gSpec.skyspec.flags & 8) != 0)
    {
        coorddef trans2;
        memset(&trans2, 0, sizeof(trans2));
        HrzSetPsxTranslation(&trans2);
        Flare::Moon(&moonPosInSky, reinterpret_cast<Draw::FlareCache *>(hsd));
    }
    if ((TrackSpec::gSpec.skyspec.flags & 0x10) != 0)
        Sky::RenderStars(&gNativeRenderCache.sky, (sint32)((uint32)Draw::gViewOtSize - 2U));
    if (TrackSpec::gSpec.skystate != 0)
        BuildSky();
    if (hrz_projchange != 0)
        SetGeomScreen(hrz_projchange);
}
