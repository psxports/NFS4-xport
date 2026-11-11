#ifndef CARIO_H
#define CARIO_H

#include "xport.h"
#include "game/common/car.h"
#include "game/psx/draw.h"

struct shapetbl;

extern sint32 inFrontEnd;

class CarIO
{
public:
    struct textureInfo
    {
        const char *tex;
        const char *pal;
        sint32 palShare;
    };

    static sint32 carPixMapCount;
    static sint32 carVRamCount;
    static sint32 licenseSFX_Count;
    static sint16 carVRamSlots[18][2];
    static sint16 carVRamSlotsMenu[6][2];
    static sint16 carVRamAdd[6];
    static sint16 carVRamOffset[6];
    static sint16 licensePlate[22][6];
    static sint16 licenseSFX_Vram[12][2];
    static Draw::tPixMap *carPixMap;
    static shapetbl *Plate1[2];
    static shapetbl *Plate2[2];
    static textureInfo textureName[51];

    static void StartUp(void);
    static void CleanUp(void);
    static void ReStart(void);
    static void CopyFromShape(sint16 *source, sint16 *dest, sint32 w, sint32 h, sint32 x, sint32 y);
    static void CopyToShape(sint16 *source, sint16 *dest, sint32 mirror);
    static void CreateLicense(char *text, sint32 carType, sint32 player);
    static void CleanUpLicense(sint32 player);
    static void LicenseCheck(sint32 reload, sint32 *license_vx, sint32 *license_vy, Car::tObj *carObj, sint32 plate);
    static void ReadInCarTextureData(char *shpfile, Car::tObj *carObj, sint32 reload, sint32 player);
    static void UpdateCarTextureData(char *shpfile, Car::tObj *carObj, sint32 player);
    static void ReleaseCarCluts(Car::tObj *carObj);
};

#endif
