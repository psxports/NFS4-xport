#ifndef BWORLDSM_H
#define BWORLDSM_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/group.h"
#include "game/common/trk.h"

class BWorldSm
{
public:
    static Trk::NewSlice *slices;

    struct Pos;

    static sint32 Init(Group *simGroup);
    static void Restart(void);
    static void DeInit(void);
    static sint32 FindClosestSlice(coorddef *pt, Pos *slicePos);
    static void SetSlice(sint32 slice, Pos *slicePos);
    static sint32 FindClosestQuadRez(coorddef *pt, Pos *slicePos, sint32 hiRezFlag);
    static sint32 FindClosestQuadMaxIterations(coorddef *pt, Pos *slicePos, sint32 maxIterations);
    static sint32 FindEdgeOff(coorddef *pt, Pos *slicePos1, Pos *slicePos2, sint32 *heightDiff);
    static sint32 QuadLight(Pos *slicePos);
    static bool TunnelFlagSm(Pos *slicePos);
    static coorddef * UNormal(Pos *slicePos);
    static coorddef * UForward(Pos *slicePos);
    static sint32 FindClosestTriangleRez(coorddef *pt, Pos *slicePos, sint32 hiRezFlag);
};

class NormalCache
{
public:
    static void AddEntry(BWorldSm::Pos *slicePos);
    static bool FindEntry(BWorldSm::Pos *slicePos);
    static void Init(void);
};

class BworldSm
{
public:
    static void UpdateSimQuad(BWorldSm::Pos *slicePos);
    static sint32 IsSimQuadValid(BWorldSm::Pos *slicePos);
};

extern sint32 gNumSlices;

void FindAbsClosestSliceCrude(coorddef *pt, BWorldSm::Pos *slicePos);

#endif
