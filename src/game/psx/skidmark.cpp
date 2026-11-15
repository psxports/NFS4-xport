#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "hd.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/bworld.h"
#include "game/common/genericpmx.h"
#include "game/common/gmesetup.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"
#include "game/psx/skidmark.h"
#include "game/psx/textureprocess.h"
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
        sint32 angle = intatan(((pt2->x - pt1->x)), (pt2->z - pt1->z));
        sint32 cosine = fastintcos(angle);
        pxp = fixedmult(tireWidth, (-cosine)) >> 1;
        pzp = fixedmult(tireWidth, fastintsin(angle)) >> 1;
    }
    r1->svx[0].vx = (sint16)((pt1->x + (uint32)pxp - (uint32)cp->x) >> 6);
    r1->svx[0].vy = (sint16)((pt1->y - (uint32)cp->y) >> 6);
    r1->svx[0].vz = (sint16)((pt1->z + (uint32)pzp - (uint32)cp->z) >> 6);
    r1->svx[1].vx = (sint16)((pt1->x - (uint32)pxp - (uint32)cp->x) >> 6);
    r1->svx[1].vy = (sint16)((pt1->y - (uint32)cp->y) >> 6);
    r1->svx[1].vz = (sint16)((pt1->z - (uint32)pzp - (uint32)cp->z) >> 6);
    r2->svx[1].vx = (sint16)((pt2->x - (uint32)pxp - (uint32)cp->x) >> 6);
    r2->svx[1].vy = (sint16)((pt2->y - (uint32)cp->y) >> 6);
    r2->svx[1].vz = (sint16)((pt2->z - (uint32)pzp - (uint32)cp->z) >> 6);
    r2->svx[0].vx = (sint16)((pt2->x + (uint32)pxp - (uint32)cp->x) >> 6);
    r2->svx[0].vy = (sint16)((pt2->y - (uint32)cp->y) >> 6);
    r2->svx[0].vz = (sint16)((pt2->z + (uint32)pzp - (uint32)cp->z) >> 6);
}

void CalcOneSegment(Skidmark::Segment *r1, coorddef *cp, coorddef *pt1, coorddef *pt2, sint32 tireWidth)
{
    FUNCTION_MARKER(0x800DE8C4, "NFS4.EXE");
    sint32 pxp;
    sint32 pzp;
    {
        sint32 angle = intatan(((pt2->x - pt1->x)), (pt2->z - pt1->z));
        sint32 cosine = fastintcos(angle);
        pxp = fixedmult(tireWidth, (-cosine)) >> 1;
        pzp = fixedmult(tireWidth, fastintsin(angle)) >> 1;
    }
    r1->svx[0].vx = (sint16)((pt2->x + (uint32)pxp - (uint32)cp->x) >> 6);
    r1->svx[0].vy = (sint16)((pt2->y - (uint32)cp->y) >> 6);
    r1->svx[0].vz = (sint16)((pt2->z + (uint32)pzp - (uint32)cp->z) >> 6);
    r1->svx[1].vx = (sint16)((pt2->x - (uint32)pxp - (uint32)cp->x) >> 6);
    r1->svx[1].vy = (sint16)((pt2->y - (uint32)cp->y) >> 6);
    r1->svx[1].vz = (sint16)((pt2->z - (uint32)pzp - (uint32)cp->z) >> 6);
}

Skidmark::Chunk *Skidmark::CheckChunk(coorddef *skidpt, sint32 newsegs, sint32 slice)
{
    FUNCTION_MARKER(0x800DE9EC, "NFS4.EXE");
    Skidmark::Chunk *returnsm = gSm + gUseSm;
    sint32 NewChunk = (returnsm->n + newsegs) > 24;
    for (sint32 axis = 0; axis < 3; ++axis)
    {
        sint32 center;
        sint32 point;
        if (axis == 0)
        {
            center = returnsm->cp.x;
            point = skidpt->x;
        }
        else if (axis == 1)
        {
            center = returnsm->cp.y;
            point = skidpt->y;
        }
        else
        {
            center = returnsm->cp.z;
            point = skidpt->z;
        }
        sint32 delta = (center - point);
        if (delta <= 0)
            delta = (point - center);
        if (delta > 0xFFFFF)
        {
            NewChunk = 1;
            break;
        }
    }
    if (NewChunk)
    {
        sint32 count = gCountSm;
        sint32 maximum = gMaxSChunk;
        if (count < maximum)
        {
            gUseSm = count;
            gCountSm = (count + 1);
        }
        else
        {
            sint32 use = gUseSm;
            if (use < (maximum - 1))
                gUseSm = (use + 1);
            else
                gUseSm = 0;
        }
        returnsm = gSm + gUseSm;
        hd_reset_skids();
        returnsm->n = 0;
    }
    if (returnsm->n == 0)
    {
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
    if (prevskid->nseg == 0)
    {
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
    }
    else
    {
        sm = CheckChunk(skidpt, 2, slice);
        if (prevskid->chunk == gUseSm)
        {
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
    if (prevskid->nseg == 0)
    {
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
    }
    else
    {
        sm = CheckChunk(skidpt, 2, slice);
        if (prevskid->chunk == gUseSm)
        {
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

void Skidmark::CaptureRenderSource(HdSkidSource *source, const DRender::tView *view)
{
    memset(source, 0, sizeof(*source));
    for (sint32 type = 0; type < 2; ++type)
        if (gSkidMarkPixmap[type] != 0)
        {
            source->pixmaps[type] = *gSkidMarkPixmap[type];
            source->pixmapValid[type] = true;
            if (source->pixmaps[type].clut == 65535)
                memcpy(source->palettes[type], gClutDepth[source->pixmaps[type].pad2], sizeof(source->palettes[type]));
        }
    if (gSm == 0 || gCountSm < 0 || gCountSm > 16)
        return;
    coorddef camera = view->cview.translation;
    for (sint32 chunk = gCountSm - 1; chunk >= 0; --chunk)
    {
        const Chunk *sm = gSm + chunk;
        coorddef center = sm->cp;
        if (sm->n < 0 || sm->n > 24 || !BWorld::IsSliceInBuildList(sm->slice) || !Draw::CircleClip(&center, &camera, 0x320000))
            continue;
        for (sint32 i = 0; i < sm->n; ++i)
        {
            const Segment *segment = sm->seg + i;
            uintptr_t next = (uintptr_t)segment->next, first = (uintptr_t)sm->seg;
            if (next < first || next >= first + sizeof(sm->seg) || (next - first) % sizeof(Segment) != 0 || !source->pixmapValid[segment->type & 1])
                continue;
            HdSkidQuad *quad = source->quads + source->count++;
            quad->owner = segment;
            quad->type = segment->type;
            quad->center.x = center.x / 1024.0f;
            quad->center.y = center.y / 1024.0f;
            quad->center.z = center.z / 1024.0f;
            const SVECTOR *points[4] = {segment->svx, segment->svx + 1, segment->next->svx, segment->next->svx + 1};
            CVECTOR solid = {64, 64, 64, 0};
            for (sint32 point = 0; point < 4; ++point)
            {
                quad->points[point].x = quad->center.x + points[point]->vx / 16.0f;
                quad->points[point].y = quad->center.y + points[point]->vy / 16.0f;
                quad->points[point].z = quad->center.z + points[point]->vz / 16.0f;
                quad->colors[point] = segment->type != 0 ? solid : point < 2 ? segment->rgb : segment->next->rgb;
            }
        }
    }
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
    for (sint32 row = 0; row < 3; ++row)
    {
        sint32 t1 = Vi->cview.mrotationInv.m[row * 3];
        sint32 t2 = Vi->cview.mrotationInv.m[row * 3 + 1];
        sint32 t3 = Vi->cview.mrotationInv.m[row * 3 + 2];
        fskid->m.m[row * 3] = t1;
        fskid->m.m[row * 3 + 1] = (-t2);
        fskid->m.m[row * 3 + 2] = t3;
    }
    Draw::kCtrlSkidmark(fskid);
}

void Skidmark::InitSkidmark(void)
{
    FUNCTION_MARKER(0x800DF504, "NFS4.EXE");
    hd_reset_skids();
    gMaxSChunk = 16;
    if (GameSetup::gData.commMode == 1)
        gMaxSChunk = 8;
    Skidmark::Chunk *chunks = (Skidmark::Chunk *)reservememadr("SkidMark", gMaxSChunk * sizeof(Skidmark::Chunk), 0);
    chunks->n = 0;
    gSm = chunks;
    gCountSm = 0;
    gUseSm = 0;
}

void Skidmark::RestartSkidmark(void)
{
    FUNCTION_MARKER(0x800DF578, "NFS4.EXE");
    hd_reset_skids();
    Skidmark::Chunk *chunks = gSm;
    gCountSm = 0;
    gUseSm = 0;
    chunks->n = 0;
}

void Skidmark::KillSkidmark(void)
{
    FUNCTION_MARKER(0x800DF58C, "NFS4.EXE");
    hd_reset_skids();
    Skidmark::Chunk *chunks = gSm;
    if (chunks)
        purgememadr(chunks);
    gSm = 0;
}
