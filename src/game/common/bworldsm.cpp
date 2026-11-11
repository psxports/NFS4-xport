#include <stddef.h>
#include <stdlib.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/bworld.h"
#include "game/common/bworldsm.h"
#include "game/common/carobj.h"
#include "game/common/chunk.h"
#include "game/common/group.h"
#include "game/common/track.h"
#include "game/common/trk.h"
#include "lib/psx/fixdmult.h"

struct tNormalCacheEntry;

struct tNormalCacheEntry;

class sim
{
public:
    static sint32 Handle;
    static Group *ItemPtr;
    static void *File;
};

class BWSM
{
public:
    static uint32 NormalCacheSysTime;
    static tNormalCacheEntry NormalCache[16];
};

struct tNormalCacheEntry
{
    sint16 sliceInd;
    uint8 triangleFlag;
    char quadInd;
    coorddef normal;
    coorddef forward;
    uint32 accessTime;
};

sint32 gNumSlices;
sint32 gMaxFindQuadSliceIterations;
Trk::NewSimQuad GlobalSimQuad = { 14 };
sint32 sim::Handle;
uint32 BWSM::NormalCacheSysTime;
Trk::NewSlice *BWorldSm::slices;
Group *sim::ItemPtr;
void *sim::File;
tNormalCacheEntry BWSM::NormalCache[16];

void RawFindClosestSlice(coorddef *pt, BWorldSm::Pos *slicePos);

void FindAbsClosestSliceCrude(coorddef *pt, BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007E970, "NFS4.EXE");
    sint32 numSlices = gNumSlices;
    sint32 currDist = 0x7fffffff;
    sint32 currInd = -1;
    for (sint32 i = 0; i < numSlices; i = (sint32)((uint32)i + 8U))
    {
        coorddef center;
        center.x = BWorldSm::slices[i].center[0];
        center.y = BWorldSm::slices[i].center[1];
        center.z = BWorldSm::slices[i].center[2];
        sint32 dist = xzsquaredist32(&center, pt);
        if (dist < currDist)
        {
            currInd = i;
            currDist = dist;
        }
    }
    slicePos->slice = (sint16)currInd;
}

sint32 BWorldSm::Init(Group *simGroup)
{
    FUNCTION_MARKER(0x8007E910, "NFS4.EXE");
    sint32 numSlices = simGroup->m_num_elements;
    slices = reinterpret_cast<Trk::NewSlice *>(simGroup + 1);
    gMaxFindQuadSliceIterations = 5;
    gNumSlices = numSlices;
    Restart();
    return 1;
}

void BWorldSm::Restart(void)
{
    FUNCTION_MARKER(0x8007E948, "NFS4.EXE");
    NormalCache::Init();
}

void BWorldSm::DeInit(void)
{
    FUNCTION_MARKER(0x8007E968, "NFS4.EXE");
}

sint32 BWorldSm::FindClosestSlice(coorddef *pt, BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007EA14, "NFS4.EXE");
    sint32 startSlice = slicePos->slice;
    coorddef center;
    center.x = slices[startSlice].center[0];
    center.y = slices[startSlice].center[1];
    center.z = slices[startSlice].center[2];
    if (Math::DistXZ(&center, pt) > 0x800000)
        FindAbsClosestSliceCrude(pt, slicePos);
    RawFindClosestSlice(pt, slicePos);
    uint8 chunk = slices[slicePos->slice].chunkIndex;
    sint32 changed = slicePos->slice != startSlice;
    slicePos->quadChanged = (char)changed;
    slicePos->sliceChanged = (char)changed;
    slicePos->chunk = chunk;
    return changed;
}

void RawFindClosestSlice(coorddef *pt, BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007EAB0, "NFS4.EXE");
    sint32 numSlices = gNumSlices;
    sint32 maxind = (sint32)((uint32)numSlices - 1U);
    Trk::NewSlice *slices = BWorldSm::slices;
    sint32 index = slicePos->slice;
    if (index != -1)
    {
        sint32 lastind;
        do
        {
            lastind = index;
            sint32 currentSlice = index;
            if (index >= maxind)
            {
                if (numSlices == 0 || (numSlices == -1 && index == (-2147483647 - 1)))
                    abort();
                currentSlice = index % numSlices;
            }
            sint32 x = (sint32)((uint32)pt->x - (uint32)slices[currentSlice].center[0]) >> 9;
            sint32 z = (sint32)((uint32)pt->z - (uint32)slices[currentSlice].center[2]) >> 9;
            sint32 distcurr = (sint32)((uint32)x * (uint32)x + (uint32)z * (uint32)z);
            sint32 nextSlice = (sint32)((uint32)index + 1U);
            if (index >= (sint32)((uint32)maxind - 1U))
            {
                if (numSlices == 0 || (numSlices == -1 && nextSlice == (-2147483647 - 1)))
                    abort();
                nextSlice %= numSlices;
            }
            x = (sint32)((uint32)pt->x - (uint32)slices[nextSlice].center[0]) >> 9;
            z = (sint32)((uint32)pt->z - (uint32)slices[nextSlice].center[2]) >> 9;
            sint32 distnext = (sint32)((uint32)x * (uint32)x + (uint32)z * (uint32)z);
            if (distnext < distcurr)
                index = nextSlice;
            else if (index > 0)
            {
                sint32 previousSlice = (sint32)((uint32)index - 1U);
                x = (sint32)((uint32)pt->x - (uint32)slices[previousSlice].center[0]) >> 9;
                z = (sint32)((uint32)pt->z - (uint32)slices[previousSlice].center[2]) >> 9;
                sint32 distprev = (sint32)((uint32)x * (uint32)x + (uint32)z * (uint32)z);
                if (distprev < distcurr)
                    index = previousSlice;
            }
            else
            {
                sint32 candidate = (sint32)((uint32)index + 1U + (uint32)maxind);
                sint32 previousSlice = (sint32)((uint32)candidate - 1U);
                if (numSlices == 0 || (numSlices == -1 && previousSlice == (-2147483647 - 1)))
                    abort();
                sint32 previousSlot = previousSlice % numSlices;
                x = (sint32)((uint32)pt->x - (uint32)slices[previousSlot].center[0]) >> 9;
                z = (sint32)((uint32)pt->z - (uint32)slices[previousSlot].center[2]) >> 9;
                sint32 distprev = (sint32)((uint32)x * (uint32)x + (uint32)z * (uint32)z);
                if (distprev < distcurr)
                    candidate = previousSlice;
                candidate = (sint32)((uint32)candidate + 1U + (uint32)maxind);
                if (numSlices == 0 || (numSlices == -1 && candidate == (-2147483647 - 1)))
                    abort();
                index = candidate % numSlices;
            }
        } while (index != lastind);
    }
    slicePos->slice = (sint16)index;
}

void GetStmQuadPts(BWorldSm::Pos *slicePos, coorddef *cp)
{
    FUNCTION_MARKER(0x8007EDB8, "NFS4.EXE");
    sint32 cx = cp->x;
    sint32 cy = cp->y;
    sint32 cz = cp->z;
    Trk::NewStrip *pStrip = slicePos->strip;
    sint32 topInd = pStrip->topVert + slicePos->stripQuadInd;
    sint32 botInd = pStrip->botVert + slicePos->stripQuadInd;
    CCOORD16 *vertices = reinterpret_cast<CCOORD16 *>(Track::chunkList[slicePos->chunk].vertexBuf + 1);
    CCOORD16 *top = &vertices[topInd];
    CCOORD16 *bottom = &vertices[botInd];
    sint32 x = (sint32)((uint32)cx + (uint32)((sint32)top[1].x * 1024));
    sint32 y = (sint32)((uint32)cy + (uint32)((sint32)top[1].y * 1024));
    sint32 z = (sint32)((uint32)cz + (uint32)((sint32)top[1].z * 1024));
    slicePos->quadPts[1].x = x;
    slicePos->quadPts[1].y = y;
    slicePos->quadPts[1].z = z;
    x = (sint32)((uint32)cx + (uint32)((sint32)top[0].x * 1024));
    y = (sint32)((uint32)cy + (uint32)((sint32)top[0].y * 1024));
    z = (sint32)((uint32)cz + (uint32)((sint32)top[0].z * 1024));
    slicePos->quadPts[2].x = x;
    slicePos->quadPts[2].y = y;
    slicePos->quadPts[2].z = z;
    x = (sint32)((uint32)cx + (uint32)((sint32)bottom[0].x * 1024));
    y = (sint32)((uint32)cy + (uint32)((sint32)bottom[0].y * 1024));
    z = (sint32)((uint32)cz + (uint32)((sint32)bottom[0].z * 1024));
    slicePos->quadPts[3].x = x;
    slicePos->quadPts[3].y = y;
    slicePos->quadPts[3].z = z;
    x = (sint32)((uint32)cx + (uint32)((sint32)bottom[1].x * 1024));
    y = (sint32)((uint32)cy + (uint32)((sint32)bottom[1].y * 1024));
    z = (sint32)((uint32)cz + (uint32)((sint32)bottom[1].z * 1024));
    slicePos->quadPts[0].x = x;
    slicePos->quadPts[0].y = y;
    slicePos->quadPts[0].z = z;
}

void SetStrip(BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007EEE0, "NFS4.EXE");
    sint32 maxIndex = slicePos->simSlice->stripIndex;
    slicePos->strip = reinterpret_cast<Trk::NewStrip *>(Track::chunkList[slicePos->chunk].stripBuf + 1);
    for (sint32 i = 0; i < maxIndex; ++i)
    {
        sint32 quadCount = (uint8)slicePos->strip->quadCount;
        slicePos->strip = reinterpret_cast<Trk::NewStrip *>(
            reinterpret_cast<uint8 *>(slicePos->strip) + sizeof(Trk::NewStrip) + quadCount * 2);
    }
    sint32 i = (sint8)slicePos->quad;
    sint32 quadCount = (uint8)slicePos->strip->quadCount;
    while (i >= quadCount)
    {
        i = (sint32)((uint32)i - (uint32)quadCount);
        slicePos->strip = reinterpret_cast<Trk::NewStrip *>(
            reinterpret_cast<uint8 *>(slicePos->strip) + sizeof(Trk::NewStrip) + quadCount * 2);
        quadCount = (uint8)slicePos->strip->quadCount;
    }
    slicePos->stripQuadInd = (sint16)i;
}

void GetFirstStmQuadPts(BWorldSm::Pos *slicePos, CCOORD16 *vertices)
{
    FUNCTION_MARKER(0x8007EF98, "NFS4.EXE");
    SetStrip(slicePos);
    Trk::NewStrip *pStrip = slicePos->strip;
    sint32 topInd = pStrip->topVert + slicePos->stripQuadInd;
    sint32 botInd = pStrip->botVert + slicePos->stripQuadInd;
    CCOORD16 *pts = vertices + topInd;
    sint16 x = pts[1].x;
    sint16 z = pts[1].z;
    slicePos->quadPts16[1].x = x;
    slicePos->quadPts16[1].z = z;
    x = pts[0].x;
    z = pts[0].z;
    slicePos->quadPts16[2].x = x;
    slicePos->quadPts16[2].z = z;
    pts = vertices + botInd;
    x = pts[0].x;
    z = pts[0].z;
    slicePos->quadPts16[3].x = x;
    slicePos->quadPts16[3].z = z;
    x = pts[1].x;
    z = pts[1].z;
    slicePos->quadPts16[0].x = x;
    slicePos->quadPts16[0].z = z;
}

void BWorldSm::SetSlice(sint32 slice, BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007ED64, "NFS4.EXE");
    slicePos->slice = (sint16)slice;
    slicePos->sliceChanged = 0;
    slicePos->quadChanged = 0;
    slicePos->offEdge = 0;
    slicePos->simSlice = 0;
    slicePos->simQuad = 0;
    slicePos->simRotFlag = 0;
    uint8 chunk = slices[slicePos->slice].chunkIndex;
    slicePos->lastRezRequested = (char)0xfe;
    slicePos->rez = 1;
    slicePos->triangleFlag = 0;
    slicePos->quad = (char)0xff;
    slicePos->chunk = chunk;
}

void BWorld::SetSimSlice(BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007F034, "NFS4.EXE");
    slicePos->chunk = BWorldSm::slices[slicePos->slice].chunkIndex;
    Chunk *chunk = &Track::chunkList[slicePos->chunk];
    Trk::NewSimSlice *simSlices = reinterpret_cast<Trk::NewSimSlice *>(chunk->simSliceBuf + 1);
    sint32 chunkSliceInd = slicePos->slice - chunk->firstSimSliceInd;
    slicePos->simSlice = &simSlices[chunkSliceInd];
}

void BworldSm::UpdateSimQuad(BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007F094, "NFS4.EXE");
    Trk::NewSimSlice *simSlice = slicePos->simSlice;
    sint32 simIndex = (sint8)slicePos->quad - simSlice->simquadStartIndex;
    if (simIndex >= 0 && simIndex < simSlice->simquadCount)
    {
        Trk::NewSimQuad *startsimquad = reinterpret_cast<Trk::NewSimQuad *>(
            Track::chunkList[slicePos->chunk].simQuadBuf + 1);
        slicePos->simQuad = startsimquad;
        slicePos->simQuad = &startsimquad[simSlice->simquadIndex + simIndex];
    }
    else
        slicePos->simQuad = &GlobalSimQuad;
}

sint32 BworldSm::IsSimQuadValid(BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007F11C, "NFS4.EXE");
    if (slicePos->simQuad == 0)
        return 0;
    return (slicePos->simQuad->surface & 15) != 14;
}

sint32 RawFindClosestQuad(coorddef *pt, BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007F14C, "NFS4.EXE");
    static sint32 sliceOffs[12] = {1, -2, 3, -4, 5, -6, 7, -8, 9, -10, 11, -12};
    sint32 attempt = 0;
    sint32 sliceVariance = 0;
    sint32 firstSliceOffEdge = 0;
    slicePos->simQuad = 0;
    slicePos->simRotFlag = 0;
    BWorld::SetSimSlice(slicePos);
    sint32 startQuadInd = (sint8)slicePos->quad;
    if (startQuadInd < 0 || startQuadInd >= (sint32)slicePos->simSlice->quadCount - 1 || slicePos->offEdge != 0)
    {
        startQuadInd = ((sint32)slicePos->simSlice->quadCount - 1) / 2;
        slicePos->quad = (char)startQuadInd;
    }
    slicePos->offEdge = 0;
    slicePos->quad = (char)startQuadInd;
    for (;;)
    {
        if (attempt >= gMaxFindQuadSliceIterations)
        {
            sint32 newSlice = (sint32)((uint32)(sint32)slicePos->slice - (uint32)sliceVariance);
            sint32 numSlices = gNumSlices;
            if (newSlice >= numSlices)
                newSlice = (sint32)((uint32)newSlice - (uint32)numSlices);
            if (newSlice < 0)
                newSlice = (sint32)((uint32)newSlice + (uint32)numSlices);
            slicePos->slice = (sint16)newSlice;
            BWorld::SetSimSlice(slicePos);
            slicePos->offEdge = (char)firstSliceOffEdge;
            slicePos->simQuad = 0;
            slicePos->triangleFlag = 0;
            return 0;
        }
        BWorld::SetSimSlice(slicePos);
        slicePos->quad = (char)startQuadInd;
        sint32 quadCount = slicePos->simSlice->quadCount;
        if (quadCount - 1 < startQuadInd)
            slicePos->quad = (char)(quadCount - 2);
        coorddef *cp = &Chunk::chunkCenters[slicePos->chunk];
        CCOORD16 pt16;
        pt16.x = (sint16)((sint32)((uint32)pt->x - (uint32)cp->x) >> 10);
        pt16.z = (sint16)((sint32)((uint32)pt->z - (uint32)cp->z) >> 10);
        CCOORD16 *vertices = reinterpret_cast<CCOORD16 *>(Track::chunkList[slicePos->chunk].vertexBuf + 1);
        for (sint32 direction = 0; direction < 2; ++direction)
        {
            if (direction != 0)
            {
                sint32 quad = slicePos->offEdge != 0 && attempt == 0 ?
                    (sint32)slicePos->simSlice->quadCount - 1 : startQuadInd - 1;
                slicePos->quad = (char)quad;
                if ((sint8)slicePos->quad < 0)
                    slicePos->quad = 0;
            }
            GetFirstStmQuadPts(slicePos, vertices);
            BworldSm::UpdateSimQuad(slicePos);
            sint32 lastDist = 0x7fffffff;
            while (direction == 0 ? (sint8)slicePos->quad <= (sint32)slicePos->simSlice->quadCount - 1 :
                                   (sint8)slicePos->quad >= 0)
            {
                GetFirstStmQuadPts(slicePos, vertices);
                const sint32 edges[4][2] = {{1, 2}, {0, 1}, {2, 3}, {3, 0}};
                bool inside = true;
                for (sint32 edge = 0; edge < 4; ++edge)
                {
                    CCOORD16 *first = &slicePos->quadPts16[edges[edge][0]];
                    CCOORD16 *second = &slicePos->quadPts16[edges[edge][1]];
                    uint32 left = (uint32)((sint32)first->x - second->x) * (uint32)((sint32)pt16.z - second->z);
                    uint32 right = (uint32)((sint32)pt16.x - second->x) * (uint32)((sint32)first->z - second->z);
                    if ((sint32)(left - right) > 0)
                    {
                        inside = false;
                        break;
                    }
                }
                if (inside && BworldSm::IsSimQuadValid(slicePos) != 0)
                {
                    slicePos->rez = 2;
                    slicePos->offEdge = 0;
                    GetStmQuadPts(slicePos, cp);
                    return 1;
                }
                if (direction == 0 ? (sint8)slicePos->quad == (sint32)slicePos->simSlice->quadCount - 1 :
                                     (sint8)slicePos->quad == 0)
                {
                    slicePos->offEdge = (char)(direction == 0 ? 2 : 1);
                    break;
                }
                if (BworldSm::IsSimQuadValid(slicePos) != 0)
                {
                    sint32 dist = 0;
                    for (sint32 corner = 0; corner < 4; ++corner)
                    {
                        sint32 x = (sint32)pt16.x - slicePos->quadPts16[corner].x;
                        sint32 z = (sint32)pt16.z - slicePos->quadPts16[corner].z;
                        if (x < 0)
                            x = -x;
                        if (z < 0)
                            z = -z;
                        dist += x + z;
                    }
                    if ((sint32)((uint32)dist - (uint32)lastDist) > 40)
                        break;
                    lastDist = dist;
                }
                slicePos->quad = (char)((uint32)(uint8)slicePos->quad + (direction == 0 ? 1U : 0xffffffffU));
                BworldSm::UpdateSimQuad(slicePos);
            }
        }
        if (attempt == 0)
            firstSliceOffEdge = (sint8)slicePos->offEdge;
        sint32 offset = sliceOffs[attempt];
        sint32 newSlice = (sint32)((uint32)(sint32)slicePos->slice + (uint32)offset);
        sint32 numSlices = gNumSlices;
        if (newSlice >= numSlices)
            newSlice = (sint32)((uint32)newSlice - (uint32)numSlices);
        if (newSlice < 0)
            newSlice = (sint32)((uint32)newSlice + (uint32)numSlices);
        slicePos->slice = (sint16)newSlice;
        ++attempt;
        sliceVariance = (sint32)((uint32)sliceVariance + (uint32)offset);
    }
}

sint32 FindClosestQuad(coorddef *pt, BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007F8F8, "NFS4.EXE");
    sint32 startSlice = slicePos->slice;
    sint32 startQuad = (sint8)slicePos->quad;
    BWorldSm::FindClosestSlice(pt, slicePos);
    sint32 foundSlice = slicePos->slice;
    sint32 done = RawFindClosestQuad(pt, slicePos);
    if (done == 0)
    {
        if (slicePos->offEdge != 0)
        {
            slicePos->slice = (sint16)foundSlice;
            BWorld::SetSimSlice(slicePos);
            slicePos->quad = slicePos->offEdge == 1 ? 0 :
                (char)((uint32)(uint8)slicePos->simSlice->quadCount - 1U);
            BworldSm::UpdateSimQuad(slicePos);
            slicePos->rez = 2;
            SetStrip(slicePos);
            GetStmQuadPts(slicePos, &Chunk::chunkCenters[(uint8)slicePos->chunk]);
        }
        static coorddef corrPt;
        corrPt = *pt;
        for (sint32 rCount = 0; slicePos->simQuad == 0 && rCount < 10; ++rCount)
        {
            sint32 deltaX = (sint32)((uint32)BWorldSm::slices[slicePos->slice].center[0] - (uint32)corrPt.x) >> 5;
            corrPt.x = (sint32)((uint32)corrPt.x + (uint32)deltaX);
            sint32 deltaZ = (sint32)((uint32)BWorldSm::slices[slicePos->slice].center[2] - (uint32)corrPt.z) >> 5;
            corrPt.z = (sint32)((uint32)corrPt.z + (uint32)deltaZ);
            RawFindClosestQuad(&corrPt, slicePos);
        }
    }
    slicePos->sliceChanged = (char)(startSlice != slicePos->slice);
    slicePos->quadChanged = (char)(startQuad != (sint8)slicePos->quad || slicePos->sliceChanged != 0);
    return slicePos->sliceChanged;
}

sint32 PointDirection(coorddef *p1, coorddef *p2, coorddef *p3)
{
    FUNCTION_MARKER(0x8007FCB0, "NFS4.EXE");
    sint32 first = fixedmult((sint32)((uint32)p1->x - (uint32)p2->x),
        (sint32)((uint32)p3->z - (uint32)p2->z));
    sint32 second = fixedmult((sint32)((uint32)p3->x - (uint32)p2->x),
        (sint32)((uint32)p1->z - (uint32)p2->z));
    return (sint32)((uint32)first - (uint32)second);
}

sint32 BWorldSm::FindClosestQuadRez(coorddef *pt, BWorldSm::Pos *slicePos, sint32 hiRezFlag)
{
    FUNCTION_MARKER(0x8007FAC4, "NFS4.EXE");
    slicePos->triangleFlag = 3;
    if (hiRezFlag == 0)
    {
        slicePos->lastRezRequested = 1;
        slicePos->rez = 1;
        slicePos->simSlice = 0;
        slicePos->simQuad = 0;
        slicePos->quad = (char)0xff;
        slicePos->triangleFlag = 0;
        return FindClosestSlice(pt, slicePos);
    }
    slicePos->lastRezRequested = 2;
    if (slicePos->simQuad != 0)
    {
        const sint32 edges[4][2] = {{1, 2}, {0, 1}, {2, 3}, {3, 0}};
        bool inside = true;
        for (sint32 edge = 0; edge < 4; ++edge)
        {
            coorddef *firstPoint = &slicePos->quadPts[edges[edge][0]];
            coorddef *secondPoint = &slicePos->quadPts[edges[edge][1]];
            sint32 first = fixedmult((sint32)((uint32)firstPoint->x - (uint32)secondPoint->x),
                (sint32)((uint32)pt->z - (uint32)secondPoint->z));
            sint32 second = fixedmult((sint32)((uint32)pt->x - (uint32)secondPoint->x),
                (sint32)((uint32)firstPoint->z - (uint32)secondPoint->z));
            if ((sint32)((uint32)first - (uint32)second) > 0)
            {
                inside = false;
                break;
            }
        }
        if (inside)
        {
            slicePos->quadChanged = 0;
            slicePos->sliceChanged = 0;
            return 0;
        }
    }
    return ::FindClosestQuad(pt, slicePos);
}

sint32 BWorldSm::FindClosestQuadMaxIterations(coorddef *pt, BWorldSm::Pos *slicePos, sint32 maxIterations)
{
    FUNCTION_MARKER(0x8007FC90, "NFS4.EXE");
    (void)maxIterations;
    return FindClosestQuadRez(pt, slicePos, 1);
}

sint32 BWorldSm::FindEdgeOff(coorddef *pt, BWorldSm::Pos *slicePos1, BWorldSm::Pos *slicePos2, sint32 *heightDiff)
{
    FUNCTION_MARKER(0x8007FD28, "NFS4.EXE");
    coorddef *pts = slicePos1->quadPts;
    sint32 ret = PointDirection(&pts[2], &pts[3], pt) >= -0x18000 ? 1 : 0;
    if (PointDirection(&pts[3], &pts[0], pt) >= 0)
        ret |= 8;
    if (PointDirection(&pts[0], &pts[1], pt) >= -0x18000)
        ret |= 2;
    if (PointDirection(&pts[1], &pts[2], pt) >= 0)
        ret |= 4;
    sint32 y = (sint32)((uint32)slicePos2->quadPts[0].y + (uint32)slicePos2->quadPts[1].y +
                    (uint32)slicePos2->quadPts[2].y + (uint32)slicePos2->quadPts[3].y) >> 2;
    *heightDiff = (sint32)((uint32)y - (uint32)pt->y);
    return ret;
}

sint32 BWorldSm::QuadLight(BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007FE44, "NFS4.EXE");
    if (slicePos->rez != 2)
        return 0x7F7F7F;
    sint32 topInd = slicePos->strip->topVert + slicePos->stripQuadInd;
    sint32 botInd = slicePos->strip->botVert + slicePos->stripQuadInd;
    CCOORD16 *vertices = reinterpret_cast<CCOORD16 *>(Track::chunkList[slicePos->chunk].vertexBuf + 1);
    CVECTOR temp0 = Chunk::lightTable[vertices[topInd + 1].light];
    CVECTOR temp1 = Chunk::lightTable[vertices[topInd].light];
    CVECTOR temp2 = Chunk::lightTable[vertices[botInd].light];
    CVECTOR temp3 = Chunk::lightTable[vertices[botInd + 1].light];
    sint32 red = (temp0.r + temp1.r + temp2.r + temp3.r) >> 2;
    sint32 green = (temp0.g + temp1.g + temp2.g + temp3.g) >> 2;
    sint32 blue = (temp0.b + temp1.b + temp2.b + temp3.b) >> 2;

    return red | (green << 8) | (blue << 16);
}

bool BWorldSm::TunnelFlagSm(BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8007FFD4, "NFS4.EXE");
    if ((slices[slicePos->slice].acousticType & 0x44) != 0)
        return true;
    sint32 surf = slicePos->simQuad ? slicePos->simQuad->surface & 15 : 14;
    return surf == 8;
}

void NormalCache::AddEntry(BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x8008002C, "NFS4.EXE");
    uint32 oldestTime = 0xFFFFFFFFu;
    sint32 oldestInd = -1;
    BWSM::NormalCacheSysTime = (uint32)BWSM::NormalCacheSysTime + 1u;
    for (sint32 i = 0; i < 16; ++i)
    {
        tNormalCacheEntry *ce = &BWSM::NormalCache[i];
        if ((uint32)ce->accessTime < oldestTime)
        {
            oldestInd = i;
            oldestTime = (uint32)ce->accessTime;
        }
    }
    tNormalCacheEntry *ce = &BWSM::NormalCache[oldestInd];
    ce->accessTime = BWSM::NormalCacheSysTime;
    ce->forward = slicePos->forward;
    ce->normal = slicePos->normal;
    ce->sliceInd = slicePos->slice;
    ce->quadInd = slicePos->quad;
    ce->triangleFlag = slicePos->triangleFlag;
}

bool NormalCache::FindEntry(BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x800800E8, "NFS4.EXE");
    BWSM::NormalCacheSysTime = (uint32)BWSM::NormalCacheSysTime + 1U;
    sint32 slice = slicePos->slice;
    sint32 quad = (sint8)slicePos->quad;
    for (sint32 i = 0; i < 16; ++i)
    {
        tNormalCacheEntry *ce = &BWSM::NormalCache[i];
        if (ce->sliceInd == slice && (sint8)ce->quadInd == quad &&
            ce->triangleFlag == (sint8)slicePos->triangleFlag)
        {
            ce->accessTime = BWSM::NormalCacheSysTime;
            slicePos->normal = ce->normal;
            slicePos->forward = ce->forward;
            return true;
        }
    }
    return false;
}

void Check_Rot(BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x800801E8, "NFS4.EXE");
    if (slicePos->simRotFlag != (sint8)slicePos->triangleFlag && !NormalCache::FindEntry(slicePos))
    {
        coorddef vecX;
        coorddef vecZ;
        sint32 triangle = (sint8)slicePos->triangleFlag;
        if (triangle == 3)
        {
            vecZ.x = (sint32)((uint32)slicePos->quadPts[2].x - (uint32)slicePos->quadPts[3].x +
                (uint32)((sint32)((uint32)slicePos->quadPts[1].x - (uint32)slicePos->quadPts[0].x) / 8));
            vecZ.y = (sint32)((uint32)slicePos->quadPts[2].y - (uint32)slicePos->quadPts[3].y +
                (uint32)((sint32)((uint32)slicePos->quadPts[1].y - (uint32)slicePos->quadPts[0].y) / 8));
            vecZ.z = (sint32)((uint32)slicePos->quadPts[2].z - (uint32)slicePos->quadPts[3].z +
                (uint32)((sint32)((uint32)slicePos->quadPts[1].z - (uint32)slicePos->quadPts[0].z) / 8));
            vecX.x = (sint32)((uint32)slicePos->quadPts[0].x - (uint32)slicePos->quadPts[3].x +
                (uint32)((sint32)((uint32)slicePos->quadPts[1].x - (uint32)slicePos->quadPts[2].x) / 8));
            vecX.y = (sint32)((uint32)slicePos->quadPts[0].y - (uint32)slicePos->quadPts[3].y +
                (uint32)((sint32)((uint32)slicePos->quadPts[1].y - (uint32)slicePos->quadPts[2].y) / 8));
            vecX.z = (sint32)((uint32)slicePos->quadPts[0].z - (uint32)slicePos->quadPts[3].z +
                (uint32)((sint32)((uint32)slicePos->quadPts[1].z - (uint32)slicePos->quadPts[2].z) / 8));
        }
        else if (triangle == 2)
        {
            vecZ.x = (sint32)((uint32)slicePos->quadPts[1].x - (uint32)slicePos->quadPts[0].x);
            vecZ.y = (sint32)((uint32)slicePos->quadPts[1].y - (uint32)slicePos->quadPts[0].y);
            vecZ.z = (sint32)((uint32)slicePos->quadPts[1].z - (uint32)slicePos->quadPts[0].z);
            vecX.x = (sint32)((uint32)slicePos->quadPts[1].x - (uint32)slicePos->quadPts[2].x);
            vecX.y = (sint32)((uint32)slicePos->quadPts[1].y - (uint32)slicePos->quadPts[2].y);
            vecX.z = (sint32)((uint32)slicePos->quadPts[1].z - (uint32)slicePos->quadPts[2].z);
        }
        else
        {
            vecZ.x = (sint32)((uint32)slicePos->quadPts[2].x - (uint32)slicePos->quadPts[3].x);
            vecZ.y = (sint32)((uint32)slicePos->quadPts[2].y - (uint32)slicePos->quadPts[3].y);
            vecZ.z = (sint32)((uint32)slicePos->quadPts[2].z - (uint32)slicePos->quadPts[3].z);
            vecX.x = (sint32)((uint32)slicePos->quadPts[0].x - (uint32)slicePos->quadPts[3].x);
            vecX.y = (sint32)((uint32)slicePos->quadPts[0].y - (uint32)slicePos->quadPts[3].y);
            vecX.z = (sint32)((uint32)slicePos->quadPts[0].z - (uint32)slicePos->quadPts[3].z);
        }
        slicePos->forward = vecZ;
        coorddef *normal = &slicePos->normal;
        coorddef *forward = &slicePos->forward;
        crossproduct(&vecZ, &vecX, normal);
        Math::NormalizeVector(normal);
        if (normal->y > 0xffff)
            normal->y = 0xfff9;
        Math::NormalizeVector(forward);
        NormalCache::AddEntry(slicePos);
    }
    slicePos->simRotFlag = (sint16)(sint8)slicePos->triangleFlag;
}

void NormalCache::Init(void)
{
    FUNCTION_MARKER(0x800801AC, "NFS4.EXE");
    BWSM::NormalCacheSysTime = 0;
    for (sint32 i = 0; i < 16; ++i)
    {
        BWSM::NormalCache[i].sliceInd = -1;
        BWSM::NormalCache[i].quadInd = (char)0xff;
        BWSM::NormalCache[i].triangleFlag = 0;
        BWSM::NormalCache[i].accessTime = 0;
    }
}

coorddef * BWorldSm::UNormal(BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x80080520, "NFS4.EXE");
    Check_Rot(slicePos);
    return &slicePos->normal;
}

coorddef * BWorldSm::UForward(BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x80080548, "NFS4.EXE");
    Check_Rot(slicePos);
    return &slicePos->forward;
}

sint32 BWorldSm::FindClosestTriangleRez(coorddef *pt, BWorldSm::Pos *slicePos, sint32 hiRezFlag)
{
    FUNCTION_MARKER(0x80080570, "NFS4.EXE");
    sint32 ret = FindClosestQuadRez(pt, slicePos, hiRezFlag);
    if (slicePos->simQuad != 0)
    {
        sint32 first = fixedmult((sint32)((uint32)slicePos->quadPts[2].x - (uint32)slicePos->quadPts[0].x),
            (sint32)((uint32)pt->z - (uint32)slicePos->quadPts[0].z));
        sint32 second = fixedmult((sint32)((uint32)pt->x - (uint32)slicePos->quadPts[0].x),
            (sint32)((uint32)slicePos->quadPts[2].z - (uint32)slicePos->quadPts[0].z));
        slicePos->triangleFlag = (sint32)((uint32)first - (uint32)second) > 0 ? 1 : 2;
    }
    return ret;
}
