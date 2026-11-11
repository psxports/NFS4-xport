#ifndef R3DCAR_H
#define R3DCAR_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"
#include "game/common/renderview.h"
#include "game/common/transformer.h"
#include "game/psx/draw.h"

class R3DCar
{
public:
    static uint32 *subOt;
    static uint32 *subOtStart[2][2];

    static char *BigFile;
    static char *LicenseShapeFile;
    static matrixtdef *orientMat;
    static coorddef *position;
    static Draw::tVertex center;
    static CVECTOR shadowColour;
    static CVECTOR eMapColour;
    static Draw::tVertex shadowVertex[4];
    static Transformer::zScene *LoadedScenePointer[2][50];

    static sint32 aSyncLoading;
    static sint32 shadowFlag;
    static sint16 yawCam;
    static sint32 Clock;
    static sint32 ClockLast;
    static sint32 rightHandDrive;
    static sint32 InMenu;
    static char ObjectVisible[57];
    static char LoadedSceneCounter[2][50];
    static char LoadedSceneCountry[50];
    static sint16 LoadedSceneColor[2][50];
    static sint16 LoadedSceneVRam[2][50][2];
    static sint32 PositionZ[9];
    static char ObjectInfo[57][6];
    static sint16 Suspension[10];
    static sint16 FlareOverlayIndex[6];
    static sint16 FlareCopSirenType[15][6];
    static char SignalBrakeFlare[28];
    static char RecessedLight[28];
    static char ForceDriveSide[28];
    static char DamageSpoiler[28];
    static char CopIndex[6][5];
    static char CopCountry[15];

    struct tEnvMapInfo
    {
        sint32 eScaleX;
        sint32 eScaleY;
        sint32 rideHeight;
        sint32 upgradeHeight;
    };

    static tEnvMapInfo EnvMapInfo[28];

    static void ChangeTrafficColor(Car::tObj *carObj, sint32 newColorIndex);
    static void StartUp(void);
    static void PostStartUp(void);
    static void Restart(void);
    static void CleanUp(void);
    static Transformer::zScene * ReadInCarData(char *filename, Car::tObj *carObj);
    static void CalcCarDimensions(Car::tObj *carObj, Transformer::zScene *scene, sint32 carType);
    static void DeInstantiate3DCar(Car::tObj *carObj);
    static void DeInstantiate3DCarMenu(Car::tObj *carObj);
    static void GetFileName(char *infilename, char *filename, const char *suffix);
    static char * LoadFileAdr(char *name);
    static void GetCarName(char *filename, sint32 carType, sint32 country);
    static void Instantiate3DCar(Car::tObj *carObj, sint32 index);
    static sint32 Visibilty(Car::tObj *carObj, DRender::tView *Vi);
    static void TurnHeadLightOn(Car::tObj *carObj, sint32 tail);
    static void TurnHeadLightOff(Car::tObj *carObj, sint32 head);
    static void MATRIX3DT_Copy(sint32 *from, sint32 *to);
    static void InsertCarFacet(Car::tObj *carObj, DRender::tView *Vi);
    static void ReadInCarTextureMenu(Car::tObj *carObj, char *bigfile, sint32 reload, sint32 player);
    static void InsertCarFacetMenu(Car::tObj *carObj, DRender::tView *Vi);
    static void InsertCarFacetII(Car::tObj *carObj);
    static void InsertCarFacetMenuII(Car::tObj *carObj, sint32 light);
    static void Showroom(DRender::tView *Vi);
    static void InsertAllListFacet(DRender::tView *Vi);
};

class R3DCcar
{
public:
    static void ReadeMapData(void);
    static void ReadTrackShadow(void);
};

#endif
