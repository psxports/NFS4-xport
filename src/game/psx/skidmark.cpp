#include <stddef.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/gmesetup.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"
#include "game/psx/skidmark.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/sinfunc.h"

struct tSkid
{
    coorddef pt;
    CVECTOR clr;
    sint32 type;
    Skidmark::Segment *nseg;
    sint32 chunk;
};

sint32 gCountSm;
sint32 gUseSm;
sint32 gMaxSChunk;
Skidmark::Chunk *gSm;

void CalcStartSegment(Skidmark::Segment *r1, Skidmark::Segment *r2, coorddef *cp, coorddef *pt1, coorddef *pt2, sint32 tireWidth)
{
    FUNCTION_MARKER(0x800DE6F4, "NFS4.EXE");
    sint32 pxp;
    sint32 pzp;
    {
        sint32 angle = intatan((sint32)((uint32)pt2->x - (uint32)pt1->x),
            (sint32)((uint32)pt2->z - (uint32)pt1->z));
        sint32 cosine = fastintcos(angle);
        pxp = fixedmult(tireWidth, (sint32)(0u - (uint32)cosine)) >> 1;
        pzp = fixedmult(tireWidth, fastintsin(angle)) >> 1;
    }
    r1->svx[0].vx = (sint16)((sint32)((uint32)pt1->x + (uint32)pxp - (uint32)cp->x) >> 6);
    r1->svx[0].vy = (sint16)((sint32)((uint32)pt1->y - (uint32)cp->y) >> 6);
    r1->svx[0].vz = (sint16)((sint32)((uint32)pt1->z + (uint32)pzp - (uint32)cp->z) >> 6);
    r1->svx[1].vx = (sint16)((sint32)((uint32)pt1->x - (uint32)pxp - (uint32)cp->x) >> 6);
    r1->svx[1].vy = (sint16)((sint32)((uint32)pt1->y - (uint32)cp->y) >> 6);
    r1->svx[1].vz = (sint16)((sint32)((uint32)pt1->z - (uint32)pzp - (uint32)cp->z) >> 6);
    r2->svx[1].vx = (sint16)((sint32)((uint32)pt2->x - (uint32)pxp - (uint32)cp->x) >> 6);
    r2->svx[1].vy = (sint16)((sint32)((uint32)pt2->y - (uint32)cp->y) >> 6);
    r2->svx[1].vz = (sint16)((sint32)((uint32)pt2->z - (uint32)pzp - (uint32)cp->z) >> 6);
    r2->svx[0].vx = (sint16)((sint32)((uint32)pt2->x + (uint32)pxp - (uint32)cp->x) >> 6);
    r2->svx[0].vy = (sint16)((sint32)((uint32)pt2->y - (uint32)cp->y) >> 6);
    r2->svx[0].vz = (sint16)((sint32)((uint32)pt2->z + (uint32)pzp - (uint32)cp->z) >> 6);
}

void CalcOneSegment(Skidmark::Segment *r1, coorddef *cp, coorddef *pt1, coorddef *pt2, sint32 tireWidth)
{
    FUNCTION_MARKER(0x800DE8C4, "NFS4.EXE");
    sint32 pxp;
    sint32 pzp;
    {
        sint32 angle = intatan((sint32)((uint32)pt2->x - (uint32)pt1->x),
            (sint32)((uint32)pt2->z - (uint32)pt1->z));
        sint32 cosine = fastintcos(angle);
        pxp = fixedmult(tireWidth, (sint32)(0u - (uint32)cosine)) >> 1;
        pzp = fixedmult(tireWidth, fastintsin(angle)) >> 1;
    }
    r1->svx[0].vx = (sint16)((sint32)((uint32)pt2->x + (uint32)pxp - (uint32)cp->x) >> 6);
    r1->svx[0].vy = (sint16)((sint32)((uint32)pt2->y - (uint32)cp->y) >> 6);
    r1->svx[0].vz = (sint16)((sint32)((uint32)pt2->z + (uint32)pzp - (uint32)cp->z) >> 6);
    r1->svx[1].vx = (sint16)((sint32)((uint32)pt2->x - (uint32)pxp - (uint32)cp->x) >> 6);
    r1->svx[1].vy = (sint16)((sint32)((uint32)pt2->y - (uint32)cp->y) >> 6);
    r1->svx[1].vz = (sint16)((sint32)((uint32)pt2->z - (uint32)pzp - (uint32)cp->z) >> 6);
}

Skidmark::Chunk * Skidmark::CheckChunk(coorddef *skidpt, sint32 newsegs, sint32 slice)
{
    FUNCTION_MARKER(0x800DE9EC, "NFS4.EXE");
    Skidmark::Chunk *returnsm = gSm + gUseSm;
    sint32 NewChunk = (sint32)((uint32)returnsm->n + (uint32)newsegs) > 24;
    for (sint32 axis = 0; axis < 3; ++axis) {
        sint32 center;
        sint32 point;
        if (axis == 0) {
            center = returnsm->cp.x;
            point = skidpt->x;
        } else if (axis == 1) {
            center = returnsm->cp.y;
            point = skidpt->y;
        } else {
            center = returnsm->cp.z;
            point = skidpt->z;
        }
        sint32 delta = (sint32)((uint32)center - (uint32)point);
        if (delta <= 0)
            delta = (sint32)((uint32)point - (uint32)center);
        if (delta > 0xFFFFF) {
            NewChunk = 1;
            break;
        }
    }
    if (NewChunk) {
        sint32 count = gCountSm;
        sint32 maximum = gMaxSChunk;
        if (count < maximum) {
            gUseSm = count;
            gCountSm = (sint32)((uint32)count + 1);
        } else {
            sint32 use = gUseSm;
            if (use < (sint32)((uint32)maximum - 1))
                gUseSm = (sint32)((uint32)use + 1);
            else
                gUseSm = 0;
        }
        returnsm = gSm + gUseSm;
        returnsm->n = 0;
    }
    if (returnsm->n == 0) {
        coorddef point = *skidpt;
        returnsm->cp = point;
        returnsm->slice = (sint16)slice;
    }
    return returnsm;
}

void Skidmark::Add(tSkid *prevskid, coorddef *skidpt, CVECTOR *color, sint32 tireWidth, sint32 type, sint32 slice)
{
    FUNCTION_MARKER(0x800DEB98, "NFS4.EXE");
    Skidmark::Chunk *sm;
    if (prevskid->nseg == 0) {
        sm = CheckChunk(skidpt, 2, slice);
        sint32 n = sm->n;
        CalcStartSegment(&sm->seg[n], &sm->seg[n + 1], &sm->cp, &prevskid->pt, skidpt, tireWidth);
        n = sm->n;
        CVECTOR currentColor = *color;
        sm->seg[n + 1].rgb = currentColor;
        sm->seg[n].rgb = sm->seg[n + 1].rgb;
        n = sm->n;
        sm->seg[n + 1].type = type;
        sm->seg[n].type = type;
    } else {
        sm = CheckChunk(skidpt, 2, slice);
        if (prevskid->chunk == gUseSm) {
            CalcOneSegment(&sm->seg[sm->n], &sm->cp, &prevskid->pt, skidpt, tireWidth);
            CVECTOR currentColor = *color;
            sm->seg[sm->n].rgb = currentColor;
            sm->seg[sm->n].type = type;
            prevskid->nseg->next = &sm->seg[sm->n];
            sm->seg[sm->n].next = 0;
            sint32 use = gUseSm;
            currentColor = *color;
            prevskid->clr = currentColor;
            prevskid->type = type;
            coorddef point = *skidpt;
            prevskid->pt = point;
            sint32 n = sm->n;
            prevskid->chunk = use;
            prevskid->nseg = &sm->seg[n];
            sm->n = (sint16)((uint16)sm->n + 1);
            return;
        }
        sint32 n = sm->n;
        CalcStartSegment(&sm->seg[n], &sm->seg[n + 1], &sm->cp, &prevskid->pt, skidpt, tireWidth);
        CVECTOR previousColor = prevskid->clr;
        sm->seg[sm->n].rgb = previousColor;
        sm->seg[sm->n].type = prevskid->type;
        CVECTOR currentColor = *color;
        sm->seg[sm->n + 1].rgb = currentColor;
        sm->seg[sm->n + 1].type = type;
    }
    sint32 n = sm->n;
    sm->seg[n].next = &sm->seg[n + 1];
    sm->seg[sm->n + 1].next = 0;
    sint32 use = gUseSm;
    CVECTOR currentColor = *color;
    prevskid->clr = currentColor;
    prevskid->type = type;
    coorddef point = *skidpt;
    prevskid->pt = point;
    n = sm->n;
    prevskid->chunk = use;
    prevskid->nseg = &sm->seg[n + 1];
    sm->n = (sint16)((uint16)sm->n + 2);
}

void Skidmark::AddStretch(Skidmark::Segment **save, sint32 *savechunk, tSkid *prevskid, coorddef *skidpt, CVECTOR *color, sint32 tireWidth, sint32 type, sint32 slice)
{
    FUNCTION_MARKER(0x800DEF6C, "NFS4.EXE");
    Skidmark::Chunk *sm;
    if (prevskid->nseg == 0) {
        sm = CheckChunk(skidpt, 2, slice);
        sint32 n = sm->n;
        CalcStartSegment(&sm->seg[n], &sm->seg[n + 1], &sm->cp, &prevskid->pt, skidpt, tireWidth);
        n = sm->n;
        CVECTOR currentColor = *color;
        sm->seg[n + 1].rgb = currentColor;
        sm->seg[n].rgb = sm->seg[n + 1].rgb;
        n = sm->n;
        sm->seg[n + 1].type = type;
        sm->seg[n].type = type;
    } else {
        sm = CheckChunk(skidpt, 2, slice);
        if (prevskid->chunk == gUseSm) {
            CalcOneSegment(&sm->seg[sm->n], &sm->cp, &prevskid->pt, skidpt, tireWidth);
            CVECTOR currentColor = *color;
            sm->seg[sm->n].rgb = currentColor;
            sm->seg[sm->n].type = type;
            prevskid->nseg->next = &sm->seg[sm->n];
            sm->seg[sm->n].next = 0;
            *save = &sm->seg[sm->n];
            *savechunk = gUseSm;
            sm->n = (sint16)((uint16)sm->n + 1);
            return;
        }
        sint32 n = sm->n;
        CalcStartSegment(&sm->seg[n], &sm->seg[n + 1], &sm->cp, &prevskid->pt, skidpt, tireWidth);
        CVECTOR previousColor = prevskid->clr;
        sm->seg[sm->n].rgb = previousColor;
        sm->seg[sm->n].type = prevskid->type;
        CVECTOR currentColor = *color;
        sm->seg[sm->n + 1].rgb = currentColor;
        sm->seg[sm->n + 1].type = type;
    }
    sint32 n = sm->n;
    sm->seg[n].next = &sm->seg[n + 1];
    sm->seg[sm->n + 1].next = 0;
    *save = &sm->seg[sm->n + 1];
    *savechunk = gUseSm;
    sm->n = (sint16)((uint16)sm->n + 2);
}

void Skidmark::Stretch(Skidmark::Segment *save, sint32 savechunk, tSkid *prevskid, coorddef *skidpt, CVECTOR *color, sint32 tireWidth, sint32 type)
{
    FUNCTION_MARKER(0x800DF308, "NFS4.EXE");
    CalcOneSegment(save, &gSm[savechunk].cp, &prevskid->pt, skidpt, tireWidth);
    CVECTOR currentColor = *color;
    save->rgb = currentColor;
    save->type = type;
}

void Skidmark::EndStretch(Skidmark::Segment *save, sint32 savechunk, tSkid *prevskid, coorddef *skidpt, CVECTOR *color, sint32 tireWidth, sint32 type)
{
    FUNCTION_MARKER(0x800DF388, "NFS4.EXE");
    CalcOneSegment(save, &gSm[savechunk].cp, &prevskid->pt, skidpt, tireWidth);
    CVECTOR currentColor = *color;
    save->rgb = currentColor;
    save->type = type;
    currentColor = *color;
    prevskid->clr = currentColor;
    prevskid->type = type;
    coorddef point = *skidpt;
    prevskid->pt = point;
    prevskid->nseg = save;
    prevskid->chunk = savechunk;
}

void Skidmark::OnyxBuildFacets(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800DF464, "NFS4.EXE");
    sint32 count = gCountSm;
    Skidmark::Chunk *chunks = gSm;
    Draw::tCtrlSkidmark fskidspace;
    Draw::tCtrlSkidmark *fskid = &fskidspace;
    coorddef translation = Vi->cview.translation;
    fskid->t = translation;
    fskid->count = count;
    fskid->smp = chunks;
    for (sint32 row = 0; row < 3; ++row) {
        sint32 t1 = Vi->cview.mrotationInv.m[row * 3];
        sint32 t2 = Vi->cview.mrotationInv.m[row * 3 + 1];
        sint32 t3 = Vi->cview.mrotationInv.m[row * 3 + 2];
        fskid->m.m[row * 3] = t1;
        fskid->m.m[row * 3 + 1] = (sint32)(0u - (uint32)t2);
        fskid->m.m[row * 3 + 2] = t3;
    }
    Draw::kCtrlSkidmark(fskid);
}

void Skidmark::InitSkidmark(void)
{
    FUNCTION_MARKER(0x800DF504, "NFS4.EXE");
    gMaxSChunk = 16;
    if (GameSetup::gData.commMode == 1)
        gMaxSChunk = 8;
    Skidmark::Chunk *chunks = (Skidmark::Chunk *)reservememadr(
        "SkidMark", (sint32)((uint32)gMaxSChunk * 688u), 0);
    chunks->n = 0;
    gSm = chunks;
    gCountSm = 0;
    gUseSm = 0;
}

void Skidmark::RestartSkidmark(void)
{
    FUNCTION_MARKER(0x800DF578, "NFS4.EXE");
    Skidmark::Chunk *chunks = gSm;
    gCountSm = 0;
    gUseSm = 0;
    chunks->n = 0;
}

void Skidmark::KillSkidmark(void)
{
    FUNCTION_MARKER(0x800DF58C, "NFS4.EXE");
    Skidmark::Chunk *chunks = gSm;
    if (chunks)
        purgememadr(chunks);
    gSm = 0;
}
