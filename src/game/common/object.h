#ifndef OBJECT_H
#define OBJECT_H

#include <stddef.h>
#include "xport.h"
#include "game/common/aistate.h"
#include "game/common/anim.h"
#include "game/common/bo.h"
#include "game/common/bworldsm.h"
#include "game/common/renderview.h"
#include "game/common/scene.h"
#include "game/common/trk.h"
#include "game/psx/draw.h"

struct AnimDef;

class ObjectAnim
{
public:
    static void *operator new(size_t size) throw();
    static void operator delete(void *memory) throw();
    virtual ~ObjectAnim(void);

    virtual sint32 Draw(DRender::tView *Vi, Draw::DCache *sd, sint32 offset) = 0;
};

class ObjectFinishedMultiAnim : public ObjectAnim
{
public:
    virtual ~ObjectFinishedMultiAnim(void);
    virtual sint32 Draw(DRender::tView *Vi, Draw::DCache *sd, sint32 offset);
};

class ObjectMultiAnim : public ObjectAnim
{
public:
    ObjectMultiAnim(coorddef *impactVel, AnimDef *def, Trk::CollideBoomInst *objCollideInstance, Trk::ObjectDef *objDef, Trk::SimObject *simObj, ObjectFinishedMultiAnim *finishedAnim);
    virtual ~ObjectMultiAnim(void);
    virtual sint32 Draw(DRender::tView *Vi, Draw::DCache *sd, sint32 offset);
    coorddef impactVel;
    Trk::SimObject *simObj;
    Trk::CollideBoomInst *objCollideInstance;
    Trk::ObjectDef *objDef;
    AnimDef *animParms;
    sint32 objectAngle, impactAngle;
    AnimScript *script;
    ObjectFinishedMultiAnim *finishedAnim;
};

class ObjectFinishedSignAnim : public ObjectAnim
{
public:
    virtual ~ObjectFinishedSignAnim(void);
    virtual sint32 Draw(DRender::tView *Vi, Draw::DCache *sd, sint32 offset);
    matrixtdef finalMatrix;
    Trk::ObjectDef *objDef;
    Trk::CollideBoomInst *objCollideInstance;
};

class ObjectSignAnim : public ObjectAnim
{
public:
    ObjectSignAnim(coorddef *impactVel, sint32 impactAngle, AnimDef *def, Trk::CollideBoomInst *objCollideInstance, Trk::ObjectDef *objDef, Trk::SimObject *simObj, coorddef *roadNormal, ObjectFinishedSignAnim *finishedAnim);
    virtual ~ObjectSignAnim(void);
    virtual sint32 Draw(DRender::tView *Vi, Draw::DCache *sd, sint32 offset);
    coorddef impactVel;
    Trk::SimObject *simObj;
    Trk::CollideBoomInst *objCollideInstance;
    Trk::ObjectDef *objDef;
    AnimDef *animParms;
    sint32 objectAngle, impactAngle;
    AnimScript *script;
    ObjectFinishedSignAnim *finishedAnim;
};

class Object
{
public:
    struct tSimObjList
    {
        sint32 numObjects;
        sint32 chunk;
        sint32 numObjects2;
        sint32 chunk2;
    };

    struct tIMassObjInfo
    {
        Trk::AnimateInst *animInst;
        coorddef dimension;
        coorddef lastPos;
        sint32 lastTick;
    };

    static sint32 customSliceNum;
    static Group *customObjInst;
    static Group *customSimObjs;
    static Group *customSFXInst;
    static tIMassObjInfo *IMassObjInst;

    static void InitCollisionCheckLoop(BWorldSm::Pos *slicePos, tSimObjList *objList, sint32 *numObj);
    static sint32 GetRadiusCollisionData(tSimObjList *objList, sint32 objIndex, coorddef *pos, sint32 *radius);
    static void GetPointsCollisionData(tSimObjList *objList, sint32 objIndex, sint32 *numPoints, coorddef *resultPoints);
    static void InitStatus(void);
    static void KillStatus(void);
    static ObjectAnim * GetAnim(Trk::SimObject *simObj);
    static sint32 CheckCollisionResults(tSimObjList *objList, sint32 objIndex, BO::tNewtonObj *N);
    static void InitCustomObjects(void);
    static void DeInitCustomObjects(void);
    static void ClearCustomObjects(void);
    static sint32 AddCustomSimObject(SceneElem *objectData, sint32 serialNum, sint32 instIndex);
    static sint32 GetObjDefID(sint32 index);
    static sint32 FindDefWithThisID(sint32 ID);
    static void AddCustomObject(SceneElem *objectData, sint32 setupSimDataFlag);
    static void InitIMassObjectInfo(void);
    static sint32 GetNumIMassObjects(void);
    static void GetIMassObjectDimensions(sint32 objIndex, coorddef *dimensions);
    static void GetIMassObjectMotion(sint32 objIndex, coorddef *cpoint, matrixtdef *orientMat, coorddef *velocity);
};

#endif
