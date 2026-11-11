#ifndef TRACK_H
#define TRACK_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/chunk.h"
#include "game/common/group.h"
#include "game/common/trk.h"
#include "game/psx/draw.h"

struct tSaveSurface;

class SaveSurface

{
public:
    sint16 fCount;

    sint16 fMaxCount;

    tSaveSurface *fStack;

    void Save(Trk::NewSimQuad *simQuad);

    SaveSurface(sint32 numEntries);

    ~SaveSurface(void);
    static void operator delete(void *memory);

    void RestoreAll(void);
};

struct TrackHeader;

class Track
{
public:
    struct tMaterial
    {
        char flag;
        char mipmap_offset;
        sint16 pmxIndex;
    };

    struct tArtresource
    {
        sint32 id;
        char *shapeFile;
        sint32 shapeCount;
        Draw::tPixMap *pPmx;
        sint32 pmxCount;
        sint32 basePmxCount;
    };

    static uint8 *gInViewCount;
    static sint16 (*gInViewList)[32];
    static Trk::ObjectDef **gObjDefs;

    static TrackHeader *header;
    static Chunk *chunkList;
    static SaveSurface *gSaveSurface;
    static tMaterial *materials;
    static SimpleMem *mem;
    static Draw::tPixMap gReflectionMaps[4];

    static sint32 gControllerCount;

    struct tMaterialController;

    struct MipMap
    {
        uint16 code[2];
        sint32 shapeParentIndex;
        sint32 mipMapIndex;
    };

    struct MultiPalette
    {
        uint8 charcode[2];
        sint16 palnum;
        sint16 origshapeindex;
        sint16 actualshapeindex;
    };

    static tMaterialController *gMatController;

    static void SetTrackNumber(sint32 tracknum);
    static char * MakeTrackPathName(char *ext);
    static char * MakeTrackDataPathName(char *ext);
    static void AnimateTextures(void);
    static sint32 GetProperMultiPalShapeIndex(sint32 shapeindex, sint32 paletteindex);
    static void ProcessFlipAndUVFlags(sint32 uvFlag, Draw::tPixMap *inputPmx, Draw::tPixMap *outputPmx);
    static void AssociateSingleMaterial(Trk::Material *inputMat, tMaterial *outputMat, tArtresource *art);
    static void LinkMaterials(SerializedGroup *group, sint32 length, tMaterial *matList);
    static void InitPersistentData(SerializedGroup *perGroup);
    static void Init(char *tempName);
    static void DeInit(void);
    static void AnimateTrackLighting(void);
    static void LoadObjectKillData(void);
};

class KillFile
{
public:
    static char * OpenRead(void);
    static void ReadEntry(char *filePtr, sint32 entryInd, sint32 &chunkInd, sint32 &objectInd);
};

extern Track::tArtresource gInitialArt;
extern Group *gPersistMidgroundObjInst;
extern Group *gPersistObjDef;
extern Group *gPersistObjDefBoundingSpheres;
extern Group *gPersistObjInst;

#endif
