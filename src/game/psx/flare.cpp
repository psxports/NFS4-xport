#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "psx_gte.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/bworld.h"
#include "game/common/camera.h"
#include "game/common/genericpmx.h"
#include "game/common/gmesetup.h"
#include "game/common/mathnfs.h"
#include "game/common/r3dcar.h"
#include "game/common/renderview.h"
#include "game/common/sim.h"
#include "game/psx/draw.h"
#include "game/psx/drawc.h"
#include "game/psx/flare.h"
#include "game/psx/trackspec.h"
#include "lib/psx/random.h"

sint32 gscale = 4096;
FLARE_DEF gFlare_LensFlare;
CVECTOR gfrgb = { 255, 255, 255, 0 };
CVECTOR gfrgb2 = { 64, 64, 128, 0 };
sint16 gfSpikePt0[8] = { 3, 3, 0, 0, 1, 1, 2, 2 };
sint16 gfSpikePt1[8] = { 1, 2, 2, 3, 3, 0, 0, 1 };
sint16 gfSpikePt2[8] = { 4, 5, 6, 7, 8, 9, 10, 11 };
sint16 gfOctPt1[8] = { 4, 5, 6, 7, 8, 9, 10, 11 };
sint16 gfOctPt2[8] = { 5, 6, 7, 8, 9, 10, 11, 4 };
sint16 gfHexPt1[6] = { 0, 1, 2, 3, 4, 5 };
sint16 gfHexPt2[6] = { 1, 2, 3, 4, 5, 0 };

SVECTOR gOv[4] =
{
    {-2, 0, 0, 0},
    {2, 0, 0, 0},
    {-2, 256, 0, 0},
    {2, 256, 0, 0},
};

SVECTOR Flare::gSpikes[4] =
{
    {0, -45, 0, 0},
    {45, 0, 0, 0},
    {0, 45, 0, 0},
    {-45, 0, 0, 0},
};

SVECTOR Flare::gOct[8] =
{
    {0, -256, 0, 0},
    {180, -180, 0, 0},
    {256, 0, 0, 0},
    {180, 180, 0, 0},
    {0, 256, 0, 0},
    {-180, 180, 0, 0},
    {-256, 0, 0, 0},
    {-180, -180, 0, 0},
};

SVECTOR Flare::gSmallOct[8] =
{
    {0, -45, 0, 0},
    {31, -31, 0, 0},
    {45, 0, 0, 0},
    {31, 31, 0, 0},
    {0, 45, 0, 0},
    {-31, 31, 0, 0},
    {-45, 0, 0, 0},
    {-31, -31, 0, 0},
};

SVECTOR Flare::gHex[6] =
{
    {-256, 0, 0, 0},
    {-128, -221, 0, 0},
    {128, -221, 0, 0},
    {256, 0, 0, 0},
    {128, 221, 0, 0},
    {-128, 221, 0, 0},
};

SVECTOR Flare::gReflectHex[6] =
{
    {256, 0, 0, 0},
    {128, 221, 0, 0},
    {-128, 221, 0, 0},
    {-256, 0, 0, 0},
    {-256, -512, 0, 0},
    {256, -512, 0, 0},
};

SVECTOR Flare::gQuad[4] =
{
    {0, -256, 0, 0},
    {256, 0, 0, 0},
    {0, 256, 0, 0},
    {-256, 0, 0, 0},
};

SVECTOR Flare::gLensOct[8] =
{
    {0, -256, 0, 0},
    {180, -180, 0, 0},
    {-180, -180, 0, 0},
    {256, 0, 0, 0},
    {-256, 0, 0, 0},
    {180, 180, 0, 0},
    {-180, 180, 0, 0},
    {0, 256, 0, 0},
};

Flare::tInfo Flare::gType[34] =
{
    {8421504, 8421504, 1024, 0},
    {3158064, 8421504, 0, 3},
    {526440, 394792, 1, 13},
    {8421504, 3158064, 0, 13},
    {4222912, 526528, 0, 13},
    {4222912, 526528, 0, 13},
    {7368816, 1052696, 0, 13},
    {526464, 526376, 0, 13},
    {526464, 526376, 0, 12},
    {557184, 534568, 0, 12},
    {8390664, 2623496, 0, 12},
    {3158064, 8421504, 0, 3},
    {526440, 394792, 2, 13},
    {8421504, 3158064, 1, 13},
    {4222912, 526528, 1, 13},
    {4222912, 526528, 1, 13},
    {7368816, 1052696, 1, 13},
    {526464, 526376, 1, 13},
    {526464, 526376, 1, 12},
    {557184, 534568, 1, 12},
    {8390664, 2623496, 1, 12},
    {526464, 526376, 0, 0},
    {662658, 132116, 675, 136},
    {48830, 17990, 0, 0},
    {12632256, 0, 770, 4},
    {3688552, 2105376, 1795, 12},
    {6314040, 1579032, 3594, 12},
    {2123920, 1581088, 3594, 12},
    {2123920, 1581088, 1281, 8},
    {6318248, 2105376, 734, 8},
    {13158600, 2105376, 16384, 9},
    {2123920, 1581088, 1281, 24},
    {7572224, 0, 1024, 12},
    {16711680, 0, 256, 8},
};

FLARE_PIECE_DEF Flare::SunFlarePieces[9] =
{
    {0, 19660, {25, 10, 0, 0}, 1},
    {-9830, 65536, {16, 5, 0, 0}, 0},
    {-16384, 6553, {15, 15, 3, 0}, 0},
    {9830, 26214, {21, 10, 0, 0}, 2},
    {29491, 26214, {6, 6, 20, 0}, 0},
    {49152, 19660, {21, 15, 8, 0}, 4},
    {55705, 13107, {10, 20, 31, 0}, 2},
    {65536, 6553, {10, 20, 10, 0}, 0},
    {75366, 19660, {31, 15, 16, 0}, 2},
};

void Flare::Tri(sint32 *cp, sint32 *p1, sint32 *p2, sint32 otz)
{
    FUNCTION_MARKER(0x800CBCB0, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_G3 *prim = (POLY_G3 *)cprim->PrimPtr;
    uint32 *ot = cprim->LastPrim + otz;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = (char *)(prim + 1);
    setaddr(ot, prim);
    prim->p[0].rgbc.rgbc = 0x32000000;
    uint32 color;
    memcpy(&color, &gfrgb, sizeof(color));
    prim->p[2].rgbc.rgbc = 0;
    setlen(prim, 6);
    prim->p[1].rgbc.rgbc = color;
    prim->p[0].xy.xy = *p2;
    prim->p[1].xy.xy = *cp;
    prim->p[2].xy.xy = *p1;
}

void Flare::SetMatrix(matrixtdef *m)
{
    FUNCTION_MARKER(0x800CBD4C, "NFS4.EXE");
    MATRIX mpsx;
    for (sint32 row = 0; row < 3; ++row)
    {
        sint32 r0 = m->m[row];
        sint32 r1 = m->m[row + 3];
        sint32 r2 = m->m[row + 6];
        mpsx.m[row][0] = (sint16)(r0 >> 4);
        mpsx.m[row][1] = (sint16)(r1 >> 4);
        mpsx.m[row][2] = (sint16)(r2 >> 4);
    }
    gte_SetRotMatrix(&mpsx);
    gte_ldtr(0, 0, 0);
}

void Flare::IdentMatrix(MATRIX *mtx)
{
    FUNCTION_MARKER(0x800CBDF8, "NFS4.EXE");
    const uint32 words[5] = { 0x1000, 0, 0x1000, 0, 0x1000 };
    memcpy(mtx, words, sizeof(words));
}

void Flare::OctFlare(sint32 *center, sint32 otz)
{
    FUNCTION_MARKER(0x800CBE14, "NFS4.EXE");
    sint32 flare_dvxy[13];
    uint32 rgb1;
    memcpy(&rgb1, &gfrgb, 4);
    for (sint32 i = 0; i < 8; ++i)
    {
        gte_ldv0(&gOct[i]);
        gte_rtps();
        gte_stsxy(&flare_dvxy[i + 4]);
    }
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    for (sint32 i = 7; i >= 0; --i)
    {
        uint32 *ot = cprim->LastPrim + otz;
        POLY_G3 *prim = reinterpret_cast<POLY_G3 *>(cprim->PrimPtr);
        setaddr(prim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
        setaddr(ot, prim);
        prim->p[0].rgbc.rgbc = 0x32000000;
        prim->p[1].rgbc.rgbc = rgb1;
        prim->p[2].rgbc.rgbc = 0;
        setlen(prim, 6);
        prim->p[0].xy.xy = flare_dvxy[gfOctPt2[i]];
        prim->p[1].xy.xy = *center;
        prim->p[2].xy.xy = flare_dvxy[gfOctPt1[i]];
    }
}

void Flare::OctFlareSpikes(sint32 *center, sint32 otz)
{
    FUNCTION_MARKER(0x800CC034, "NFS4.EXE");
    sint32 flare_dvxy[14];
    for (sint32 i = 0; i < 4; ++i)
    {
        gte_ldv0(&gSpikes[i]);
        xport_gte_execute(0x180001);
        gte_stsxy(&flare_dvxy[i]);
    }
    for (sint32 i = 0; i < 8; ++i)
    {
        gte_ldv0(&gOct[i]);
        xport_gte_execute(0x180001);
        gte_stsxy(&flare_dvxy[i + 4]);
    }
    uint32 rgb1, rgb2;
    memcpy(&rgb1, &gfrgb, sizeof(rgb1));
    memcpy(&rgb2, &gfrgb2, sizeof(rgb2));
    sint32 cent = *center;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    for (sint32 i = 7; i >= 0; --i)
    {
        sint32 id0 = gfSpikePt0[i];
        sint32 id1 = gfSpikePt1[i];
        sint32 id2 = gfSpikePt2[i];
        POLY_G4 *prim4 = (POLY_G4 *)cprim->PrimPtr;
        uint32 *ot = cprim->LastPrim + otz;
        setaddr(prim4, getaddr(ot));
        cprim->PrimPtr = (char *)(prim4 + 1);
        setaddr(ot, prim4);
        prim4->p[0].rgbc.rgbc = 0x3A000000;
        setlen(prim4, 8);
        prim4->p[1].rgbc.rgbc = rgb2;
        prim4->p[2].rgbc.rgbc = 0;
        prim4->p[3].rgbc.rgbc = 0;
        uint32 xy0 = flare_dvxy[id0];
        prim4->p[1].xy.xy = cent;
        prim4->p[0].xy.xy = xy0;
        prim4->p[2].xy.xy = flare_dvxy[id2];
        prim4->p[3].xy.xy = flare_dvxy[id1];
        id0 = gfOctPt1[i];
        id1 = gfOctPt2[i];
        POLY_G3 *prim3 = (POLY_G3 *)cprim->PrimPtr;
        ot = cprim->LastPrim + otz;
        setaddr(prim3, getaddr(ot));
        cprim->PrimPtr = (char *)(prim3 + 1);
        setaddr(ot, prim3);
        prim3->p[0].rgbc.rgbc = 0x32000000;
        setlen(prim3, 6);
        prim3->p[1].rgbc.rgbc = rgb1;
        prim3->p[2].rgbc.rgbc = 0;
        xy0 = flare_dvxy[id0];
        prim3->p[1].xy.xy = cent;
        prim3->p[0].xy.xy = xy0;
        prim3->p[2].xy.xy = flare_dvxy[id1];
    }
}

void Flare::Spikes(sint32 *center, sint32 otz)
{
    FUNCTION_MARKER(0x800CC3B8, "NFS4.EXE");
    sint32 flare_dvxy[13];
    for (sint32 i = 0; i < 4; ++i)
    {
        gte_ldv0(&gSpikes[i]);
        gte_rtps();
        gte_stsxy(&flare_dvxy[i]);
    }
    for (sint32 i = 0; i < 8; ++i)
    {
        gte_ldv0(&gOct[i]);
        gte_rtps();
        gte_stsxy(&flare_dvxy[i + 4]);
    }
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    for (sint32 i = 7; i >= 0; --i)
    {
        uint32 *ot = cprim->LastPrim + otz;
        POLY_G4 *prim = reinterpret_cast<POLY_G4 *>(cprim->PrimPtr);
        setaddr(prim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
        setaddr(ot, prim);
        prim->p[0].rgbc.rgbc = 0x3A000000;
        uint32 rgb;
        memcpy(&rgb, &gfrgb2, 4);
        prim->p[2].rgbc.rgbc = 0;
        prim->p[3].rgbc.rgbc = 0;
        setlen(prim, 8);
        prim->p[1].rgbc.rgbc = rgb;
        prim->p[0].xy.xy = flare_dvxy[gfSpikePt0[i]];
        prim->p[1].xy.xy = *center;
        prim->p[2].xy.xy = flare_dvxy[gfSpikePt2[i]];
        prim->p[3].xy.xy = flare_dvxy[gfSpikePt1[i]];
    }
}

void Flare::HexFlare(sint32 *center, sint32 otz)
{
    FUNCTION_MARKER(0x800CC694, "NFS4.EXE");
    sint32 flare_dvxy[8];
    for (sint32 i = 0; i < 6; ++i)
    {
        gte_ldv0(&gHex[i]);
        xport_gte_execute(0x180001);
        gte_stsxy(&flare_dvxy[i]);
    }
    for (sint32 i = 5; i >= 0; --i)
        Tri(center, &flare_dvxy[gfHexPt1[i]], &flare_dvxy[gfHexPt2[i]], otz);
}

void Flare::ReflectHexFlare(sint32 *center, sint32 otz)
{
    FUNCTION_MARKER(0x800CC868, "NFS4.EXE");
    sint32 flare_dvxy[8];
    for (sint32 i = 0; i < 6; ++i)
    {
        gte_ldv0(&gReflectHex[i]);
        xport_gte_execute(0x180001);
        gte_stsxy(&flare_dvxy[i]);
    }
    for (sint32 i = 5; i >= 0; --i)
        Tri(center, &flare_dvxy[gfHexPt1[i]], &flare_dvxy[gfHexPt2[i]], otz);
}

void Flare::QuadFlare(sint32 *center, sint32 otz)
{
    FUNCTION_MARKER(0x800CCA3C, "NFS4.EXE");
    sint32 pt[3];
    gte_ldv0(&gQuad[0]);
    xport_gte_execute(0x180001);
    gte_stsxy(&pt[2]);
    gte_ldv0(&gQuad[1]);
    xport_gte_execute(0x180001);
    gte_stsxy(&pt[1]);
    Tri(center, &pt[2], &pt[1], otz);
    gte_ldv0(&gQuad[2]);
    xport_gte_execute(0x180001);
    gte_stsxy(&pt[0]);
    Tri(center, &pt[1], &pt[0], otz);
    gte_ldv0(&gQuad[3]);
    xport_gte_execute(0x180001);
    gte_stsxy(&pt[1]);
    Tri(center, &pt[0], &pt[1], otz);
    Tri(center, &pt[1], &pt[2], otz);
}

void Flare::CarShapedHalo(sint32 type, COORD16 *ptCenter, COORD16 *pt1, COORD16 *pt2, sint16 flag, sint32 otz, Draw::tCacheHeader *sd)
{
    FUNCTION_MARKER(0x800CCB58, "NFS4.EXE");
    if ((uintptr_t)sd->cprim.PrimPtr >= (uintptr_t)sd->cprim.MPrimPtr - 1024)
        return;
    COORD16 vec1, vec2, crossprod;
    vec1.x = (sint16)(pt1->x - ptCenter->x);
    vec1.y = (sint16)(pt1->y - ptCenter->y);
    vec1.z = (sint16)(pt1->z - ptCenter->z);
    vec2.x = (sint16)(pt2->x - ptCenter->x);
    vec2.y = (sint16)(pt2->y - ptCenter->y);
    vec2.z = (sint16)(pt2->z - ptCenter->z);
    crossprod.x = (sint16)(vec2.y * (uint32)vec1.z - vec2.z * (uint32)vec1.y);
    crossprod.y = (sint16)(vec2.z * (uint32)vec1.x - vec2.x * (uint32)vec1.z);
    crossprod.z = (sint16)(vec2.x * (uint32)vec1.y - vec2.y * (uint32)vec1.x);
    COORD16 *ptEdge = flag < 0 ? pt1 : pt2;
    sint32 tableIndex = (type & 0x7F) + (R3DCar::InMenu == 0 ? 11 : 1);
    if ((type & 0x100) != 0)
        ptCenter->y = (sint16)(-DrawC::gReflectOffset - ptCenter->y);
    gte_ldv0(ptCenter);
    xport_gte_execute(0x180001);
    CVECTOR color[2];
    uint32 chalo = gType[tableIndex].chalo;
    memcpy(&color[0], &chalo, sizeof(chalo));
    uint32 cbeam = gType[tableIndex].cbeam;
    memcpy(&color[1], &cbeam, sizeof(cbeam));
    sint32 scale = gType[tableIndex].scale;
    sint32 flags = gType[tableIndex].flags;
    if ((type & 0x80) != 0)
        for (sint32 i = 0; i < 2; ++i)
        {
            color[i].r >>= 1;
            color[i].g >>= 1;
            color[i].b >>= 1;
        }
    VECTOR diff;
    gte_stlvnl(&diff);
    if (diff.vx > diff.vz || -diff.vx > diff.vz || diff.vz < 128)
        return;
    sint32 sp;
    gte_stsxy(&sp);
    DVECTOR screen;
    memcpy(&screen, &sp, sizeof(screen));
    sint32 angleZ = (flags & 8) != 0 ? (screen.vx + screen.vy) * 4 : 0;
    sint32 angleOuter = 0;
    sint32 sizeOuter = 0;
    if ((flags & 4) != 0)
    {
        gte_ldv0(ptEdge);
        xport_gte_execute(0x180001);
        VECTOR diff2;
        gte_stlvnl(&diff2);
        DVECTOR sp2;
        gte_stsxy(&sp2);
        sint32 dify = sp2.vy - screen.vy;
        sint32 difx = sp2.vx - screen.vx;
        angleOuter = fixedatan(dify, difx) >> 4;
        uint32 distance = isqrt(difx * (uint32)difx + dify * (uint32)dify);
        sizeOuter = (distance * diff.vz);
    }
    gfrgb = color[0];
    gfrgb2 = color[1];
    COORD16 *edgeVector = flag < 0 ? &vec2 : &vec1;
    uint32 squares = (edgeVector->x * edgeVector->x) + (uint32)(edgeVector->y * edgeVector->y) + (uint32)(edgeVector->z * edgeVector->z);
    gscale = (sint32)squares >> 1;
    sint32 scaled = ((uint32)gscale << (scale & 31));
    gscale = scaled;
    if ((flags & 1) != 0)
    {
        gte_ldtr(0, 0, 0);
        gte_ldv0(&crossprod);
        xport_gte_execute(0x180001);
        VECTOR transformed, normalized;
        gte_stlvnl(&transformed);
        VectorNormal(&transformed, &normalized);
        if ((flags & 2) != 0)
            normalized.vz = ((normalized.vz - 0xF33) * 20);
        sint32 intensity = (normalized.vz - 600);
        if (intensity < 0)
            intensity = 0;
        normalized.vz = intensity;
        gscale = (gscale * intensity) >> 11;
    }
    if ((type & 0x100) != 0)
    {
        sint32 reflectionScale = (scaled + (sizeOuter >> 7));
        CVECTOR save = gfrgb;
        uint32 words[5] = { (uint32)reflectionScale, 0, (uint32)reflectionScale, 0, 0 };
        MATRIX scalemat, mtx;
        memcpy(&scalemat, words, sizeof(words));
        gfrgb.r >>= 1;
        gfrgb.g >>= 1;
        gfrgb.b >>= 1;
        IdentMatrix(&mtx);
        gte_SetTransVector(&diff);
        RotMatrixZ(0x800, &mtx);
        gte_SetRotMatrix(&mtx);
        for (sint32 column = 0; column < 3; ++column)
        {
            gte_ldclmv(&scalemat.m[0][column]);
            gte_rtir();
            gte_stclmv(&mtx.m[0][column]);
        }
        gte_SetRotMatrix(&mtx);
        ReflectHexFlare(&sp, otz);
        gfrgb = save;
    }
    else
    {
        sint32 radius = gscale;
        uint32 words[5] = { radius + (uint32)(sizeOuter >> 5), 0, (uint32)radius, 0, 0 };
        MATRIX scalemat, mtx, mtx2;
        memcpy(&scalemat, words, sizeof(words));
        IdentMatrix(&mtx);
        IdentMatrix(&mtx2);
        gte_SetTransVector(&diff);
        RotMatrixZ(angleZ, &mtx);
        RotMatrixZ(angleOuter, &mtx2);
        gte_SetRotMatrix(&mtx2);
        for (sint32 column = 0; column < 3; ++column)
        {
            gte_ldclmv(&scalemat.m[0][column]);
            gte_rtir();
            gte_stclmv(&scalemat.m[0][column]);
        }
        gte_SetRotMatrix(&scalemat);
        for (sint32 column = 0; column < 3; ++column)
        {
            gte_ldclmv(&mtx.m[0][column]);
            gte_rtir();
            gte_stclmv(&mtx.m[0][column]);
        }
        gte_SetRotMatrix(&mtx);
        if (diff.vz < 0xC80)
            OctFlareSpikes(&sp, otz);
        else if (diff.vz < 0x1B80)
        {
            HexFlare(&sp, otz);
            PreCalcHexLightBeam(&sp, otz);
        }
        else
            QuadFlare(&sp, otz);
    }
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    DR_MODE *aprim = (DR_MODE *)cprim->PrimPtr;
    uint32 *ot = cprim->LastPrim + otz;
    setaddr(aprim, getaddr(ot));
    cprim->PrimPtr = (char *)(aprim + 1);
    setaddr(ot, aprim);
    SetDrawMode(aprim, 0, (flags & 0x40) != 0, 0x120, 0);
}

void Flare::Halo2(DRender::tView *Vi, sint32 scale, sint32 type, coorddef *fpt, coorddef *fpt2, Draw::FlareCache *sd)
{
    FUNCTION_MARKER(0x800CD530, "NFS4.EXE");
    sint32 flare_type = (uint8)type;
    sint32 flags = gType[flare_type].flags;
    sint32 angleZ = 0;
    if (fpt2 == 0)
        flags &= ~5;
    coorddef translation = Vi->cview.translation;
    SVECTOR sdiff;
    sdiff.vx = (sint16)((fpt->x - (uint32)translation.x) >> 10);
    sdiff.vy = (sint16)((fpt->y - (uint32)translation.y) >> 10);
    sdiff.vz = (sint16)((fpt->z - (uint32)translation.z) >> 10);
    SVECTOR sdiff2;
    if ((flags & 5) != 0)
    {
        sdiff2.vx = (sint16)((fpt2->x - (uint32)translation.x) >> 10);
        sdiff2.vy = (sint16)((fpt2->y - (uint32)translation.y) >> 10);
        sdiff2.vz = (sint16)((fpt2->z - (uint32)translation.z) >> 10);
    }
    if ((flags & 0x10) != 0)
    {
        if ((simGlobal.gameTicks & 0x40U) != 0)
            return;
    }
    else if ((flags & 0x20) != 0)
    {
        if (((simGlobal.gameTicks + 27U) & 0x20U) != 0)
            return;
    }
    SetMatrix(&gWorldMat);
    gte_ldv0(&sdiff);
    gte_rtps();
    CVECTOR color[2];
    memcpy(&color[0], &gType[flare_type].chalo, 4);
    memcpy(&color[1], &gType[flare_type].cbeam, 4);
    if (scale == -1)
        scale = gType[flare_type].scale;
    VECTOR diff;
    gte_stlvnl(&diff);
    if (diff.vx > diff.vz || -diff.vx > diff.vz || diff.vz < 128)
        return;
    DVECTOR sp;
    gte_stsxy(&sp);
    sint32 otz;
    gte_stsz(&otz);
    otz = (otz >> 2) >> 1;
    if (otz < 0 || otz > (Draw::gViewOtSize - 3))
        return;
    if ((flags & 0x80) != 0)
    {
        uint32 sample = random() % 20U;
        scale = (scale + sample * 4U);
        color[0].r = (uint8)(color[0].r + sample);
        color[0].g = (uint8)(color[0].g + sample);
        color[0].b = (uint8)(color[0].b + sample);
        color[1].r = (uint8)(color[1].r + sample);
        color[1].g = (uint8)(color[1].g + sample);
        color[1].b = (uint8)(color[1].b + sample);
    }
    if ((flags & 8) != 0)
        angleZ = (sp.vx + sp.vy) * 4;
    memcpy(&gfrgb, &color[0], 4);
    memcpy(&gfrgb2, &color[1], 4);
    gscale = scale;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    if ((flags & 0x40) != 0)
    {
        DR_MODE *aprim = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim + otz;
        setaddr(aprim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(aprim + 1);
        setaddr(ot, aprim);
        SetDrawMode(aprim, 0, 0, 0x120, 0);
    }
    sint32 z = diff.vz;
    VECTOR diff2;
    DVECTOR sp2;
    if ((flags & 5) != 0)
    {
        gte_ldv0(&sdiff2);
        gte_rtps();
        gte_stlvnl(&diff2);
        gte_stsxy(&sp2);
    }
    if ((flags & 4) == 0)
    {
        if ((flags & 1) != 0)
        {
            VECTOR tvec;
            tvec.vx = (diff.vx - diff2.vx);
            tvec.vy = (diff.vy - diff2.vy);
            tvec.vz = (diff.vz - diff2.vz);
            VECTOR tvec2;
            VectorNormal(&tvec, &tvec2);
            if ((flags & 2) != 0)
                tvec2.vz = ((tvec2.vz - 0xF33) * 20);
            tvec2.vz = (tvec2.vz - 600);
            if (tvec2.vz < 0)
                tvec2.vz = 0;
            if (tvec2.vz == 0)
                return;
            gscale = (gscale * tvec2.vz) >> 11;
            if (type == 30)
                z /= 4;
        }
        MATRIX scalemat;
        const uint32 scaleWords[5] = { (uint32)gscale, 0, (uint32)gscale, 0, 0 };
        memcpy(&scalemat, scaleWords, sizeof(scaleWords));
        MATRIX mtx;
        IdentMatrix(&mtx);
        gte_SetTransVector(&diff);
        RotMatrixZ(angleZ, &mtx);
        gte_SetRotMatrix(&mtx);
        for (sint32 column = 0; column < 3; ++column)
        {
            gte_ldclmv(&scalemat.m[0][column]);
            gte_rtir();
            gte_stclmv(&mtx.m[0][column]);
        }
        gte_SetRotMatrix(&mtx);
    }
    else
    {
        sint32 dify = sp2.vy - sp.vy;
        sint32 difx = sp2.vx - sp.vx;
        sint32 r = fixedatan(dify, difx) >> 4;
        uint32 distance = isqrt(difx * (uint32)difx + dify * (uint32)dify);
        MATRIX scalemat;
        uint32 outer = gscale + (uint32)((sint32)(distance * diff.vz) >> 4);
        const uint32 scaleWords[5] = { outer, 0, (uint32)gscale, 0, 0 };
        memcpy(&scalemat, scaleWords, sizeof(scaleWords));
        MATRIX mtx, mtx2;
        IdentMatrix(&mtx);
        IdentMatrix(&mtx2);
        gte_SetTransVector(&diff);
        RotMatrixZ(angleZ, &mtx);
        RotMatrixZ(r, &mtx2);
        gte_SetRotMatrix(&mtx2);
        for (sint32 column = 0; column < 3; ++column)
        {
            gte_ldclmv(&scalemat.m[0][column]);
            gte_rtir();
            gte_stclmv(&scalemat.m[0][column]);
        }
        gte_SetRotMatrix(&scalemat);
        for (sint32 column = 0; column < 3; ++column)
        {
            gte_ldclmv(&mtx.m[0][column]);
            gte_rtir();
            gte_stclmv(&mtx.m[0][column]);
        }
        gte_SetRotMatrix(&mtx);
    }
    sint32 center;
    memcpy(&center, &sp, 4);
    if (z < 3200)
        OctFlareSpikes(&center, otz);
    else if (z < 7040)
    {
        HexFlare(&center, otz);
        PreCalcHexLightBeam(&center, otz);
    }
    else
        QuadFlare(&center, otz);
    DR_MODE *aprim = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim + otz;
    setaddr(aprim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(aprim + 1);
    setaddr(ot, aprim);
    SetDrawMode(aprim, 0, (flags & 0x40) != 0, 0x120, 0);
}

void Flare::Halo(DRender::tView *Vi, sint32 scale, sint32 type, coorddef *fpt, Draw::FlareCache *sd)
{
    FUNCTION_MARKER(0x800CDE9C, "NFS4.EXE");
    Halo2(Vi, scale, type, fpt, 0, sd);
}

void Flare::_2DSpike(sint32 *center, sint32 *end, sint32 otz)
{
    FUNCTION_MARKER(0x800CDEC4, "NFS4.EXE");
    sint32 pt[2];
    pt[0] = *center;
    pt[1] = *end;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    uint32 *ot = cprim->LastPrim + otz;
    LINE_G2 *prim = reinterpret_cast<LINE_G2 *>(cprim->PrimPtr);
    setaddr(prim, getaddr(ot));
    setaddr(ot, prim);
    uint32 colorWord;
    memcpy(&colorWord, &gfrgb2, 4);
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setlen(prim, 4);
    prim->p[1].rgbc.rgbc = 0;
    memcpy(&prim->p[0].rgbc, &colorWord, 4);
    prim->p[0].rgbc.code = 0x52;
    prim->p[0].xy.xy = pt[0];
    prim->p[1].xy.xy = pt[1];
}

void Flare::_2DHalo(sint32 x, sint32 y, sint32 scalex, sint32 scaley, sint32 type)
{
    FUNCTION_MARKER(0x800CDF70, "NFS4.EXE");
    DVECTOR pt2;
    pt2.vx = (sint16)x;
    pt2.vy = (sint16)y;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    if ((uintptr_t)cprim->PrimPtr >= (uintptr_t)cprim->MPrimPtr - 0x1000U)
        return;
    memcpy(&gfrgb, &gType[(uint8)type].chalo, 4);
    memcpy(&gfrgb2, &gType[(uint8)type].cbeam, 4);
    {
        DR_MODE *aprim = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(aprim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(aprim + 1);
        setaddr(ot, aprim);
        SetDrawMode(aprim, 0, 0, 0x120, 0);
    }
    DVECTOR octring[8];
    for (sint32 i = 0; i < 8; ++i)
    {
        uint32 product = (gOct[i].vx) * (uint32)scalex;
        if ((sint32)product < 0)
            product += 0xFFFFU;
        octring[i].vx = (sint16)(product >> 16);
        product = (gOct[i].vy) * (uint32)scaley;
        if ((sint32)product < 0)
            product += 0xFFFFU;
        octring[i].vy = (sint16)(product >> 16);
    }
    DVECTOR save1;
    save1.vx = (sint16)(((uint16)octring[0].vx) + (uint32)x);
    save1.vy = (sint16)(((uint16)octring[0].vy) + (uint32)y);
    DVECTOR npt[2];
    npt[1].vx = (sint16)(((uint16)octring[1].vx) + (uint32)x);
    npt[1].vy = (sint16)(((uint16)octring[1].vy) + (uint32)y);
    sint32 center, first, packed[2];
    memcpy(&center, &pt2, 4);
    memcpy(&first, &save1, 4);
    memcpy(&packed[1], &npt[1], 4);
    Tri(&center, &first, &packed[1], 0);
    _2DSpike(&center, &packed[1], 0);
    for (sint32 i = 2; i < 8; ++i)
    {
        sint32 slot = i & 1;
        npt[slot].vx = (sint16)(((uint16)octring[i].vx) + (uint32)x);
        npt[slot].vy = (sint16)(((uint16)octring[i].vy) + (uint32)y);
        memcpy(&packed[slot], &npt[slot], 4);
        Tri(&center, &packed[slot ^ 1], &packed[slot], 0);
        _2DSpike(&center, &packed[slot], 0);
    }
    Tri(&center, &packed[1], &first, 0);
    _2DSpike(&center, &first, 0);
    {
        DR_MODE *aprim = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(aprim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(aprim + 1);
        setaddr(ot, aprim);
        SetDrawMode(aprim, 0, 1, 0x120, 0);
    }
}

void Flare::PreCalcHexLightBeam(sint32 *center, sint32 otz)
{
    FUNCTION_MARKER(0x800CE34C, "NFS4.EXE");
    sint32 pt[2];
    pt[0] = *center;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    for (sint32 i = 0; i < 8; ++i)
    {
        gte_ldv0(&gOct[i]);
        LINE_G2 *prim = (LINE_G2 *)cprim->PrimPtr;
        uint32 *ot = cprim->LastPrim + otz;
        setaddr(prim, getaddr(ot));
        cprim->PrimPtr = (char *)(prim + 1);
        setaddr(ot, prim);
        xport_gte_execute(0x180001);
        uint32 color;
        memcpy(&color, &gfrgb2, sizeof(color));
        setlen(prim, 4);
        prim->p[1].rgbc.rgbc = 0;
        prim->p[0].rgbc.rgbc = color;
        setcode(prim, 0x52);
        gte_stsxy(&pt[1]);
        prim->p[0].xy.xy = pt[0];
        prim->p[1].xy.xy = pt[1];
    }
}

void Flare::Quad(sint32 *pt, CVECTOR *color, sint32 otz)
{
    FUNCTION_MARKER(0x800CE420, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    uint32 *ot = cprim->LastPrim + otz;
    POLY_F4 *prim = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    uint32 colorWord;
    memcpy(&colorWord, color, 4);
    setlen(prim, 5);
    memcpy(&prim->rgbc, &colorWord, 4);
    prim->rgbc.code = 0x2A;
    prim->p[0].xy = pt[0];
    prim->p[1].xy = pt[1];
    prim->p[2].xy = pt[2];
    prim->p[3].xy = pt[3];
}

void Flare::QuadNotTransparent(sint32 *pt, CVECTOR *color, sint32 otz)
{
    FUNCTION_MARKER(0x800CE4C4, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    uint32 *ot = cprim->LastPrim + otz;
    POLY_F4 *prim = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    uint32 colorWord;
    memcpy(&colorWord, color, 4);
    setlen(prim, 5);
    memcpy(&prim->rgbc, &colorWord, 4);
    prim->rgbc.code = 0x28;
    prim->p[0].xy = pt[0];
    prim->p[1].xy = pt[1];
    prim->p[2].xy = pt[2];
    prim->p[3].xy = pt[3];
}

void Flare::QuadRing(sint32 *pt, CVECTOR *color, sint32 otz)
{
    FUNCTION_MARKER(0x800CE568, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    uint32 *ot = cprim->LastPrim + otz;
    POLY_G4 *prim = reinterpret_cast<POLY_G4 *>(cprim->PrimPtr);
    setaddr(prim, getaddr(ot));
    setaddr(ot, prim);
    uint32 colorWord;
    memcpy(&colorWord, color, 4);
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    prim->p[1].rgbc.rgbc = 0;
    memcpy(&prim->p[0].rgbc, &colorWord, 4);
    memcpy(&colorWord, color, 4);
    setlen(prim, 8);
    prim->p[3].rgbc.rgbc = 0;
    prim->p[0].rgbc.code = 0x3A;
    memcpy(&prim->p[2].rgbc, &colorWord, 4);
    prim->p[0].xy.xy = pt[0];
    prim->p[1].xy.xy = pt[1];
    prim->p[2].xy.xy = pt[2];
    prim->p[3].xy.xy = pt[3];
}

void Flare::TextureQuad(sint32 *pt, CVECTOR *color, char type, sint32 otz)
{
    FUNCTION_MARKER(0x800CE61C, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    uint32 *ot = cprim->LastPrim + otz;
    POLY_FT4 *prim = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    uint32 colorWord;
    memcpy(&colorWord, color, 4);
    setlen(prim, 9);
    memcpy(&prim->rgbc, &colorWord, 4);
    prim->rgbc.code = 0x2E;
    prim->p[0].xy.xy = pt[0];
    prim->p[1].xy.xy = pt[1];
    prim->p[2].xy.xy = pt[2];
    prim->p[3].xy.xy = pt[3];
    Draw::tPixMap *pixmap = gFlarePixmap[(uint8)type];
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

void Flare::SingleColorTex(DVECTOR *xy, CVECTOR *color, sint32 width, sint32 height, char type, sint32 otz)
{
    FUNCTION_MARKER(0x800CE6F8, "NFS4.EXE");
    DVECTOR pt[4];
    pt[0].vx = (sint16)(((uint16)xy->vx) - (uint32)width);
    pt[0].vy = (sint16)(((uint16)xy->vy) + (uint32)height);
    pt[1].vx = (sint16)(((uint16)xy->vx) + (uint32)width);
    pt[1].vy = (sint16)(((uint16)xy->vy) + (uint32)height);
    pt[2].vx = (sint16)(((uint16)xy->vx) - (uint32)width);
    pt[2].vy = (sint16)(((uint16)xy->vy) - (uint32)height);
    pt[3].vx = (sint16)(((uint16)xy->vx) + (uint32)width);
    pt[3].vy = (sint16)(((uint16)xy->vy) - (uint32)height);
    sint32 packed[4];
    memcpy(packed, pt, sizeof(pt));
    TextureQuad(packed, color, type, otz);
}

void Flare::SingleColorHex(DVECTOR *xy, CVECTOR *color, sint32 width, sint32 height, sint32 otz)
{
    FUNCTION_MARKER(0x800CE79C, "NFS4.EXE");
    DVECTOR pt[6];
    sint32 quarterWidth = width / 4;
    sint32 halfHeight = height / 2;
    sint32 halfWidth = width / 2;
    pt[0].vx = (sint16)((uint16)xy->vx - quarterWidth);
    pt[0].vy = (sint16)((uint16)xy->vy + halfHeight);
    pt[1].vx = (sint16)((uint16)xy->vx + quarterWidth);
    pt[1].vy = (sint16)((uint16)xy->vy + halfHeight);
    pt[2].vx = (sint16)((uint16)xy->vx - halfWidth);
    pt[2].vy = xy->vy;
    pt[3].vx = (sint16)((uint16)xy->vx + halfWidth);
    pt[3].vy = xy->vy;
    pt[4].vx = (sint16)((uint16)xy->vx - quarterWidth);
    pt[4].vy = (sint16)((uint16)xy->vy - halfHeight);
    pt[5].vx = (sint16)((uint16)xy->vx + quarterWidth);
    pt[5].vy = (sint16)((uint16)xy->vy - halfHeight);
    sint32 packed[6];
    memcpy(packed, pt, sizeof(pt));
    Quad(packed, color, otz);
    Quad(packed + 2, color, otz);
}

void Flare::SingleColorOct(DVECTOR *xy, CVECTOR *color, sint32 width, sint32 height, sint32 otz)
{
    FUNCTION_MARKER(0x800CE8B0, "NFS4.EXE");
    DVECTOR pt[8];
    for (sint32 i = 0; i < 8; ++i)
    {
        uint32 product = width * (uint32)(sint32)gLensOct[i].vx;
        if ((sint32)product < 0)
            product += 0xFFU;
        pt[i].vx = (sint16)((uint16)xy->vx + (product >> 8));
        product = height * (uint32)(sint32)gLensOct[i].vy;
        if ((sint32)product < 0)
            product += 0xFFU;
        pt[i].vy = (sint16)((uint16)xy->vy + (product >> 8));
    }
    sint32 packed[8];
    memcpy(packed, pt, sizeof(pt));
    Quad(packed, color, otz);
    Quad(packed + 2, color, otz);
    Quad(packed + 4, color, otz);
}

void Flare::SingleColorOctRing(DVECTOR *xy, CVECTOR *color, sint32 width, sint32 height, sint32 otz)
{
    FUNCTION_MARKER(0x800CE988, "NFS4.EXE");
    DVECTOR pt[18];
    uint32 width2 = width - 5U;
    uint32 height2 = height - 5U;
    for (sint32 i = 0; i < 9; ++i)
    {
        sint32 index = i & 7;
        sint32 i2 = i * 2;
        uint32 product = width * (uint32)(sint32)gOct[index].vx;
        if ((sint32)product < 0)
            product += 0xFFU;
        pt[i2].vx = (sint16)((uint16)xy->vx + (product >> 8));
        product = height * (uint32)(sint32)gOct[index].vy;
        if ((sint32)product < 0)
            product += 0xFFU;
        pt[i2].vy = (sint16)((uint16)xy->vy + (product >> 8));
        product = width2 * (gOct[index].vx);
        if ((sint32)product < 0)
            product += 0xFFU;
        pt[i2 + 1].vx = (sint16)((uint16)xy->vx + (product >> 8));
        product = height2 * (gOct[index].vy);
        if ((sint32)product < 0)
            product += 0xFFU;
        pt[i2 + 1].vy = (sint16)((uint16)xy->vy + (product >> 8));
    }
    sint32 packed[18];
    memcpy(packed, pt, sizeof(pt));
    for (sint32 i = 0; i < 8; ++i)
        QuadRing(packed + i * 2, color, otz);
}

void Flare::InitLensFlare(void)
{
    FUNCTION_MARKER(0x800CEB44, "NFS4.EXE");
    gFlare_LensFlare.size = 0x50;
    gFlare_LensFlare.piece = SunFlarePieces;
    gFlare_LensFlare.isDrawn[1] = 0;
    gFlare_LensFlare.isDrawn[0] = 0;
}

void Flare::LensFlare(DVECTOR *screenPos, Draw::FlareCache *sd)
{
    FUNCTION_MARKER(0x800CEB6C, "NFS4.EXE");
    if ((uintptr_t)sd->head.cprim.PrimPtr >= (uintptr_t)sd->head.cprim.MPrimPtr - 0x400U)
        return;
    sint32 sx = screenPos->vx;
    sint32 sy = screenPos->vy;
    DVECTOR pt[4];
    pt[0].vx = (sint16)(sx - 2);
    pt[0].vy = (sint16)(sy - 2);
    pt[1].vx = (sint16)(sx + 3);
    pt[1].vy = (sint16)(sy - 2);
    pt[2].vx = (sint16)(sx - 2);
    pt[2].vy = (sint16)(sy + 3);
    pt[3].vx = (sint16)(sx + 3);
    pt[3].vy = (sint16)(sy + 3);
    sint32 packed[4];
    memcpy(packed, pt, sizeof(pt));
    CVECTOR white = { 255, 255, 255, 0 };
    sint32 angleZ = (sx + sy) * 8;
    sint32 angleZ2 = (sx + sy) * 6;
    QuadNotTransparent(packed, &white, (Draw::gViewOtSize - 2));
    sint32 dx = 320 - sx;
    sint32 dy = 240 - sy;
    sint16 oldX = gFlare_LensFlare.pos[0].vx;
    sint16 oldY = gFlare_LensFlare.pos[0].vy;
    gFlare_LensFlare.pos[0].vx = (sint16)sx;
    gFlare_LensFlare.pos[0].vy = (sint16)sy;
    gFlare_LensFlare.isDrawn[0] = 1;
    gFlare_LensFlare.oldpos[0].vx = oldX;
    gFlare_LensFlare.oldpos[0].vy = oldY;
    uint8 flareVis = 0;
    for (sint32 i = 0; i < 25; ++i)
        if ((gFlare_LensFlare.screenData[0][i / 5][i % 5] & 0x7FFF) == 0x7FFF)
            ++flareVis;
    if (flareVis == 0)
        return;
    MATRIX scalemat;
    uint32 scaleWords[5] = { (uint32)flareVis << 7, 0, (uint32)flareVis << 6, 0, 0 };
    memcpy(&scalemat, scaleWords, sizeof(scaleWords));
    memcpy(&gfrgb2, &TrackSpec::gSpec.skyspec.sunBeamColor, 4);
    MATRIX mtx;
    IdentMatrix(&mtx);
    RotMatrixZ(angleZ, &mtx);
    gte_SetRotMatrix(&scalemat);
    for (sint32 column = 0; column < 3; ++column)
    {
        gte_ldclmv(&mtx.m[0][column]);
        gte_rtir();
        gte_stclmv(&mtx.m[0][column]);
    }
    gte_SetRotMatrix(&mtx);
    sint32 center;
    memcpy(&center, screenPos, 4);
    Spikes(&center, 0);
    scaleWords[0] = flareVis << 6;
    memcpy(&scalemat, scaleWords, 4);
    IdentMatrix(&mtx);
    RotMatrixZ(angleZ2, &mtx);
    gte_SetRotMatrix(&scalemat);
    for (sint32 column = 0; column < 3; ++column)
    {
        gte_ldclmv(&mtx.m[0][column]);
        gte_rtir();
        gte_stclmv(&mtx.m[0][column]);
    }
    gte_SetRotMatrix(&mtx);
    memcpy(&center, screenPos, 4);
    Spikes(&center, 0);
    memcpy(&gFlare_LensFlare.piece[0].color, &TrackSpec::gSpec.skyspec.sunHaloColor, 4);
    CVECTOR col;

    col.cd = (uint8)((uint16)pt[0].vy >> 8);
    for (sint32 i = 0; i < 9; ++i)
    {
        FLARE_PIECE_DEF *piece = gFlare_LensFlare.piece + i;
        DVECTOR pxy;
        uint32 distance = piece->distance;
        uint32 product = (0x10000U - distance) * sx + distance * dx;
        if ((sint32)product < 0)
            product += 0xFFFFU;
        pxy.vx = (sint16)(product >> 16);
        distance = piece->distance;
        product = (0x10000U - distance) * sy + distance * dy;
        if ((sint32)product < 0)
            product += 0xFFFFU;
        pxy.vy = (sint16)(product >> 16);
        product = gFlare_LensFlare.size * (uint32)piece->size;
        if ((sint32)product < 0)
            product += 0xFFFFU;
        sint32 width = (sint32)product >> 16;
        if (width < 4)
            continue;
        col.r = (uint8)((uint32)piece->color.r * flareVis / 25U);
        col.g = (uint8)((uint32)piece->color.g * flareVis / 25U);
        col.b = (uint8)((uint32)piece->color.b * flareVis / 25U);
        switch ((uint8)piece->type)
        {
        case 0:
        case 1:
            SingleColorTex(&pxy, &col, width, width, piece->type, 0);
            break;
        case 2:
            SingleColorHex(&pxy, &col, width, width, 0);
            break;
        case 3:
            SingleColorOct(&pxy, &col, width, width, 0);
            break;
        case 4:
            SingleColorOctRing(&pxy, &col, width, width, 0);
            break;
        }
    }
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    DR_MODE *aprim = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(aprim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(aprim + 1);
    setaddr(ot, aprim);
    SetDrawMode(aprim, 0, 0, 0x120, 0);
}

void Flare::Sun(SVECTOR *worldPos, Draw::FlareCache *sd)
{
    FUNCTION_MARKER(0x800CF1D0, "NFS4.EXE");
    sint32 pshift = GameSetup::gData.commMode == 1 ? 60 : 120;
    if ((uintptr_t)sd->head.cprim.PrimPtr >= (uintptr_t)sd->head.cprim.MPrimPtr - 0x400U)
        return;
    VECTOR diff;
    gte_ldv0(worldPos);
    gte_rtps();
    gte_stlvnl(&diff);
    if (diff.vx > diff.vz || -diff.vx > diff.vz)
        return;
    CVECTOR color;
    uint32 colorWord = 0x00808080;
    memcpy(&color, &colorWord, 4);
    DVECTOR posOnScreen;
    gte_stsxy(&posOnScreen);
    sint32 commMode = GameSetup::gData.commMode;
    posOnScreen.vy = (sint16)((diff.vy >> 2) + pshift);
    sint32 vertRezBy2 = commMode == 1 ? 60 : 120;
    diff.vy = (posOnScreen.vy - vertRezBy2) * 4;
    sint32 dvz = ((uint32)Camera::gGeomScreen << 2);
    sint32 sx = posOnScreen.vx;
    diff.vx = (sx - 160) * 4;
    if (dvz > 0xB50)
        dvz = 0xB50;
    diff.vz = dvz;
    gte_SetTransVector(&diff);
    if (sx < 317 && commMode != 1 && (TrackSpec::gSpec.skyspec.flags & 0x100) == 0)
        LensFlare(&posOnScreen, sd);
    sint32 otz = (Draw::gViewOtSize - 2);
    memcpy(&gfrgb, &color, 4);
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    {
        DR_MODE *aprim = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim + otz;
        setaddr(aprim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(aprim + 1);
        setaddr(ot, aprim);
        SetDrawMode(aprim, 0, 0, 0x120, 0);
    }
    MATRIX scalemat;
    const uint32 rotationWords[5] = { 0x400, 0, 0x400, 0, 0 };
    memcpy(&scalemat, rotationWords, sizeof(rotationWords));
    gte_SetRotMatrix(&scalemat);
    if ((TrackSpec::gSpec.skyspec.flags & 0x100) == 0)
    {
        sint32 center;
        memcpy(&center, &posOnScreen, 4);
        OctFlare(&center, otz);
        memcpy(&posOnScreen, &center, 4);
    }
    else
        SingleColorTex(&posOnScreen, &TrackSpec::gSpec.skyspec.sunHaloColor, 16, 16, 0, otz);
    {
        DR_MODE *aprim = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim + otz;
        setaddr(aprim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(aprim + 1);
        setaddr(ot, aprim);
        SetDrawMode(aprim, 0, 0, 0x120, 0);
    }
}

void Flare::Moon(SVECTOR *worldPos, Draw::FlareCache *sd)
{
    FUNCTION_MARKER(0x800CF4BC, "NFS4.EXE");
    sint32 pshift = GameSetup::gData.commMode == 1 ? 60 : 120;
    VECTOR diff;
    gte_ldv0(worldPos);
    gte_rtps();
    CVECTOR color;
    uint32 colorWord = 0x00808080;
    memcpy(&color, &colorWord, 4);
    gte_stlvnl(&diff);
    DVECTOR posOnScreen;
    gte_stsxy(&posOnScreen);
    posOnScreen.vy = (sint16)((diff.vy >> 2) + pshift);
    if (diff.vx <= diff.vz && -diff.vx <= diff.vz)
        SingleColorTex(&posOnScreen, &color, 16, 16, 0, (Draw::gViewOtSize - 2));
}
