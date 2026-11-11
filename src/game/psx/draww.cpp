#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "psx_gte.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/anim.h"
#include "game/common/bworld.h"
#include "game/common/bworldsm.h"
#include "game/common/cameraobj.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/chunk.h"
#include "game/common/genericpmx.h"
#include "game/common/gmesetup.h"
#include "game/common/group.h"
#include "game/common/object.h"
#include "game/common/quatern.h"
#include "game/common/renderview.h"
#include "game/common/sim.h"
#include "game/common/track.h"
#include "game/common/trk.h"
#include "game/psx/draw.h"
#include "game/psx/draww.h"
#include "game/psx/flare.h"
#include "game/psx/night.h"
#include "game/psx/rendercache.h"
#include "game/psx/skidmark.h"
#include "game/psx/textureprocess.h"
#include "game/psx/trackspec.h"
#include "game/psx/trsproj.h"
#include "lib/psx/fixdmult.h"

struct ChunkObjectInfo
{
    Trk::SimObject *simObjs;
    Group *objInstanceBuf;
    sint32 doFrustumClip;
    sint32 zClipSq;
    sint32 offset;
    sint16 *visList;
};

struct tBuildEntry

{
    sint16 chunkInd;

    char geomRez;

    char enableBits;
};

static char goffsets[8] = { 125, 125, 50, 15, 255, 125, 0, 0 };
uint32 gWSavePtr;
sint32 gSD_gt4counter;
sint32 gSD_gt3counter;
sint32 stackSpeedUpEnbabledFlag;
Draw::SubdivStruct gDiv;
ChunkObjectInfo gChunkObjInfo;
DRender::tView *gVi;
MATRIX gIdentTemplate = { {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}}, {0, 0, 0} };
sint32 animation_timer[12];

sint32 trk0[9][2] =
{
    { 410, 530 },
    { 800, 850 },
    { 800, 850 },
    { 800, 850 },
    { 800, 850 },
    { 800, 850 },
    { 815, 885 },
    { 815, 885 },
    { 815, 885 }
};

sint32 trk4[10][2] =
{
    { 300, 440 },
    { 300, 440 },
    { 300, 440 },
    { 300, 440 },
    { 300, 440 },
    { 705, 910 },
    { 705, 910 },
    { 705, 910 },
    { 705, 910 },
    { 705, 910 }
};

CCOORD16 gVertex3d[160];

void DrawW::AddSubdividPrimGT4(POLY_GT4 *prim, Draw::SVertex *v0, Draw::SVertex *v1, Draw::SVertex *v2, Draw::SVertex *v3, Draw::tGiveShelbyMoreCache *sd)
{
    FUNCTION_MARKER(0x800C5028, "NFS4.EXE");
    setlen(prim, 12);
    {
        uint32 a, b, c, d;
        memcpy(&a, &v0->r, sizeof(a));
        memcpy(&b, &v1->r, sizeof(b));
        memcpy(&c, &v2->r, sizeof(c));
        memcpy(&d, &v3->r, sizeof(d));
        prim->p[0].rgbc.rgbc = a;
        prim->p[1].rgbc.rgbc = b;
        prim->p[3].rgbc.rgbc = c;
        prim->p[2].rgbc.rgbc = d;
    }
    {
        uint32 a, b, c, d;
        memcpy(&a, &v0->dvx, sizeof(a));
        memcpy(&b, &v1->dvx, sizeof(b));
        memcpy(&c, &v2->dvx, sizeof(c));
        memcpy(&d, &v3->dvx, sizeof(d));
        prim->p[0].xy.xy = a;
        prim->p[1].xy.xy = b;
        prim->p[3].xy.xy = c;
        prim->p[2].xy.xy = d;
    }
    {
        uint16 a, b, c, d;
        memcpy(&a, &v0->u, sizeof(a));
        memcpy(&b, &v1->u, sizeof(b));
        memcpy(&c, &v2->u, sizeof(c));
        memcpy(&d, &v3->u, sizeof(d));
        prim->p[0].uv.uv = a;
        prim->p[1].uv.uv = b;
        prim->p[3].uv.uv = c;
        prim->p[2].uv.uv = d;
    }
    uint8 a = sd->GT4Prim.p[0].rgbc.code;
    uint16 b = sd->GT4Prim.p[1].attr;
    uint16 c = sd->GT4Prim.p[0].attr;
    setcode(prim, a);
    prim->p[1].attr = b;
    prim->p[0].attr = c;
}

void DrawW::AddSubdividPrimGT3(POLY_GT3 *prim, Draw::SVertex *v0, Draw::SVertex *v1, Draw::SVertex *v2, Draw::tGiveShelbyMoreCache *sd)
{
    FUNCTION_MARKER(0x800C50B4, "NFS4.EXE");
    setlen(prim, 9);
    {
        uint32 a, b, c;
        memcpy(&a, &v0->r, sizeof(a));
        memcpy(&b, &v1->r, sizeof(b));
        memcpy(&c, &v2->r, sizeof(c));
        prim->p[0].rgbc.rgbc = a;
        prim->p[1].rgbc.rgbc = b;
        prim->p[2].rgbc.rgbc = c;
    }
    {
        uint32 a, b, c;
        memcpy(&a, &v0->dvx, sizeof(a));
        memcpy(&b, &v1->dvx, sizeof(b));
        memcpy(&c, &v2->dvx, sizeof(c));
        prim->p[0].xy.xy = a;
        prim->p[1].xy.xy = b;
        prim->p[2].xy.xy = c;
    }
    {
        uint16 a, b, c;
        memcpy(&a, &v0->u, sizeof(a));
        memcpy(&b, &v1->u, sizeof(b));
        memcpy(&c, &v2->u, sizeof(c));
        prim->p[0].uv.uv = a;
        prim->p[1].uv.uv = b;
        prim->p[2].uv.uv = c;
    }
    uint8 a = sd->GT4Prim.p[0].rgbc.code;
    uint16 b = sd->GT4Prim.p[1].attr;
    uint16 c = sd->GT4Prim.p[0].attr;
    setcode(prim, a);
    prim->p[1].attr = b;
    prim->p[0].attr = c;
}

void DrawW::DivVertice(Draw::SVertex *v0, Draw::SVertex *v1, Draw::SVertex *v2)
{
    FUNCTION_MARKER(0x800C5124, "NFS4.EXE");
    v0->vx = (sint16)((v1->vx + v2->vx + 1) >> 1);
    v0->vy = (sint16)((v1->vy + v2->vy + 1) >> 1);
    v0->vz = (sint16)((v1->vz + v2->vz + 1) >> 1);
    v0->u = (uint8)((v1->u + v2->u + 1) >> 1);
    v0->v = (uint8)((v1->v + v2->v + 1) >> 1);
    v0->r = (uint8)((v1->r + v2->r) >> 1);
    v0->g = (uint8)((v1->g + v2->g) >> 1);
    v0->b = (uint8)((v1->b + v2->b) >> 1);
}

sint32 DrawW::CalcSubdivision(Draw::tGiveShelbyMoreCache *sd, Draw::SVertex *v0, Draw::SVertex *v1, Draw::SVertex *v2, Draw::SVertex *v3)
{
    FUNCTION_MARKER(0x800C51FC, "NFS4.EXE");
    sint32 z0 = v0->vz;
    sint32 z1 = v1->vz;
    sint32 z2 = v2->vz;
    sint32 z3 = v3->vz;
    if (z1 < z0)
        z0 = z1;
    if (z2 < z0)
        z0 = z2;
    if (z3 < z0)
        z0 = z3;
    z0 += sd->offsubdivid;
    if (z0 > 0x800)
        return 0;
    if (z0 > 0x500)
        return 1;
    if (z0 > 0x200)
        return 2;
    return 3;
}

void DrawW::SubdividFacet(Draw::tGiveShelbyMoreCache *sd, sint32 l, Draw::SVertex *v0, Draw::SVertex *v1, Draw::SVertex *v2, Draw::SVertex *v3, sint16 n, sint16 subDivide)
{
    FUNCTION_MARKER(0x800C5284, "NFS4.EXE");
    sint32 clipW = sd->head.clipW;
    if ((v0->dvx > clipW && v1->dvx > clipW && v2->dvx > clipW && v3->dvx > clipW) ||
        (v0->dvx < 0 && v1->dvx < 0 && v2->dvx < 0 && v3->dvx < 0))
        return;
    sint32 clipH = sd->head.clipH;
    if ((v0->dvy > clipH && v1->dvy > clipH && v2->dvy > clipH && v3->dvy > clipH) ||
        (v0->dvy < 0 && v1->dvy < 0 && v2->dvy < 0 && v3->dvy < 0) ||
        (v0->vz < 0 && v1->vz < 0 && v2->vz < 0 && v3->vz < 0))
        return;
    Draw::SubdivStruct *r_div = &gDiv;
    if (l < CalcSubdivision(sd, v0, v1, v2, v3))
    {
        Draw::SVertex *v4 = &r_div->v[(sint16)n];
        Draw::SVertex *v5 = &r_div->v[(sint16)(n + 1)];
        Draw::SVertex *v6 = &r_div->v[(sint16)(n + 2)];
        Draw::SVertex *v7 = &r_div->v[(sint16)(n + 3)];
        Draw::SVertex *v8 = &r_div->v[(sint16)(n + 4)];
        n = (sint16)(n + 5);
        DivVertice(v4, v0, v1);
        gte_ldv0(v4);
        xport_gte_execute(0x180001);
        DivVertice(v5, v1, v2);
        sint32 flag;
        gte_stflg(&flag);
        v4->a = flag < 0 ? 1 : 0;
        gte_stsxy(&v4->dvx);
        gte_ldv0(v5);
        xport_gte_execute(0x180001);
        DivVertice(v6, v2, v3);
        gte_stflg(&flag);
        v5->a = flag < 0 ? 1 : 0;
        gte_stsxy(&v5->dvx);
        gte_ldv0(v6);
        xport_gte_execute(0x180001);
        DivVertice(v7, v3, v0);
        gte_stflg(&flag);
        v6->a = flag < 0 ? 1 : 0;
        gte_stsxy(&v6->dvx);
        gte_ldv0(v7);
        xport_gte_execute(0x180001);
        DivVertice(v8, v0, v2);
        gte_stflg(&flag);
        v7->a = flag < 0 ? 1 : 0;
        gte_stsxy(&v7->dvx);
        gte_ldv0(v8);
        xport_gte_execute(0x180001);
        gte_stflg(&flag);
        v8->a = flag < 0 ? 1 : 0;
        if (sd->doublelayer != 0)
        {
            sd->GT4Prim.p[0].rgbc.code &= 0xF7;
            sint32 otz = (sint32)((uint32)sd->otz + 8U);
            Draw::SVertex *edge0[4] = { v0, v1, v2, v3 };
            Draw::SVertex *edge1[4] = { v1, v2, v3, v0 };
            Draw::SVertex *midpoint[4] = { v4, v5, v6, v7 };
            for (sint32 edge = 0; edge < 4; ++edge)
            {
                if (edge0[edge]->a == 0 && edge1[edge]->a == 0 && midpoint[edge]->a == 0)
                {
                    Draw::PrimStruct *cprim = &gNativeRenderCache.world.head.cprim;
                    POLY_GT3 *prim = (POLY_GT3 *)cprim->PrimPtr;
                    uint32 *ot = cprim->LastPrim + otz;
                    setaddr(prim, getaddr(ot));
                    cprim->PrimPtr = (char *)(prim + 1);
                    setaddr(ot, prim);
                    AddSubdividPrimGT3(prim, edge0[edge], edge1[edge], midpoint[edge], sd);
                }
            }
            sd->GT4Prim.p[0].rgbc.code |= 8;
        }
        l = (sint32)((uint32)l + 1U);
        gte_stsxy(&v8->dvx);
        SubdividFacet(sd, l, v0, v4, v8, v7, n, subDivide);
        SubdividFacet(sd, l, v4, v1, v5, v8, n, subDivide);
        SubdividFacet(sd, l, v8, v5, v2, v6, n, subDivide);
        SubdividFacet(sd, l, v7, v8, v6, v3, n, subDivide);
        return;
    }
    if (subDivide != 0)
    {
        uint32 xy0, xy1, xy2;
        memcpy(&xy0, &v0->dvx, sizeof(xy0));
        memcpy(&xy2, &v2->dvx, sizeof(xy2));
        memcpy(&xy1, &v1->dvx, sizeof(xy1));
        gte_lwc2(12, xy0);
        gte_lwc2(14, xy2);
        gte_lwc2(13, xy1);
        gte_nclip();
        sint32 bfct;
        gte_swc2(24, &bfct);
        if (sd->head.mirror == 1)
            bfct = (sint32)(0U - (uint32)bfct);
        if (bfct < 0)
        {
            memcpy(&xy0, &v0->dvx, sizeof(xy0));
            memcpy(&xy2, &v2->dvx, sizeof(xy2));
            memcpy(&xy1, &v3->dvx, sizeof(xy1));
            gte_lwc2(12, xy0);
            gte_lwc2(14, xy1);
            gte_lwc2(13, xy2);
            gte_nclip();
            gte_swc2(24, &bfct);
            if (sd->head.mirror == 1)
                bfct = (sint32)(0U - (uint32)bfct);
            if (bfct < 0)
                return;
        }
    }
    POLY_GT4 *prim = (POLY_GT4 *)sd->head.cprim.PrimPtr;
    uint32 *ot = sd->head.cprim.LastPrim + sd->otz;
    sd->head.cprim.PrimPtr = (char *)(prim + 1);
    prim->tag = 0x0C000000 | getaddr(ot);
    setaddr(ot, prim);
    AddSubdividPrimGT4(prim, v0, v1, v2, v3, sd);
}

void DrawW::LoadPrecVECTOR(Draw::SVertex *v, VECTOR *dv)
{
    FUNCTION_MARKER(0x800C5BB4, "NFS4.EXE");
    uint32 x = dv->vx;
    uint32 y = dv->vy;
    uint32 z = dv->vz;
    uint32 temp = (y << 18) | ((x << 2) & 0xFFFF);
    memcpy(&v->vx, &temp, sizeof(temp));
    v->vz = (sint16)(z << 2);
}

void DrawW::SetUpSubdividFacet(sint32 face, Draw::tGiveShelbyMoreCache *sd)
{
    FUNCTION_MARKER(0x800C5BE0, "NFS4.EXE");
    Draw::SVertex *v0 = &sd->v0;
    Draw::SVertex *v1 = &sd->v1;
    Draw::SVertex *v2 = &sd->v2;
    Draw::SVertex *v3 = &sd->v3;
    LoadPrecVECTOR(v0, &sd->tVn0);
    LoadPrecVECTOR(v1, &sd->tVn1);
    LoadPrecVECTOR(v2, &sd->tVn2);
    LoadPrecVECTOR(v3, &sd->tVn3);
    POLY_GT4 *prim = &sd->GT4Prim;
    uint32 t0 = prim->p[0].xy.xy;
    uint32 t1 = prim->p[1].xy.xy;
    uint32 t2 = prim->p[3].xy.xy;
    uint32 t3 = prim->p[2].xy.xy;
    memcpy(&v0->dvx, &t0, sizeof(t0));
    memcpy(&v1->dvx, &t1, sizeof(t1));
    memcpy(&v2->dvx, &t2, sizeof(t2));
    memcpy(&v3->dvx, &t3, sizeof(t3));
    Draw::SVertex *vertices[4] = { v0, v1, v2, v3 };
    VECTOR *depths[4] = { &sd->tVn0, &sd->tVn1, &sd->tVn2, &sd->tVn3 };
    for (sint32 i = 0; i < 4; ++i)
    {
        if (depths[i]->vz < 320)
        {
            gte_ldv0(vertices[i]);
            xport_gte_execute(0x180001);
            gte_stsxy(&vertices[i]->dvx);
        }
    }
    t0 = prim->p[0].rgbc.rgbc;
    t1 = prim->p[1].rgbc.rgbc;
    t2 = prim->p[3].rgbc.rgbc;
    t3 = prim->p[2].rgbc.rgbc;
    memcpy(&v0->r, &t0, sizeof(t0));
    memcpy(&v1->r, &t1, sizeof(t1));
    memcpy(&v2->r, &t2, sizeof(t2));
    memcpy(&v3->r, &t3, sizeof(t3));
    uint16 u0 = prim->p[0].uv.uv;
    uint16 u1 = prim->p[1].uv.uv;
    uint16 u3 = prim->p[2].uv.uv;
    uint16 u2 = prim->p[3].uv.uv;
    memcpy(&v0->u, &u0, sizeof(u0));
    memcpy(&v1->u, &u1, sizeof(u1));
    memcpy(&v2->u, &u2, sizeof(u2));
    memcpy(&v3->u, &u3, sizeof(u3));
    for (sint32 i = 0; i < 4; ++i)
    {
        Draw::SVertex *vertex = vertices[i];
        vertex->a = 0;
        if (vertex->dvx > 1000 || vertex->dvy > 1000 || vertex->dvx < -1000 || vertex->dvy < -1000 || vertex->vz < 101)
            vertex->a = 1;
    }
    SubdividFacet(sd, 0, v0, v1, v2, v3, 0, face == 0 ? 1 : 0);
}

void Night::NightCopCalc(VECTOR *v, sint16 *idx)
{
    FUNCTION_MARKER(0x800C5F18, "NFS4.EXE");
    sint32 z = v->vz;
    if ((uint32)z + 0x7FFU >= 0xFFFU)
        return;
    sint32 x = v->vx;
    if ((uint32)x + 0x3FFU >= 0x7FFU)
        return;
    sint32 side = z < 0 ? 1 : 0;
    if (z < 0)
        z = -z;
    sint32 index = (z >> 5) * 64 + ((x + 1024) >> 5);
    sint32 strength = (uint8)gNightTbl[index];
    sint32 lookup = *idx;
    const uint8 *coplookuptbl = (const uint8 *)gCopColor[side];
    *idx = coplookuptbl[lookup * 8 + (strength >> 1)];
}

void Night::NightCalc(VECTOR *v, sint16 *idx, Draw::tGiveShelbyMoreCache *sd)
{
    FUNCTION_MARKER(0x800C5FB8, "NFS4.EXE");
    sint32 zShift = sd->night_ZDistShift;
    sint32 z = v->vz;
    sint32 znear = sd->night_ZNear;
    sint32 drawLightning = sd->night_DrawLightning;
    sint32 zfar = (sint32)((uint32)znear + (1U << ((zShift + 6) & 31)));
    if (drawLightning != 0)
        *idx = (*gWeatherLightingTable[sd->night_LightningType])[*idx];
    if (z <= znear || z >= zfar)
        return;
    sint32 xShift = sd->night_XDistShift;
    sint32 x = v->vx;
    sint32 xdist = (sint32)(1U << ((xShift + 5) & 31));
    sint32 negativeXdist = (sint32)(0U - (uint32)xdist);
    if (x <= negativeXdist || x >= xdist)
        return;
    sint32 column = (sint32)((uint32)x + (uint32)xdist) >> (xShift & 31);
    sint32 row = (sint32)((uint32)z - (uint32)znear) >> (sd->night_ZDistShift & 31);
    sint32 lookup = (sint32)(((uint32)row << 6) + (uint32)column);
    char *nightTable = gNightTbl;
    sint32 current = *idx;
    const uint8 *table = (const uint8 *)gCurrentNightColor;
    sint32 index = (uint8)nightTable[lookup];
    *idx = table[current * 16 + index];
}

void DrawW::NightColorCalc(Draw::tGiveShelbyMoreCache *sd, POLY_GT4 *prim, CCOORD16 *vt0, CCOORD16 *vt1, CCOORD16 *vt2, CCOORD16 *vt3)
{
    FUNCTION_MARKER(0x800C609C, "NFS4.EXE");
    bool perVertex = sd->light == -1;
    CCOORD16 *vertices[4] = { vt0, vt1, vt2, vt3 };
    for (sint32 pass = 0; pass < 2; ++pass)
    {
        if (((uint8)sd->nightFlags & (1 << pass)) == 0)
            continue;
        MATRIX *matrix = pass == 0 ? &sd->matNight : &sd->matCop;
        gte_SetRotMatrix(matrix);
        gte_SetTransMatrix(matrix);
        VECTOR temp0;
        gte_ldv0(vt0);
        gte_rt();
        gte_stlvnl(&temp0);
        if (perVertex)
        {
            for (sint32 i = 1; i < 4; ++i)
            {
                gte_ldv0(vertices[i]);
                gte_rt();
                if (pass == 0)
                    Night::NightCalc(&temp0, &vertices[i - 1]->light, sd);
                else
                    Night::NightCopCalc(&temp0, &vertices[i - 1]->light);
                gte_stlvnl(&temp0);
            }
            if (pass == 0)
                Night::NightCalc(&temp0, &vt3->light, sd);
            else
                Night::NightCopCalc(&temp0, &vt3->light);
        }
        else
        {
            if (pass == 0)
                Night::NightCalc(&temp0, &sd->light, sd);
            else
                Night::NightCopCalc(&temp0, &sd->light);
        }
    }
    if (perVertex)
    {
        sint32 index0 = vt0->light;
        CVECTOR *lightTable = Chunk::lightTable;
        sint32 index1 = vt1->light;
        sint32 index3 = vt3->light;
        sint32 index2 = vt2->light;
        uint32 a, b, c, d;
        memcpy(&a, lightTable + index0, sizeof(a));
        memcpy(&b, lightTable + index1, sizeof(b));
        memcpy(&c, lightTable + index3, sizeof(c));
        memcpy(&d, lightTable + index2, sizeof(d));
        prim->p[0].rgbc.rgbc = a;
        prim->p[1].rgbc.rgbc = b;
        prim->p[2].rgbc.rgbc = c;
        prim->p[3].rgbc.rgbc = d;
    }
    else
    {
        sint32 index = sd->light;
        CVECTOR *lightTable = Chunk::lightTable;
        uint32 color;
        memcpy(&color, lightTable + index, sizeof(color));
        prim->p[0].rgbc.rgbc = color;
        prim->p[1].rgbc.rgbc = color;
        prim->p[2].rgbc.rgbc = color;
        prim->p[3].rgbc.rgbc = color;
    }
    gte_SetRotMatrix(&sd->matB);
    gte_SetTransMatrix(&sd->matB);
}

void DrawW::DrawQuad(Draw::tGiveShelbyMoreCache *sd, Trk::Quad *inQuad)
{
    FUNCTION_MARKER(0x800C64F8, "NFS4.EXE");
    CCOORD16 *geomVertices = sd->vertices;
    sint16 tx = sd->trans.x;
    sint16 ty = sd->trans.y;
    sint16 tz = sd->trans.z;
    uint8 indices[4] = { inQuad->aPoints[1], inQuad->aPoints[0], inQuad->aPoints[3], inQuad->aPoints[2] };
    CCOORD16 vt[4];
    uint32 dvxy[4];
    VECTOR *vectors[4] = { &sd->tVn0, &sd->tVn1, &sd->tVn2, &sd->tVn3 };
    for (sint32 i = 0; i < 4; ++i)
    {
        CCOORD16 vertex = geomVertices[indices[i]];
        vt[i].x = (sint16)(vertex.x + tx);
        vt[i].y = (sint16)(vertex.y + ty);
        vt[i].z = (sint16)(vertex.z + tz);
        vt[i].light = vertex.light;
        gte_ldv0(&vt[i]);
        gte_rtps();
        gte_stlvnl(vectors[i]);
        if (i == 0)
            gte_stsxy(&dvxy[0]);
    }
    if (sd->tVn3.vz < sd->tVn3.vx && sd->tVn0.vz < sd->tVn0.vx &&
        sd->tVn1.vz < sd->tVn1.vx && sd->tVn2.vz < sd->tVn2.vx)
        return;
    if (sd->tVn3.vz < (sint32)(0U - uint32(sd->tVn3.vx)) &&
        sd->tVn0.vz < (sint32)(0U - uint32(sd->tVn0.vx)) &&
        sd->tVn1.vz < (sint32)(0U - uint32(sd->tVn1.vx)) &&
        sd->tVn2.vz < (sint32)(0U - uint32(sd->tVn2.vx)))
        return;
    if (sd->tVn0.vz < 0 && sd->tVn1.vz < 0 && sd->tVn2.vz < 0 && sd->tVn3.vz < 0)
        return;
    gte_avsz4();
    Track::tMaterial *currentQuadMat = sd->materials + inQuad->material;
    uint8 flag = (uint8)currentQuadMat->flag;
    gte_stotz(&sd->otz);
    sint32 face = (flag >> 3) & 2;
    gte_stsxy3(&dvxy[1], &dvxy[2], &dvxy[3]);
    bool doSubdivision = sd->otz < 200;
    if (face == 0 && !doSubdivision)
    {
        sint32 bfct;
        gte_nclip();
        gte_swc2(24, &bfct);
        if (sd->head.mirror == 1)
            bfct = (sint32)(0U - uint32(bfct));
        if (bfct < 0)
        {
            gte_ldsxy3(dvxy[0], dvxy[1], dvxy[3]);
            gte_nclip();
            gte_swc2(24, &bfct);
            if (sd->head.mirror == 1)
                bfct = (sint32)(0U - uint32(bfct));
            if (bfct < 0)
                return;
        }
    }
    sint32 depthcue;
    gte_stdp(&depthcue);
    Draw::tPixMap *pixmaps = sd->artInfo->pPmx;
    sint32 pmxIndex = currentQuadMat->pmxIndex;
    Draw::tPixMap *workPmx = pixmaps + pmxIndex;
    if ((flag & 8) != 0 && sd->fogstate != 0 && sd->startfog <= sd->otz)
        workPmx = pixmaps + pmxIndex + (uint8)currentQuadMat->mipmap_offset;
    sint32 originalOtz = sd->otz;
    sint32 offset = sd->offset;
    sint32 save_pre_otz = originalOtz >> 1;
    sd->otz = save_pre_otz;
    if (offset == Draw::gMidGroundOtz)
    {
        save_pre_otz = (sint32)(uint32(save_pre_otz) << 2);
        sd->otz = (sint32)(uint32(originalOtz >> 4) + uint32(offset));
    }
    else
        sd->otz = (sint32)(uint32(save_pre_otz) + uint32(offset));
    if (sd->otz <= 0 || (sint32)(uint32(Draw::gViewOtSize) - 3) < sd->otz)
        return;
    if ((flag & 0x80) != 0)
    {
        PSX_RECT r = { 0, 0, 0, 0 };
        Draw::PrimStruct *cprim = &gNativeRenderCache.world.head.cprim;
        DR_TWIN *aprim = reinterpret_cast<DR_TWIN *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim + sd->otz;
        setaddr(aprim, getaddr(ot));
        cprim->PrimPtr += sizeof(*aprim);
        setaddr(ot, aprim);
        SetTexWindow(aprim, &r);
    }
    POLY_GT4 *prim;
    if (doSubdivision)
        prim = &sd->GT4Prim;
    else
    {
        prim = reinterpret_cast<POLY_GT4 *>(sd->head.cprim.PrimPtr);
        uint32 *ot = sd->head.cprim.LastPrim + sd->otz;
        sd->head.cprim.PrimPtr = reinterpret_cast<char *>(prim + 1);
        prim->tag = 0x0C000000 | getaddr(ot);
        setaddr(ot, prim);
    }
    prim->p[0].xy.xy = dvxy[0];
    prim->p[1].xy.xy = dvxy[1];
    prim->p[2].xy.xy = dvxy[3];
    prim->p[3].xy.xy = dvxy[2];
    if (sd->nightFlags == 0)
    {
        gte_lddp(depthcue);
        CVECTOR *lightTable = Chunk::lightTable;
        if (sd->light == -1)
        {
            uint32 color;
            memcpy(&color, lightTable + vt[2].light, sizeof(color));
            gte_lwc2(6, color);
            gte_dpcs();
            uint32 a, b, c;
            memcpy(&c, lightTable + vt[3].light, sizeof(c));
            memcpy(&a, lightTable + vt[0].light, sizeof(a));
            memcpy(&b, lightTable + vt[1].light, sizeof(b));
            gte_swc2(22, &prim->p[3].rgbc);
            gte_lwc2(20, a);
            gte_lwc2(21, b);
            gte_lwc2(22, c);
            gte_lwc2(6, c);
            gte_dpct();
            gte_swc2(20, &prim->p[0].rgbc);
            gte_swc2(21, &prim->p[1].rgbc);
            gte_swc2(22, &prim->p[2].rgbc);
        }
        else
        {
            uint32 color;
            memcpy(&color, lightTable + sd->light, sizeof(color));
            gte_lwc2(6, color);
            gte_dpcs();
            gte_swc2(22, &color);
            prim->p[0].rgbc.rgbc = color;
            prim->p[1].rgbc.rgbc = color;
            prim->p[2].rgbc.rgbc = color;
            prim->p[3].rgbc.rgbc = color;
        }
    }
    else
        NightColorCalc(sd, prim, &vt[0], &vt[1], &vt[2], &vt[3]);
    setlen(prim, 12);
    setcode(prim, (uint8)workPmx->flag | 0x3C);
    uint32 uv[4];
    memcpy(uv, workPmx, sizeof(uv));
    prim->p[0].uv_attr = uv[0];
    prim->p[1].uv_attr = uv[1];
    prim->p[2].uv_attr = uv[2];
    prim->p[3].uv_attr = uv[3];
    if (prim->p[0].attr == 0xFFFF)
    {
        sint32 depth_index = (sint32)((uint32(save_pre_otz) - uint32((sint32)sd->startfog)) << 4);
        depth_index >>= (uint16)sd->distfog & 31;
        if (depth_index < 0)
            depth_index = 0;
        else if (depth_index > 15)
            depth_index = 15;
        prim->p[0].attr = (uint16)gClutDepth[workPmx->pad2][depth_index];
    }
    if (doSubdivision)
    {
        uint8 zeroTransFlag = (uint8)sd->zeroGTETransFlag;
        gte_SetRotMatrix(&sd->identMat);
        if (zeroTransFlag == 0)
            gte_SetTransMatrix(&sd->identMat);
        if (stackSpeedUpEnbabledFlag == 0)
            SetUpSubdividFacet(face, sd);
        else
        {
            stackSpeedUpEnbabledFlag = 0;
            SetUpSubdividFacet(face, sd);
            stackSpeedUpEnbabledFlag = 1;
        }
        gte_SetRotMatrix(&sd->matB);
        if (zeroTransFlag == 0)
            gte_SetTransMatrix(&sd->matB);
    }
    if ((flag & 0x80) != 0)
    {
        sint32 dU = sint32(workPmx->u3) - sint32(workPmx->u0);
        if (dU < 0)
            dU = -dU;
        sint32 dV = sint32(workPmx->v3) - sint32(workPmx->v0);
        if (dV < 0)
            dV = -dV;
        PSX_RECT r = { 0, 0, (sint16)(dU + 1), (sint16)(dV + 1) };
        Draw::PrimStruct *cprim = &gNativeRenderCache.world.head.cprim;
        DR_TWIN *aprim = reinterpret_cast<DR_TWIN *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim + sd->otz;
        setaddr(aprim, getaddr(ot));
        cprim->PrimPtr += sizeof(*aprim);
        setaddr(ot, aprim);
        SetTexWindow(aprim, &r);
    }
}

void DrawW::kCtrlWorld_High(Draw::tGiveShelbyMoreCache *sd)
{
    FUNCTION_MARKER(0x800C6E38, "NFS4.EXE");
    sint32 numQuads = sd->quadCount;
    Trk::Quad *pquad = (Trk::Quad *)sd->quads;
    while (numQuads-- != 0)
    {
        if (sd->head.cprim.PrimPtr < sd->head.cprim.MPrimPtr)
        {
            DrawQuad(sd, pquad);
            ++pquad;
        }
    }
}

void DrawW::StripDraw_High(Draw::tGiveShelbyMoreCache *sd)
{
    FUNCTION_MARKER(0x800C6EB8, "NFS4.EXE");
    Trk::NewStrip *stripPtr = sd->stripPtr;
    sd->doublelayer = 1;
    while (true)
    {
        sd->numStrips = (sint16)(sd->numStrips - 1);
        if (sd->numStrips == -1)
            break;
        sint32 numQuads = (uint8)stripPtr->quadCount;
        const uint8 *material = (const uint8 *)(stripPtr + 1);
        for (sint32 iquad = 0; iquad < numQuads; ++iquad, material += 2)
        {
            if (sd->head.cprim.PrimPtr < sd->head.cprim.MPrimPtr)
            {
                Trk::Quad newQuad;
                memcpy(&newQuad.material, material, sizeof(newQuad.material));
                sint32 r0 = stripPtr->topVert + iquad;
                sint32 r2 = stripPtr->botVert + iquad;
                newQuad.aPoints[1] = (uint8)r0;
                newQuad.aPoints[2] = (uint8)r2;
                newQuad.aPoints[0] = (uint8)(r0 + 1);
                newQuad.aPoints[3] = (uint8)(r2 + 1);
                DrawQuad(sd, &newQuad);
            }
        }
        stripPtr = (Trk::NewStrip *)((uint8 *)stripPtr + (uint8)stripPtr->size);
    }
}

void DrawW::DoTrough(DRender::tView *Vi, tBuildEntry *buildList)
{
    FUNCTION_MARKER(0x800C6FA0, "NFS4.EXE");
    Draw::tGiveShelbyMoreCache *sd = &gNativeRenderCache.world;
    sint32 chunkCount = BWorld::gChunkCount;
    sd->doublelayer = 1;
    memcpy(&sd->identMat, &gIdentTemplate, sizeof(sd->identMat));
    sd->offsubdivid = 0;
    for (sint32 buildInd = 0; buildInd < chunkCount; ++buildInd, ++buildList)
    {
        if ((buildList->enableBits & 1) == 0)
            continue;
        sd->artInfo = &gInitialArt;
        sd->chunkInd = (uint8)buildList->chunkInd;
        sd->rezInd = (uint8)buildList->geomRez;
        Chunk *chunkDat = Track::chunkList + sd->chunkInd;
        WorldSetUpMatrix(&gWorldMat, &sd->matB);
        Group *vertexBuf = chunkDat->vertexBuf;
        sd->nightFlags = 0;
        Track::tMaterial *materials = Track::materials;
        sd->light = -1;
        sd->vertices = reinterpret_cast<CCOORD16 *>(vertexBuf + 1);
        sd->materials = materials;
        coorddef *pChunkCp = Chunk::chunkCenters + sd->chunkInd;
        if (gNight_renderNight != 0)
        {
            sd->nightFlags = 4;
            BO::tNewtonObj *target = Camera::gInfo[Vi->player].target;
            sint32 cx = (sint32)((uint32)pChunkCp->x - (uint32)target->position.x) >> 10;
            sint32 cz = (sint32)((uint32)pChunkCp->z - (uint32)target->position.z) >> 10;
            sint32 dist = (sint32)((uint32)cx * (uint32)cx + (uint32)cz * (uint32)cz);
            if (dist < 0x47E0000)
            {
                if ((Cars::gList[Vi->player]->control.lights & 6) != 0)
                    sd->nightFlags = 5;
                coorddef tmp, tmp2;
                tmp.x = (sint32)((uint32)Vi->cview.translation.x - (uint32)Camera::gInfo[Vi->player].target->position.x);
                tmp.y = (sint32)((uint32)Vi->cview.translation.y - (uint32)Camera::gInfo[Vi->player].target->position.y);
                tmp.z = (sint32)((uint32)Vi->cview.translation.z - (uint32)Camera::gInfo[Vi->player].target->position.z);
                transform(&tmp, &gNightMat, &tmp2);
                WorldSetUpTranslation(&tmp2, &sd->matNight);
            }
            Car::tObj *cop = BW::gCopCarObj;
            if (cop != 0)
            {
                cx = (sint32)((uint32)pChunkCp->x - (uint32)cop->N.position.x) >> 10;
                cz = (sint32)((uint32)pChunkCp->z - (uint32)cop->N.position.z) >> 10;
                dist = (sint32)((uint32)cx * (uint32)cx + (uint32)cz * (uint32)cz);
                if (dist < 0x47E0000)
                {
                    sd->nightFlags |= 2;
                    coorddef tmp, tmp2;
                    tmp.x = (sint32)((uint32)Vi->cview.translation.x - (uint32)cop->N.position.x);
                    tmp.y = (sint32)((uint32)Vi->cview.translation.y - (uint32)cop->N.position.y);
                    tmp.z = (sint32)((uint32)Vi->cview.translation.z - (uint32)cop->N.position.z);
                    transform(&tmp, &gCopMat, &tmp2);
                    WorldSetUpTranslation(&tmp2, &sd->matCop);
                }
            }
        }
        sd->trans.x = (sint16)((sint32)((uint32)pChunkCp->x - (uint32)Vi->cview.translation.x) >> 10);
        sd->trans.y = (sint16)((sint32)((uint32)pChunkCp->y - (uint32)Vi->cview.translation.y) >> 10);
        sd->trans.z = (sint16)((sint32)((uint32)pChunkCp->z - (uint32)Vi->cview.translation.z) >> 10);
        sd->matB.t[2] = 0;
        sd->matB.t[1] = 0;
        sd->matB.t[0] = 0;
        gte_SetTransMatrix(&sd->matB);
        if (sd->rezInd != 0)
        {
            Group *group = chunkDat->stripBuf;
            if (group != 0)
            {
                sd->stripPtr = reinterpret_cast<Trk::NewStrip *>(group + 1);
                sd->offset = 125;
                sd->numStrips = (sint16)group->m_num_elements;
                StripDraw_High(sd);
                sd->quadCount = chunkDat->quadCounts[5];
                if (sd->quadCount != 0)
                {
                    Trk::Quad *quads = chunkDat->renderQuads[3];
                    sd->offset = 30;
                    sd->zeroGTETransFlag = 1;
                    sd->light = -1;
                    sd->quads = quads;
                    kCtrlWorld_High(sd);
                }
            }
        }
        else
        {
            Group *group = chunkDat->lorezstripBuf;
            if (group != 0)
            {
                sint32 count = group->m_num_elements;
                sd->stripPtr = reinterpret_cast<Trk::NewStrip *>(group + 1);
                sd->numStrips = (sint16)count;
                if (sd->stripPtr != 0)
                {
                    sd->offset = 125;
                    StripDraw_High(sd);
                }
            }
            sd->quadCount = chunkDat->quadCounts[0];
            if (sd->quadCount != 0)
            {
                Trk::Quad *quads = chunkDat->renderQuads[0];
                sd->offset = 125;
                sd->zeroGTETransFlag = 1;
                sd->light = -1;
                sd->quads = quads;
                kCtrlWorld_High(sd);
            }
            sd->quadCount = chunkDat->quadCounts[1];
            if (sd->quadCount != 0)
            {
                Trk::Quad *quads = chunkDat->renderQuads[1];
                sd->offset = 30;
                sd->zeroGTETransFlag = 1;
                sd->light = -1;
                sd->quads = quads;
                kCtrlWorld_High(sd);
            }
        }
        sd->vertices = reinterpret_cast<CCOORD16 *>(chunkDat->objVertexBuf + 1);
        sd->quadCount = chunkDat->quadCounts[2];
        if (sd->quadCount != 0)
        {
            Group *group = chunkDat->objQuadBuf;
            sd->offset = 125;
            sd->zeroGTETransFlag = 1;
            sd->light = -1;
            sd->quads = group + 1;
            kCtrlWorld_High(sd);
        }
        sd->quadCount = chunkDat->quadCounts[3];
        if (sd->quadCount != 0)
        {
            Group *group = chunkDat->objQuadInstanceBuf;
            sd->offset = 50;
            sd->zeroGTETransFlag = 1;
            sd->light = -1;
            sd->quads = group + 1;
            kCtrlWorld_High(sd);
        }
    }
}

void DrawW::WorldSetUpMatrix(matrixtdef *m, MATRIX *mat)
{
    FUNCTION_MARKER(0x800C753C, "NFS4.EXE");
    for (sint32 i = 0; i < 3; ++i)
    {
        sint32 r0 = m->m[i];
        sint32 r1 = m->m[i + 3];
        sint32 r2 = m->m[i + 6];
        mat->m[i][0] = (sint16)(r0 >> 4);
        mat->m[i][1] = (sint16)(r1 >> 4);
        mat->m[i][2] = (sint16)(r2 >> 4);
    }
    gte_SetRotMatrix(mat);
}

void DrawW::WorldSetUpTranslation(coorddef *t, MATRIX *mat)
{
    FUNCTION_MARKER(0x800C75D8, "NFS4.EXE");
    sint32 tx = t->x;
    sint32 ty = t->y;
    sint32 tz = t->z;
    mat->t[0] = tx >> 10;
    mat->t[1] = ty >> 10;
    mat->t[2] = tz >> 10;
    gte_SetTransMatrix(mat);
}

void DrawW::ResetAnimationTimer(void)
{
    FUNCTION_MARKER(0x800C761C, "NFS4.EXE");
    for (sint32 i = 11; i >= 0; --i)
        animation_timer[i] = 0;
}

sint32 DrawW::GetAnimationTime(Trk::AnimateInst *animInst)
{
    FUNCTION_MARKER(0x800C7644, "NFS4.EXE");
    sint32 objIndex = animInst->objectIndex;
    sint32 track = GameSetup::gData.track;
    if (objIndex == 0 || track == 3 || track == 7)
        return simGlobal.gameTicks;
    sint32 maxTick = (sint32)((uint32)(animInst->count - 2) * (uint32)animInst->interval);
    sint32 time = animation_timer[animInst->objectIndex - 1];
    return time < maxTick ? time : maxTick;
}

void DrawW::SetAnimationTime(Trk::AnimateInst *animInst, sint32 *table, sint32 time)
{
    FUNCTION_MARKER(0x800C76C8, "NFS4.EXE");
    (void)time;
    sint32 objIndex = animInst->objectIndex - 1;
    sint32 *timer = &animation_timer[objIndex];
    if (*timer < 3841) {
        if (*timer > 0) {
            *timer = (sint32)((uint32)*timer + 1);
            return;
        }
        sint32 numCars = Cars::gNumHumanRaceCars;
        for (sint32 i = 0; i < numCars; ++i) {
            sint32 slice = Cars::gHumanRaceCarList[i]->N.simRoadInfo.slice;
            if (table[objIndex * 2] <= slice && slice <= table[objIndex * 2 + 1])
                *timer = (sint32)((uint32)*timer + 1);
        }
    } else {
        sint32 numCars = Cars::gNumHumanRaceCars;
        for (sint32 i = 0; i < numCars; ++i) {
            sint32 slice = Cars::gHumanRaceCarList[i]->N.simRoadInfo.slice;
            if (slice < table[objIndex * 2] || table[objIndex * 2 + 1] < slice)
                *timer = 0;
        }
    }
}

void DrawW::DoObjectAnimations(void)
{
    FUNCTION_MARKER(0x800C7808, "NFS4.EXE");
    sint32 track = GameSetup::gData.track;
    sint32 *trackPtr;
    uint32 objectCount;
    if (track == 0)
    {
        trackPtr = &trk0[0][0];
        objectCount = sizeof(trk0) / sizeof(trk0[0]);
    }
    else if (track == 4)
    {
        trackPtr = &trk4[0][0];
        objectCount = sizeof(trk4) / sizeof(trk4[0]);
    }
    else
        return;
    sint32 time = simGlobal.gameTicks;

    for (sint32 i = 1; i < 16; ++i)
    {
        Trk::AnimateInst *animInst = Anim::gInstanceFromIndex[i];
        if (animInst != 0 && animInst->objectIndex != 0 && animInst->objectIndex <= objectCount)
            SetAnimationTime(animInst, trackPtr, time);
    }
}

sint32 DrawW::BuildObjectFacets(DRender::tView *Vi, ChunkObjectInfo *gObjInfo)
{
    FUNCTION_MARKER(0x800C78A8, "NFS4.EXE");
    Group *group = gObjInfo->objInstanceBuf;
    Trk::SimpleInst *objInstance = reinterpret_cast<Trk::SimpleInst *>(group + 1);
    sint32 groupNumElements = group->m_num_elements;
    sint32 totalCount = 0;
    if (groupNumElements == 0)
        return 0;
    sint32 offset = gObjInfo->offset;
    sint32 doFrustumClip = gObjInfo->doFrustumClip;
    sint32 zClipSq = gObjInfo->zClipSq;
    sint16 *visList = gObjInfo->visList;
    Draw::tGiveShelbyMoreCache *world = &gNativeRenderCache.world;
    Draw::DCache *sd = &gNativeRenderCache.horizonDraw;
    world->matB.t[2] = 0;
    world->matB.t[1] = 0;
    world->matB.t[0] = 0;
    gte_SetTransMatrix(&world->matB);
    matrixtdef matrix;
    coorddef cp;
    for (sint32 objectIndex = 0; objectIndex < groupNumElements; ++objectIndex,
        objInstance = reinterpret_cast<Trk::SimpleInst *>(reinterpret_cast<uint8 *>(objInstance) + objInstance->size))
    {
        if (visList != 0 && (static_cast<uint16>(visList[objectIndex]) & 0x1000) == 0)
            continue;
        sint32 objectOffset = offset;
        if (objectOffset == 0)
            objectOffset = (sint8)goffsets[objInstance->zoffset];
        if (objInstance->type == 1)
        {
            sint32 index = objInstance->pad;
            Trk::ObjectDef *objDef = Track::gObjDefs[index];
            coorddef *position = reinterpret_cast<coorddef *>(&objInstance->x);
            if (doFrustumClip != 0 && ObjectClipped(Vi, index, position, world))
                continue;
            if (zClipSq != -1 && xzsquaredist32(position, &Vi->cview.translation) >= zClipSq)
                continue;
            sint32 count = DrawObjectSimple(Vi, sd, objDef, position, objectOffset);
            totalCount = (sint32)(uint32(totalCount) + uint32(count));
        }
        else if (objInstance->type == 3 || objInstance->type == 7)
        {
            Trk::AnimateInst *animInst = reinterpret_cast<Trk::AnimateInst *>(objInstance);
            sint32 time = GetAnimationTime(animInst);
            Anim::GetRotPos(animInst, 1, time, &cp, &matrix);
            if (zClipSq != -1 && xzsquaredist32(&cp, &Vi->cview.translation) >= zClipSq)
                continue;
            Trk::ObjectDef *objDef = Track::gObjDefs[animInst->pad];
            sint32 count = DrawObjectTransform(Vi, sd, &matrix, objDef, &cp, objectOffset, -1);
            totalCount = (sint32)(uint32(totalCount) + uint32(count));
            if ((animInst->flags & 2) != 0)
            {
                coorddef pt2;
                pt2.x = (sint32)(uint32(cp.x) - (uint32(matrix.m[6]) << 4));
                pt2.y = (sint32)(uint32(cp.y) - (uint32(matrix.m[7]) << 4));
                pt2.z = (sint32)(uint32(cp.z) - (uint32(matrix.m[8]) << 4));
                Flare::Halo2(Vi, -1, 30, &cp, &pt2, &gNativeRenderCache.flare);
            }
        }
    }
    return totalCount;
}

sint32 DrawW::BuildCustomObjectFacets(DRender::tView *Vi, Draw::DCache *sd, Trk::SimObject *simObjs, Group *group, sint32 zClipSq)
{
    FUNCTION_MARKER(0x800C7B9C, "NFS4.EXE");
    static char offsets[8] = { 125, 125, 50, 15, 255, 125, 0, 0 };
    Trk::SimpleInst *objInstance = reinterpret_cast<Trk::SimpleInst *>(group + 1);
    sint32 groupNumElements = group->m_num_elements;
    sint32 totalCount = 0;
    if (groupNumElements == 0)
        return 0;
    sd->matB.t[2] = 0;
    sd->matB.t[1] = 0;
    sd->matB.t[0] = 0;
    gte_SetTransMatrix(&sd->matB);
    for (sint32 objectIndex = 0; objectIndex < groupNumElements; ++objectIndex,
        objInstance = reinterpret_cast<Trk::SimpleInst *>(reinterpret_cast<uint8 *>(objInstance) + objInstance->size))
    {
        sint32 objectOffset = (sint8)offsets[objInstance->zoffset];
        uint8 type = objInstance->type;
        if (type != 5 && type != 2)
            continue;
        Trk::ObjectDef *objDef = Track::gObjDefs[objInstance->pad];
        Trk::CollideBoomInst *objCollideBoomInstance = reinterpret_cast<Trk::CollideBoomInst *>(objInstance);
        if (type != 2 && Object::GetAnim(simObjs + objCollideBoomInstance->simIndex) != 0)
        {
            ObjectAnim *anim = Object::GetAnim(simObjs + objCollideBoomInstance->simIndex);
            anim->Draw(Vi, sd, objectOffset);
            continue;
        }
        coorddef *position = reinterpret_cast<coorddef *>(&objInstance->x);
        if (xzsquaredist32(position, &Vi->cview.translation) >= zClipSq)
            continue;
        if (objInstance->type == 2 && ObjectClipped(Vi, objInstance->pad, position,
            reinterpret_cast<Draw::tGiveShelbyMoreCache *>(sd)))
            continue;
        tQuat quat;
        memcpy(&quat, &objCollideBoomInstance->qx, sizeof(quat));
        matrixtdef matrix;
        Quatern::QuatToMat(&quat, &matrix);
        sint32 sx = objCollideBoomInstance->sx * 256;
        sint32 sy = objCollideBoomInstance->sy * 256;
        sint32 sz = objCollideBoomInstance->sz * 256;
        sint32 scale[3] = { sx, sy, sz };
        for (sint32 column = 0; column < 3; ++column)
        {
            sint32 t1 = fixedmult(matrix.m[column], scale[column]);
            sint32 t2 = fixedmult(matrix.m[column + 3], scale[column]);
            sint32 t3 = fixedmult(matrix.m[column + 6], scale[column]);
            matrix.m[column] = t1;
            matrix.m[column + 3] = t2;
            matrix.m[column + 6] = t3;
        }
        sint32 count = DrawObjectTransform(Vi, sd, &matrix, objDef, position, objectOffset, -1);
        totalCount = (sint32)(uint32(totalCount) + uint32(count));
    }
    return totalCount;
}

sint32 DrawObjectTransform(DRender::tView *Vi, Draw::DCache *sd, matrixtdef *matrix, Trk::ObjectDef *objDef, coorddef *pCp, sint32 offset, sint16 light)
{
    FUNCTION_MARKER(0x800C7EBC, "NFS4.EXE");
    Draw::tGiveShelbyMoreCache *world = reinterpret_cast<Draw::tGiveShelbyMoreCache *>(sd);
    world->vertices = reinterpret_cast<CCOORD16 *>(objDef + 1);
    world->quadCount = objDef->quadCount;
    uint8 vertexCount = objDef->vertexCount;
    Track::tMaterial *materials = Track::materials;
    world->nightFlags = 0;
    sint32 renderNight = gNight_renderNight;
    world->offset = offset;
    world->quads = reinterpret_cast<Trk::Quad *>(reinterpret_cast<CCOORD16 *>(objDef + 1) + vertexCount);
    world->materials = materials;
    coorddef tmp, tmp2;
    if (renderNight != 0)
    {
        world->nightFlags = 4;
        if ((Cars::gList[Vi->player]->control.lights & 6) != 0)
            world->nightFlags = 5;
        tmp.x = (sint32)(uint32(pCp->x) - uint32(Camera::gInfo[Vi->player].target->position.x));
        tmp.y = (sint32)(uint32(pCp->y) - uint32(Camera::gInfo[Vi->player].target->position.y));
        tmp.z = (sint32)(uint32(pCp->z) - uint32(Camera::gInfo[Vi->player].target->position.z));
        transform(&tmp, &gNightMat, &tmp2);
        DrawW::WorldSetUpTranslation(&tmp2, &world->matNight);
        if (BW::gCopCarObj != 0)
        {
            world->nightFlags |= 2;
            Car::tObj *cop = BW::gCopCarObj;
            tmp.x = (sint32)(uint32(pCp->x) - uint32(cop->N.position.x));
            tmp.y = (sint32)(uint32(pCp->y) - uint32(cop->N.position.y));
            tmp.z = (sint32)(uint32(pCp->z) - uint32(cop->N.position.z));
            transform(&tmp, &gCopMat, &tmp2);
            DrawW::WorldSetUpTranslation(&tmp2, &world->matCop);
        }
        world->matB.t[2] = 0;
        world->matB.t[1] = 0;
        world->matB.t[0] = 0;
        gte_SetTransMatrix(&world->matB);
    }
    tmp.x = (sint32)(uint32(pCp->x) - uint32(Vi->cview.translation.x));
    tmp.y = (sint32)(uint32(pCp->y) - uint32(Vi->cview.translation.y));
    tmp.z = (sint32)(uint32(pCp->z) - uint32(Vi->cview.translation.z));
    TrsProj::SetPsxTransZero();
    TrsProj::TransPt(&tmp, &tmp2);
    if (offset == -1)
    {
        world->offset = Draw::gMidGroundOtz;
        tmp2.x >>= 2;
        tmp2.z >>= 2;
        tmp2.y >>= 2;
    }
    world->trans.x = 0;
    world->trans.y = 0;
    world->trans.z = 0;
    matrixtdef mattemp;
    Math::fasttransmult(matrix, &gWorldMat, &mattemp);
    DrawW::WorldSetUpMatrix(&mattemp, &world->matB);
    DrawW::WorldSetUpTranslation(&tmp2, &world->matB);
    world->zeroGTETransFlag = 0;
    world->light = light;
    DrawW::kCtrlWorld_High(world);
    DrawW::WorldSetUpMatrix(&gWorldMat, &world->matB);
    world->matB.t[2] = 0;
    world->matB.t[1] = 0;
    world->matB.t[0] = 0;
    gte_SetTransMatrix(&world->matB);
    return objDef->quadCount;
}

sint32 DrawObjectSimple(DRender::tView *Vi, Draw::DCache *sd, Trk::ObjectDef *objDef, coorddef *pCp, sint32 offset)
{
    FUNCTION_MARKER(0x800C8214, "NFS4.EXE");
    Draw::tGiveShelbyMoreCache *world = reinterpret_cast<Draw::tGiveShelbyMoreCache *>(sd);
    Track::tMaterial *materials = Track::materials;
    world->vertices = reinterpret_cast<CCOORD16 *>(objDef + 1);
    world->materials = materials;
    world->quadCount = objDef->quadCount;
    uint8 vertexCount = objDef->vertexCount;
    world->nightFlags = 0;
    sint32 renderNight = gNight_renderNight;
    world->offset = offset;
    world->quads = reinterpret_cast<Trk::Quad *>(reinterpret_cast<CCOORD16 *>(objDef + 1) + vertexCount);
    coorddef tmp, tmp2;
    if (renderNight != 0)
    {
        world->nightFlags = 4;
        if ((Cars::gList[Vi->player]->control.lights & 6) != 0)
            world->nightFlags = 5;
        tmp.x = (sint32)(uint32(Vi->cview.translation.x) - uint32(Camera::gInfo[Vi->player].target->position.x));
        tmp.y = (sint32)(uint32(Vi->cview.translation.y) - uint32(Camera::gInfo[Vi->player].target->position.y));
        tmp.z = (sint32)(uint32(Vi->cview.translation.z) - uint32(Camera::gInfo[Vi->player].target->position.z));
        transform(&tmp, &gNightMat, &tmp2);
        DrawW::WorldSetUpTranslation(&tmp2, &world->matNight);
        if (BW::gCopCarObj != 0)
        {
            world->nightFlags |= 2;
            Car::tObj *cop = BW::gCopCarObj;
            tmp.x = (sint32)(uint32(Vi->cview.translation.x) - uint32(cop->N.position.x));
            tmp.y = (sint32)(uint32(Vi->cview.translation.y) - uint32(cop->N.position.y));
            tmp.z = (sint32)(uint32(Vi->cview.translation.z) - uint32(cop->N.position.z));
            transform(&tmp, &gCopMat, &tmp2);
            DrawW::WorldSetUpTranslation(&tmp2, &world->matCop);
        }
        world->matB.t[2] = 0;
        world->matB.t[1] = 0;
        world->matB.t[0] = 0;
        gte_SetTransMatrix(&world->matB);
    }
    sint32 shift = 10;
    if (offset == -1)
    {
        world->offset = Draw::gMidGroundOtz;
        shift = 12;
    }
    world->trans.x = (sint16)((sint32)(uint32(pCp->x) - uint32(Vi->cview.translation.x)) >> shift);
    world->trans.y = (sint16)((sint32)(uint32(pCp->y) - uint32(Vi->cview.translation.y)) >> shift);
    world->trans.z = (sint16)((sint32)(uint32(pCp->z) - uint32(Vi->cview.translation.z)) >> shift);
    world->zeroGTETransFlag = 1;
    world->light = -1;
    DrawW::kCtrlWorld_High(world);
    return objDef->quadCount;
}

sint32 DrawW::BuildChunkObjectFacets(DRender::tView *Vi, ChunkObjectInfo *gObjInfo)
{
    FUNCTION_MARKER(0x800C8508, "NFS4.EXE");
    Group *instGroup = gObjInfo->objInstanceBuf;
    Trk::SimObject *simObjs = gObjInfo->simObjs;
    Trk::SimpleInst *objInstance = reinterpret_cast<Trk::SimpleInst *>(instGroup + 1);
    sint32 groupNumElements = instGroup->m_num_elements;
    sint32 doFrustumClip = gObjInfo->doFrustumClip;
    sint32 totalCount = 0;
    if (groupNumElements == 0)
        return 0;
    Draw::tGiveShelbyMoreCache *world = &gNativeRenderCache.world;
    Draw::DCache *sd = &gNativeRenderCache.horizonDraw;
    world->matB.t[2] = 0;
    world->matB.t[1] = 0;
    world->matB.t[0] = 0;
    gte_SetTransMatrix(&world->matB);
    matrixtdef matrix;
    for (sint32 objectIndex = 0; objectIndex < groupNumElements; ++objectIndex,
        objInstance = reinterpret_cast<Trk::SimpleInst *>(reinterpret_cast<uint8 *>(objInstance) + objInstance->size))
    {
        uint8 type = objInstance->type;
        sint32 objectOffset = (sint8)goffsets[objInstance->zoffset];
        if ((type & 0x80) != 0)
            continue;
        coorddef *position = reinterpret_cast<coorddef *>(&objInstance->x);
        if (type != 5 && doFrustumClip != 0 &&
            ObjectClipped(Vi, objInstance->pad, position, world) && objInstance->type != 2)
            continue;
        world->offsubdivid = 1024;
        sint16 light = -1;
        bool billboard = false;
        if ((objInstance->flags & 1) != 0)
        {
            sint32 dx = (sint32)(uint32(objInstance->x) - uint32(Vi->cview.translation.x));
            sint32 dz = (sint32)(uint32(objInstance->z) - uint32(Vi->cview.translation.z));
            sint32 angle = fixedatan(dx, dz);
            fixedxformy(&matrix, angle);
            billboard = type == 9;
        }
        else if (type == 1)
        {
            Trk::ObjectDef *objDef = Track::gObjDefs[objInstance->pad];
            sint32 count = DrawObjectSimple(Vi, sd, objDef, position, objectOffset);
            totalCount = (sint32)(uint32(totalCount) + uint32(count));
            continue;
        }
        else if (type == 2 || type == 5)
        {
            Trk::CollideBoomInst *instance = reinterpret_cast<Trk::CollideBoomInst *>(objInstance);
            Trk::ObjectDef *objDef = Track::gObjDefs[objInstance->pad];
            if (type == 5)
            {
                if (Object::GetAnim(simObjs + instance->simIndex) != 0)
                {
                    ObjectAnim *anim = Object::GetAnim(simObjs + instance->simIndex);
                    anim->Draw(Vi, sd, objectOffset);
                    continue;
                }
            }
            Quatern::QuatToMat(reinterpret_cast<tQuat *>(&instance->qx), &matrix);
            sint32 sx = instance->sx * 256;
            sint32 sy = instance->sy * 256;
            sint32 sz = instance->sz * 256;
            sint32 scale[3] = { sx, sy, sz };
            for (sint32 column = 0; column < 3; ++column)
            {
                sint32 t1 = fixedmult(matrix.m[column], scale[column]);
                sint32 t2 = fixedmult(matrix.m[column + 3], scale[column]);
                sint32 t3 = fixedmult(matrix.m[column + 6], scale[column]);
                matrix.m[column] = t1;
                matrix.m[column + 3] = t2;
                matrix.m[column + 6] = t3;
            }
            if (type == 2)
            {
                memcpy(&light, reinterpret_cast<uint8 *>(objInstance) + 34, sizeof(light));
            }
            sint32 count = DrawObjectTransform(Vi, sd, &matrix, objDef, position, objectOffset, light);
            totalCount = (sint32)(uint32(totalCount) + uint32(count));
            continue;
        }
        else if (type == 9)
        {
            sint16 angle;
            memcpy(&angle, reinterpret_cast<uint8 *>(objInstance) + 20, sizeof(angle));
            xformy(&matrix, angle);
            billboard = true;
        }
        else
            continue;
        if (billboard)
        {
            sint16 sx, sy;
            memcpy(&sx, reinterpret_cast<uint8 *>(objInstance) + 24, sizeof(sx));
            memcpy(&sy, reinterpret_cast<uint8 *>(objInstance) + 22, sizeof(sy));
            sint32 scale[3] = { sx * 256, sy * 256, sx * 256 };
            for (sint32 column = 0; column < 3; ++column)
            {
                sint32 t1 = fixedmult(matrix.m[column], scale[column]);
                sint32 t2 = fixedmult(matrix.m[column + 3], scale[column]);
                sint32 t3 = fixedmult(matrix.m[column + 6], scale[column]);
                matrix.m[column] = t1;
                matrix.m[column + 3] = t2;
                matrix.m[column + 6] = t3;
            }
            memcpy(&light, reinterpret_cast<uint8 *>(objInstance) + 26, sizeof(light));
            world->offsubdivid = 0;
        }
        Trk::ObjectDef *objDef = Track::gObjDefs[objInstance->pad];
        sint32 count = DrawObjectTransform(Vi, sd, &matrix, objDef, position, objectOffset, light);
        totalCount = (sint32)(uint32(totalCount) + uint32(count));
    }
    return totalCount;
}

bool ObjectClipped(DRender::tView *Vi, sint32 ind, coorddef *pCp, Draw::tGiveShelbyMoreCache *sd)
{
    FUNCTION_MARKER(0x800C8BD0, "NFS4.EXE");
    (void)sd;
    tBoundingSphere *bSphere = reinterpret_cast<tBoundingSphere *>(gPersistObjDefBoundingSpheres + 1) + ind;
    coorddef tmp;
    tmp.x = (sint32)((uint32)(sint32(bSphere->cp.x) * 1024) + (uint32)pCp->x);
    tmp.y = (sint32)((uint32)(sint32(bSphere->cp.y) * 1024) + (uint32)pCp->y);
    tmp.z = (sint32)((uint32)(sint32(bSphere->cp.z) * 1024) + (uint32)pCp->z);
    coorddef trans = Vi->cview.translationInv;
    coorddef tmp2;
    transform(&tmp, &Vi->cview.mrotationInv, &tmp2);
    tmp2.x = (sint32)((uint32)tmp2.x + (uint32)trans.x);
    tmp2.y = (sint32)((uint32)tmp2.y + (uint32)trans.y);
    tmp2.z = (sint32)((uint32)tmp2.z + (uint32)trans.z);
    sint32 zLimit = (sint32)((uint32)tmp2.z + (uint32)(sint32(bSphere->radius) * 1024));
    return zLimit < tmp2.x || zLimit < (sint32)(0U - (uint32)tmp2.x);
}

void DepthCue_Init(void)
{
    FUNCTION_MARKER(0x800CA950, "NFS4.EXE");
    sint32 distance = (sint32)((uint32)TrackSpec::gSpec.depthcuespec.distance << 6);
    SetFogNear(distance, 160);
    SetFarColor(TrackSpec::gSpec.depthcuespec.color.r, TrackSpec::gSpec.depthcuespec.color.g, TrackSpec::gSpec.depthcuespec.color.b);
}

void DrawW::DoObjects(DRender::tView *Vi, tBuildEntry *buildList)
{
    FUNCTION_MARKER(0x800C8CC4, "NFS4.EXE");
    Draw::tGiveShelbyMoreCache *world = &gNativeRenderCache.world;
    sint32 chunkCount = BWorld::gChunkCount;
    sint32 thisChunkInd = gCurrContext->currentChunk;
    gVi = Vi;
    world->artInfo = &gInitialArt;
    for (sint32 buildInd = 0; buildInd < chunkCount; ++buildInd, ++buildList)
    {
        if ((buildList->enableBits & 2) == 0)
            continue;
        Chunk *chunkDat = Track::chunkList + buildList->chunkInd;
        Group *simGroup = chunkDat->simObjBuf;
        Trk::SimObject *simObjs = simGroup != 0 ? reinterpret_cast<Trk::SimObject *>(simGroup + 1) :
            reinterpret_cast<Trk::SimObject *>(sizeof(Group));
        if (chunkDat->objInstanceBuf != 0)
        {
            world->doublelayer = 1;
            gChunkObjInfo.simObjs = simObjs;
            gChunkObjInfo.doFrustumClip = buildList->geomRez == 4;
            gChunkObjInfo.objInstanceBuf = chunkDat->objInstanceBuf;
            stackSpeedUpEnbabledFlag = 1;
            BuildChunkObjectFacets(gVi, &gChunkObjInfo);
            stackSpeedUpEnbabledFlag = 0;
        }
        if (GameSetup::gData.Time == 0 && GameSetup::gData.Weather == 0 && chunkDat->objSpecialInstanceBuf != 0)
        {
            world->offsubdivid = 1024;
            world->doublelayer = 0;
            gChunkObjInfo.visList = 0;
            gChunkObjInfo.simObjs = simObjs;
            gChunkObjInfo.zClipSq = -1;
            gChunkObjInfo.offset = 50;
            gChunkObjInfo.doFrustumClip = 1;
            gChunkObjInfo.objInstanceBuf = chunkDat->objSpecialInstanceBuf;
            BuildObjectFacets(gVi, &gChunkObjInfo);
        }
    }
    world->doublelayer = 0;
    world->offsubdivid = 1024;
    if (gPersistObjInst != 0)
    {
        sint32 track = GameSetup::gData.track;
        uint32 chunk = uint32(thisChunkInd);
        if ((track != 4 || (chunk - 1U >= 40U && chunk - 61U >= 30U && chunk - 108U >= 9U)) &&
            (track != 0 || (chunk - 1U >= 53U && chunk - 68U >= 28U && chunk - 108U >= 20U)))
        {
            gChunkObjInfo.objInstanceBuf = gPersistObjInst;
            gChunkObjInfo.simObjs = 0;
            gChunkObjInfo.offset = 125;
            gChunkObjInfo.doFrustumClip = 1;
            gChunkObjInfo.zClipSq = gCurrContext->polyFarZClipSq;
            gChunkObjInfo.visList = 0;
            stackSpeedUpEnbabledFlag = 1;
            BuildObjectFacets(gVi, &gChunkObjInfo);
            stackSpeedUpEnbabledFlag = 0;
        }
    }
    if (gPersistMidgroundObjInst != 0)
    {
        gChunkObjInfo.simObjs = 0;
        gChunkObjInfo.objInstanceBuf = gPersistMidgroundObjInst;
        gChunkObjInfo.zClipSq = -1;
        gChunkObjInfo.offset = -1;
        gChunkObjInfo.doFrustumClip = 1;
        gChunkObjInfo.visList = Track::gInViewList[thisChunkInd];
        stackSpeedUpEnbabledFlag = 1;
        BuildObjectFacets(gVi, &gChunkObjInfo);
        stackSpeedUpEnbabledFlag = 0;
    }
    if (Object::customObjInst != 0 && Object::customObjInst->m_num_elements > 0)
    {
        Group *simGroup = Object::customSimObjs;
        Trk::SimObject *simObjs = simGroup != 0 ? reinterpret_cast<Trk::SimObject *>(simGroup + 1) :
            reinterpret_cast<Trk::SimObject *>(sizeof(Group));
        BuildCustomObjectFacets(Vi, &gNativeRenderCache.horizonDraw, simObjs,
            Object::customObjInst, gCurrContext->polyFarZClipSq);
    }
}

sint32 Draw::CircleClip(coorddef *pt1, coorddef *pt2, sint32 r)
{
    FUNCTION_MARKER(0x800C903C, "NFS4.EXE");
    sint32 dx = (sint32)((uint32)pt1->x - (uint32)pt2->x);
    if (dx <= 0)
        dx = (sint32)((uint32)pt2->x - (uint32)pt1->x);
    sint32 dz = (sint32)((uint32)pt1->z - (uint32)pt2->z);
    if (dz <= 0)
        dz = (sint32)((uint32)pt2->z - (uint32)pt1->z);
    sint32 dist;
    if (dz < dx)
        dist = (sint32)((uint32)dx + (uint32)(dz >> 2));
    else
        dist = (sint32)((uint32)dz + (uint32)(dx >> 2));
    return dist < r;
}

void Draw::kCtrlSkidmark(Draw::tCtrlSkidmark *fskid)
{
    FUNCTION_MARKER(0x800C909C, "NFS4.EXE");
    sint32 ccount = fskid->count;
    matrixtdef *m = &fskid->m;
    coorddef *t = &fskid->t;
    Draw::tGiveShelbyMoreCache *sd = &gNativeRenderCache.world;
    for (sint32 row = 0; row < 3; ++row)
    {
        sint32 r0 = m->m[row];
        sint32 r1 = m->m[row + 3];
        sint32 r2 = m->m[row + 6];
        sd->matB.m[row][0] = (sint16)(r0 >> 4);
        sd->matB.m[row][1] = (sint16)(r1 >> 4);
        sd->matB.m[row][2] = (sint16)(r2 >> 4);
    }
    while (true)
    {
        ccount = (sint32)(uint32(ccount) - 1);
        if (ccount == -1)
            return;
        Skidmark::Chunk *sm = fskid->smp + ccount;
        if (!BWorld::IsSliceInBuildList(sm->slice) || !CircleClip(&sm->cp, t, 0x320000))
            continue;
        coorddef ts, td;
        ts.x = (sint32)(uint32(sm->cp.x) - uint32(t->x));
        ts.y = (sint32)(uint32(sm->cp.y) - uint32(t->y));
        ts.z = (sint32)(uint32(sm->cp.z) - uint32(t->z));
        transform(&ts, m, &td);
        sd->matB.t[0] = td.x >> 6;
        sd->matB.t[1] = td.y >> 6;
        sd->matB.t[2] = td.z >> 6;
        gte_SetRotMatrix(&sd->matB);
        gte_SetTransMatrix(&sd->matB);
        sint32 count = sm->n;
        for (sint32 i = 0; i < count; ++i)
        {
            Skidmark::Segment *segment = sm->seg + i;
            if (sd->head.cprim.PrimPtr >= sd->head.cprim.MPrimPtr || segment->next == 0)
                continue;
            gte_ldv0(&segment->svx[0]);
            gte_rtps();
            gte_stlvnl(&sd->tVn0);
            POLY_GT4 *prim = reinterpret_cast<POLY_GT4 *>(sd->head.cprim.PrimPtr);
            gte_stsxy(&prim->p[0].xy);
            gte_ldv0(&segment->svx[1]);
            gte_rtps();
            gte_stlvnl(&sd->tVn1);
            gte_ldv0(&segment->next->svx[1]);
            gte_rtps();
            gte_stlvnl(&sd->tVn2);
            gte_ldv0(&segment->next->svx[0]);
            gte_rtps();
            gte_stlvnl(&sd->tVn3);
            if (!(sd->tVn0.vx < sd->tVn0.vz || sd->tVn1.vx < sd->tVn1.vz ||
                sd->tVn2.vx < sd->tVn2.vz || sd->tVn3.vx < sd->tVn3.vz))
                continue;
            if (!((sint32)(0U - uint32(sd->tVn0.vx)) < sd->tVn0.vz ||
                (sint32)(0U - uint32(sd->tVn1.vx)) < sd->tVn1.vz ||
                (sint32)(0U - uint32(sd->tVn2.vx)) < sd->tVn2.vz ||
                (sint32)(0U - uint32(sd->tVn3.vx)) < sd->tVn3.vz))
                continue;
            sint32 type = segment->type;
            Draw::tPixMap *pmx = gSkidMarkPixmap[type & 1];
            gte_stsxy3(&prim->p[1].xy, &prim->p[3].xy, &prim->p[2].xy);
            gte_avsz4();
            gte_stotz(&sd->otz);
            sint32 save_pre_otz = sd->otz >> 5;
            sd->otz = (sint32)(uint32(save_pre_otz) + 50);
            if (sd->otz <= 0 || (sint32)(uint32(Draw::gViewOtSize) - 3) < sd->otz)
                return;
            if (type == 0)
            {
                memcpy(&prim->p[0].rgbc, &segment->rgb, 4);
                memcpy(&prim->p[1].rgbc, &segment->rgb, 4);
                memcpy(&prim->p[3].rgbc, &segment->next->rgb, 4);
                memcpy(&prim->p[2].rgbc, &segment->next->rgb, 4);
            }
            else
            {
                prim->p[0].rgbc.rgbc = 0x00404040;
                prim->p[1].rgbc.rgbc = 0x00404040;
                prim->p[3].rgbc.rgbc = 0x00404040;
                prim->p[2].rgbc.rgbc = 0x00404040;
            }
            setcode(prim, 0x3E);
            setlen(prim, 12);
            uint32 uv[4];
            memcpy(uv, pmx, sizeof(uv));
            prim->p[0].uv_attr = uv[0];
            prim->p[1].uv_attr = uv[1];
            prim->p[2].uv_attr = uv[2];
            prim->p[3].uv_attr = uv[3];
            if (prim->p[0].uv_attr >> 16 == 0xFFFF)
            {
                sint32 depth_index = (sint32)((uint32(save_pre_otz) - uint32((sint32)sd->startfog)) << 4);
                depth_index >>= (uint16)sd->distfog & 31;
                if (depth_index < 0)
                    depth_index = 0;
                else if (depth_index > 15)
                    depth_index = 15;
                uint16 clut = (uint16)gClutDepth[pmx->pad2][depth_index];
                prim->p[0].uv_attr = (prim->p[0].uv_attr & 0xFFFF) | (uint32(clut) << 16);
            }
            prim = reinterpret_cast<POLY_GT4 *>(sd->head.cprim.PrimPtr);
            uint32 *ot = sd->head.cprim.LastPrim + sd->otz;
            sd->head.cprim.PrimPtr = reinterpret_cast<char *>(prim + 1);
            prim->tag = 0x0C000000 | getaddr(ot);
            setaddr(ot, prim);
        }
    }
}

void DrawW::SetUpSubdividFacet_Line(Draw::tGiveShelbyMoreCache *sd)
{
    FUNCTION_MARKER(0x800C9620, "NFS4.EXE");
    Draw::SVertex *v0 = &sd->v0;
    Draw::SVertex *v1 = &sd->v1;
    Draw::SVertex *v2 = &sd->v2;
    Draw::SVertex *v3 = &sd->v3;
    LoadPrecVECTOR(v0, &sd->tVn0);
    LoadPrecVECTOR(v1, &sd->tVn1);
    LoadPrecVECTOR(v2, &sd->tVn2);
    LoadPrecVECTOR(v3, &sd->tVn3);
    POLY_GT4 *prim = &sd->GT4Prim;
    gte_ldv0(v0);
    xport_gte_execute(0x180001);
    gte_stsxy(&v0->dvx);
    gte_ldv3(v1, v2, v3);
    xport_gte_execute(0x280030);
    uint32 a = prim->p[1].rgbc.rgbc;
    uint32 b = prim->p[0].rgbc.rgbc;
    memcpy(&v2->r, &a, sizeof(a));
    memcpy(&v1->r, &a, sizeof(a));
    memcpy(&v3->r, &b, sizeof(b));
    memcpy(&v0->r, &b, sizeof(b));
    gte_stsxy3(&v1->dvx, &v2->dvx, &v3->dvx);
    Draw::SVertex *vertices[4] = { v0, v1, v2, v3 };
    uint16 u0 = prim->p[0].uv.uv;
    uint16 u1 = prim->p[1].uv.uv;
    uint16 u2 = prim->p[3].uv.uv;
    uint16 u3 = prim->p[2].uv.uv;
    memcpy(&v0->u, &u0, sizeof(u0));
    memcpy(&v1->u, &u1, sizeof(u1));
    memcpy(&v2->u, &u2, sizeof(u2));
    memcpy(&v3->u, &u3, sizeof(u3));
    for (sint32 i = 0; i < 4; ++i)
    {
        Draw::SVertex *vertex = vertices[i];
        vertex->a = 0;
        if (vertex->dvx > 1000 || vertex->dvy > 1000 || vertex->dvx < -1000 || vertex->dvy < -1000 || vertex->vz < 101)
            vertex->a = 1;
    }
    SubdividFacet(sd, 0, v0, v1, v2, v3, 0, 0);
}

void DrawW::OnyxLinePrim(CCOORD16 *geomVertices, Trk::Line *lineQuad, sint32 count, Draw::tGiveShelbyMoreCache *sd)
{
    FUNCTION_MARKER(0x800C98B8, "NFS4.EXE");
    sd->offsubdivid = 512;
    sint32 end = (sint32)(uint32(count) - 1);
    for (sint32 lineQuadCount = 0; lineQuadCount < end; ++lineQuadCount, ++lineQuad, geomVertices += 2)
    {
        sint32 linetype = lineQuad->type;
        if (linetype == 255)
            continue;
        Draw::tPixMap *pmx = gDLPixmap[linetype];
        CCOORD16 vt0 = geomVertices[3];
        CCOORD16 vt1 = geomVertices[1];
        CCOORD16 vt2 = geomVertices[0];
        CCOORD16 vt3 = geomVertices[2];
        gte_ldv0(&vt0);
        gte_rtps();
        gte_stlvnl(&sd->tVn0);
        POLY_GT4 *prim = reinterpret_cast<POLY_GT4 *>(gNativeRenderCache.world.head.cprim.PrimPtr);
        gte_stsxy(&prim->p[0].xy);
        gte_ldv0(&vt1);
        gte_rtps();
        gte_stlvnl(&sd->tVn1);
        gte_ldv0(&vt2);
        gte_rtps();
        gte_stlvnl(&sd->tVn2);
        gte_ldv0(&vt3);
        gte_rtps();
        gte_stlvnl(&sd->tVn3);
        if (!(sd->tVn0.vx < sd->tVn0.vz || sd->tVn1.vx < sd->tVn1.vz ||
            sd->tVn2.vx < sd->tVn2.vz || sd->tVn3.vx < sd->tVn3.vz))
            continue;
        if (!((sint32)(0U - uint32(sd->tVn0.vx)) < sd->tVn0.vz ||
            (sint32)(0U - uint32(sd->tVn1.vx)) < sd->tVn1.vz ||
            (sint32)(0U - uint32(sd->tVn2.vx)) < sd->tVn2.vz ||
            (sint32)(0U - uint32(sd->tVn3.vx)) < sd->tVn3.vz))
            continue;
        gte_stsxy3(&prim->p[1].xy, &prim->p[3].xy, &prim->p[2].xy);
        gte_avsz4();
        gte_stotz(&sd->otz);
        sint32 bfct;
        gte_nclip();
        gte_swc2(24, &bfct);
        if (sd->head.mirror == 1)
            bfct = (sint32)(0U - uint32(bfct));
        if (bfct < 0)
            continue;
        sint32 rawOtz = sd->otz;
        sint32 save_pre_otz = rawOtz >> 1;
        sd->otz = (sint32)(uint32(save_pre_otz) + 75);
        bool doSubdivision = rawOtz < 200;
        if (sd->otz <= 0 || (sint32)(uint32(Draw::gViewOtSize) - 3) < sd->otz)
            continue;
        if (doSubdivision)
            prim = &sd->GT4Prim;
        else
        {
            prim = reinterpret_cast<POLY_GT4 *>(sd->head.cprim.PrimPtr);
            uint32 *ot = sd->head.cprim.LastPrim + sd->otz;
            sd->head.cprim.PrimPtr = reinterpret_cast<char *>(prim + 1);
            prim->tag = 0x0C000000 | getaddr(ot);
            setaddr(ot, prim);
        }
        sint32 depthcue;
        gte_stdp(&depthcue);
        if (sd->nightFlags != 0)
        {
            VECTOR temp0;
            if ((sd->nightFlags & 1) != 0)
            {
                gte_SetRotMatrix(&sd->matNight);
                gte_SetTransMatrix(&sd->matNight);
                gte_ldv0(&vt2);
                gte_rt();
                gte_stlvnl(&temp0);
                gte_ldv0(&vt3);
                gte_rt();
                Night::NightCalc(&temp0, &vt2.light, sd);
                gte_stlvnl(&temp0);
                Night::NightCalc(&temp0, &vt3.light, sd);
            }
            if (BW::gCopCarObj != 0)
            {
                gte_SetRotMatrix(&sd->matCop);
                gte_SetTransMatrix(&sd->matCop);
                gte_ldv0(&vt2);
                gte_rt();
                gte_stlvnl(&temp0);
                gte_ldv0(&vt3);
                gte_rt();
                Night::NightCopCalc(&temp0, &vt2.light);
                gte_stlvnl(&temp0);
                Night::NightCopCalc(&temp0, &vt3.light);
            }
            CVECTOR *lightTable = Chunk::lightTable;
            uint32 a, b;
            memcpy(&a, lightTable + vt2.light, sizeof(a));
            memcpy(&b, lightTable + vt3.light, sizeof(b));
            prim->p[3].rgbc.rgbc = a;
            prim->p[1].rgbc.rgbc = a;
            prim->p[2].rgbc.rgbc = b;
            prim->p[0].rgbc.rgbc = b;
            gte_SetRotMatrix(&sd->matB);
            gte_SetTransMatrix(&sd->matB);
        }
        else
        {
            uint32 a;
            memcpy(&a, Chunk::lightTable + vt3.light, sizeof(a));
            gte_lwc2(6, a);
            gte_lddp(depthcue);
            gte_dpcs();
            gte_swc2(22, &prim->p[2].rgbc);
            prim->p[0].rgbc.rgbc = prim->p[2].rgbc.rgbc;
            memcpy(&a, Chunk::lightTable + vt2.light, sizeof(a));
            gte_lwc2(6, a);
            gte_lddp(depthcue);
            gte_dpcs();
            gte_swc2(22, &prim->p[3].rgbc);
            prim->p[1].rgbc.rgbc = prim->p[3].rgbc.rgbc;
        }
        setcode(prim, 0x3E);
        setlen(prim, 12);
        uint32 uv[4];
        memcpy(uv, pmx, sizeof(uv));
        prim->p[0].uv_attr = uv[0];
        prim->p[1].uv_attr = uv[1];
        prim->p[2].uv_attr = uv[2];
        prim->p[3].uv_attr = uv[3];
        if (prim->p[0].attr == 0xFFFF)
        {
            sint32 depth_index = (sint32)((uint32(save_pre_otz) - uint32((sint32)sd->startfog)) << 4);
            depth_index >>= (uint16)sd->distfog & 31;
            if (depth_index < 0)
                depth_index = 0;
            else if (depth_index > 15)
                depth_index = 15;
            prim->p[0].attr = (uint16)gClutDepth[pmx->pad2][depth_index];
        }
        if (doSubdivision)
        {
            gte_SetRotMatrix(&sd->identMat);
            gte_SetTransMatrix(&sd->identMat);
            if (stackSpeedUpEnbabledFlag == 0)
                SetUpSubdividFacet_Line(sd);
            else
            {
                stackSpeedUpEnbabledFlag = 0;
                SetUpSubdividFacet_Line(sd);
                stackSpeedUpEnbabledFlag = 1;
            }
            gte_SetRotMatrix(&sd->matB);
            gte_SetTransMatrix(&sd->matB);
        }
    }
}

void DrawW::BuildChunkCenterLineFacets(Chunk *chunkDat, Group *group, Draw::tGiveShelbyMoreCache *sd, COORD16 *trans)
{
    FUNCTION_MARKER(0x800CA0A4, "NFS4.EXE");
    CCOORD16 *pts3d = gVertex3d;
    Trk::Line *lineQuad = reinterpret_cast<Trk::Line *>(group + 1);
    sint32 sliceBase = chunkDat->firstSimSliceInd;
    uint16 tx = trans->x;
    uint16 ty = trans->y;
    uint16 tz = trans->z;
    CCOORD16 *wpts = reinterpret_cast<CCOORD16 *>(chunkDat->vertexBuf + 1);
    Trk::NewSlice *slices = BWorldSm::slices;
    for (sint32 i = 0; i < (sint32)((uint32)group->m_num_elements << 1); i += 2)
    {
        sint32 point = lineQuad->firstPoint;
        sint32 slice = sliceBase + lineQuad->slice;
        ++lineQuad;
        CCOORD16 *pts = wpts + point;
        sint32 wx = (sint8)slices[slice].right[0] >> 3;
        sint32 wy = (sint8)slices[slice].right[1] >> 3;
        sint32 wz = (sint8)slices[slice].right[2] >> 3;
        sint32 x = (uint16)pts->x;
        sint32 y = (uint16)pts->y;
        sint32 z = (uint16)pts->z;
        pts3d[0].x = (sint16)(tx + x - wx);
        pts3d[0].y = (sint16)(ty + y - wy);
        pts3d[0].z = (sint16)(tz + z - wz);
        pts3d[0].light = pts->light;
        pts3d[1].x = (sint16)(tx + x + wx);
        pts3d[1].y = (sint16)(ty + y + wy);
        pts3d[1].z = (sint16)(tz + z + wz);
        pts3d[1].light = pts->light;
        pts3d += 2;
    }
    OnyxLinePrim(gVertex3d, reinterpret_cast<Trk::Line *>(group + 1), group->m_num_elements, sd);
}

void DrawW::DoLines(DRender::tView *Vi, tBuildEntry *buildList, Draw::DCache *sd)
{
    FUNCTION_MARKER(0x800CA204, "NFS4.EXE");
    sint32 chunkCount = BWorld::gChunkCount;
    Draw::tGiveShelbyMoreCache *world = reinterpret_cast<Draw::tGiveShelbyMoreCache *>(sd);
    world->doublelayer = 0;
    world->matB.t[2] = 0;
    world->matB.t[1] = 0;
    world->matB.t[0] = 0;
    gte_SetTransMatrix(&world->matB);
    for (sint32 buildInd = 0; buildInd < chunkCount; ++buildInd, ++buildList)
    {
        if (buildList->geomRez != 4)
            continue;
        Chunk *chunkDat = Track::chunkList + buildList->chunkInd;
        Group *group = chunkDat->lineBuf;
        if (group == 0 || (buildList->enableBits & 4) == 0)
            continue;
        if (gNight_renderNight != 0)
        {
            uint8 nightFlags = world->nightFlags;
            world->nightFlags = nightFlags | 4;
            if ((Cars::gList[Vi->player]->control.lights & 6) != 0)
                world->nightFlags = nightFlags | 5;
            coorddef tmp, tmp2;
            tmp.x = (sint32)((uint32)Vi->cview.translation.x - (uint32)Camera::gInfo[Vi->player].target->position.x);
            tmp.y = (sint32)((uint32)Vi->cview.translation.y - (uint32)Camera::gInfo[Vi->player].target->position.y);
            tmp.z = (sint32)((uint32)Vi->cview.translation.z - (uint32)Camera::gInfo[Vi->player].target->position.z);
            transform(&tmp, &gNightMat, &tmp2);
            WorldSetUpTranslation(&tmp2, &world->matNight);
            Car::tObj *cop = BW::gCopCarObj;
            if (cop != 0)
            {
                tmp.x = (sint32)((uint32)Vi->cview.translation.x - (uint32)cop->N.position.x);
                tmp.y = (sint32)((uint32)Vi->cview.translation.y - (uint32)cop->N.position.y);
                tmp.z = (sint32)((uint32)Vi->cview.translation.z - (uint32)cop->N.position.z);
                transform(&tmp, &gCopMat, &tmp2);
                WorldSetUpTranslation(&tmp2, &world->matCop);
            }
            world->matB.t[2] = 0;
            world->matB.t[1] = 0;
            world->matB.t[0] = 0;
            gte_SetTransMatrix(&world->matB);
        }
        coorddef *pChunkCp = Chunk::chunkCenters + buildList->chunkInd;
        COORD16 trans;
        trans.x = (sint16)((sint32)((uint32)pChunkCp->x - (uint32)Vi->cview.translation.x) >> 10);
        trans.y = (sint16)((sint32)((uint32)pChunkCp->y - (uint32)Vi->cview.translation.y) >> 10);
        trans.z = (sint16)((sint32)((uint32)pChunkCp->z - (uint32)Vi->cview.translation.z) >> 10);
        BuildChunkCenterLineFacets(chunkDat, group, world, &trans);
    }
}

void DrawW::BuildSpikeBelt(DRender::tView *Vi, sint32 scale, Draw::DCache *sd)
{
    FUNCTION_MARKER(0x800CA520, "NFS4.EXE");
    sint32 slice = gSpikeBeltSlice;
    Trk::NewSlice *sliceData = BWorldSm::slices + slice;
    sint32 step = scale / 8;
    sint16 wx = (sint16)(uint32(fixedmult((sint8)sliceData->right[0] * 512, step)) >> 10);
    sint16 wy = (sint16)(uint32(fixedmult((sint8)BWorldSm::slices[slice].right[1] * 512, step)) >> 10);
    sint16 wz = (sint16)(uint32(fixedmult((sint8)BWorldSm::slices[slice].right[2] * 512, step)) >> 10);
    sint16 fx = (sint8)BWorldSm::slices[slice].forward[0] >> 1;
    sint16 fy = (sint8)BWorldSm::slices[slice].forward[1] >> 1;
    sint16 fz = (sint8)BWorldSm::slices[slice].forward[2] >> 1;
    sint16 sx = (sint16)(fixedmult(gSpikeBeltX, (sint8)BWorldSm::slices[slice].right[0] * 512) >> 10);
    sint16 sy = (sint16)(fixedmult(gSpikeBeltX, (sint8)BWorldSm::slices[slice].right[1] * 512) >> 10);
    sint16 sz = (sint16)(fixedmult(gSpikeBeltX, (sint8)BWorldSm::slices[slice].right[2] * 512) >> 10);
    CCOORD16 vertex3d[27];
    vertex3d[0].x = (sint16)(sx - fx);
    vertex3d[0].y = (sint16)(sy - fy);
    vertex3d[0].z = (sint16)(sz - fz);
    vertex3d[0].light = 0;
    for (sint32 i = 1; i < 9; ++i)
    {
        vertex3d[i].x = (sint16)(vertex3d[i - 1].x + wx);
        vertex3d[i].y = (sint16)(vertex3d[i - 1].y + wy);
        vertex3d[i].z = (sint16)(vertex3d[i - 1].z + wz);
        vertex3d[i].light = 0;
    }
    vertex3d[9].x = sx;
    vertex3d[9].y = (sint16)(sy + 25);
    vertex3d[9].z = sz;
    vertex3d[9].light = 0;
    for (sint32 i = 10; i < 18; ++i)
    {
        vertex3d[i].x = (sint16)(vertex3d[i - 1].x + wx);
        vertex3d[i].y = (sint16)(vertex3d[i - 1].y + wy);
        vertex3d[i].z = (sint16)(vertex3d[i - 1].z + wz);
        vertex3d[i].light = 0;
    }
    vertex3d[18].x = (sint16)(sx + fx);
    vertex3d[18].y = (sint16)(sy + fy);
    vertex3d[18].z = (sint16)(sz + fz);
    vertex3d[18].light = 0;
    for (sint32 i = 19; i < 27; ++i)
    {
        vertex3d[i].x = (sint16)(vertex3d[i - 1].x + wx);
        vertex3d[i].y = (sint16)(vertex3d[i - 1].y + wy);
        vertex3d[i].z = (sint16)(vertex3d[i - 1].z + wz);
        vertex3d[i].light = 0;
    }
    Trk::Quad quads[16];
    for (sint32 i = 0; i < 8; ++i)
    {
        quads[i].material = 0;
        quads[i].aPoints[0] = (uint8)(i + 1);
        quads[i].aPoints[1] = (uint8)(i + 10);
        quads[i].aPoints[2] = (uint8)(i + 9);
        quads[i].aPoints[3] = (uint8)i;
        quads[i + 8].material = 0;
        quads[i + 8].aPoints[0] = (uint8)(i + 18);
        quads[i + 8].aPoints[1] = (uint8)(i + 9);
        quads[i + 8].aPoints[2] = (uint8)(i + 10);
        quads[i + 8].aPoints[3] = (uint8)(i + 19);
    }
    Track::tMaterial material;
    material.flag = 0;
    material.mipmap_offset = 0;
    material.pmxIndex = (sint16)(uint32(gInitialArt.shapeCount) - 1);
    Draw::tGiveShelbyMoreCache *world = reinterpret_cast<Draw::tGiveShelbyMoreCache *>(sd);
    world->nightFlags = 0;
    coorddef tmp, tmp2;
    tmp.x = (sint32)(uint32(sliceData->center[0]) - uint32(Vi->cview.translation.x));
    tmp.y = (sint32)(uint32(sliceData->center[1]) - uint32(Vi->cview.translation.y));
    tmp.z = (sint32)(uint32(sliceData->center[2]) - uint32(Vi->cview.translation.z));
    transform(&tmp, &gWorldMat, &tmp2);
    WorldSetUpTranslation(&tmp2, &world->matB);
    world->vertices = vertex3d;
    world->quadCount = 16;
    world->quads = quads;
    world->offset = 35;
    world->materials = &material;
    world->trans.x = 0;
    world->trans.y = 0;
    world->trans.z = 0;
    world->zeroGTETransFlag = 0;
    world->light = -1;
    kCtrlWorld_High(world);
}
