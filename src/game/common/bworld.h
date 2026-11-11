#ifndef BWORLD_H
#define BWORLD_H

#include "xport.h"
#include "game/common/bworldsm.h"
#include "game/common/carobj.h"
#include "game/common/group.h"
#include "game/common/matrixtdef.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"

extern matrixtdef gWorldMat;

sint32 xzsquaredist32(coorddef *c1, coorddef *c2);
char *BWAllocMem(sint32 size);

struct tBuildEntry;

class BWorld
{
public:
    static tBuildEntry gChunkBuildList[36];

    static sint32 gChunkCount;

    static void BuildGlareEffects(DRender::tView *Vi, Draw::DCache *sd, Group *group);
    static void InitSpikeBelt(void);
    static void SetSpikeBelt(sint32 slice, sint32 x, sint32 width);
    static sint32 GetSpikeBelt(sint32 *slice, sint32 *x, sint32 *width);
    static sint32 CheckChunkVisible(BWorldSm::Pos *slicePosSource, BWorldSm::Pos *slicePosTest);
    static bool IsSliceInBuildList(sint32 slice);
    static void OnyxBuildFacets(DRender::tView *Vi);
    static void InitContexts(void);
    static void DeInitContexts(void);
    static sint32 OpenContext(sint32 contextType, sint32 client);
    static void Restart(void);
    static void StartLoop(void);
    static void Init(void);
    static void DeInit(void);
    static void SetSimSlice(BWorldSm::Pos *slicePos);
};

class BW
{
public:
    struct tContext;

    struct tContextMgr;
    static Car::tObj *gCopCarObj;
};

struct BW::tContext
{
    sint32 client;
    BWorldSm::Pos slicePos;
    sint32 currentChunk;
    sint32 chunkFarZClipSq;
    sint32 polyFarZClipSq;
    sint32 lowDetailDistSq;
    sint32 lineFarZClipSq;
};

extern BW::tContext *gCurrContext;
extern sint32 SceneLoaded;
extern matrixtdef gCopMat;
extern matrixtdef gNightMat;
extern sint32 gSpikeBeltSlice;
extern sint32 gSpikeBeltX;

#endif
