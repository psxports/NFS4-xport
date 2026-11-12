#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/anim.h"
#include "game/common/audedit.h"
#include "game/common/bworld.h"
#include "game/common/bworldsm.h"
#include "game/common/cameraobj.h"
#include "game/common/car.h"
#include "game/common/cars.h"
#include "game/common/chunk.h"
#include "game/common/gmesetup.h"
#include "game/common/group.h"
#include "game/common/object.h"
#include "game/common/renderview.h"
#include "game/common/replay.h"
#include "game/common/scene.h"
#include "game/common/track.h"
#include "game/common/trgsfx.h"
#include "game/common/trk.h"
#include "game/psx/draww.h"
#include "game/psx/flare.h"
#include "game/psx/hrzsku.h"
#include "game/psx/loading.h"
#include "game/psx/night.h"
#include "game/psx/platform.h"
#include "game/psx/rendercache.h"
#include "game/psx/trackspec.h"
#include "game/psx/trsproj.h"

struct BW::tContextMgr
{
    sint32 initialized;
    sint32 count;
    BW::tContext contexts[2];
};

struct tBuildEntry
{
    sint16 chunkInd;
    char geomRez;
    char enableBits;
};

sint32 gSpikeBelt;
sint32 gSpikeBeltSlice;
sint32 gSpikeBeltChunk;
sint32 gSpikeBeltX;
sint32 gBWSlice;
sint32 gSpikeBeltWidth;
sint32 gSpikeBeltColourTableIndex;
sint32 SceneLoaded;
matrixtdef gWorldMat;
matrixtdef gNightMat;
matrixtdef gCopMat;
BW::tContextMgr gContextMan;
BW::tContext *gCurrContext;
DRender::tView *gVi2;
sint32 BWorld::gChunkCount;
Car::tObj *BW::gCopCarObj;
tBuildEntry BWorld::gChunkBuildList[36];

void SetContext(sint32 contextHandle);

sint32 xzsquaredist32(coorddef *c1, coorddef *c2)
{
    FUNCTION_MARKER(0x8007D5C4, "NFS4.EXE");
    sint32 x = (c2->x - c1->x) >> 12;
    sint32 z = (c2->z - c1->z) >> 12;
    sint32 xSquared = (x * x);
    sint32 zSquared = (z * z);
    return (((xSquared >> 6)) + (zSquared >> 6));
}

void UpdateContext(DRender::tView *Vi, sint32 contextHandle)
{
    FUNCTION_MARKER(0x8007D608, "NFS4.EXE");
    SetContext(contextHandle);
    coorddef *pt = &Vi->cview.translation;
    if (contextHandle == 0)
        gBWSlice = gCurrContext->slicePos.slice;
    BWorldSm::FindClosestQuadRez(pt, &gCurrContext->slicePos, 1);
    if (gCurrContext->slicePos.offEdge != 0) {
        FindAbsClosestSliceCrude(pt, &gCurrContext->slicePos);
        BWorldSm::FindClosestQuadRez(pt, &gCurrContext->slicePos, 1);
    }
    BW::tContext *context = gCurrContext;
    sint32 chunk = context->slicePos.chunk;
    if (context->currentChunk != chunk)
        context->currentChunk = chunk;
}

void SetupBuildMatrices(DRender::tView *Vi, Draw::DCache *sd)
{
    FUNCTION_MARKER(0x8007D940, "NFS4.EXE");

    sint32 contextHandle = Vi->id == Draw::gPlayer1View || Vi->id == Draw::gPlayer2View ? Vi->player : 1;
    UpdateContext(Vi, contextHandle);
    for (sint32 column = 0; column < 3; ++column)
    {
        sint32 t1 = Vi->cview.mrotationInv.m[column * 3];
        sint32 t2 = Vi->cview.mrotationInv.m[column * 3 + 1];
        sint32 t3 = Vi->cview.mrotationInv.m[column * 3 + 2];
        gWorldMat.m[column * 3] = t1;
        gWorldMat.m[column * 3 + 1] = (-t2);
        gWorldMat.m[column * 3 + 2] = t3;
    }
    if (gNight_renderNight == 0)
        return;
    transpose(&Camera::gInfo[Vi->player].target->orientMat, &gNightMat);
    gNightMat.m[1] = (-gNightMat.m[1]);
    gNightMat.m[7] = (-gNightMat.m[7]);
    gNightMat.m[4] = (-gNightMat.m[4]);
    DrawW::WorldSetUpMatrix(&gNightMat, &sd->matNight);
    BW::gCopCarObj = 0;
    if (GameSetup::gData.commMode == 1)
        return;
    sint32 maxdist = 0xB40000;
    sint32 numCops = Cars::gNumCopCars;
    for (sint32 i = -1; i < numCops; ++i)
    {
        Car::tObj *theCar = i < 0 ? Cars::gList[0] : Cars::gCopCarList[i];
        if (theCar->N.active == 0 || (theCar->AIFlags & 2) == 0 || (theCar->render.damageParts & 4) != 0)
            continue;
        sint32 dx = (theCar->N.position.x - Vi->cview.translation.x);
        sint32 dz = (theCar->N.position.z - Vi->cview.translation.z);
        if (dx < 0)
            dx = (-dx);
        if (dz < 0)
            dz = (-dz);
        sint32 diff = dz < dx ? (dx + (dz >> 2)) : (dz + (dx >> 2));
        if (diff < maxdist)
        {
            maxdist = diff;
            BW::gCopCarObj = theCar;
        }
    }
    if (BW::gCopCarObj != 0)
    {
        Night::SetCopColor(BW::gCopCarObj->carInfo);
        static sint32 cop_angle = 0;
        cop_angle = (cop_angle + 64);
        matrixtdef rotY;
        xformy(&rotY, cop_angle);
        matrixtdef tm;
        transpose(&BW::gCopCarObj->N.orientMat, &tm);
        Math::fasttransmult(&tm, &rotY, &gCopMat);
        gCopMat.m[1] = (-gCopMat.m[1]);
        gCopMat.m[7] = (-gCopMat.m[7]);
        gCopMat.m[4] = (-gCopMat.m[4]);
        DrawW::WorldSetUpMatrix(&gCopMat, &sd->matCop);
    }
}

sint32 GetRezIndex(sint32 dist)
{
    FUNCTION_MARKER(0x8007DD54, "NFS4.EXE");
    return dist <= gCurrContext->lowDetailDistSq ? 4 : 0;
}

sint32 SetupChunkBuildList(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x8007DD74, "NFS4.EXE");

    tBuildEntry *entry = BWorld::gChunkBuildList;
    sint32 chunkCount = 0;
    sint32 currentChunk = gCurrContext->currentChunk;
    sint16 *view = reinterpret_cast<sint16 *>(Track::gInViewList) + currentChunk * 32;
    sint32 totalVisChunks = Track::gInViewCount[currentChunk];
    for (sint32 viewInd = 0; viewInd < totalVisChunks; ++viewInd, ++view)
    {
        sint32 chunkInd = (uint16)*view & 0x3FF;
        coorddef *pChunkCp = Chunk::chunkCenters + chunkInd;
        sint32 chunkDist = xzsquaredist32(pChunkCp, &Vi->cview.translation);
        if (chunkDist > gCurrContext->chunkFarZClipSq || ((uint16)*view & 0x800) != 0)
            continue;
        coorddef tmp;
        tmp.x = (pChunkCp->x - Vi->cview.translation.x);
        tmp.y = (BWorldSm::slices[chunkInd * 8].center[1] - Vi->cview.translation.y);
        tmp.z = (pChunkCp->z - Vi->cview.translation.z);
        Chunk *chunkPtr = Track::chunkList + chunkInd;
        TrsProj::SetPsxTransZero();
        coorddef tmp2;
        TrsProj::TransPt(&tmp, &tmp2);
        TrsProj::SetPsxTrans(&tmp2);
        coorddef tmpPts[4];
        TrsProj::TransPtN16(chunkPtr->boundPts, tmpPts, 4);
        if (tmpPts[0].x > tmpPts[0].z && tmpPts[1].x > tmpPts[1].z && tmpPts[2].x > tmpPts[2].z && tmpPts[3].x > tmpPts[3].z)
            continue;
        if (-tmpPts[0].x > tmpPts[0].z && -tmpPts[1].x > tmpPts[1].z && -tmpPts[2].x > tmpPts[2].z && -tmpPts[3].x > tmpPts[3].z)
            continue;
        if (tmpPts[0].z < 0 && tmpPts[1].z < 0 && tmpPts[2].z < 0 && tmpPts[3].z < 0)
            continue;
        entry->enableBits = 3;
        if (chunkDist < gCurrContext->lineFarZClipSq)
            entry->enableBits = 7;
        if (((uint16)*view & 0x4000) != 0)
            entry->enableBits = (char)((uint8)entry->enableBits & 0xFD);
        if (((uint16)*view & 0x2000) != 0)
            entry->enableBits = (char)((uint8)entry->enableBits & 0xFE);
        entry->geomRez = (char)GetRezIndex(chunkDist);
        entry->chunkInd = (sint16)chunkInd;
        ++entry;
        ++chunkCount;
    }
    return chunkCount;
}

char *BWAllocMem(sint32 size)
{
    FUNCTION_MARKER(0x8007E3F8, "NFS4.EXE");
    static sint32 totalMem;
    static char name[] = "bworld";
    totalMem = (totalMem + size);
    return Platform::GetDCTBuffer(size, name);
}

void SetContext(sint32 contextHandle)
{
    FUNCTION_MARKER(0x8007E480, "NFS4.EXE");
    gCurrContext = &gContextMan.contexts[contextHandle];
}

void BWorld::BuildGlareEffects(DRender::tView *Vi, Draw::DCache *sd, Group *group)
{
    FUNCTION_MARKER(0x8007D6C0, "NFS4.EXE");

    Trk::SFX *objInstance = reinterpret_cast<Trk::SFX *>(group + 1);
    sint32 numObjects = group->m_num_elements;
    for (sint32 i = 0; i < numObjects; ++i)
    {
        Trk::SFX *effect = objInstance + i;
        uint16 pad = effect->pad;
        sint16 type = effect->type;
        if (type == 100)
        {
            coorddef dir = { 0, 655360, 0 };
            TrgSfx::AddEnviroEffect(i, 0x101, reinterpret_cast<coorddef *>(effect), &dir);
            return;
        }
        if (pad == 0)
            Flare::Halo(Vi, -1, type, reinterpret_cast<coorddef *>(effect), reinterpret_cast<Draw::FlareCache *>(sd));
        else
        {
            sint32 j;
            for (j = 0; j < numObjects; ++j)
                if ((pad & 0x7FFF) == ((uint16)objInstance[j].pad & 0x7FFF))
                    break;
            if (j < numObjects && (pad & 0x8000) != 0)
                Flare::Halo2(Vi, -1, type, reinterpret_cast<coorddef *>(effect), reinterpret_cast<coorddef *>(objInstance + j), reinterpret_cast<Draw::FlareCache *>(sd));
        }
    }
}

void BWorld::InitSpikeBelt(void)
{
    FUNCTION_MARKER(0x8007D818, "NFS4.EXE");
    sint32 leastDist = 0x7FFFFFFF;
    sint32 leastDistInd = -1;
    sint32 count = Chunk::numLight;
    CVECTOR *light = Chunk::lightTable;
    gSpikeBelt = 0;
    gSpikeBeltSlice = 0;
    gSpikeBeltChunk = 0;
    for (sint32 i = 0; i < count; ++i) {
        sint32 red = 128 - light[i].r;
        sint32 green = 128 - light[i].g;
        sint32 blue = 128 - light[i].b;
        sint32 dist = red * red + green * green + blue * blue;
        if (dist < leastDist) {
            leastDist = dist;
            leastDistInd = i;
        }
    }
    gSpikeBeltColourTableIndex = leastDistInd;
}

void BWorld::SetSpikeBelt(sint32 slice, sint32 x, sint32 width)
{
    FUNCTION_MARKER(0x8007D8C4, "NFS4.EXE");
    gSpikeBelt = 1;
    Trk::NewSlice *slices = BWorldSm::slices;
    gSpikeBeltSlice = slice;
    sint32 chunk = slices[slice].chunkIndex;
    gSpikeBeltWidth = width;
    gSpikeBeltX = x;
    gSpikeBeltChunk = chunk;
}

sint32 BWorld::GetSpikeBelt(sint32 *slice, sint32 *x, sint32 *width)
{
    FUNCTION_MARKER(0x8007D8F8, "NFS4.EXE");
    if (gSpikeBelt != 1) {
        *width = 0;
        *x = 0;
        *slice = 0;
        return 0;
    }
    *slice = gSpikeBeltSlice;
    *x = gSpikeBeltX;
    *width = gSpikeBeltWidth;
    return 1;
}

sint32 BWorld::CheckChunkVisible(BWorldSm::Pos *slicePosSource, BWorldSm::Pos *slicePosTest)
{
    FUNCTION_MARKER(0x8007DC14, "NFS4.EXE");
    if (slicePosSource == slicePosTest)
        return 1;
    sint32 testChunkIndFwd = slicePosTest->slice + 2;
    if (testChunkIndFwd >= gNumSlices)
        testChunkIndFwd = (slicePosTest->slice - (gNumSlices - 2));
    testChunkIndFwd = BWorldSm::slices[testChunkIndFwd].chunkIndex;
    sint32 testChunkIndBwd = slicePosTest->slice - 2;
    if (testChunkIndBwd < 0)
        testChunkIndBwd = ((slicePosTest->slice + gNumSlices) - 2);
    sint32 sourceChunkInd = slicePosSource->chunk;
    sint32 count = Track::gInViewCount[sourceChunkInd] - 1;
    testChunkIndBwd = BWorldSm::slices[testChunkIndBwd].chunkIndex;
    sint32 vis = 0;
    if (count == -1)
        return 0;
    sint16 *chunkViewList = Track::gInViewList[sourceChunkInd];
    for (; count != -1; --count) {
        sint32 chunkInd = chunkViewList[count];
        if ((chunkInd & 0x3FF) == testChunkIndFwd) {
            if (chunkInd & 0x800)
                return vis == 2;
            ++vis;
            if (vis != 1)
                return vis == 2;
        }
        if ((chunkInd & 0x3FF) == testChunkIndBwd) {
            if (chunkInd & 0x800)
                return vis == 2;
            ++vis;
            if (vis != 1)
                return vis == 2;
        }
    }
    return vis == 2;
}

bool BWorld::IsSliceInBuildList(sint32 slice)
{
    FUNCTION_MARKER(0x8007E0A0, "NFS4.EXE");
    sint32 chunk = slice / 8;
    sint32 count = gChunkCount;
    for (sint32 bi = 0; bi < count; ++bi) {
        if (gChunkBuildList[bi].chunkInd == chunk)
            return true;
    }
    return false;
}

void BWorld::OnyxBuildFacets(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x8007E0F4, "NFS4.EXE");

    gVi2 = Vi;
    Draw::DCache *sd = &gNativeRenderCache.horizonDraw;

    stackSpeedUpEnbabledFlag = 1;
    SetupBuildMatrices(gVi2, sd);
    DrawW::WorldSetUpMatrix(&gWorldMat, &sd->matB);
    gChunkCount = SetupChunkBuildList(gVi2);
    stackSpeedUpEnbabledFlag = 0;
    Draw::tGiveShelbyMoreCache *world = &gNativeRenderCache.world;
    world->startfog = (sint16)TrackSpec::gSpec.fogspec.start;
    world->distfog = (sint16)TrackSpec::gSpec.fogspec.dist2base;
    world->fogstate = (uint8)TrackSpec::gSpec.fogstate;
    if (GameSetup::gData.Time != 0)
    {
        world->night_ZNear = (sint16)Night::gZNear;
        world->night_XDistShift = (uint8)Night::gXDistShift;
        world->night_ZDistShift = (uint8)Night::gZDistShift;
        world->night_DrawLightning = (uint8)Night::gDrawLightning;
        world->night_LightningType = (uint8)Night::gLightningType;
    }
    stackSpeedUpEnbabledFlag = 1;
    DrawW::DoTrough(Vi, gChunkBuildList);
    stackSpeedUpEnbabledFlag = 0;
    if (GameSetup::gData.track != 0 || GameSetup::gData.Weather != 1)
    {
        stackSpeedUpEnbabledFlag = 1;
        DrawW::DoLines(Vi, gChunkBuildList, sd);
        stackSpeedUpEnbabledFlag = 0;
    }
    if (gSpikeBelt != 0)
    {
        for (sint32 buildInd = 0; buildInd < gChunkCount; ++buildInd)
            if (gChunkBuildList[buildInd].chunkInd == gSpikeBeltChunk)
                DrawW::BuildSpikeBelt(Vi, gSpikeBeltWidth, sd);
    }
    if (GameSetup::gData.commMode == 0)
    {
        for (sint32 buildInd = 0; buildInd < gChunkCount; ++buildInd)
        {
            sint32 chunkInd = gChunkBuildList[buildInd].chunkInd;
            Group *effects = Track::chunkList[chunkInd].sfxBuf;
            if (effects != 0)
                BuildGlareEffects(Vi, sd, effects);
        }
    }
    if (Object::customSFXInst != 0 && IsSliceInBuildList(Object::customSliceNum))
        BuildGlareEffects(Vi, sd, Object::customSFXInst);
    DrawW::WorldSetUpMatrix(&gWorldMat, &sd->matB);
    DrawW::DoObjects(Vi, gChunkBuildList);
}

void BWorld::InitContexts(void)
{
    FUNCTION_MARKER(0x8007E428, "NFS4.EXE");
    gContextMan.initialized = 1;
    gContextMan.count = 0;
    for (sint32 i = 1; i >= 0; --i)
        gContextMan.contexts[i].client = -1;
}

void BWorld::DeInitContexts(void)
{
    FUNCTION_MARKER(0x8007E460, "NFS4.EXE");
    InitContexts();
}

sint32 BWorld::OpenContext(sint32 contextType, sint32 client)
{
    FUNCTION_MARKER(0x8007E4AC, "NFS4.EXE");
    BW::tContext *context = &gContextMan.contexts[gContextMan.count];
    context->slicePos.slice = 0;
    context->client = client;
    if (contextType == 0) {
        context->polyFarZClipSq = 0x9C400;
        context->lineFarZClipSq = 0x4C90;
        context->chunkFarZClipSq = 0xCEA40;
        context->lowDetailDistSq = 0xE100;
    } else if (contextType == 1) {
        context->polyFarZClipSq = 0x44944;
        context->lineFarZClipSq = 0x2710;
        context->chunkFarZClipSq = 0x52210;
        context->lowDetailDistSq = 0x8D04;
    }
    sint32 handle = gContextMan.count;
    gContextMan.count = (handle + 1);
    return handle;
}

void BWorld::Restart(void)
{
    FUNCTION_MARKER(0x8007E564, "NFS4.EXE");
    Object::KillStatus();
    Object::InitStatus();
    SetContext(0);
    InitSpikeBelt();
    Anim::Restart();
    SceneLoaded = 0;
    Object::ClearCustomObjects();
    Draw::gDoVSync = 0;
    BWorldSm::Restart();
}

void BWorld::StartLoop(void)
{
    FUNCTION_MARKER(0x8007E5C0, "NFS4.EXE");
}

void BWorld::Init(void)
{
    FUNCTION_MARKER(0x8007E5C8, "NFS4.EXE");
    if (Replay::ReplayMode == 0) {
        if (GameSetup::gData.commMode == 1 || GameSetup::gData.raceType == 2) {
            GameSetup::gData.SceneNumber = 99;
            GameSetup::gData.SceneStartLap = 99;
            GameSetup::gData.SceneEndLap = 99;
        } else {
            GameSetup::gData.SceneNumber = randPSX() / 0x4000;
            sint32 draw = randPSX();
            sint32 laps = GameSetup::gData.numLaps;
            if (laps >= 2)
                GameSetup::gData.SceneStartLap = (draw * laps) / 0x8000;
            else
                GameSetup::gData.SceneStartLap = draw / 0x4000;
            draw = randPSX();
            sint32 interval = (draw * GameSetup::gData.numLaps) / 0x8000;
            sint32 start = GameSetup::gData.SceneStartLap;
            sint32 end = (start + interval);
            GameSetup::gData.SceneEndLap = end;
            if (start == end)
                GameSetup::gData.SceneEndLap = (start + 1);
            if (GameSetup::gData.Weather != 0) {
                GameSetup::gData.SceneNumber = (GameSetup::gData.SceneNumber + 10);
                GameSetup::gData.SceneEndLap = (GameSetup::gData.SceneEndLap + 5);
            } else if (GameSetup::gData.trafficDensity != 0) {
                GameSetup::gData.SceneNumber = (GameSetup::gData.SceneNumber + 20);
            }
            SceneLoaded = 0;
        }
    }
    Object::InitStatus();
    Track::SetTrackNumber(GameSetup::gData.track);
    InitContexts();
    sint32 contextType;
    if (GameSetup::gData.commMode == 1) {
        OpenContext(1, 0);
        contextType = 1;
    } else {
        contextType = 0;
    }
    OpenContext(contextType, contextType);
    SetContext(0);
    static char day[] = ".grp";
    static char wetDay[] = "W.grp";
    static char night[] = "N.grp";
    static char wetNight[] = "S.grp";
    char *AudioScene;
    if (GameSetup::gData.Time != 0)
        AudioScene = GameSetup::gData.Weather != 0 ? wetNight : night;
    else
        AudioScene = GameSetup::gData.Weather != 0 ? wetDay : day;
    Track::Init(Track::MakeTrackPathName(AudioScene));
    Object::InitCustomObjects();
    Object::InitIMassObjectInfo();
    Group *persistent = gPersistObjDef;
    if (persistent)
        Scene::Init(persistent->m_num_elements);
    Loading::UpdateLoadingScreen(5);
    static char dataExt[] = "";
    Anim::InitSystem(Track::MakeTrackDataPathName(dataExt));
    if (GameSetup::gData.commMode != 1 && GameSetup::gData.raceType != 1 && GameSetup::gData.raceType != 5)
        Scene::LoadSceneFile(GameSetup::gData.SceneNumber);
    sint32 audioFile = GameSetup::gData.Time != 0;
    if (GameSetup::gData.Weather != 0)
        audioFile += 2;
    AudList::LoadAudioFile(audioFile);
    InitSpikeBelt();
}

void BWorld::DeInit(void)
{
    FUNCTION_MARKER(0x8007E8B4, "NFS4.EXE");
    Object::KillStatus();
    Track::DeInit();
    InitContexts();
    if (Object::IMassObjInst)
        purgememadr(Object::IMassObjInst);
    Scene::PurgeScene();
    Object::DeInitCustomObjects();
    SceneLoaded = 0;
    AudList::PurgeAudio();
    Anim::DeInitSystem();
}
