#include <new>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/bworld.h"
#include "game/common/carobj.h"
#include "game/common/chunk.h"
#include "game/common/genericpmx.h"
#include "game/common/group.h"
#include "game/common/paths.h"
#include "game/common/sim.h"
#include "game/common/simplemem.h"
#include "game/common/track.h"
#include "game/common/trk.h"
#include "game/psx/draw.h"
#include "game/psx/draww.h"
#include "game/psx/hrzsku.h"
#include "game/psx/texture.h"
#include "game/psx/textureprocess.h"
#include "game/psx/trackspec.h"
#include "lib/psx/fixdsqrt.h"
#include "lib/psx/shpsubs.h"

struct TrackHeader
{
    sint32 type;
    sint32 version;
    sint32 maxMetaChunkSize;
    sint32 maxGeomCollSize;
    sint32 maxFullSize;
    sint32 maxSplitSize;
    sint32 metaChunkCount;
    sint32 chunkCount;
};

typedef void Trk_Chunk;

typedef Trk_Chunk *lpTrk_Chunk;

struct shapetbl;

struct Track::tMaterialController
{
    sint16 type;
    uint8 textureMax;
    char pad;
    uint8 uv0;
    uint8 uv1;
    uint8 uv2;
    uint8 uv3;
    char textureCount;
    uint8 interval;
    sint16 pmxIndex;
    Track::tMaterial *matPtr;
};

struct tSaveSurface
{
    Trk::NewSimQuad *fSimQuad;
    uint8 fSurface;
};

static Track::MultiPalette *gTempMultiPalInfo;
static Track::MipMap *gTempMipMapInfo;
sint32 gtrackNumber;
Track::tArtresource gInitialArt;
Group *gPersistObjDef;
Group *gPersistObjInst;
Group *gPersistMidgroundObjInst;
Group *gPersistObjDefBoundingSpheres;
Group *gObjDefOffsetsGroup;
sint32 Track::gControllerCount;
sint32 Chunk::numLight;
uint8 *Track::gInViewCount;
TrackHeader *Track::header;
Chunk *Track::chunkList;
SaveSurface *Track::gSaveSurface;
Track::tMaterial *Track::materials;
SimpleMem *Track::mem;
Track::tMaterialController *Track::gMatController;
Draw::tPixMap Track::gReflectionMaps[4];

void AllocArtResource(Track::tArtresource *artRes, sint32 numPmx)
{
    FUNCTION_MARKER(0x800B9398, "NFS4.EXE");
    artRes->id = -1;
    artRes->basePmxCount = 0;
    artRes->pPmx = reinterpret_cast<Draw::tPixMap *>(BWAllocMem(((uint32)numPmx << 4)));
    artRes->shapeCount = 0;
    artRes->shapeFile = 0;
}

void InitArtResources(void)
{
    FUNCTION_MARKER(0x800B93D8, "NFS4.EXE");
    AllocArtResource(&gInitialArt, 0x352);
    Track::materials = reinterpret_cast<Track::tMaterial *>(BWAllocMem(0xd48));
}

void DeInitArtResources(void)
{
    FUNCTION_MARKER(0x800B9408, "NFS4.EXE");
    gInitialArt.pPmx = 0;
}

void LoadShapesAndMakePmx(char *shapefile, Draw::tPixMap *pmxList, sint32 loadFlags, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x800B942C, "NFS4.EXE");
    sint32 cluttype = -1;
    gTempMipMapInfo = 0;
    if (TrackSpec::gSpec.fogstate != 0)
    {
        cluttype = -3;
        gTempMipMapInfo = static_cast<Track::MipMap *>(reservememadr("mipmapinf", 32 * sizeof(Track::MipMap), 0x10));
        for (sint32 cnt = 0; cnt < 32; ++cnt)
        {
            gTempMipMapInfo[cnt].code[0] = 0xFFFF;
            gTempMipMapInfo[cnt].code[1] = 0xFFFF;
        }
    }
    gTempMultiPalInfo = static_cast<Track::MultiPalette *>(reservememadr("mpalinfo", 128 * sizeof(Track::MultiPalette), 0x10));
    for (sint32 pcnt = 127; pcnt >= 0; --pcnt)
    {
        gTempMultiPalInfo[pcnt].palnum = -1;
        gTempMultiPalInfo[pcnt].origshapeindex = -1;
    }

    DrawSync(0);
    sint32 mipmapcounter = 0;
    sint32 paletteCount = 0;
    Draw::tPixMap *pPmx = pmxList;
    for (sint32 i = 0; i < (sint32)shapecount(shapefile); ++i)
    {
        char *shape = static_cast<char *>(shapepointer(shapefile, i));
        if (shape == 0)
            continue;

        char name[4];
        shapename(shapefile, i, name);
        sint32 tempclut = cluttype;
        if (i < 12 || (shape[0] & 3) == 1 || name[0] == '#')
            tempclut = -1;
        pPmx->pad2 = 0;
        Texture::LoadPmx(0, shape, loadFlags | 2, x, y, tempclut, tempclut, pPmx);

        if (name[0] == '!')
        {
            Track::MultiPalette &palette = gTempMultiPalInfo[paletteCount++];
            palette.charcode[0] = (uint8)name[1];
            palette.charcode[1] = (uint8)name[2];
            sint32 palnum = name[3] >= '0' && name[3] <= '9' ? name[3] - '0' : 0;
            palette.palnum = (sint16)palnum;
            palette.actualshapeindex = (sint16)i;
            if (palnum == 0)
                palette.origshapeindex = (sint16)i;
            else
            {
                for (sint32 j = 0; j < 128; ++j)
                {
                    const Track::MultiPalette &base = gTempMultiPalInfo[j];
                    if (base.palnum == 0 && base.charcode[0] == palette.charcode[0] && base.charcode[1] == palette.charcode[1])
                    {
                        palette.origshapeindex = base.origshapeindex;
                        break;
                    }
                }
            }
        }
        if (TrackSpec::gSpec.fogstate != 0 && name[0] == 'Z')
        {
            uint16 icode[2] = { (uint8)name[2], (uint8)name[3] };
            if (name[1] == 'R' && (shape[0] & 3) == 1)
            {
                Track::MipMap &mipmap = gTempMipMapInfo[mipmapcounter++];
                mipmap.code[0] = icode[0];
                mipmap.code[1] = icode[1];
                mipmap.shapeParentIndex = i;
                mipmap.mipMapIndex = i;
            }
            else if (name[1] == 'Z')
            {
                for (sint32 j = 0; j < 32; ++j)
                {
                    Track::MipMap &mipmap = gTempMipMapInfo[j];
                    if (mipmap.code[0] == icode[0] && mipmap.code[1] == icode[1])
                    {
                        mipmap.mipMapIndex = i;
                        break;
                    }
                }
            }
        }
        ++pPmx;
    }
}

void LoadShapesAndMakePmx_EnvMap(char *shapefile, Draw::tPixMap *pmxList, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x800B9778, "NFS4.EXE");
    Draw::tPixMap *pPmx = pmxList;
    for (sint32 i = 0; i < (sint32)shapecount(shapefile); ++i) {
        char *shape = static_cast<char *>(shapepointer(shapefile, i));
        if (shape != 0) {
            Texture::LoadPmx(0, shape, 0x42, x, y, -1, -1, pPmx);
            ++pPmx;
        }
    }
}

void Track::SetTrackNumber(sint32 tracknum)
{
    FUNCTION_MARKER(0x800B92F8, "NFS4.EXE");
    gtrackNumber = tracknum & 15;
}

char * Track::MakeTrackPathName(char *ext)
{
    FUNCTION_MARKER(0x800B9308, "NFS4.EXE");
    static char strspc[64];
    sprintf(strspc, "%sTr%02d%s", Paths_Paths[6], gtrackNumber, ext);
    return strspc;
}

void TexturesLoadInitial(void)
{
    FUNCTION_MARKER(0x800B9830, "NFS4.EXE");
    static char day[] = "0.psh";
    static char wetDay[] = "W0.psh";
    static char night[] = "N0.psh";
    static char wetNight[] = "S0.psh";
    static char reflection[] = "r.psh";
    char *ext;
    if (GameSetup::gData.Time == 0)
        ext = GameSetup::gData.Weather == 0 ? day : wetDay;
    else
        ext = GameSetup::gData.Weather == 0 ? night : wetNight;
    gInitialArt.shapeFile = static_cast<char *>(loadshapeadr(Track::MakeTrackPathName(ext), 0));
    if (gInitialArt.shapeFile == 0)
        for (;;) {}
    Texture::ResetPaletteSharing();
    gInitialArt.shapeCount = shapecount(gInitialArt.shapeFile);
    LoadShapesAndMakePmx(gInitialArt.shapeFile, gInitialArt.pPmx, 0x40, 0x100, 0);
    Draw::tPixMap spike = *gSpikeBeltPixmap;
    gInitialArt.pPmx[gInitialArt.shapeCount] = spike;
    gInitialArt.shapeCount = (gInitialArt.shapeCount + 1);
    gInitialArt.pmxCount = gInitialArt.shapeCount;
    gInitialArt.basePmxCount = gInitialArt.shapeCount;
    purgememadr(gInitialArt.shapeFile);
    Hrz::GetHorizonPixMap(gInitialArt.pPmx);
    char *tmpShapes = static_cast<char *>(loadshapeadr(Track::MakeTrackPathName(reflection), 0));
    if (tmpShapes != 0) {
        Texture::ResetPaletteSharing();
        LoadShapesAndMakePmx_EnvMap(tmpShapes, Track::gReflectionMaps, 0x3e0, 0);
        purgememadr(tmpShapes);
    }
}

void ReduceObjectPrecision(Group *instGroup, Group *defGroup, sint32 bits)
{
    FUNCTION_MARKER(0x800BA25C, "NFS4.EXE");
    if (instGroup == 0)
        return;
    Trk::SimpleInst *inst = reinterpret_cast<Trk::SimpleInst *>(instGroup + 1);
    sint32 count = instGroup->m_num_elements;
    Trk::ObjectDef **objDefs = Track::gObjDefs;
    while ((count = (count - 1)) != -1) {
        if (defGroup != 0) {
            Trk::ObjectDef *objDef = objDefs[inst->pad];
            CCOORD16 *pts = reinterpret_cast<CCOORD16 *>(objDef + 1);
            sint32 count = objDef->vertexCount;
            while (count-- != 0) {
                pts->x = (sint16)(pts->x >> (bits & 31));
                pts->y = (sint16)(pts->y >> (bits & 31));
                pts->z = (sint16)(pts->z >> (bits & 31));
                ++pts;
            }
        }
        inst = reinterpret_cast<Trk::SimpleInst *>(reinterpret_cast<uint8 *>(inst) + inst->size);
    }
}

void InvalidatePersistentCollideBoomObjects(Group *instGroup, Group *defGroup)
{
    FUNCTION_MARKER(0x800BA2FC, "NFS4.EXE");
    if (instGroup == 0 || defGroup == 0)
        return;
    sint32 count = instGroup->m_num_elements;
    Trk::SimpleInst *inst = reinterpret_cast<Trk::SimpleInst *>(instGroup + 1);
    while ((count = (count - 1)) != -1) {
        if (Track::gObjDefs[inst->pad]->id != -1)
            inst->type = 0;
        inst = reinterpret_cast<Trk::SimpleInst *>(reinterpret_cast<uint8 *>(inst) + inst->size);
    }
}

void CalcObjectBoundingSphere(Group *defGroup, Group *boundingSphereGroup)
{
    FUNCTION_MARKER(0x800BA368, "NFS4.EXE");
    tBoundingSphere *bSphere = reinterpret_cast<tBoundingSphere *>(boundingSphereGroup + 1);
    sint32 objCount = defGroup->m_num_elements;
    for (sint32 i = 0; i < objCount; ++i, ++bSphere) {
        Trk::ObjectDef *objDef = Track::gObjDefs[i];
        CCOORD16 *pts = reinterpret_cast<CCOORD16 *>(objDef + 1);
        coorddef cp = { 0, 0, 0 };
        sint32 ptCount = objDef->vertexCount;
        for (sint32 j = 0; j < ptCount; ++j) {
            cp.x += pts[j].x;
            cp.y += pts[j].y;
            cp.z += pts[j].z;
        }
        if (ptCount == 0)
            abort();
        cp.x /= ptCount;
        cp.y /= ptCount;
        cp.z /= ptCount;
        sint32 radius = 0;
        for (sint32 j = 0; j < ptCount; ++j) {
            sint32 dx = (cp.x - pts[j].x) >> 6;
            sint32 dy = (cp.y - pts[j].y) >> 6;
            sint32 dz = (cp.z - pts[j].z) >> 6;
            sint32 diff = dx * dx + dy * dy + dz * dz;
            if (radius < diff)
                radius = diff;
        }
        bSphere->cp.x = (sint16)cp.x;
        bSphere->cp.y = (sint16)cp.y;
        bSphere->cp.z = (sint16)cp.z;
        bSphere->radius = (sint16)((uint32)fixedsqrt(((uint32)radius << 10)) >> 7);
        boundingSphereGroup->m_num_elements = (boundingSphereGroup->m_num_elements + 1);
    }
}

void CalcObjDefPtrs(void)
{
    FUNCTION_MARKER(0x800BA5C8, "NFS4.EXE");
    Group *defs = gPersistObjDef;
    Trk::ObjectDef **objDefs = static_cast<Trk::ObjectDef **>(reservememadr("objDefs", (sint32)((uint32)defs->m_num_elements * sizeof(*objDefs)), 0));
    if (objDefs == 0)
        abort();
    const uint8 *offsets = reinterpret_cast<const uint8 *>(gObjDefOffsetsGroup + 1);
    objDefs[0] = reinterpret_cast<Trk::ObjectDef *>(defs + 1);
    for (sint32 i = 1; i < defs->m_num_elements; ++i) {
        sint32 offset;
        memcpy(&offset, offsets + i * sizeof(offset), sizeof(offset));
        objDefs[i] = reinterpret_cast<Trk::ObjectDef *>(reinterpret_cast<uint8 *>(objDefs[i - 1]) + offset);
    }
    Track::gObjDefs = objDefs;
}

char * Track::MakeTrackDataPathName(char *ext)
{
    FUNCTION_MARKER(0x800B9350, "NFS4.EXE");
    static char strspc[64];
    sprintf(strspc, "%sTr%02d%s", Paths_Paths[8], gtrackNumber, ext);
    return strspc;
}

void Track::AnimateTextures(void)
{
    FUNCTION_MARKER(0x800B99DC, "NFS4.EXE");
    tMaterialController *controlPtr = gMatController;
    sint32 controlCount = gControllerCount;
    while ((controlCount = (controlCount - 1)) != -1) {
        if (controlPtr->type == 0x80) {
            sint32 interval = controlPtr->interval;
            if (interval == 0)
                abort();
            sint32 frame = simGlobal.gameTicks / interval;
            sint32 textureMax = controlPtr->textureMax;
            if (textureMax == 0)
                abort();
            sint32 d = frame % textureMax;
            Draw::tPixMap *pmx = &gInitialArt.pPmx[controlPtr->matPtr->pmxIndex];
            pmx->v0 = controlPtr->uv0 + d;
            pmx->v1 = controlPtr->uv1 + d;
            pmx->v2 = controlPtr->uv2 + d;
            pmx->v3 = controlPtr->uv3 + d;
        } else if (controlPtr->type == 4) {
            sint32 interval = controlPtr->interval;
            if (interval == 0)
                abort();
            sint32 frame = simGlobal.gameTicks / interval;
            sint32 textureCount = (uint8)controlPtr->textureCount;
            if (textureCount == 0)
                abort();
            controlPtr->matPtr->pmxIndex = (sint16)((uint16)controlPtr->pmxIndex + frame % textureCount);
        }
        ++controlPtr;
    }
}

sint32 Track::GetProperMultiPalShapeIndex(sint32 shapeindex, sint32 paletteindex)
{
    FUNCTION_MARKER(0x800B9B74, "NFS4.EXE");
    MultiPalette *info = gTempMultiPalInfo;
    for (sint32 t = 0; t < 128; ++t, ++info)
        if (info->origshapeindex == shapeindex && info->palnum == paletteindex)
            return info->actualshapeindex;
    return shapeindex;
}

void Track::ProcessFlipAndUVFlags(sint32 uvFlag, Draw::tPixMap *inputPmx, Draw::tPixMap *outputPmx)
{
    FUNCTION_MARKER(0x800B9BC4, "NFS4.EXE");
    Draw::tPixMap newPmx[4];
    sint32 pmxProcessCount = 0;
    newPmx[0] = *inputPmx;
    if (uvFlag & 0x10) {
        Texture::CloneUVPmx(&newPmx[0], 0, &newPmx[1]);
        pmxProcessCount = 1;
    }
    if (uvFlag & 0x40) {
        Texture::CloneUVPmx(&newPmx[pmxProcessCount], 1, &newPmx[pmxProcessCount + 1]);
        ++pmxProcessCount;
    }
    if (uvFlag & 0x0e) {
        sint32 mode = (uvFlag & 2) ? 2 : ((uvFlag & 4) ? 3 : 4);
        Texture::CloneUVPmx(&newPmx[pmxProcessCount], mode, &newPmx[pmxProcessCount + 1]);
        ++pmxProcessCount;
    }
    *outputPmx = newPmx[pmxProcessCount];
}

void Track::AssociateSingleMaterial(Trk::Material *inputMat, Track::tMaterial *outputMat, Track::tArtresource *art)
{
    FUNCTION_MARKER(0x800B9D2C, "NFS4.EXE");
    outputMat->flag = inputMat->flag;
    for (sint32 animCount = 0; animCount == 0 || animCount < (uint8)inputMat->textureCount; ++animCount) {
        if (inputMat->uvFlag & 0x5e) {
            sint32 shapeIndex = inputMat->shapeIndex;
            if (inputMat->flag & 2)
                shapeIndex = GetProperMultiPalShapeIndex(shapeIndex, inputMat->interval);
            Draw::tPixMap originalPmx = art->pPmx[shapeIndex + animCount];
            ProcessFlipAndUVFlags(inputMat->uvFlag, &originalPmx, &art->pPmx[art->pmxCount]);
            if (animCount == 0)
                outputMat->pmxIndex = (sint16)art->pmxCount;
            art->pmxCount = (art->pmxCount + 1);
        } else {
            sint32 shapeIndex = inputMat->shapeIndex;
            if (inputMat->flag & 2)
                shapeIndex = GetProperMultiPalShapeIndex(shapeIndex, inputMat->interval);
            if (animCount == 0)
                outputMat->pmxIndex = (sint16)shapeIndex;
        }
    }
}

void Track::LinkMaterials(SerializedGroup *group, sint32 length, Track::tMaterial *matList)
{
    FUNCTION_MARKER(0x800B9E98, "NFS4.EXE");
    if (group == 0)
        return;
    Trk::Material *mats = reinterpret_cast<Trk::Material *>(group + 1);
    gControllerCount = 0;
    gInitialArt.pmxCount = gInitialArt.basePmxCount;
    sint32 matCount = (uint32)length / 10;
    for (sint32 i = 0; i < matCount; ++i)
        if (mats[i].flag & 0x84)
            ++gControllerCount;
    if (gControllerCount != 0)
        gMatController = reinterpret_cast<tMaterialController *>(BWAllocMem(((uint32)gControllerCount << 4)));
    sint32 controlIndex = 0;
    for (sint32 i = 0; i < matCount; ++i, ++mats, ++matList) {
        AssociateSingleMaterial(mats, matList, &gInitialArt);
        if (mats->flag & 4) {
            gMatController[controlIndex].type = mats->flag;
            gMatController[controlIndex].interval = mats->interval;
            gMatController[controlIndex].textureCount = mats->textureCount;
            gMatController[controlIndex].pmxIndex = matList->pmxIndex;
            gMatController[controlIndex].matPtr = matList;
            ++controlIndex;
        } else if (mats->flag & 0x80) {
            gMatController[controlIndex].type = mats->flag;
            if (mats->interval == 0)
                mats->interval = 1;
            gMatController[controlIndex].interval = mats->interval;
            gMatController[controlIndex].textureCount = mats->textureCount;
            gMatController[controlIndex].pmxIndex = matList->pmxIndex;
            gMatController[controlIndex].matPtr = matList;
            Draw::tPixMap *pmx = &gInitialArt.pPmx[matList->pmxIndex];
            gMatController[controlIndex].uv0 = pmx->v0;
            gMatController[controlIndex].uv1 = pmx->v1;
            gMatController[controlIndex].uv2 = pmx->v2;
            gMatController[controlIndex].uv3 = pmx->v3;
            sint32 d = pmx->v3 - pmx->v0;
            if (d < 0)
                d = -d;
            gMatController[controlIndex].textureMax = d + 1;
            ++controlIndex;
        }
        if (TrackSpec::gSpec.fogstate != 0) {
            for (sint32 mm = 0; mm < 32; ++mm) {
                MipMap *info = &gTempMipMapInfo[mm];
                if ((info->code[0] != 0xffff || info->code[1] != 0xffff) && info->shapeParentIndex == mats->shapeIndex) {
                    sint32 mipmap_offset = (info->mipMapIndex - info->shapeParentIndex);
                    matList->flag |= 8;
                    if (mats->shapeIndex == matList->pmxIndex) {
                        matList->mipmap_offset = (char)mipmap_offset;
                    } else {
                        sint32 shapeIndex = (mats->shapeIndex + mipmap_offset);
                        Draw::tPixMap originalPmx = gInitialArt.pPmx[shapeIndex];
                        ProcessFlipAndUVFlags(mats->uvFlag, &originalPmx, &gInitialArt.pPmx[gInitialArt.pmxCount]);
                        gInitialArt.pmxCount = (gInitialArt.pmxCount + 1);
                        matList->mipmap_offset = 1;
                    }
                }
            }
        }
    }
    if (gTempMipMapInfo != 0)
        purgememadr(gTempMipMapInfo);
    purgememadr(gTempMultiPalInfo);
    gInitialArt.shapeFile = 0;
    gInitialArt.pmxCount = gInitialArt.shapeCount;
}

void Track::InitPersistentData(SerializedGroup *perGroup)
{
    FUNCTION_MARKER(0x800BA62C, "NFS4.EXE");
    gObjDefOffsetsGroup = 0;
    size_t bytes = (size_t)(uint32)perGroup->m_num_elements * sizeof(SerializedGroup *);
    SerializedGroup **persistentGroups = static_cast<SerializedGroup **>(malloc(bytes ? bytes : sizeof(SerializedGroup *)));
    if (persistentGroups == 0)
        abort();
    for (sint32 i = 0; i < perGroup->m_num_elements; ++i)
        persistentGroups[i] = perGroup->LocateGroupNum(i);
    perGroup->LocateGroupType(8, 0);
    sint32 count = perGroup->m_num_elements;
    for (sint32 i = 0; i < count; ++i) {
        SerializedGroup *group = persistentGroups[i];
        switch (group->m_type) {
        case 2:
            LinkMaterials(group, (group->m_length - 16), materials);
            break;
        case 7:
            gPersistObjInst = group->CreateLiteGroup(group, mem);
            break;
        case 8:
            gPersistObjDef = group->CreateLiteGroup(group, mem);
            break;
        case 0x0f:
            BWorldSm::Init(group->CreateLiteGroup(group, mem));
            break;
        case 0x24:
            gPersistMidgroundObjInst = group->CreateLiteGroup(group, mem);
            break;
        case 0x26:
            gObjDefOffsetsGroup = group->CreateLiteGroup(group, mem);
            break;
        }
    }
    free(persistentGroups);
    if (gObjDefOffsetsGroup != 0)
        CalcObjDefPtrs();
}

void Track::Init(char *tempName)
{
    FUNCTION_MARKER(0x800BA808, "NFS4.EXE");
    gSaveSurface = 0;
    gObjDefs = 0;
    char trackName[128];
    sprintf(trackName, "%s", tempName);
    sint32 trackFileSize = filesize(tempName);
    Chunk::lightTable = static_cast<CVECTOR *>(reservememadr("lighttbl", 0x404, 0));
    TextureProcess::Init();
    void *trackData = malloc(trackFileSize);
    if (trackData == 0)
        abort();
    SerializedGroup *trackGroup = static_cast<SerializedGroup *>(loadfileatadr(trackName, trackData));
    SerializedGroup *sourceHeaderGroup = trackGroup->LocateGroupType(0x1f, 0);
    TrackHeader *sourceHeader = reinterpret_cast<TrackHeader *>(sourceHeaderGroup + 1);
    sint32 nativeChunkExtra = (sint32)(sizeof(Chunk) - 112);
    sint32 size = (sint32)((uint32)trackFileSize + 0x9080 + sourceHeader->chunkCount * (uint32)nativeChunkExtra + sizeof(void *));
    InitArtResources();
    TexturesLoadInitial();
    SimpleMem *pool = new (reservememadr(0, sizeof(SimpleMem), 0)) SimpleMem;
    pool->heap = reservememadr("Track_mem", size, 0);
    pool->freeMem = pool->heap;
    if (pool->heap == 0)
        size = 0;
    pool->freeMemSize = size;
    mem = pool;
    Group *headerGroup = trackGroup->LocateCreateGroupType(0x1f, mem, 0);
    header = reinterpret_cast<TrackHeader *>(headerGroup + 1);
    Group *centerGroup = trackGroup->LocateCreateGroupType(0x20, mem, 0);
    Chunk::chunkCenters = reinterpret_cast<coorddef *>(centerGroup + 1);
    SerializedGroup *lightGroup = trackGroup->LocateGroupType(0x23, 0);
    memcpy(Chunk::lightTable, lightGroup + 1, 0x400);
    lightGroup = trackGroup->LocateGroupType(0x23, 0);
    Chunk::numLight = ((uint32)lightGroup->m_length - 16) >> 2;
    gInViewList = static_cast<sint16 (*)[32]>(mem->Alloc((header->chunkCount * 72), 0));
    gInViewCount = static_cast<uint8 *>(mem->Alloc(header->chunkCount, 0));
    mem->Alloc((sint32)(((~(reinterpret_cast<uintptr_t>(mem->freeMem)) + 1)) & (sizeof(void *) - 1)), 0);
    chunkList = static_cast<Chunk *>(mem->Alloc((sint32)((uint32)header->chunkCount * sizeof(*chunkList)), 0));
    SerializedGroup *persistentGroup = trackGroup->LocateGroupType(0x21, 0);
    SerializedGroup *chunkGroup = trackGroup->LocateGroupType(0x1d, 0);
    for (sint32 i = 0; i < header->chunkCount; ++i) {
        Chunk *chunkDat = &chunkList[i];
        SerializedGroup *nextChunkGroup = 0;
        if (i < (header->chunkCount - 1))
            nextChunkGroup = chunkGroup->LocateNextGroupType(0x1d);
        chunkDat->InstanceGroup(chunkGroup, mem);
        SerializedGroup *group = chunkGroup->LocateGroupType(4, 0);
        sint32 count = group->m_num_elements;
        const uint16 *src = reinterpret_cast<const uint16 *>(group + 1);
        if (count > 36)
            count = 36;
        sint16 *visList = reinterpret_cast<sint16 *>(gInViewList) + i * 32;
        sint32 j = 0;
        for (sint32 srcDataInd = 0; srcDataInd < count; ++srcDataInd) {
            uint16 value = src[srcDataInd];
            if ((value & 0x3ff) < header->chunkCount)
                visList[j++] = value;
        }
        gInViewCount[i] = j;
        while (j < 36)
            visList[j++] = 0x3ff;
        chunkGroup = nextChunkGroup;
    }
    InitPersistentData(persistentGroup);
    free(trackData);
    mem->ResizeToFit();
    static char groupExt[] = ".grp";
    MakeTrackPathName(groupExt);
    gPersistObjDefBoundingSpheres = static_cast<Group *>(reservememadr("bsphere", (((uint32)gPersistObjDef->m_num_elements << 3) | 4), 0));
    CalcObjectBoundingSphere(gPersistObjDef, gPersistObjDefBoundingSpheres);
    ReduceObjectPrecision(gPersistMidgroundObjInst, gPersistObjDef, 2);
    InvalidatePersistentCollideBoomObjects(gPersistObjInst, gPersistObjDef);
    gSaveSurface = ::new (reservememadr(0, sizeof(SaveSurface), 0)) SaveSurface(0x30);
    LoadObjectKillData();
}

void Track::DeInit(void)
{
    FUNCTION_MARKER(0x800BAC74, "NFS4.EXE");
    SimpleMem *pool = mem;
    if (pool != 0) {
        purgememadr(pool->heap);
        pool->freeMem = 0;
        purgememadr(pool);
    }
    if (gSaveSurface != 0) {
        delete gSaveSurface;
        gSaveSurface = 0;
    }
    if (gInitialArt.shapeFile != 0)
        gInitialArt.shapeFile = 0;
    BWorldSm::DeInit();
    DeInitArtResources();
    if (Chunk::lightTable != 0) {
        purgememadr(Chunk::lightTable);
        Chunk::lightTable = 0;
    }
    if (gPersistObjDefBoundingSpheres != 0)
        purgememadr(gPersistObjDefBoundingSpheres);
    gPersistObjDefBoundingSpheres = 0;
    gPersistMidgroundObjInst = 0;
    gPersistObjInst = 0;
    gPersistObjDef = 0;
    purgememadr(gObjDefs);
    gObjDefs = 0;
    gObjDefOffsetsGroup = 0;
}

void Track::AnimateTrackLighting(void)
{
    FUNCTION_MARKER(0x800BAD48, "NFS4.EXE");
}

char * KillFile::OpenRead(void)
{
    FUNCTION_MARKER(0x800BAD50, "NFS4.EXE");
    static char ext[] = ".kil";
    char pathName[128];
    sprintf(pathName, "%s", Track::MakeTrackPathName(ext));
    return static_cast<char *>(loadfileadrz(pathName, 0));
}

void KillFile::ReadEntry(char *filePtr, sint32 entryInd, sint32 &chunkInd, sint32 &objectInd)
{
    FUNCTION_MARKER(0x800BAD94, "NFS4.EXE");
    char *entry = filePtr + (sint32)((uint32)entryInd << 3);
    sint32 value;
    memcpy(&value, entry + 4, sizeof(value));
    chunkInd = value;
    memcpy(&value, entry + 8, sizeof(value));
    objectInd = value;
}

void Track::LoadObjectKillData(void)
{
    FUNCTION_MARKER(0x800BADB4, "NFS4.EXE");
    char *filePtr = KillFile::OpenRead();
    if (filePtr == 0)
        return;
    for (sint32 i = 0; ; ++i) {
        sint32 entryCount;
        memcpy(&entryCount, filePtr, sizeof(entryCount));
        if (i >= entryCount)
            break;
        sint32 chunkInd, objInd;
        KillFile::ReadEntry(filePtr, i, chunkInd, objInd);
        Chunk *chunkDat = &chunkList[chunkInd];
        Group *group = chunkDat->objInstanceBuf;
        if (group != 0 && objInd < group->m_num_elements) {
            Trk::SimpleInst *inst = reinterpret_cast<Trk::SimpleInst *>(group + 1);
            for (sint32 index = 0; index < objInd; ++index)
                inst = reinterpret_cast<Trk::SimpleInst *>(reinterpret_cast<uint8 *>(inst) + inst->size);
            Group *simGroup = chunkDat->simObjBuf;
            if (simGroup != 0) {
                sint32 numElements = simGroup->m_num_elements;
                Trk::SimObject *simObjs = reinterpret_cast<Trk::SimObject *>(simGroup + 1);
                for (sint32 j = 0; j < numElements; ++j) {
                    coorddef simPos = { simObjs[j].point[0], simObjs[j].point[1], simObjs[j].point[2] };
                    coorddef instPos = { inst->x, inst->y, inst->z };
                    if (Math::DistXZ(&simPos, &instPos) < 0x1999)
                        simObjs[j].type = 0x10;
                }
            }
            inst->type |= 0x80;
        }
    }
    purgememadr(filePtr);
}

void SaveSurface::Save(Trk::NewSimQuad *simQuad)
{
    FUNCTION_MARKER(0x800BAF0C, "NFS4.EXE");
    if (fCount <= fMaxCount && simQuad != 0) {
        fStack[fCount].fSimQuad = simQuad;
        fStack[fCount].fSurface = simQuad->surface;
        fCount = (sint16)((uint16)fCount + 1);
    }
}

SaveSurface::SaveSurface(sint32 numEntries)
{
    FUNCTION_MARKER(0x800BAF70, "NFS4.EXE");
    fMaxCount = (sint16)numEntries;
    fCount = 0;
    fStack = static_cast<tSaveSurface *>(reservememadr("Surface Save", ((uint32)numEntries << 3), 0));
}

void SaveSurface::operator delete(void *memory)
{
    purgememadr(memory);
}

SaveSurface::~SaveSurface(void)
{
    FUNCTION_MARKER(0x800BAFB4, "NFS4.EXE");
    purgememadr(fStack);
}

void SaveSurface::RestoreAll(void)
{
    FUNCTION_MARKER(0x800BAFFC, "NFS4.EXE");
    for (sint32 i = 0; i < fCount; ++i) {
        fStack[i].fSimQuad->surface = fStack[i].fSurface;
        fStack[i].fSimQuad = 0;
        fStack[i].fSurface = 0;
    }
    fCount = 0;
}

sint16 (*Track::gInViewList)[32];
Trk::ObjectDef **Track::gObjDefs;
