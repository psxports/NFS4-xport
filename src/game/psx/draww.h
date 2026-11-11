#ifndef DRAWW_H
#define DRAWW_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"
#include "game/common/renderview.h"
#include "game/common/track.h"
#include "game/common/trk.h"
#include "game/psx/draw.h"

struct tBuildEntry;

struct ChunkObjectInfo;

struct Draw::SVertex

{
    sint16 vx;

    sint16 vy;

    sint16 vz;

    uint8 u;

    uint8 v;

    sint16 dvx;

    sint16 dvy;

    uint8 r;

    uint8 g;

    uint8 b;

    uint8 a;
};

struct Draw::SubdivStruct
{
    Draw::SVertex v[15];
};

struct Draw::tGiveShelbyMoreCache
{
    Draw::tCacheHeader head;
    MATRIX matB;
    MATRIX matNight;
    MATRIX matCop;
    MATRIX identMat;
    sint32 otz;
    VECTOR tVn0;
    VECTOR tVn1;
    VECTOR tVn2;
    VECTOR tVn3;
    sint16 light;
    sint16 doublelayer;
    sint16 startfog;
    sint16 distfog;
    CCOORD16 *vertices;
    uint8 chunkInd;
    uint8 rezInd;
    uint8 garb3;
    uint8 quadCount;
    void *quads;
    sint32 offset;
    Track::tMaterial *materials;
    COORD16 trans;
    Track::tArtresource *artInfo;
    Trk::NewStrip *stripPtr;
    sint16 numStrips;
    char nightFlags;
    char zeroGTETransFlag;
    sint16 night_ZNear;
    uint8 night_XDistShift;
    uint8 night_ZDistShift;
    uint8 night_DrawLightning;
    uint8 night_LightningType;
    uint8 fogstate;
    uint8 padnight;
    POLY_GT4 GT4Prim;
    uint16 tpage;
    uint16 clut;
    sint16 offsubdivid;
    uint8 code;
    uint8 magiknum;
    Draw::SVertex v0;
    Draw::SVertex v1;
    Draw::SVertex v2;
    Draw::SVertex v3;
    Draw::SubdivStruct div;
};

struct tBoundingSphere
{
    COORD16 cp;
    sint16 radius;
};

class DrawW
{
public:
    static void AddSubdividPrimGT4(POLY_GT4 *prim, Draw::SVertex *v0, Draw::SVertex *v1, Draw::SVertex *v2, Draw::SVertex *v3, Draw::tGiveShelbyMoreCache *sd);
    static void AddSubdividPrimGT3(POLY_GT3 *prim, Draw::SVertex *v0, Draw::SVertex *v1, Draw::SVertex *v2, Draw::tGiveShelbyMoreCache *sd);
    static void DivVertice(Draw::SVertex *v0, Draw::SVertex *v1, Draw::SVertex *v2);
    static sint32 CalcSubdivision(Draw::tGiveShelbyMoreCache *sd, Draw::SVertex *v0, Draw::SVertex *v1, Draw::SVertex *v2, Draw::SVertex *v3);
    static void SubdividFacet(Draw::tGiveShelbyMoreCache *sd, sint32 l, Draw::SVertex *v0, Draw::SVertex *v1, Draw::SVertex *v2, Draw::SVertex *v3, sint16 n, sint16 subDivide);
    static void LoadPrecVECTOR(Draw::SVertex *v, VECTOR *dv);
    static void SetUpSubdividFacet(sint32 face, Draw::tGiveShelbyMoreCache *sd);
    static void NightColorCalc(Draw::tGiveShelbyMoreCache *sd, POLY_GT4 *prim, CCOORD16 *vt0, CCOORD16 *vt1, CCOORD16 *vt2, CCOORD16 *vt3);
    static void DrawQuad(Draw::tGiveShelbyMoreCache *sd, Trk::Quad *inQuad);
    static void kCtrlWorld_High(Draw::tGiveShelbyMoreCache *sd);
    static void StripDraw_High(Draw::tGiveShelbyMoreCache *sd);
    static void DoTrough(DRender::tView *Vi, tBuildEntry *buildList);
    static void WorldSetUpMatrix(matrixtdef *m, MATRIX *mat);
    static void WorldSetUpTranslation(coorddef *t, MATRIX *mat);
    static void ResetAnimationTimer(void);
    static sint32 GetAnimationTime(Trk::AnimateInst *animInst);
    static void SetAnimationTime(Trk::AnimateInst *animInst, sint32 *table, sint32 time);
    static void DoObjectAnimations(void);
    static sint32 BuildObjectFacets(DRender::tView *Vi, ChunkObjectInfo *gObjInfo);
    static sint32 BuildCustomObjectFacets(DRender::tView *Vi, Draw::DCache *sd, Trk::SimObject *simObjs, Group *group, sint32 zClipSq);
    static sint32 BuildChunkObjectFacets(DRender::tView *Vi, ChunkObjectInfo *gObjInfo);
    static void DoObjects(DRender::tView *Vi, tBuildEntry *buildList);
    static void SetUpSubdividFacet_Line(Draw::tGiveShelbyMoreCache *sd);
    static void OnyxLinePrim(CCOORD16 *geomVertices, Trk::Line *lineQuad, sint32 count, Draw::tGiveShelbyMoreCache *sd);
    static void BuildChunkCenterLineFacets(Chunk *chunkDat, Group *group, Draw::tGiveShelbyMoreCache *sd, COORD16 *trans);
    static void DoLines(DRender::tView *Vi, tBuildEntry *buildList, Draw::DCache *sd);
    static void BuildSpikeBelt(DRender::tView *Vi, sint32 scale, Draw::DCache *sd);
};

sint32 DrawObjectTransform(DRender::tView *Vi, Draw::DCache *sd, matrixtdef *matrix, Trk::ObjectDef *objDef, coorddef *pCp, sint32 offset, sint16 light);

sint32 DrawObjectSimple(DRender::tView *Vi, Draw::DCache *sd, Trk::ObjectDef *objDef, coorddef *pCp, sint32 offset);

bool ObjectClipped(DRender::tView *Vi, sint32 ind, coorddef *pCp, Draw::tGiveShelbyMoreCache *sd);

extern sint32 stackSpeedUpEnbabledFlag;

void DepthCue_Init(void);

#endif
