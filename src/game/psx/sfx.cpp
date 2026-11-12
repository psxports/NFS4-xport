#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "psx_gte.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/bworld.h"
#include "game/common/genericpmx.h"
#include "game/common/gmesetup.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"
#include "game/psx/rendercache.h"
#include "game/psx/sfx.h"
#include "game/psx/trsproj.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/random.h"
#include "lib/psx/sinfunc.h"

struct sfxsouffle
{
    Draw::tPixMap pmx;
    SVECTOR v0;
    SVECTOR v1;
    SVECTOR v2;
    SVECTOR v3;
};

static void BuildSfxQuad(Souffle::tISouffle *is, sfxsouffle *dSouffle, sint32 radius)
{
    sint32 cosa = fixedmult(fastintcos(is->angle), radius);
    sint32 sina = fixedmult(fastintsin(is->angle), radius);
    dSouffle->v0.vx = (sint16)(is->trans.vx - sina);
    dSouffle->v0.vy = (sint16)(is->trans.vy + cosa);
    dSouffle->v0.vz = is->trans.vz;
    dSouffle->v1.vx = (sint16)(is->trans.vx + cosa);
    dSouffle->v1.vy = (sint16)(is->trans.vy + sina);
    dSouffle->v1.vz = is->trans.vz;
    dSouffle->v2.vx = (sint16)(is->trans.vx + sina);
    dSouffle->v2.vy = (sint16)(is->trans.vy - cosa);
    dSouffle->v2.vz = is->trans.vz;
    dSouffle->v3.vx = (sint16)(is->trans.vx - cosa);
    dSouffle->v3.vy = (sint16)(is->trans.vy - sina);
    dSouffle->v3.vz = is->trans.vz;
}

static void WriteSfxUv(POLY_FT4 *prim, Draw::tPixMap *pmx)
{
    uint32 l0, l1, l2, l3;
    memcpy(&l0, (const uint8 *)pmx, 4);
    memcpy(&l1, (const uint8 *)pmx + 4, 4);
    memcpy(&l2, (const uint8 *)pmx + 8, 4);
    memcpy(&l3, (const uint8 *)pmx + 12, 4);
    memcpy(&prim->p[0].uv_attr, &l0, 4);
    memcpy(&prim->p[1].uv_attr, &l1, 4);
    memcpy(&prim->p[2].uv_attr, &l2, 4);
    memcpy(&prim->p[3].uv_attr, &l3, 4);
}

static void LinkSfxQuad(Sfx::tCache *sd)
{
    Draw::PrimStruct &active = gNativeRenderCache.sfx.head.cprim;
    POLY_FT4 *prim = (POLY_FT4 *)active.PrimPtr;
    uint32 *last = active.LastPrim;
    uint32 *ot = last + sd->otz;
    setaddr(prim, getaddr(ot));
    ot = last + sd->otz;
    active.PrimPtr = (char *)(prim + 1);
    setaddr(ot, prim);
}

void Sfx::Transform(coorddef *worldpt, SVECTOR *campt, coorddef *t)
{
    FUNCTION_MARKER(0x800DD050, "NFS4.EXE");
    TrsProj::SetPsxMatrix(&gWorldMat, 0);
    SVECTOR pt;
    pt.vx = (sint16)((worldpt->x - (uint32)t->x) >> 10);
    pt.vy = (sint16)((worldpt->y - (uint32)t->y) >> 10);
    pt.vz = (sint16)((worldpt->z - (uint32)t->z) >> 10);
    gte_ldv0(&pt);
    gte_rt();
    VECTOR campt32;
    gte_stlvnl(&campt32);
    campt->vx = (sint16)campt32.vx;
    campt->vy = (sint16)campt32.vy;
    campt->vz = (sint16)campt32.vz;
    MATRIX ident = { 0 };
    ident.m[0][0] = 4096;
    ident.m[1][1] = 4096;
    ident.m[2][2] = 4096;
    gte_SetRotMatrix(&ident);
    gte_SetTransMatrix(&ident);
}

void Sfx::BuildSmokeFacet(Souffle::tISouffle *is, sfxsouffle *dSouffle, Draw::tPixMap *cpixmap)
{
    FUNCTION_MARKER(0x800DD1A4, "NFS4.EXE");
    sint32 radius = (31 - (uint8)is->cycle) * 4 + 38;
    sint32 cosa = fixedmult(fastintcos(is->angle), radius);
    sint32 sina = fixedmult(fastintsin(is->angle), radius);
    dSouffle->v0.vx = (sint16)(is->trans.vx - sina);
    dSouffle->v0.vy = (sint16)(is->trans.vy + cosa);
    dSouffle->v0.vz = is->trans.vz;
    dSouffle->v1.vx = (sint16)(is->trans.vx + cosa);
    dSouffle->v1.vy = (sint16)(is->trans.vy + sina);
    dSouffle->v1.vz = is->trans.vz;
    dSouffle->v2.vx = (sint16)(is->trans.vx + sina);
    dSouffle->v2.vy = (sint16)(is->trans.vy - cosa);
    dSouffle->v2.vz = is->trans.vz;
    dSouffle->v3.vx = (sint16)(is->trans.vx - cosa);
    dSouffle->v3.vy = (sint16)(is->trans.vy - sina);
    dSouffle->v3.vz = is->trans.vz;
    sint32 iclut = (31 - (uint8)is->cycle) >> 1;
    if (iclut >= 16)
        iclut = 15;
    Draw::tPixMap texture;
    memcpy(&texture, gSMokePixmap[is->rndpixmap], sizeof(texture));
    memcpy(&dSouffle->pmx, &texture, sizeof(texture));
    dSouffle->pmx.clut = (uint16)(cpixmap->clut + iclut);
}

void Sfx::ThickenXZ(SVECTOR *d, coorddef *pt1, coorddef *pt2, coorddef *cp)
{
    FUNCTION_MARKER(0x800DD370, "NFS4.EXE");
    sint32 angle = intatan(((pt2->x - pt1->x)), (pt2->z - pt1->z));
    sint32 pxp = fixedmult(0xE666, (-fastintcos(angle))) >> 1;
    sint32 pzp = fixedmult(0xE666, fastintsin(angle)) >> 1;
    d[1].vx = (sint16)((pt1->x + (uint32)pxp - (uint32)cp->x) >> 10);
    d[1].vy = (sint16)((pt1->y - (uint32)cp->y) >> 10);
    d[1].vz = (sint16)((pt1->z + (uint32)pzp - (uint32)cp->z) >> 10);
    d[2].vx = (sint16)((pt1->x - (uint32)pxp - (uint32)cp->x) >> 10);
    d[2].vy = (sint16)((pt1->y - (uint32)cp->y) >> 10);
    d[2].vz = (sint16)((pt1->z - (uint32)pzp - (uint32)cp->z) >> 10);
    d[0].vx = (sint16)((pt2->x + (uint32)pxp - (uint32)cp->x) >> 10);
    d[0].vy = (sint16)((pt2->y - (uint32)cp->y) >> 10);
    d[0].vz = (sint16)((pt2->z + (uint32)pzp - (uint32)cp->z) >> 10);
    d[3].vx = (sint16)((pt2->x - (uint32)pxp - (uint32)cp->x) >> 10);
    d[3].vy = (sint16)((pt2->y - (uint32)cp->y) >> 10);
    d[3].vz = (sint16)((pt2->z - (uint32)pzp - (uint32)cp->z) >> 10);
}

void Sfx::BuildFastDisolveFacet(Souffle::tISouffle *is, sfxsouffle *dSouffle, Draw::tPixMap *t, Draw::tPixMap *c)
{
    FUNCTION_MARKER(0x800DD528, "NFS4.EXE");
    sint32 iclut = 8 - (uint8)is->cycle;
    if (iclut >= 16)
        iclut = 15;
    Draw::tPixMap texture;
    memcpy(&texture, t, sizeof(texture));
    memcpy(&dSouffle->pmx, &texture, sizeof(texture));
    dSouffle->pmx.clut = (uint16)(c->clut + iclut);
}

void Sfx::AdditivePrim(Draw::tPixMap *pmx, SVECTOR *pt, sint32 mode, sint32 offset, Sfx::tCache *sd)
{
    FUNCTION_MARKER(0x800DD598, "NFS4.EXE");
    if ((uintptr_t)sd->head.cprim.PrimPtr >= (uintptr_t)sd->head.cprim.MPrimPtr)
        return;
    Draw::PrimStruct &active = gNativeRenderCache.sfx.head.cprim;
    gte_ldv0(&pt[0]);
    gte_rtps();
    POLY_FT4 *prim = (POLY_FT4 *)active.PrimPtr;
    gte_stsxy(&prim->p[0].xy);
    gte_ldv3(&pt[1], &pt[2], &pt[3]);
    gte_rtpt();
    uint32 colour;
    if (mode & 2)
        colour = 0x2E5898B8;
    else
        colour = GameSetup::gData.Time == 0 ? 0x2E181818 : 0x2E080808;
    memcpy(&prim->rgbc, &colour, sizeof(colour));
    gte_stsxy3(&prim->p[1].xy, &prim->p[3].xy, &prim->p[2].xy);
    gte_avsz4();
    gte_stotz(&sd->otz);
    sd->otz = (((sd->otz >> 1)) + offset);
    sint32 limit = (Draw::gViewOtSize - 3);
    if (sd->otz < 0 || limit < sd->otz)
        return;
    setlen(prim, 9);
    uint32 l0, l1, l2, l3;
    memcpy(&l0, (const uint8 *)pmx, 4);
    memcpy(&l1, (const uint8 *)pmx + 4, 4);
    memcpy(&l2, (const uint8 *)pmx + 8, 4);
    memcpy(&l3, (const uint8 *)pmx + 12, 4);
    memcpy(&prim->p[0].uv_attr, &l0, 4);
    memcpy(&prim->p[1].uv_attr, &l1, 4);
    memcpy(&prim->p[2].uv_attr, &l2, 4);
    memcpy(&prim->p[3].uv_attr, &l3, 4);
    uint16 tpage = pmx->tpage;
    ChangeTPage(&tpage, (mode & 1) ? 2 : 1);
    prim->p[1].attr = tpage;
    POLY_FT4 *linkPrim = (POLY_FT4 *)active.PrimPtr;
    uint32 *last = active.LastPrim;
    uint32 *ot = last + sd->otz;
    setaddr(linkPrim, getaddr(ot));
    ot = last + sd->otz;
    active.PrimPtr = (char *)(linkPrim + 1);
    setaddr(ot, linkPrim);
}

void Sfx::BuildSouffleFacet(DRender::tView *Vi, Souffle::tISouffle *is)
{
    FUNCTION_MARKER(0x800DD790, "NFS4.EXE");
    sfxsouffle dSouffle;
    tCache *sd = &gNativeRenderCache.sfx;
    sint32 type = (uint8)is->type;
    switch (type)
    {
    case 1:
        BuildSmokeFacet(is, &dSouffle, gSMokePalette);
        AdditivePrim(&dSouffle.pmx, &dSouffle.v0, 0, 15, sd);
        return;
    case 2:
        BuildSmokeFacet(is, &dSouffle, gSMokePalette);
        if ((uint8)is->cycle < 18)
            is->type = 1;
        AdditivePrim(&dSouffle.pmx, &dSouffle.v0, 0, 15, sd);
        return;
    case 3:
        BuildSmokeFacet(is, &dSouffle, gSMokePalette);
        AdditivePrim(&dSouffle.pmx, &dSouffle.v0, 1, 15, sd);
        return;
    case 4:
        {
            coorddef invertedm = is->motion;
            Math::NormalizeVector(&invertedm);
            invertedm.x = (is->source.x - ((uint32)invertedm.x << 2));
            invertedm.y = (is->source.y - ((uint32)invertedm.y << 2));
            invertedm.z = (is->source.z - ((uint32)invertedm.z << 2));
            SVECTOR ptrans;
            Transform(&invertedm, &ptrans, &Vi->cview.translation);
            if (ptrans.vz < 64)
                return;
            sint32 dx = ptrans.vx - is->trans.vx;
            if (dx <= 0)
                dx = is->trans.vx - ptrans.vx;
            if (dx < 32)
                ptrans.vx = (sint16)(is->trans.vx + (ptrans.vx > is->trans.vx ? 32 : -32));
            dSouffle.v0.vx = is->trans.vx;
            dSouffle.v0.vy = (sint16)(is->trans.vy + 32);
            dSouffle.v0.vz = is->trans.vz;
            dSouffle.v3.vx = is->trans.vx;
            dSouffle.v3.vy = (sint16)(is->trans.vy - 32);
            dSouffle.v3.vz = is->trans.vz;
            dSouffle.v1.vx = ptrans.vx;
            dSouffle.v1.vy = (sint16)(ptrans.vy + 32);
            dSouffle.v1.vz = ptrans.vz;
            dSouffle.v2.vx = ptrans.vx;
            dSouffle.v2.vy = (sint16)(ptrans.vy - 32);
            dSouffle.v2.vz = ptrans.vz;
            Draw::tPixMap texture;
            memcpy(&texture, gSparkHPixmap[6 - (uint8)is->cycle], sizeof(texture));
            memcpy(&dSouffle.pmx, &texture, sizeof(texture));
            AdditivePrim(&dSouffle.pmx, &dSouffle.v0, 2, 40, sd);
            return;
        }
    case 6:
    case 7:
    case 9:
    case 11:
        {
            sint32 radius;
            if (type == 6)
                radius = 136 - (uint8)is->cycle;
            else
                radius = (8 - (uint8)is->cycle) * 4 + (type == 11 ? 12 : 25);
            BuildSfxQuad(is, &dSouffle, radius);
            Draw::tPixMap *palette;
            Draw::tPixMap *texture;
            if (type == 6)
            {
                palette = gDirtPalette;
                texture = gSMokePixmap[is->rndpixmap];
            }
            else if (type == 7)
            {
                palette = gGrassPalette;
                texture = gGravelPixmap[is->rndpixmap];
            }
            else if (type == 9)
            {
                palette = gSnowPalette;
                texture = gGravelPixmap[is->rndpixmap];
            }
            else
            {
                palette = gSnowPalette;
                texture = gSMokePixmap[is->rndpixmap];
            }
            BuildFastDisolveFacet(is, &dSouffle, texture, palette);
            AdditivePrim(&dSouffle.pmx, &dSouffle.v0, 0, 15, sd);
            return;
        }
    case 8:
    case 10:
        {
            uint32 colour = type == 8 ? 0x2E301818 : 0x2E181010;
            coorddef invertedm = is->motion;
            Math::NormalizeVector(&invertedm);
            coorddef source = is->source;
            sint32 scale = (sint16)(is->rndpixmap >> 16);
            invertedm.x = (source.x - ((sint32)((uint32)invertedm.x * (uint32)scale) >> 4));
            invertedm.y = (source.y - ((sint32)((uint32)invertedm.y * (uint32)scale) >> 4));
            invertedm.z = (source.z - ((sint32)((uint32)invertedm.z * (uint32)scale) >> 4));
            SVECTOR dest[4];
            ThickenXZ(dest, &invertedm, &source, &Vi->cview.translation);
            TrsProj::SetPsxMatrix(&gWorldMat, 0);
            Draw::PrimStruct &active = sd->head.cprim;
            if ((uintptr_t)active.PrimPtr >= (uintptr_t)active.MPrimPtr)
                return;
            gte_ldv0(&dest[1]);
            gte_rtps();
            POLY_FT4 *prim = (POLY_FT4 *)active.PrimPtr;
            VECTOR check;
            gte_stlvnl(&check);
            if (check.vz < 32)
                return;
            gte_stsxy(&prim->p[1].xy);
            gte_ldv3(&dest[0], &dest[2], &dest[3]);
            gte_rtpt();
            memcpy(&prim->rgbc, &colour, sizeof(colour));
            gte_stsxy3(&prim->p[0].xy, &prim->p[3].xy, &prim->p[2].xy);
            gte_avsz4();
            gte_stotz(&sd->otz);
            sd->otz = (sd->otz >> 1) + 50;
            if (sd->otz < 0 || (Draw::gViewOtSize - 3) < sd->otz)
                return;
            setlen(prim, 9);
            Draw::tPixMap *pmx = gSparkHPixmap[6 - (uint8)is->cycle];
            WriteSfxUv(prim, pmx);
            uint16 tpage = pmx->tpage;
            ChangeTPage(&tpage, type == 8 ? 2 : 1);
            prim->p[1].attr = tpage;
            LinkSfxQuad(sd);
            return;
        }
    case 13:
    case 14:
        {
            BuildSfxQuad(is, &dSouffle, 6);
            Draw::PrimStruct &active = sd->head.cprim;
            if ((uintptr_t)active.PrimPtr >= (uintptr_t)active.MPrimPtr)
                return;
            gte_ldv0(&dSouffle.v0);
            gte_rtps();
            POLY_FT4 *prim = (POLY_FT4 *)active.PrimPtr;
            gte_stsxy(&prim->p[0].xy);
            gte_ldv3(&dSouffle.v1, &dSouffle.v2, &dSouffle.v3);
            gte_rtpt();
            uint32 colour = is->colour | 0x2C000000;
            memcpy(&prim->rgbc, &colour, sizeof(colour));
            gte_stsxy3(&prim->p[1].xy, &prim->p[3].xy, &prim->p[2].xy);
            gte_avsz4();
            gte_stotz(&sd->otz);
            sd->otz = (sd->otz >> 1) + 15;
            if (sd->otz < 0 || (Draw::gViewOtSize - 3) < sd->otz)
                return;
            setlen(prim, 9);
            WriteSfxUv(prim, gLeafPixmap);
            LinkSfxQuad(sd);
            return;
        }
    }
}

void Sfx::Add(Souffle::tISouffle *is)
{
    FUNCTION_MARKER(0x800DE638, "NFS4.EXE");
    switch ((uint8)is->type)
    {
    case 1:
    case 2:
    case 3:
        is->cycle = 31;
        is->rndpixmap = random() & 1;
        break;
    case 4:
    case 8:
    case 10:
        is->cycle = 6;
        break;
    case 6:
    case 7:
    case 9:
        is->cycle = 8;
        is->rndpixmap = random() & 1;
        break;
    case 11:
        is->rndpixmap = random() & 1;
        is->cycle = 2;
        break;
    case 13:
        is->cycle = 64;
        {
            sint32 z = is->motion.z;
            sint32 x = is->motion.x;
            is->extramotion.y = 0;
            is->extramotion.x = (-z);
            is->extramotion.z = x;
        }
        break;
    }
}
