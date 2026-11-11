#ifndef SCENE_H
#define SCENE_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/mathnfs.h"

sint32 GetPlaneY(const coorddef *norm, const coorddef *pointOnPlane, const coorddef *testPoint);

class SceneElem
{
public:
    sint32 type;
    sint32 size;
    sint32 committed;
    sint32 visible;
    coorddef cp;
    sint32 height;
    matrixtdef orient;
    sint32 subType;
    sint32 subTypeIndex;
    sint32 scalar1;
    sint32 scalar2;
    sint32 scalar3;
    sint32 scalar4;
};

class Scene
{
public:
    static void Init(sint32 numObjDefs);
    static void DeInit(void);
    static void PurgeScene(void);
    static sint32 BuildCustomSceneList(void);
    static void LoadSceneFile(sint32 sceneFileIndex);
};

#endif
