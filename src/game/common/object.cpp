#include <limits.h>
#include <new>
#include <stddef.h>
#include <stdlib.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/aihigh.h"
#include "game/common/anim.h"
#include "game/common/bo.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/chunk.h"
#include "game/common/group.h"
#include "game/common/mathnfs.h"
#include "game/common/object.h"
#include "game/common/quatern.h"
#include "game/common/renderview.h"
#include "game/common/scene.h"
#include "game/common/sim.h"
#include "game/common/track.h"
#include "game/common/trk.h"
#include "game/psx/draww.h"
#include "lib/psx/blkfill.h"
#include "lib/psx/fixdmult.h"

struct AnimDef
{
    sint32 type;
    sint32 numPieces;
    sint32 objDefIndex;
    sint32 baseAnim;
    sint32 animIndex;
};

sint32 gNumIMassObjects;

AnimDef gAnimDefs[14] =
{
    {0, 1, -1, 2, 0},
    {1, 10, 0, 0, 1},
    {1, 20, 0, 0, 1},
    {1, 6, 0, 0, 1},
    {1, 2, 0, 0, 1},
    {1, 1, 0, 0, 1},
    {1, 10, 0, 0, 1},
    {1, 14, 0, 0, 1},
    {1, 5, 0, 0, 1},
    {1, 5, 0, 0, 1},
    {1, 1, 0, 0, 1},
    {1, 8, 0, 0, 1},
    {0, 1, -1, 2, 0},
    {0, 1, -1, 2, 0},
};

ObjectAnim *gSimObjAnims[450];
sint32 Object::customSliceNum;
Group *Object::customObjInst;
Group *Object::customSimObjs;
Group *Object::customSFXInst;
Object::tIMassObjInfo *Object::IMassObjInst;

void *ObjectAnim::operator new(size_t size) throw()
{
    return reservememadr(0, (sint32)size, 0);
}

void ObjectAnim::operator delete(void *memory) throw()
{
    purgememadr(memory);
}

sint32 CalcObjYawAngle(CCOORD16 *pts)
{
    FUNCTION_MARKER(0x800A4AE4, "NFS4.EXE");
    sint32 dx = pts[1].x - pts[0].x;
    sint32 dz = pts[1].z - pts[0].z;
    return (fixedatan(dx, dz) - 0x4000) >> 8;
}

void CalcObjExtentPoints(coorddef *cp, CCOORD16 *pts, coorddef *resultPts, tQuat *quat)
{
    FUNCTION_MARKER(0x800A4B1C, "NFS4.EXE");
    sint32 cpx = cp->x;
    sint32 cpz = cp->z;

    coorddef pt0 = { pts[0].x * 1024, 0, pts[0].z * 1024 };
    coorddef pt1 = { pts[1].x * 1024, 0, pts[1].z * 1024 };
    resultPts[0].x = cpx;
    resultPts[0].z = cpz;
    matrixtdef matrix;
    Quatern::QuatToMat(quat, &matrix);
    transform(&pt0, &matrix, &resultPts[1]);
    transform(&pt1, &matrix, &resultPts[2]);
    resultPts[1].x = (resultPts[1].x + cpx);
    resultPts[1].z = (resultPts[1].z + cpz);
    resultPts[2].x = (resultPts[2].x + cpx);
    resultPts[2].z = (resultPts[2].z + cpz);
}

void BuildObjCollisionMatrix(sint32 weight, sint32 objAngle, sint32 impactAngle, matrixtdef *mat)
{
    FUNCTION_MARKER(0x800A4BE8, "NFS4.EXE");
    sint32 impactWeight = fixedmult((-impactAngle), (0x10000 - weight));
    sint32 objWeight = fixedmult((-objAngle), weight);
    matrixtdef objAngleMat;
    fixedxformy(&objAngleMat, ((impactWeight + objWeight)));
    matrixtdef impactMat;
    fixedxformy(&impactMat, impactAngle);
    matrixtdef tmpMat;
    Math::fasttransmult(&objAngleMat, mat, &tmpMat);
    Math::fasttransmult(&tmpMat, &impactMat, mat);
}

Trk::SimpleInst *FindObjInstanceFromSerialNum(Group *group, sint32 index)
{
    FUNCTION_MARKER(0x800A4C8C, "NFS4.EXE");
    if ((uint32)index - 126 < 2)
        return 0;
    if ((index & 0x80) != 0) {
        group = Object::customObjInst;
        index &= ~0x80;
    }
    if (index >= group->m_num_elements)
        return 0;
    Trk::SimpleInst *objInstance = reinterpret_cast<Trk::SimpleInst *>(group + 1);
    index = (index - 1);
    while (index != -1) {
        sint32 size = objInstance->size;
        index = (index - 1);
        objInstance = reinterpret_cast<Trk::SimpleInst *>(reinterpret_cast<char *>(objInstance) + size);
    }
    return objInstance;
}

Trk::SimObject *GetSimObj(sint32 objIndex, Object::tSimObjList *objList, sint32 *chunk)
{
    FUNCTION_MARKER(0x800A4F14, "NFS4.EXE");
    sint32 first = objList->numObjects;
    sint32 second = objList->numObjects2;
    sint32 total = (first + second);
    if (objIndex >= total) {
        sint32 index = ((objIndex - first) - second);
        Group *group = Object::customSimObjs;
        *chunk = -1;
        return reinterpret_cast<Trk::SimObject *>(group + 1) + index;
    }
    *chunk = objList->chunk;
    first = objList->numObjects;
    sint32 index = objIndex;
    if (index >= first) {
        sint32 nextChunk = objList->chunk2;
        index = (index - first);
        *chunk = nextChunk;
    }
    Group *group = Track::chunkList[*chunk].simObjBuf;
    Trk::SimObject *simObj = 0;
    if (group) {
        simObj = reinterpret_cast<Trk::SimObject *>(group + 1) + index;
        if (simObj->type == 0x10)
            simObj = 0;
    }
    return simObj;
}

void SetCautionSurface(coorddef *pt, BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x800A573C, "NFS4.EXE");
    BWorldSm::FindClosestQuadRez(pt, slicePos, 1);
    Trk::NewSimQuad *simQuad = slicePos->simQuad;
    if (simQuad && (simQuad->surface & 0x40) == 0) {
        Track::gSaveSurface->Save(simQuad);
        simQuad = slicePos->simQuad;
        simQuad->surface |= 0x40;
    }
}

void GetObjMaxDimensions(Trk::ObjectDef **pObjDefs, Trk::SimpleInst *objInstance, coorddef *dimensions)
{
    FUNCTION_MARKER(0x800A5D58, "NFS4.EXE");
    Trk::ObjectDef *objDef = pObjDefs[objInstance->pad];
    CCOORD16 minDim = { 0 };
    CCOORD16 maxDim = { 0 };
    sint32 vertCount = objDef->vertexCount;
    CCOORD16 *pts = reinterpret_cast<CCOORD16 *>(objDef + 1);
    for (sint32 i = 0; i < vertCount; ++i, ++pts) {
        if (maxDim.x < pts->x)
            maxDim.x = pts->x;
        else if (pts->x < minDim.x)
            minDim.x = pts->x;
        if (maxDim.y < pts->y)
            maxDim.y = pts->y;
        else if (pts->y < minDim.y)
            minDim.y = pts->y;
        if (maxDim.z < pts->z)
            maxDim.z = pts->z;
        else if (pts->z < minDim.z)
            minDim.z = pts->z;
    }
    dimensions->x = (maxDim.x - minDim.x) * 512;
    dimensions->y = (maxDim.y - minDim.y) * 512;
    dimensions->z = (maxDim.z - minDim.z) * 512;
}

void Object::InitCollisionCheckLoop(BWorldSm::Pos *slicePos, Object::tSimObjList *objList, sint32 *numObj)
{
    FUNCTION_MARKER(0x800A4CFC, "NFS4.EXE");
    Group *group = Track::chunkList[slicePos->chunk].simObjBuf;
    objList->numObjects = group ? group->m_num_elements : 0;
    sint32 chunk = slicePos->chunk;
    objList->numObjects2 = 0;
    objList->chunk = chunk;
    sint32 slice = slicePos->slice;
    sint32 next = slice + 1;
    if (next >= gNumSlices)
        next = (slice - (gNumSlices - 1));
    sint32 altChunk = BWorldSm::slices[next].chunkIndex;
    group = 0;
    if (altChunk != slicePos->chunk)
        group = Track::chunkList[altChunk].simObjBuf;
    if (!group) {
        slice = slicePos->slice;
        sint32 previous = slice - 1;
        if (previous < 0)
            previous = ((slice + gNumSlices) - 1);
        altChunk = BWorldSm::slices[previous].chunkIndex;
        if (altChunk != slicePos->chunk)
            group = Track::chunkList[altChunk].simObjBuf;
    }
    sint32 total;
    if (group) {
        objList->numObjects2 = group->m_num_elements;
        sint32 first = objList->numObjects;
        sint32 second = objList->numObjects2;
        objList->chunk2 = altChunk;
        total = (first + second);
    } else {
        total = objList->numObjects;
    }
    *numObj = total;
    group = customSimObjs;
    if (group && group->m_num_elements > 0 && Math::DistXZ(reinterpret_cast<coorddef *>(BWorldSm::slices + customSliceNum), reinterpret_cast<coorddef *>(BWorldSm::slices + slicePos->slice)) < 0xC00000) {
        sint32 customCount = customSimObjs->m_num_elements;
        *numObj = (((*numObj)) + customCount);
    }
}

sint32 Object::GetRadiusCollisionData(Object::tSimObjList *objList, sint32 objIndex, coorddef *pos, sint32 *radius)
{
    FUNCTION_MARKER(0x800A4FE8, "NFS4.EXE");
    sint32 chunk;
    Trk::SimObject *simObj = GetSimObj(objIndex, objList, &chunk);
    if (simObj) {
        if ((simObj->type & 0x80) != 0 && !gSimObjAnims[simObj->serialNum]) {
            Trk::SimpleInst *objInstance = FindObjInstanceFromSerialNum( Track::chunkList[chunk].objInstanceBuf, simObj->instIndex);
            sint32 dummy;
            Anim::GetPos(reinterpret_cast<Trk::AnimateInst *>(objInstance), 1, simGlobal.gameTicks, reinterpret_cast<coorddef *>(simObj->point), &dummy, &dummy);
        }
        coorddef value;
        value.x = simObj->point[0];
        value.y = simObj->point[1];
        value.z = simObj->point[2];
        *pos = value;
        *radius = ((uint32)(sint32)simObj->radius << 7);
    } else {
        *radius = 0;
    }
    return 0;
}

void Object::GetPointsCollisionData(Object::tSimObjList *objList, sint32 objIndex, sint32 *numPoints, coorddef *resultPoints)
{
    FUNCTION_MARKER(0x800A50F4, "NFS4.EXE");
    sint32 chunk;
    Trk::SimObject *simObj = GetSimObj(objIndex, objList, &chunk);
    if (!simObj) {
        *numPoints = 0;
        return;
    }
    Group *group = chunk == -1 ? 0 : Track::chunkList[chunk].objInstanceBuf;
    Trk::SimpleInst *objInstance = FindObjInstanceFromSerialNum(group, simObj->instIndex);
    if (!objInstance) {
        *numPoints = 1;
        coorddef value;
        value.x = simObj->point[0];
        value.y = simObj->point[1];
        value.z = simObj->point[2];
        *resultPoints = value;
        return;
    }
    Trk::ObjectDef *objDef = Track::gObjDefs[objInstance->pad];
    if (objInstance->type == 5 && reinterpret_cast<Trk::CollideBoomInst *>(objInstance)->boomIndex == 0) {
        CalcObjExtentPoints(reinterpret_cast<coorddef *>(simObj->point), reinterpret_cast<CCOORD16 *>(objDef + 1), resultPoints, reinterpret_cast<tQuat *>(objInstance + 1));
        *numPoints = 3;
    } else {
        coorddef value;
        value.x = simObj->point[0];
        value.y = simObj->point[1];
        value.z = simObj->point[2];
        *resultPoints = value;
        *numPoints = 1;
    }
}

void Object::InitStatus(void)
{
    FUNCTION_MARKER(0x800A5230, "NFS4.EXE");
    for (sint32 i = 449; i >= 0; --i)
        gSimObjAnims[i] = 0;
}

void Object::KillStatus(void)
{
    FUNCTION_MARKER(0x800A5258, "NFS4.EXE");
    for (sint32 i = 0; i < 450; ++i) {
        ObjectAnim *object = gSimObjAnims[i];
        if (object)
            delete object;
    }
}

ObjectAnim * Object::GetAnim(Trk::SimObject *simObj)
{
    FUNCTION_MARKER(0x800A52C4, "NFS4.EXE");
    if (simObj)
        return gSimObjAnims[simObj->serialNum];
    return 0;
}

sint32 Object::CheckCollisionResults(Object::tSimObjList *objList, sint32 objIndex, BO::tNewtonObj *N)
{
    FUNCTION_MARKER(0x800A52F0, "NFS4.EXE");
    sint32 vx = N->linearVel.x;
    sint32 vz = N->linearVel.z;
    if (vx < 0)
        vx = (-vx);
    if (vz < 0)
        vz = (-vz);
    sint32 vel = (vx + vz) >> 16;
    sint32 chunk;
    Trk::SimObject *simObj = GetSimObj(objIndex, objList, &chunk);
    ObjectAnim *objStatus = gSimObjAnims[simObj->serialNum];
    sint32 type = simObj->type & 0xF;
    if (type == 1)
        return 1;
    if (type == 3)
        return 2;
    if (type != 2 || objStatus)
        return 0;
    if (vel < 0)
        return 1;
    sint32 instanceIndex = simObj->instIndex;
    Group *group = 0;

    if ((instanceIndex & 0x80) == 0)
        group = Track::chunkList[chunk].objInstanceBuf;
    Trk::SimpleInst *objInstance = FindObjInstanceFromSerialNum(group, instanceIndex);
    sint32 boomIndex = objInstance->type == 6 ? reinterpret_cast<Trk::AnimateBoomInst *>(objInstance)->boomIndex : reinterpret_cast<Trk::CollideBoomInst *>(objInstance)->boomIndex;
    AnimDef *animDef = &gAnimDefs[boomIndex];
    Trk::ObjectDef *objDef;
    if (animDef->animIndex)
        objDef = reinterpret_cast<Trk::ObjectDef *>(gPersistObjDef + 1);
    else
        objDef = Track::gObjDefs[objInstance->pad];
    if (animDef->type == 1) {
        void *storage = reservememadr(0, sizeof(ObjectFinishedMultiAnim), 0);
        ObjectFinishedMultiAnim *finishedAnim = ::new (storage) ObjectFinishedMultiAnim;
        storage = reservememadr(0, sizeof(ObjectMultiAnim), 0);
        objStatus = ::new (storage) ObjectMultiAnim(&N->linearVel, animDef, reinterpret_cast<Trk::CollideBoomInst *>(objInstance), objDef, simObj, finishedAnim);
    } else if (animDef->type == 0) {
        void *storage = reservememadr(0, sizeof(ObjectFinishedSignAnim), 0);
        ObjectFinishedSignAnim *finishedAnim = ::new (storage) ObjectFinishedSignAnim;
        storage = reservememadr(0, sizeof(ObjectSignAnim), 0);
        sint32 impactAngle = fixedatan(N->linearVel.x >> 8, N->linearVel.z >> 8) >> 8;
        objStatus = ::new (storage) ObjectSignAnim(&N->linearVel, impactAngle, animDef, reinterpret_cast<Trk::CollideBoomInst *>(objInstance), objDef, simObj, reinterpret_cast<coorddef *>(&N->roadMatrix.m[3]), finishedAnim);
    } else {
        return -1;
    }
    gSimObjAnims[simObj->serialNum] = objStatus;
    return -1;
}

void Object::InitCustomObjects(void)
{
    FUNCTION_MARKER(0x800A5588, "NFS4.EXE");
    Group *group = static_cast<Group *>(reservememadr("Custom Objects", 1024, 0));
    customObjInst = group;
    group->m_num_elements = 0;
    group = static_cast<Group *>(reservememadr("Custom SimObjects", 1024, 0));
    customSimObjs = group;
    group->m_num_elements = 0;
    blockfill(group + 1, 1020, 0);
    group = static_cast<Group *>(reservememadr("Custom SimObjects", 1024, 0));
    group->m_num_elements = 0;
    customSFXInst = group;
    customSliceNum = 0;
}

void Object::DeInitCustomObjects(void)
{
    FUNCTION_MARKER(0x800A560C, "NFS4.EXE");
    Group *group = customObjInst;
    if (group)
        purgememadr(group);
    group = customSFXInst;
    if (group)
        purgememadr(group);
    group = customSimObjs;
    if (group)
        purgememadr(group);
    customObjInst = 0;
    customSFXInst = 0;
    customSimObjs = 0;
}

void Object::ClearCustomObjects(void)
{
    FUNCTION_MARKER(0x800A566C, "NFS4.EXE");
    Group *objects = customObjInst;
    Group *simObjects = customSimObjs;
    Group *effects = customSFXInst;
    sint32 count = Cars::gNumTrafficCars;
    objects->m_num_elements = 0;
    simObjects->m_num_elements = 0;
    effects->m_num_elements = 0;
    for (sint32 i = 0; i < count; ++i)
        Cars::gTrafficCarList[i]->carFlags &= ~0x400u;
    count = Cars::gNumCopCars;
    for (sint32 i = 0; i < count; ++i)
        Cars::gCopCarList[i]->carFlags &= ~0x400u;
    SaveSurface *surface = Track::gSaveSurface;
    if (surface)
        surface->RestoreAll();
}

sint32 Object::AddCustomSimObject(SceneElem *objectData, sint32 serialNum, sint32 instIndex)
{
    FUNCTION_MARKER(0x800A57B0, "NFS4.EXE");
    if (objectData->type == 0) {
        Group *group = customSimObjs;
        Trk::SimObject *simObj = reinterpret_cast<Trk::SimObject *>(group + 1) + group->m_num_elements;
        simObj->point[0] = objectData->cp.x;
        simObj->point[1] = objectData->cp.y;
        simObj->point[2] = objectData->cp.z;
        BWorldSm::Pos slicePos;
        BWorldSm::SetSlice(1, &slicePos);
        coorddef pt = objectData->cp;
        SetCautionSurface(&pt, &slicePos);
        pt.x = (pt.x - 0x40000);
        SetCautionSurface(&pt, &slicePos);
        pt.x = (pt.x + 0x80000);
        SetCautionSurface(&pt, &slicePos);
        pt.x = (pt.x - 0x40000);
        pt.z = (pt.z - 0x40000);
        SetCautionSurface(&pt, &slicePos);
        pt.z = (pt.z + 0x80000);
        SetCautionSurface(&pt, &slicePos);
        sint32 slice = slicePos.slice;
        sint32 radius = objectData->scalar2;
        simObj->serialNum = (sint16)(serialNum + 400u);
        customSliceNum = slice;
        simObj->radius = (sint16)(radius >> 9);
        simObj->instIndex = (uint8)(instIndex - 128u);
        simObj->type = objectData->scalar1 < 9 ? 2 : 1;
        group = customSimObjs;
        group->m_num_elements = (group->m_num_elements + 1);
    }
    return (customSimObjs->m_num_elements - 1);
}

sint32 Object::GetObjDefID(sint32 index)
{
    FUNCTION_MARKER(0x800A5958, "NFS4.EXE");
    if (gPersistObjDef != 0 && index < gPersistObjDef->m_num_elements)
        return Track::gObjDefs[index]->id;
    return 0;
}

sint32 Object::FindDefWithThisID(sint32 ID)
{
    FUNCTION_MARKER(0x800A59AC, "NFS4.EXE");
    Group *definitions = gPersistObjDef;
    Trk::ObjectDef **entry = Track::gObjDefs;
    for (sint32 i = 0; i < definitions->m_num_elements; ++i, ++entry) {
        if (ID == (*entry)->id)
            return i;
    }
    return -1;
}

void Object::AddCustomObject(SceneElem *objectData, sint32 setupSimDataFlag)
{
    FUNCTION_MARKER(0x800A5A04, "NFS4.EXE");
    if (objectData->type == 0) {
        Group *group = customObjInst;
        sint32 index = (group->m_num_elements - 1);
        Trk::CollideBoomInst *objBoomInstance = reinterpret_cast<Trk::CollideBoomInst *>(group + 1);
        if (index != -1) {
            sint32 stride = objBoomInstance->size;
            do {
                index = (index - 1);
                objBoomInstance = reinterpret_cast<Trk::CollideBoomInst *>( reinterpret_cast<char *>(objBoomInstance) + stride);
            } while (index != -1);
        }
        objBoomInstance->size = 36;
        objBoomInstance->type = objectData->scalar1 < 9 ? 5 : 2;
        objBoomInstance->zoffset = 3;
        objBoomInstance->flags = 0;
        objBoomInstance->x = objectData->cp.x;
        objBoomInstance->y = objectData->cp.y;
        sint32 z = objectData->cp.z;
        objBoomInstance->objectIndex = 0;
        Group *definitions = gPersistObjDef;
        objBoomInstance->z = z;
        objBoomInstance->pad = (sint16)objectData->subTypeIndex;
        if (objBoomInstance->pad >= definitions->m_num_elements)
            objBoomInstance->pad = 0;

        if (GetObjDefID(objBoomInstance->pad) != objectData->scalar1 && objectData->committed && objectData->visible) {
            FindDefWithThisID(objectData->scalar1);
            sint32 newInd = FindDefWithThisID(objectData->scalar1);
            if (newInd != -1) {
                objBoomInstance->pad = (sint16)newInd;
                sint32 requestedID = objectData->scalar1;
                objectData->subTypeIndex = newInd;
                FindDefWithThisID(requestedID);
            } else {
                objectData->scalar1 = GetObjDefID(objBoomInstance->pad);
            }
        }
        tQuat quat;
        Quatern::MatToQuat(&objectData->orient, &quat);
        objBoomInstance->qw = quat.w;
        objBoomInstance->qx = quat.x;
        objBoomInstance->qy = quat.y;
        sint16 qz = quat.z;
        objBoomInstance->sx = 256;
        objBoomInstance->sy = 256;
        objBoomInstance->sz = 256;
        objBoomInstance->qz = qz;
        if (setupSimDataFlag) {
            sint32 count = customObjInst->m_num_elements;
            sint32 simIndex = AddCustomSimObject(objectData, count, count);
            objBoomInstance->simIndex = (uint8)simIndex;
            objBoomInstance->boomIndex = (uint8)objectData->scalar1;
        } else {
            objBoomInstance->simIndex = 0;
            objBoomInstance->boomIndex = 0;
        }
        group = customObjInst;
        group->m_num_elements = (group->m_num_elements + 1);
    } else if (objectData->type == 2) {
        Group *group = customSFXInst;
        sint32 count = group->m_num_elements;
        Trk::SFX *sfxInstance = reinterpret_cast<Trk::SFX *>(group + 1) + count;
        sfxInstance->point[0] = objectData->cp.x;
        sfxInstance->point[1] = objectData->cp.y;
        sfxInstance->point[2] = objectData->cp.z;
        uint16 type = (uint16)objectData->subType;
        sfxInstance->pad = 0;
        sfxInstance->type = type;
        group->m_num_elements = (group->m_num_elements + 1);
    } else if (objectData->type == 1 && objectData->subType == 0) {
        sint32 count = Cars::gNumTrafficCars;
        if (count > 0) {
            sint32 i;
            for (i = 0; i < count; ++i) {
                Car::tObj *carObj = Cars::gTrafficCarList[i];
                if ((carObj->carFlags & 0x400) == 0 && carObj->render.currentCarType == objectData->scalar3)
                    break;
            }
            if (i < Cars::gNumTrafficCars) {
                Car::tObj *carObj = Cars::gTrafficCarList[i];
                AIHigh_Traffic *aicar = static_cast<AIHigh_Traffic *>(highLevelAIObjs[carObj->carIndex]);
                aicar->accidentData_ = objectData;
            }
        }
    }
}

void Object::InitIMassObjectInfo(void)
{
    FUNCTION_MARKER(0x800A5ED0, "NFS4.EXE");
    Group *instances = gPersistObjInst;
    gNumIMassObjects = 0;
    IMassObjInst = 0;
    if (instances && gPersistObjDef) {
        IMassObjInst = static_cast<tIMassObjInfo *>(reservememadr("IMObj info", ((uint32)instances->m_num_elements << 5), 0));
        if (IMassObjInst) {
            instances = gPersistObjInst;
            sint32 count = instances->m_num_elements;
            Trk::AnimateInst *objInst = reinterpret_cast<Trk::AnimateInst *>(instances + 1);
            for (sint32 objIndex = 0; objIndex < count; ++objIndex) {
                if (objInst->type == 7) {
                    tIMassObjInfo *info = &IMassObjInst[gNumIMassObjects];
                    info->animInst = objInst;
                    GetObjMaxDimensions(Track::gObjDefs, reinterpret_cast<Trk::SimpleInst *>(objInst), &info->dimension);
                    sint32 index = gNumIMassObjects;
                    info = &IMassObjInst[index];
                    sint32 next = (index + 1);
                    info->lastPos.x = 0;
                    info->lastPos.y = 0;
                    info->lastPos.z = 0;
                    info->lastTick = 0;
                    gNumIMassObjects = next;
                }
                sint32 stride = objInst->size;
                count = gPersistObjInst->m_num_elements;
                objInst = reinterpret_cast<Trk::AnimateInst *>(reinterpret_cast<char *>(objInst) + stride);
            }
        }
    }
}

sint32 Object::GetNumIMassObjects(void)
{
    FUNCTION_MARKER(0x800A6004, "NFS4.EXE");
    return gNumIMassObjects;
}

void Object::GetIMassObjectDimensions(sint32 objIndex, coorddef *dimensions)
{
    FUNCTION_MARKER(0x800A6010, "NFS4.EXE");
    coorddef value = IMassObjInst[objIndex].dimension;
    *dimensions = value;
}

void Object::GetIMassObjectMotion(sint32 objIndex, coorddef *cpoint, matrixtdef *orientMat, coorddef *velocity)
{
    FUNCTION_MARKER(0x800A603C, "NFS4.EXE");
    sint32 objTime = DrawW::GetAnimationTime(IMassObjInst[objIndex].animInst);
    Anim::GetRotPos(IMassObjInst[objIndex].animInst, 1, objTime, cpoint, orientMat);
    tIMassObjInfo *info = &IMassObjInst[objIndex];
    sint32 timeDiff = (objTime - info->lastTick);
    if (timeDiff > 0) {
        sint32 delta = (cpoint->x - info->lastPos.x);
        velocity->x = ((uint32)(delta / timeDiff) << 6);
        delta = (cpoint->y - info->lastPos.y);
        velocity->y = ((uint32)(delta / timeDiff) << 6);
        delta = (cpoint->z - info->lastPos.z);
        velocity->z = ((uint32)(delta / timeDiff) << 6);
    } else {
        velocity->x = 0;
        velocity->y = 0;
        velocity->z = 0;
    }
    info = &IMassObjInst[objIndex];
    coorddef point = *cpoint;
    info->lastPos = point;
    info->lastTick = objTime;
}

sint32 ObjectFinishedMultiAnim::Draw(DRender::tView *Vi, Draw::DCache *sd, sint32 offset)
{
    FUNCTION_MARKER(0x800A61F8, "NFS4.EXE");
    return 2;
}

ObjectMultiAnim::ObjectMultiAnim(coorddef *impactVel, AnimDef *def, Trk::CollideBoomInst *objCollideInstance, Trk::ObjectDef *objDef, Trk::SimObject *simObj, ObjectFinishedMultiAnim *finishedAnim)
{
    FUNCTION_MARKER(0x800A6200, "NFS4.EXE");
    this->impactVel.x = impactVel->x >> 6;
    this->impactVel.y = impactVel->y >> 6;
    sint32 vz = impactVel->z;
    animParms = def;
    this->objCollideInstance = objCollideInstance;
    this->objDef = objDef;
    this->simObj = simObj;
    this->impactVel.z = vz >> 6;
    bool compact = objCollideInstance->type == 6;
    void *storage = reservememadr(0, sizeof(AnimScript), 0);
    Group *instances = gPersistObjInst;
    AnimDef *parameters = animParms;
    sint32 boomIndex;
    if (compact)
        boomIndex = reinterpret_cast<const Trk::AnimateBoomInst *>(objCollideInstance)->boomIndex;
    else
        boomIndex = objCollideInstance->boomIndex;
    script = ::new (storage) AnimScript(instances, 8, boomIndex, parameters->numPieces);
    script->SetAnimAttrib(2);
    this->finishedAnim = finishedAnim;
}

sint32 ObjectMultiAnim::Draw(DRender::tView *Vi, Draw::DCache *sd, sint32 offset)
{
    FUNCTION_MARKER(0x800A62F8, "NFS4.EXE");
    matrixtdef matrix;
    coorddef animcp;
    sint32 status = script->GetTimedAnimPosRot(0, &animcp, &matrix);
    if ((uint32)status + 1 < 2) {
        sint32 i = simObj->serialNum;
        ObjectAnim *anim = finishedAnim;
        finishedAnim = 0;
        ObjectAnim **slot = &gSimObjAnims[i];
        ObjectAnim *current = *slot;
        if (current)
            delete current;
        *slot = anim;
        return anim->Draw(Vi, sd, offset);
    }
    AnimScript *animation = script;
    Trk::AnimateInst *animInst = *animation->inst;
    sint32 limit = (animInst->interval * animInst->count) >> 1;
    sint32 ticks = (simGlobal.gameTicks - animation->baseTicks);
    sint32 partCount = animParms->numPieces;
    if (limit < ticks)
        ticks = limit;
    coorddef impact;
    impact.x = (impactVel.x * ticks);
    impact.y = (impactVel.y * ticks);
    impact.z = (impactVel.z * ticks);
    matrixtdef RSmatrix;
    Trk::CollideBoomInst *instance = objCollideInstance;
    if (instance->type != 6) {
        tQuat quat;
        quat.x = instance->qx;
        quat.y = instance->qy;
        quat.z = instance->qz;
        quat.w = instance->qw;
        Quatern::QuatToMat(&quat, &RSmatrix);
        instance = objCollideInstance;
        sint32 sx = instance->sx * 256;
        sint32 sy = instance->sy * 256;
        sint32 sz = instance->sz * 256;
        sint32 scale[3] = { sx, sy, sz };
        for (sint32 column = 0; column < 3; ++column) {
            sint32 t1 = fixedmult(RSmatrix.m[column], scale[column]);
            sint32 t2 = fixedmult(RSmatrix.m[column + 3], scale[column]);
            sint32 t3 = fixedmult(RSmatrix.m[column + 6], scale[column]);
            RSmatrix.m[column] = t1;
            RSmatrix.m[column + 3] = t2;
            RSmatrix.m[column + 6] = t3;
        }
    }
    Trk::AnimateBoomInst *objInst = reinterpret_cast<Trk::AnimateBoomInst *>(gPersistObjInst + 1);
    for (sint32 animIndex = 0; animIndex < partCount; ++animIndex) {
        script->GetTimedAnimPosRot(animIndex, &animcp, &matrix);
        if (objCollideInstance->type != 6)
            Math::fasttransmult(&matrix, &RSmatrix, &matrix);
        Trk::SimObject *object = simObj;
        coorddef cp;
        cp.x = object->point[0];
        cp.y = object->point[1];
        cp.z = object->point[2];
        cp.y = (cp.y + animcp.y);
        cp.x = ((cp.x + animcp.x) + impact.x);
        cp.z = ((cp.z + animcp.z) + impact.z);
        for (;;) {
            if (objInst->type == 8) {
                instance = objCollideInstance;
                sint32 type = instance->type;
                if ((type != 5 || objInst->boomIndex == instance->boomIndex) && (type != 6 || objInst->boomIndex == reinterpret_cast<Trk::AnimateBoomInst *>(instance)->boomIndex))
                    break;
            }
            objInst = reinterpret_cast<Trk::AnimateBoomInst *>( reinterpret_cast<char *>(objInst) + objInst->size);
        }
        Trk::ObjectDef *pObjDef = Track::gObjDefs[objInst->pad];
        DrawObjectTransform(Vi, sd, &matrix, pObjDef, &cp, offset, -1);
        objInst = reinterpret_cast<Trk::AnimateBoomInst *>( reinterpret_cast<char *>(objInst) + objInst->size);
    }
    return 4;
}

sint32 ObjectFinishedSignAnim::Draw(DRender::tView *Vi, Draw::DCache *sd, sint32 offset)
{
    FUNCTION_MARKER(0x800A671C, "NFS4.EXE");
    DrawObjectTransform(Vi, sd, &finalMatrix, objDef, reinterpret_cast<coorddef *>(&objCollideInstance->x), offset, -1);
    return 2;
}

ObjectSignAnim::ObjectSignAnim(coorddef *impactVel, sint32 impactAngle, AnimDef *def, Trk::CollideBoomInst *objCollideInstance, Trk::ObjectDef *objDef, Trk::SimObject *simObj, coorddef *roadNormal, ObjectFinishedSignAnim *finishedAnim)
{
    FUNCTION_MARKER(0x800A6764, "NFS4.EXE");
    sint32 vx = impactVel->x;
    sint32 vz = impactVel->z;
    coorddef velocity = *impactVel;
    this->impactVel = velocity;
    this->impactAngle = ((uint32)impactAngle << 8);
    animParms = def;
    if (vx < 0)
        vx = (-vx);
    if (vz < 0)
        vz = (-vz);
    this->objCollideInstance = objCollideInstance;
    this->objDef = objDef;
    this->simObj = simObj;
    sint32 vel = (vx + vz) >> 16;
    this->objectAngle = ((uint32)CalcObjYawAngle(reinterpret_cast<CCOORD16 *>(objDef + 1)) << 8);
    bool slow = vel < 10;
    void *storage = reservememadr(0, sizeof(AnimScript), 0);
    AnimDef *parameters = animParms;
    sint32 anim = parameters->baseAnim;
    sint32 numParts = parameters->numPieces;
    if (!slow)
        anim = (anim + 1);
    script = ::new (storage) AnimScript(anim, numParts);
    script->SetAnimAttrib(2);
    this->finishedAnim = finishedAnim;
    matrixtdef *basis = &finishedAnim->finalMatrix;
    coorddef normal = *roadNormal;
    basis->m[3] = normal.x;
    basis->m[4] = normal.y;
    basis->m[5] = normal.z;
    basis->m[6] = 0;
    basis->m[7] = 0;
    basis->m[8] = 0x10000;
    sint32 first = fixedmult(basis->m[4], 0x10000);
    sint32 second = fixedmult(basis->m[5], basis->m[7]);
    basis->m[0] = (first - second);
    first = fixedmult(basis->m[5], basis->m[6]);
    second = fixedmult(basis->m[3], basis->m[8]);
    basis->m[1] = (first - second);
    first = fixedmult(basis->m[3], basis->m[7]);
    second = fixedmult(basis->m[4], basis->m[6]);
    basis->m[2] = (first - second);
    first = fixedmult(basis->m[1], basis->m[5]);
    second = fixedmult(basis->m[2], basis->m[4]);
    basis->m[6] = (first - second);
    first = fixedmult(basis->m[2], basis->m[3]);
    second = fixedmult(basis->m[0], basis->m[5]);
    basis->m[7] = (first - second);
    first = fixedmult(basis->m[0], basis->m[4]);
    second = fixedmult(basis->m[1], basis->m[3]);
    basis->m[8] = (first - second);
    reorthogonalize(basis);
    matrixtdef mat;
    fixedxformx(&mat, 0x4000);
    matrixtdef objAngleMat;
    fixedxformy(&objAngleMat, (-this->objectAngle));
    matrixtdef yawMat;
    fixedxformy(&yawMat, this->impactAngle);
    matrixtdef tmpMat;
    Math::fasttransmult(&objAngleMat, &mat, &tmpMat);
    Math::fasttransmult(&tmpMat, &yawMat, &tmpMat);
    Math::fasttransmult(&tmpMat, basis, basis);
    finishedAnim->objDef = objDef;
    finishedAnim->objCollideInstance = objCollideInstance;
}

sint32 ObjectSignAnim::Draw(DRender::tView *Vi, Draw::DCache *sd, sint32 offset)
{
    FUNCTION_MARKER(0x800A6A38, "NFS4.EXE");
    matrixtdef matrix;
    coorddef animcp;
    sint32 status = script->GetTimedAnimPosRot(0, &animcp, &matrix);
    if ((uint32)status + 1 < 2) {
        sint32 i = simObj->serialNum;
        ObjectAnim *anim = finishedAnim;
        finishedAnim = 0;
        ObjectAnim **slot = &gSimObjAnims[i];
        ObjectAnim *current = *slot;
        if (current)
            delete current;
        *slot = anim;
        return anim->Draw(Vi, sd, offset);
    }
    Trk::ObjectDef *pObjDef = objDef;
    script->GetTimedAnimPosRot(0, &animcp, &matrix);
    sint32 frame;
    sint32 numFrames;
    script->GetAnimFrameInfo(&frame, &numFrames);
    sint32 weight = ((uint32)frame << 16);
    if (numFrames == 0 || (numFrames == -1 && weight == INT_MIN))
        abort();
    weight /= numFrames;
    coorddef cp;
    cp.x = objCollideInstance->x;
    cp.y = objCollideInstance->y;
    cp.z = objCollideInstance->z;
    cp.x = (cp.x + animcp.x);
    cp.y = (cp.y + animcp.y);
    cp.z = (cp.z + animcp.z);
    BuildObjCollisionMatrix(weight, objectAngle, impactAngle, &matrix);
    DrawObjectTransform(Vi, sd, &matrix, pObjDef, &cp, offset, -1);
    return 4;
}

ObjectAnim::~ObjectAnim(void)
{
    FUNCTION_MARKER(0x800A6DE0, "NFS4.EXE");
}

ObjectFinishedMultiAnim::~ObjectFinishedMultiAnim(void)
{
    FUNCTION_MARKER(0x800A6DAC, "NFS4.EXE");
}

ObjectMultiAnim::~ObjectMultiAnim(void)
{
    FUNCTION_MARKER(0x800A6CF8, "NFS4.EXE");
    AnimScript *animation = script;
    if (animation) {
        Trk::AnimateInst **parts = animation->inst;
        if (parts)
            purgememadr(parts);
        purgememadr(animation);
    }
    ObjectFinishedMultiAnim *finished = finishedAnim;
    if (finished)
        delete finished;
}

ObjectFinishedSignAnim::~ObjectFinishedSignAnim(void)
{
    FUNCTION_MARKER(0x800A6CC4, "NFS4.EXE");
}

ObjectSignAnim::~ObjectSignAnim(void)
{
    FUNCTION_MARKER(0x800A6C10, "NFS4.EXE");
    AnimScript *animation = script;
    if (animation) {
        Trk::AnimateInst **parts = animation->inst;
        if (parts)
            purgememadr(parts);
        purgememadr(animation);
    }
    ObjectFinishedSignAnim *finished = finishedAnim;
    if (finished)
        delete finished;
}
