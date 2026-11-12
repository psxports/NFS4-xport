#include <stddef.h>
#include <stdio.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/gmesetup.h"
#include "game/common/object.h"
#include "game/common/paths.h"
#include "game/common/scene.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

struct SceneSortedElem
{
    sint32 slice_;
    SceneElem *se;
};

struct CSceneList
{
    sint32 id_;
    sint32 numElements_;
    sint32 slice_;
    sint32 versionNumber_;
};

CSceneList *gGameSceneList;

sint32 GetPlaneY(const coorddef *norm, const coorddef *pointOnPlane, const coorddef *testPoint)
{
    FUNCTION_MARKER(0x800B6114, "NFS4.EXE");
    sint32 x = fixedmult(norm->x, pointOnPlane->x);
    sint32 y = fixedmult(norm->y, pointOnPlane->y);
    sint32 z = fixedmult(norm->z, pointOnPlane->z);
    sint32 D = (-((x + y) + z));
    x = fixedmult(norm->x, testPoint->x);
    z = fixedmult(norm->z, testPoint->z);
    sint32 numerator = (-((x + z) + D));
    return fixeddiv(numerator, norm->y);
}

void Scene::Init(sint32 numObjDefs)
{
    FUNCTION_MARKER(0x800B5F7C, "NFS4.EXE");
    if (numObjDefs > 0)
        gGameSceneList = 0;
}

void Scene::DeInit(void)
{
    FUNCTION_MARKER(0x800B5F90, "NFS4.EXE");
    PurgeScene();
}

void Scene::PurgeScene(void)
{
    FUNCTION_MARKER(0x800B5FB0, "NFS4.EXE");
    CSceneList *scene = gGameSceneList;
    if (scene) {
        purgememadr(scene);
        gGameSceneList = 0;
    }
    Object::ClearCustomObjects();
}

sint32 Scene::BuildCustomSceneList(void)
{
    FUNCTION_MARKER(0x800B5FE4, "NFS4.EXE");
    Object::ClearCustomObjects();
    CSceneList *scene = gGameSceneList;
    sint32 slice = -1;
    if (scene) {
        SceneElem *se = reinterpret_cast<SceneElem *>(scene + 1);
        slice = scene->slice_;
        sint32 count = scene->numElements_;
        Object::customSliceNum = slice;
        for (sint32 i = 0; i < count; ++i, ++se) {
            if (se->type >= 0 && se->type < 3)
                Object::AddCustomObject(se, 1);
            count = gGameSceneList->numElements_;
        }
    }
    return slice;
}

void Scene::LoadSceneFile(sint32 sceneFileIndex)
{
    FUNCTION_MARKER(0x800B6084, "NFS4.EXE");
    Object::ClearCustomObjects();
    char fname[128];
    NativeFileSlot *bigFile = 0;
    sprintf(fname, "%sscene.viv", Paths_Paths[6]);
    XPORT_UNPLACED::FILE::addbigsync(fname, 16, 100, &bigFile);
    sprintf(fname, "tr%02d%02d.scn", GameSetup::gData.track, sceneFileIndex);
    gGameSceneList = 0;
    gGameSceneList = static_cast<CSceneList *>(loadfileadr(fname, 0));
    XPORT_UNPLACED::FILE::delbigsync(bigFile, 100);
}
