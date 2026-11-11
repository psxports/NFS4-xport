#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/aitune.h"
#include "game/common/audiomus.h"
#include "game/common/cameraobj.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/clock.h"
#include "game/common/color.h"
#include "game/common/newton.h"
#include "game/common/paths.h"
#include "game/common/r3dcar.h"
#include "game/common/render.h"
#include "game/common/renderview.h"
#include "game/common/replay.h"
#include "game/common/sim.h"
#include "game/common/transformer_obj.h"
#include "game/psx/cario.h"
#include "game/psx/draw.h"
#include "game/psx/drawc.h"
#include "game/psx/draww.h"
#include "game/psx/night.h"
#include "game/psx/platform.h"
#include "game/psx/texture.h"
#include "game/psx/trsproj.h"
#include "lib/psx/blkmov.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/fixdsqrt.h"

struct PCOORD16
{
    sint16 x;
    sint16 y;
    sint16 z;
    sint16 p;
};

struct Draw::tVertex
{
    PCOORD16 sv;
};

sint32 R3DCar::aSyncLoading;
sint32 R3DCar::shadowFlag;
sint16 R3DCar::yawCam;
sint32 R3DCar::Clock;
sint32 R3DCar::ClockLast;
sint32 R3DCar::rightHandDrive;
sint32 R3DCar::InMenu;
char *R3DCar::BigFile;
char *R3DCar::LicenseShapeFile;
uint32 *R3DCar::subOt;
char R3DCar::ObjectVisible[57];
char R3DCar::LoadedSceneCounter[2][50];
char R3DCar::LoadedSceneCountry[50];
sint16 R3DCar::LoadedSceneColor[2][50];
sint16 R3DCar::LoadedSceneVRam[2][50][2];
sint32 R3DCar::PositionZ[9];

char R3DCar::ObjectInfo[57][6] = {
    { 0, 73, 0, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 0 },
    { 32, 2, 1, 1, 0, 0 },
    { 48, 0, 1, 1, 0, 0 },
    { 248, 0, 0, 0, 1, 0 },
    { 240, 8, 10, 10, 0, 0 },
    { 224, 0, 12, 0, 0, 0 },
    { 224, 0, 0, 12, 0, 0 },
    { 236, 137, 11, 11, 0, 11 },
    { 240, 136, 11, 11, 0, 11 },
    { 236, 137, 12, 12, 0, 12 },
    { 240, 136, 12, 12, 0, 12 },
    { 232, 0, 1, 0, 0, 0 },
    { 232, 0, 0, 1, 0, 0 },
    { 212, 0, 17, 0, 0, 0 },
    { 212, 0, 17, 0, 0, 0 },
    { 225, 8, 1, 0, 0, 0 },
    { 225, 8, 0, 1, 0, 0 },
    { 212, 0, 18, 18, 18, 0 },
    { 226, 0, 1, 0, 0, 0 },
    { 226, 0, 0, 1, 0, 0 },
    { 212, 0, 19, 19, 19, 0 },
    { 226, 24, 15, 16, 0, 0 },
    { 226, 8, 0, 1, 0, 0 },
    { 212, 16, 20, 20, 20, 0 },
    { 226, 24, 15, 16, 0, 0 },
    { 226, 8, 0, 1, 0, 0 },
    { 212, 16, 21, 21, 21, 0 },
    { 232, 8, 1, 0, 0, 0 },
    { 232, 8, 0, 1, 0, 0 },
    { 212, 0, 22, 22, 22, 0 },
    { 216, 0, 1, 1, 0, 0 },
    { 244, 0, 13, 0, 0, 0 },
    { 244, 0, 14, 0, 0, 0 },
    { 212, 0, 17, 0, 0, 0 },
    { 48, 0, 2, 1, 0, 0 },
    { 40, 2, 3, 0, 0, 3 },
    { 40, 2, 3, 0, 0, 3 },
    { 38, 2, 4, 0, 0, 0 },
    { 36, 2, 4, 0, 0, 4 },
    { 36, 2, 4, 0, 0, 4 },
    { 0, 73, 1, 0, 0, 1 },
    { 0, 73, 1, 0, 0, 1 },
    { 240, 128, 5, 0, 0, 5 },
    { 240, 128, 6, 0, 0, 6 },
    { 232, 137, 7, 7, 0, 7 },
    { 232, 137, 8, 8, 0, 8 },
    { 31, 0, 1, 1, 0, 0 },
    { 31, 0, 1, 1, 0, 0 },
    { 32, 0, 1, 0, 0, 0 },
    { 32, 0, 1, 0, 0, 0 },
    { 32, 0, 9, 1, 0, 0 },
    { 32, 0, 9, 1, 0, 0 },
    { 32, 0, 1, 0, 0, 0 },
    { 32, 0, 1, 0, 0, 0 },
    { 32, 0, 9, 1, 0, 0 },
    { 32, 0, 9, 1, 0, 0 },
};

sint16 R3DCar::Suspension[10] = { 0, 1, 0, 1, 0, 1, 2, 3, 2, 3 };
sint16 R3DCar::FlareOverlayIndex[6] = { -1, 24, 26, 27, 28, 29 };
sint16 R3DCar::FlareCopSirenType[15][6] = { { 0, 0, 0, 1801, 2311, 0 }, { 0, 0, 0, 2311, 1801, 0 }, { 0, 0, 0, 2313, 2313, 0 }, { 0, 0, 0, 1799, 1799, 0 }, { 0, 0, 0, 2313, 2313, 0 }, { 0, 0, 0, 2311, 1801, 0 }, { 0, 0, 0, 2311, 1801, 0 }, { 0, 0, 0, 2313, 2313, 0 }, { 0, 0, 0, 1799, 1799, 0 }, { 0, 0, 0, 2313, 2313, 0 }, { 0, 0, 0, 2313, 2313, 0 }, { 0, 0, 0, 1799, 1799, 0 }, { 0, 0, 0, 2313, 2313, 0 }, { 0, 0, 0, 2311, 1801, 0 }, { 0, 0, 0, 1801, 2311, 0 } };
matrixtdef *R3DCar::orientMat;
coorddef *R3DCar::position;
Draw::tVertex R3DCar::center;
CVECTOR R3DCar::shadowColour;
CVECTOR R3DCar::eMapColour;
char R3DCar::SignalBrakeFlare[28] = { 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 1, 0, 1, 0, 0, 0, 0, 0, 3, 3, 2, 0, 0, 0, 0, 0, 0, 0 };
char R3DCar::RecessedLight[28] = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
char R3DCar::ForceDriveSide[28] = { 255, 255, 1, 1, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255, 255, 255, 255, 1, 255, 255, 255, 255, 255, 1, 255, 255, 255, 255 };
char R3DCar::DamageSpoiler[28] = { 0, 0, 0, 0, 128, 128, 0, 0, 0, 0, 0, 129, 0, 0, 0, 0, 129, 0, 0, 0, 0, 0, 0, 0, 0, 0, 129, 0 };

char R3DCar::CopIndex[6][5] = {
    { 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1 },
    { 2, 3, 4, 5, 5 },
    { 6, 6, 6, 6, 6 },
    { 7, 8, 9, 9, 9 },
    { 10, 11, 12, 13, 14 },
};

char R3DCar::CopCountry[15] = { 99, 99, 98, 102, 103, 97, 99, 98, 102, 103, 98, 102, 103, 97, 117 };
R3DCar::tEnvMapInfo R3DCar::EnvMapInfo[28];
Draw::tVertex R3DCar::shadowVertex[4];
Transformer::zScene *R3DCar::LoadedScenePointer[2][50];
uint32 *R3DCar::subOtStart[2][2];

void R3DCar::ChangeTrafficColor(Car::tObj *carObj, sint32 newColorIndex)
{
    FUNCTION_MARKER(0x800ADC2C, "NFS4.EXE");
    if (carObj->render.palCopy == 0)
        return;
    uint16 oldColor = (uint16)carObj->render.colorIndex;
    newColorIndex = (sint32)((uint32)newColorIndex & 7U);
    sint32 color = (oldColor & 8) + newColorIndex;
    if (color == (sint16)oldColor)
        return;
    DrawSync(0);
    if (carObj->render.currentCarType < 28)
    {
        Texture::CarColor = newColorIndex + (((uint8)carObj->render.upgradeFlags & 2) << 2);
        Texture::ProcessPaletteCopy((Texture::pal8bit *)carObj->render.palCopy, 0, 1);
        Texture::pal8bit *palCopy = (Texture::pal8bit *)carObj->render.palCopy;
        sint32 palNum = carObj->render.palNum;
        Texture::CarColor = newColorIndex + (((uint8)carObj->render.upgradeFlags & 1) << 3);
        Texture::ProcessPaletteCopy(palCopy, 1, palNum);
    }
    else
    {
        Texture::pal8bit *palCopy = (Texture::pal8bit *)carObj->render.palCopy;
        sint32 palNum = carObj->render.palNum;
        Texture::CarColor = newColorIndex;
        Texture::ProcessPaletteCopy(palCopy, 0, palNum);
    }
    carObj->render.colorIndex = (sint16)color;
}

void R3DCar::StartUp(void)
{
    FUNCTION_MARKER(0x800ADD14, "NFS4.EXE");
    for (sint32 i = 0; i < 50; ++i)
    {
        LoadedScenePointer[1][i] = 0;
        LoadedScenePointer[0][i] = 0;
        LoadedSceneCounter[1][i] = 0;
        LoadedSceneCounter[0][i] = 0;
        LoadedSceneCountry[i] = (char)0xFF;
        LoadedSceneColor[1][i] = -1;
        LoadedSceneColor[0][i] = -1;
    }
    orientMat = static_cast<matrixtdef *>(reservememadr("orientMat", 0x804, 0));
    position = static_cast<coorddef *>(reservememadr("position", 0x2B8, 0));
    char name[100];
    sprintf(name, "%slicense.psh", Paths_Paths[25]);
    LicenseShapeFile = static_cast<char *>(loadfileadr(name, 0));
}

void R3DCar::PostStartUp(void)
{
    FUNCTION_MARKER(0x800ADDE4, "NFS4.EXE");
    sint32 otSize = 0x400;
    if (InMenu == 0)
        otSize = (sint32)((uint32)Cars::gNumCars * 0x1C0U);
    if (otSize < 0x400)
        otSize = 0x400;
    sint32 size = (sint32)((uint32)otSize << 2);
    subOtStart[0][0] = static_cast<uint32 *>(reservememadr("sub_ot0", size, 0));
    subOtStart[1][0] = static_cast<uint32 *>(reservememadr("sub_ot1", size, 0));
    if (InMenu != 0 || GameSetup::gData.commMode == 1)
    {
        subOtStart[0][1] = static_cast<uint32 *>(reservememadr("sub_ot0m", size, 0));
        subOtStart[1][1] = static_cast<uint32 *>(reservememadr("sub_ot1m", size, 0));
    }
}

void R3DCar::Restart(void)
{
    FUNCTION_MARKER(0x800ADEC8, "NFS4.EXE");
    sint32 count = Cars::gNumCars;
    for (sint32 i = 0; i < count; ++i)
    {
        Car::tObj *carObj = Cars::gList[i];
        carObj->render.headLight = 0;
        carObj->render.brakeLight = 0;
        if (GameSetup::gData.Time != 0)
        {
            carObj->render.headLight = 0x33;
            carObj->render.brakeLight = 2;
        }
        carObj->render.signalLight[0] = 0;
        carObj->render.signalLight[1] = 0;
        carObj->render.damageParts = 0;
    }
}

void R3DCar::CleanUp(void)
{
    FUNCTION_MARKER(0x800ADF40, "NFS4.EXE");
    for (sint32 i = 0; i < 50; ++i)
    {
        if (LoadedScenePointer[0][i] != 0)
        {
            purgememadr(LoadedScenePointer[0][i]);
            LoadedScenePointer[0][i] = 0;
            LoadedSceneCounter[0][i] = 0;
        }
        if (LoadedScenePointer[1][i] != 0)
        {
            purgememadr(LoadedScenePointer[1][i]);
            LoadedScenePointer[1][i] = 0;
            LoadedSceneCounter[1][i] = 0;
        }
    }
    if (orientMat != 0)
        purgememadr(orientMat);
    coorddef *oldPosition = position;
    orientMat = 0;
    if (oldPosition != 0)
        purgememadr(oldPosition);
    char *license = LicenseShapeFile;
    position = 0;
    if (license != 0)
        purgememadr(license);
    uint32 *firstOt = subOtStart[0][0];
    LicenseShapeFile = 0;
    if (firstOt != 0)
        purgememadr(firstOt);
    if (subOtStart[1][0] != 0)
        purgememadr(subOtStart[1][0]);
    subOtStart[1][0] = 0;
    subOtStart[0][0] = 0;
    if (InMenu != 0 || GameSetup::gData.commMode == 1)
    {
        if (subOtStart[0][1] != 0)
            purgememadr(subOtStart[0][1]);
        if (subOtStart[1][1] != 0)
            purgememadr(subOtStart[1][1]);
        subOtStart[1][1] = 0;
        subOtStart[0][1] = 0;
    }
}

Transformer::zScene * R3DCar::ReadInCarData(char *filename, Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800AE0A8, "NFS4.EXE");
    char infilename[15];
    strcpy(infilename, filename);
    strcat(infilename, ".geo");
    sint32 offset = 0x24C;
    char *filestart = locatebig(BigFile, infilename);
    sint32 filesize;
    locatebigentry(BigFile, infilename, 0, 0, &filesize);
    char *rawData = static_cast<char *>(reservememadr(infilename, filesize, 0));
    Transformer::zScene *scene = reinterpret_cast<Transformer::zScene *>(rawData);
    blockmove(filestart, rawData, filesize);
    sint32 carType = carObj->render.currentCarType;
    sint32 eScaleX = EnvMapInfo[carType].eScaleX;
    sint32 eScaleY = EnvMapInfo[carType].eScaleY;
    carObj->render.rideHeight = (sint32)((uint32)EnvMapInfo[carType].rideHeight << 7);
    carObj->render.upgradeHeight = (sint32)((uint32)EnvMapInfo[carType].upgradeHeight << 7);
    for (sint32 i = 0; i < 57; ++i)
    {
        Transformer::zObj *Nobj = reinterpret_cast<Transformer::zObj *>(rawData + offset);
        offset += 28;
        scene->obj[i] = Nobj;
        if (i == 39)
            Nobj->translation.x = (sint32)((uint32)Nobj->translation.x - 0x7AEU);
        if (i == 40)
            Nobj->translation.x = (sint32)((uint32)Nobj->translation.x + 0x7AEU);
        if (Nobj->numVertex != 0)
        {
            Nobj->vertex = reinterpret_cast<COORD16 *>(rawData + offset);
            offset += Nobj->numVertex * 6;
            if ((Nobj->numVertex & 1) != 0)
                offset += 2;
            if ((ObjectInfo[i][1] & 1) != 0 && carType < 28)
            {
                Nobj->Nvertex = reinterpret_cast<COORD16 *>(rawData + offset);
                offset += Nobj->numVertex * 6;
                if ((Nobj->numVertex & 1) != 0)
                    offset += 2;
                sint16 tx = (sint16)((uint32)Nobj->translation.x >> 8);
                sint16 ty = (sint16)((uint32)Nobj->translation.y >> 8);
                sint16 tz = (sint16)((uint32)Nobj->translation.z >> 8);
                for (sint32 j = 0; j < Nobj->numVertex; ++j)
                {
                    VECTOR vt;
                    SVECTOR nm;
                    vt.vx = Nobj->vertex[j].x + tx;
                    vt.vy = Nobj->vertex[j].y + ty;
                    vt.vz = (Nobj->vertex[j].z + tz) >> 2;
                    VectorNormalS(&vt, &nm);
                    if ((ObjectInfo[i][1] & 0x40) != 0)
                    {
                        vt.vx = Nobj->Nvertex[j].x + nm.vx;
                        vt.vy = Nobj->Nvertex[j].y + nm.vy;
                        vt.vz = Nobj->Nvertex[j].z + nm.vz;
                        VectorNormalS(&vt, &nm);
                    }
                    sint16 nm_vx = nm.vx;
                    if (eScaleX == 0)
                        abort();
                    sint16 scaledX = (sint16)(nm_vx / eScaleX);
                    if (eScaleY == 0)
                        abort();
                    sint32 scaledY = nm.vy / eScaleY;
                    nm.vx = (sint16)(nm_vx / 65);
                    nm.vz = (sint16)(nm.vz / 65);
                    nm.vy = (sint16)((uint32)scaledY * (uint32)(sint32)nm.vx);
                    sint32 projectedY = nm.vy / 256;
                    if (scaledX < -63)
                        scaledX = -63;
                    if (scaledX > 63)
                        scaledX = 63;
                    if (projectedY < -63)
                        projectedY = -63;
                    if (projectedY > 63)
                        projectedY = 63;
                    nm.vy = (sint16)(scaledX - projectedY);
                    Nobj->Nvertex[j].x = nm.vx;
                    Nobj->Nvertex[j].y = nm.vy;
                    Nobj->Nvertex[j].z = nm.vz;
                }
            }
        }
        if (Nobj->numFacet != 0)
        {
            Nobj->facet = reinterpret_cast<Transformer::zFacet *>(rawData + offset);
            offset += Nobj->numFacet * 12;
        }
    }
    return scene;
}

void R3DCcar::ReadeMapData(void)
{
    FUNCTION_MARKER(0x800AE5B0, "NFS4.EXE");
    char name[256];
    sprintf(name, "%scarmap.dat", Paths_Paths[25]);
    char *RenderingFileData = static_cast<char *>(loadfileadr(name, 0x10));
    char *ScaneData = RenderingFileData;
    for (sint32 i = 0; i < 28; ++i)
    {
        R3DCar::EnvMapInfo[i].eScaleX = Risk_ReadNextValue(&ScaneData);
        R3DCar::EnvMapInfo[i].eScaleY = Risk_ReadNextValue(&ScaneData);
        R3DCar::EnvMapInfo[i].rideHeight = Risk_ReadNextValue(&ScaneData);
        R3DCar::EnvMapInfo[i].upgradeHeight = Risk_ReadNextValue(&ScaneData);
    }
    purgememadr(RenderingFileData);
}

void R3DCcar::ReadTrackShadow(void)
{
    FUNCTION_MARKER(0x800AE668, "NFS4.EXE");
    char name[256];
    sprintf(name, "%strack.dat", Paths_Paths[25]);
    char *RenderingFileData = static_cast<char *>(loadfileadr(name, 0x10));
    char *ScaneData = RenderingFileData;
    sint32 i = (sint32)(((uint32)GameSetup::gData.track << 2) +
        (uint32)GameSetup::gData.Weather + ((uint32)GameSetup::gData.Time << 1));
    for (; i != -1; i = (sint32)((uint32)i - 1U))
    {
        R3DCar::shadowColour.r = (uint8)Risk_ReadNextValue(&ScaneData);
        R3DCar::shadowColour.g = (uint8)Risk_ReadNextValue(&ScaneData);
        R3DCar::shadowColour.b = (uint8)Risk_ReadNextValue(&ScaneData);
        R3DCar::eMapColour.r = (uint8)Risk_ReadNextValue(&ScaneData);
        R3DCar::eMapColour.g = (uint8)Risk_ReadNextValue(&ScaneData);
        R3DCar::eMapColour.b = (uint8)Risk_ReadNextValue(&ScaneData);
    }
    purgememadr(RenderingFileData);
}

void R3DCar::CalcCarDimensions(Car::tObj *carObj, Transformer::zScene *scene, sint32 carType)
{
    FUNCTION_MARKER(0x800AE750, "NFS4.EXE");
    coorddef minp = { 0x630000, 0x630000, 0x630000 };
    coorddef maxp = { -0x630000, -0x630000, -0x630000 };
    for (sint32 i = 0; i < 57; ++i)
    {
        Transformer::zObj *obj = scene->obj[i];
        if (ObjectInfo[i][3] != 1 || obj->numVertex == 0)
            continue;
        if (i == 0 || i >= 47 || (i == 2 && carType == 28))
        {
            for (sint32 j = 0; j < obj->numVertex; ++j)
            {
                sint32 x = obj->vertex[j].x + obj->translation.x / 256;
                sint32 y = obj->vertex[j].y + obj->translation.y / 256;
                sint32 z = obj->vertex[j].z + obj->translation.z / 256;
                if (x < minp.x) minp.x = x;
                if (y < minp.y) minp.y = y;
                if (z < minp.z) minp.z = z;
                if (x >= maxp.x) maxp.x = x;
                if (y >= maxp.y) maxp.y = y;
                if (z >= maxp.z) maxp.z = z;
            }
        }
        if (i >= 47)
        {
            sint32 minWheelX = 0x630000;
            sint32 minWheelZ = 0x630000;
            sint32 maxWheelX = -0x630000;
            sint32 maxWheelZ = -0x630000;
            for (sint32 j = 0; j < obj->numVertex; ++j)
            {
                sint32 x = obj->vertex[j].x + obj->translation.x / 256;
                sint32 z = obj->vertex[j].z + obj->translation.z / 256;
                if (x < 0) x = -x;
                if (z < 0) z = -z;
                if (x < minWheelX) minWheelX = x;
                if (z < minWheelZ) minWheelZ = z;
                if (x >= maxWheelX) maxWheelX = x;
                if (z >= maxWheelZ) maxWheelZ = z;
            }
            sint32 wheelX = (sint32)((uint32)((minWheelX + maxWheelX + 1) >> 1) << 8);
            sint32 wheelZ = (sint32)((uint32)((minWheelZ + maxWheelZ + 1) >> 1) << 8);
            sint32 wheelWidth = (sint32)(((uint32)(maxWheelX - minWheelX) << 8) + 0xCCC);
            if (i >= 53)
            {
                carObj->N.wheelBackX = wheelX;
                carObj->N.wheelBackZ = wheelZ;
                carObj->N.wheelWidthB = wheelWidth;
            }
            else
            {
                carObj->N.wheelFrontX = wheelX;
                carObj->N.wheelFrontZ = wheelZ;
                carObj->N.wheelWidthF = wheelWidth;
            }
        }
    }
    sint32 x = (sint32)((uint32)maxp.x - (uint32)minp.x);
    sint32 y = (sint32)((uint32)maxp.y - (uint32)minp.y);
    sint32 z = (sint32)((uint32)maxp.z - (uint32)minp.z);
    carObj->N.dimension.x = (sint32)((uint32)(x / 2) << 8);
    carObj->N.dimension.y = (sint32)((uint32)(y / 2) << 8);
    carObj->N.dimension.z = (sint32)((uint32)(z / 2) << 8);
    if (carType == 28)
        carObj->N.dimension.z >>= 1;
    x = carObj->N.dimension.x / 256;
    y = carObj->N.dimension.y / 256;
    sint32 radius = fixedsqrt((sint32)((uint32)x * (uint32)x + (uint32)y * (uint32)y));
    carObj->N.dimensionRadius = radius;
    x = radius / 256;
    z = carObj->N.dimension.z / 256;
    carObj->N.dimensionRadius = fixedsqrt((sint32)((uint32)x * (uint32)x + (uint32)z * (uint32)z));
}

void R3DCar::DeInstantiate3DCar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800AEC78, "NFS4.EXE");
    if (carObj->render.palCopy != 0)
        purgememadr(carObj->render.palCopy);
    carObj->render.palCopy = 0;
}

void R3DCar::DeInstantiate3DCarMenu(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800AECB4, "NFS4.EXE");
    sint32 currentCarType = carObj->render.currentCarType;
    if (currentCarType >= 0)
    {
        sint32 countryFlag = (uint8)carObj->render.currentCountry >> 7;
        uint8 count = (uint8)((uint8)LoadedSceneCounter[countryFlag][currentCarType] - 1U);
        LoadedSceneCounter[countryFlag][currentCarType] = (char)count;
        if (count == 0)
        {
            purgememadr(LoadedScenePointer[countryFlag][currentCarType]);
            LoadedScenePointer[countryFlag][currentCarType] = 0;
        }
        CarIO::ReleaseCarCluts(carObj);
    }
    carObj->render.currentCarType = -1;
    carObj->render.newCarType = (sint16)((uint16)carObj->render.newCarType | 0x80U);
    if (carObj->async_handle != 0)
    {
        sint32 status;
        while ((status = getasyncreadstatus((uint32)carObj->async_handle)) == 0)
            systemtask(0);
        if (status > 0 || status == -1)
        {
            char *bigFile = getasyncreadadr((uint32)carObj->async_handle);
            if (bigFile != 0)
                purgememadr(bigFile);
        }
        carObj->async_handle = 0;
        aSyncLoading = -1;
    }
    if (carObj->render.palCopy != 0)
        purgememadr(carObj->render.palCopy);
    carObj->render.palCopy = 0;
}

void R3DCar::GetFileName(char *infilename, char *filename, const char *suffix)
{
    FUNCTION_MARKER(0x800AEDF4, "NFS4.EXE");
    char workFile[10];
    strcpy(workFile, filename);
    strcat(workFile, suffix);
    strcpy(infilename, workFile);
    strcat(infilename, ".psh");
}

char * R3DCar::LoadFileAdr(char *name)
{
    FUNCTION_MARKER(0x800AEE50, "NFS4.EXE");
    sint32 size = filesize(name);
    static char allocationName[] = "bigfile";
    char *buf = Platform::TempReserveMemory(size, allocationName);
    loadfileatadr(name, buf);
    return buf;
}

void R3DCar::GetCarName(char *filename, sint32 carType, sint32 country)
{
    FUNCTION_MARKER(0x800AEE98, "NFS4.EXE");
    sprintf(filename, "zz%s", GameSetup::gCarNames[carType]);
    if ((uint32)carType - 22U < 6)
    {
        sint32 index = (uint8)CopIndex[carType - 22][country];
        filename[2] = CopCountry[index];
    }
}

void R3DCar::Instantiate3DCar(Car::tObj *carObj, sint32 index)
{
    FUNCTION_MARKER(0x800AEF2C, "NFS4.EXE");
    sint32 carType = carObj->carInfo->carType;
    if ((uint32)carType - 22U >= 6)
        carObj->carInfo->Country = 0;
    if (InMenu == 0)
    {
        GameSetup::tCarData *info = carObj->carInfo;
        carObj->render.colorIndex = (sint16)((uint16)GameSetup::gData.carInfo[index].Colour & 15);
        carObj->render.upgradeFlags = (char)((uint8)info->EngineMods +
            (uint8)info->WeightTransfer * 2 + (uint8)info->GroundEffects * 4);
        if ((uint32)carType - 16U < 3)
            carObj->render.upgradeFlags = 7;
    }
    carObj->render.inside = 0;
    carObj->render.medOnly = 0;
    if (carType < 28)
    {
        if (InMenu != 0)
            carObj->render.inside = 1;
        else if (GameSetup::gData.commMode == 1)
            carObj->render.medOnly = 1;
        else if ((carObj->carFlags & 4) == 0 &&
            (sint32)((uint32)Cars::gNumCars - (uint32)Cars::gNumTrafficCars) > 2)
            carObj->render.medOnly = 1;
        else
            carObj->render.inside = 1;
    }
    sint32 menu = InMenu;
    carObj->render.headLight = 0;
    carObj->render.brakeLight = 0;
    if (menu == 0)
    {
        if (GameSetup::gData.Time != 0)
        {
            carObj->render.headLight = 0x33;
            carObj->render.brakeLight = 2;
        }
    }
    else
        carObj->render.headLight = 0x33;
    GameSetup::tCarData *info = carObj->carInfo;
    carObj->render.newCarType = (sint16)carType;
    carObj->render.currentCarType = (sint16)carType;
    carObj->render.palCopy = 0;
    carObj->render.currentCountry = (char)info->Country;
    if (InMenu != 0)
    {
        uint16 inside = (uint16)carObj->render.inside;
        carObj->render.currentCarType = -1;
        uint16 type = (uint16)carObj->render.newCarType;
        carObj->async_handle = 0;
        carObj->render.inside = (sint16)(inside << 4);
        uint8 country = (uint8)carObj->render.currentCountry;
        carObj->render.newCarType = (sint16)(type | 0x80);
        carObj->render.newCountry = (char)country;
        BigFile = 0;
        aSyncLoading = -1;
        return;
    }
    char filename[10];
    char workFile[10];
    char bigname[100];
    GetCarName(filename, carType, carObj->carInfo->Country);
    strcpy(bigname, Paths_Paths[24]);
    strcat(bigname, filename);
    if ((carObj->render.inside & 1) != 0)
        strcat(bigname, "h");
    else if (carObj->render.medOnly != 0 && carType < 28)
        strcat(bigname, "s");
    strcat(bigname, ".viv");
    BigFile = LoadFileAdr(bigname);
    strcpy(workFile, filename);
    if ((carObj->render.inside & 1) != 0)
        strcat(workFile, "h");
    else if (carObj->render.medOnly != 0 && carType < 28)
        strcat(workFile, "s");
    sint32 reload;
    if (LoadedScenePointer[0][carType] == 0)
    {
        sint16 color = carObj->render.colorIndex;
        Transformer::zScene *scene = ReadInCarData(workFile, carObj);
        reload = 0;
        LoadedScenePointer[0][carType] = scene;
        LoadedSceneCounter[0][carType] = (char)((uint8)LoadedSceneCounter[0][carType] + 1);
        LoadedSceneCountry[carType] = carObj->render.currentCountry;
        LoadedSceneColor[color >> 3][carType] = (sint16)((uint16)color & 8);
    }
    else
    {
        uint8 country = (uint8)carObj->render.currentCountry;
        if ((sint8)LoadedSceneCountry[carType] != country)
        {
            carObj->render.currentCountry = (char)(country | 0x80);
            Transformer::zScene *scene = ReadInCarData(workFile, carObj);
            LoadedScenePointer[1][carType] = scene;
            reload = 0;
            LoadedSceneCounter[1][carType] = (char)((uint8)LoadedSceneCounter[1][carType] + 1);
        }
        else
        {
            sint16 color = carObj->render.colorIndex;
            LoadedSceneCounter[0][carType] = (char)((uint8)LoadedSceneCounter[0][carType] + 1);
            sint32 bank = color >> 3;
            reload = 0;
            if (LoadedSceneColor[bank][carType] == ((uint16)color & 8))
            {
                carObj->render.VRamX = LoadedSceneVRam[bank][carType][0];
                reload = 1;
                carObj->render.VRamY = LoadedSceneVRam[bank][carType][1];
            }
            else
                LoadedSceneColor[bank][carType] = (sint16)((uint16)color & 8);
        }
    }
    CalcCarDimensions(carObj, LoadedScenePointer[(uint8)carObj->render.currentCountry >> 7][carType], carType);
    if (carObj->carInfo->ColourChange != 0)
        carObj->render.palCopy = static_cast<char *>(reservememadr("palCopy", 0x28A0, 0));
    Texture::palNum = 0;
    if (carType < 28)
    {
        char infilenames[4][15];
        char *shpfiles[4];
        sint32 index = 1;
        GetFileName(infilenames[0], filename, (carObj->render.inside & 1) != 0 ? "h" : "");
        if ((carObj->render.inside & 1) != 0)
        {
            GetFileName(infilenames[index], filename, "i");
            index = 2;
        }
        GetFileName(infilenames[index], filename, carObj->render.colorIndex < 8 ? "l" : "d");
        GetFileName(infilenames[index + 1], filename, "g");
        for (sint32 i = 0; i < index + 2; ++i)
            shpfiles[i] = locatebig(BigFile, infilenames[i]);
        index = 1;
        info = carObj->carInfo;
        carObj->render.textureOffsetV = 0;
        carObj->render.textureOffsetU = 0;
        CarIO::CreateLicense(info->license, carType, 0);
        Texture::CarColor = ((uint16)carObj->render.colorIndex & 7) +
            ((uint8)carObj->render.upgradeFlags & 2) * 4;
        CarIO::ReadInCarTextureData(shpfiles[0], carObj, reload | (reload << 1), 0);
        if ((carObj->render.inside & 1) != 0)
        {
            index = 2;
            CarIO::ReadInCarTextureData(shpfiles[1], carObj, 0x19, 0);
        }
        Texture::CarColor = ((uint16)carObj->render.colorIndex & 7) +
            ((uint8)carObj->render.upgradeFlags & 1) * 8;
        CarIO::ReadInCarTextureData(shpfiles[index], carObj, (reload << 1) | 0x91, 0);
        ++index;
        CarIO::CleanUpLicense(0);
        carObj->render.palNum = (sint16)Texture::palNum;
        Texture::CarColor = ((uint16)carObj->render.colorIndex & 7) +
            ((uint8)carObj->render.upgradeFlags & 2) * 4;
        CarIO::UpdateCarTextureData(shpfiles[index], carObj, 0);
    }
    else
    {
        char infilename[15];
        strcpy(infilename, workFile);
        strcat(infilename, ".psh");
        char *shpfile = locatebig(BigFile, infilename);
        Texture::CarColor = (uint16)carObj->render.colorIndex & 7;
        CarIO::ReadInCarTextureData(shpfile, carObj, reload | 0x88, 0);
        carObj->render.palNum = (sint16)Texture::palNum;
    }
    if (reload == 0)
    {
        sint32 bank = carObj->render.colorIndex >> 3;
        LoadedSceneVRam[bank][carType][0] = carObj->render.VRamX;
        LoadedSceneVRam[bank][carType][1] = carObj->render.VRamY;
    }
    char *palCopy = carObj->render.palCopy;
    if (palCopy != 0)
        resizememadr(palCopy, (sint32)((uint32)(sint32)carObj->render.palNum * 0x208U));
    BigFile = 0;
}

sint32 R3DCar::Visibilty(Car::tObj *carObj, DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800AF74C, "NFS4.EXE");
    {
        sint32 player = Vi->player;
        sint32 zoom = (uint8)Camera::gInfo[player].zooming & 3;
        sint32 inCarCam = 0;
        if (Camera::gInfo[player].anchor == &carObj->N && (carObj->carFlags & 4) != 0 &&
            Camera::gInfo[player].inCar != 0)
        {
            if (Camera::GetMode(player) == 0)
                goto notVisible;
            if (Camera::gInfo[Vi->player].inCar != 0 && Camera::GetMode(Vi->player) == 1)
            {
                inCarCam = 1;
                if ((carObj->render.inside & 1) == 0)
                    goto notVisible;
            }
        }
        if (carObj->N.active == 0)
            goto notVisible;
        sint32 maxMax = 0xC0000;
        sint32 maxMid = 0x3C0000;
        carObj->render.detail = 0;
        if (Replay::ReplayMode == 2 || Camera::GetMode(Vi->player) == 14)
        {
            maxMax = 0x240000;
            maxMid = 0x780000;
        }
        if (zoom != 0)
        {
            maxMax *= zoom * 3;
            maxMid *= zoom * 2;
        }
        coorddef car;
        car.x = (sint32)((uint32)carObj->N.position.x - (uint32)Vi->cview.translation.x);
        car.y = (sint32)((uint32)carObj->N.position.y - (uint32)Vi->cview.translation.y);
        car.z = (sint32)((uint32)carObj->N.position.z - (uint32)Vi->cview.translation.z);
        sint32 x = (sint32)((uint32)(car.x / 256) * (uint32)(Vi->cview.mrotation.m[0] / 256) +
            (uint32)(car.y / 256) * (uint32)(Vi->cview.mrotation.m[1] / 256) +
            (uint32)(car.z / 256) * (uint32)(Vi->cview.mrotation.m[2] / 256));
        sint32 carZ = (sint32)((uint32)(car.x / 256) * (uint32)(Vi->cview.mrotation.m[6] / 256) +
            (uint32)(car.y / 256) * (uint32)(Vi->cview.mrotation.m[7] / 256) +
            (uint32)(car.z / 256) * (uint32)(Vi->cview.mrotation.m[8] / 256));
        if ((uint32)carZ + 0x30000U > 0xCB0000U)
            goto notVisible;
        if (maxMax < carZ)
            carObj->render.detail = 1;
        if (maxMid < carZ)
            carObj->render.detail = 2;
        sint32 left = (sint32)((uint32)x - (uint32)carObj->N.dimension.x);
        sint32 farZ = (sint32)((uint32)carZ + (uint32)carObj->N.dimension.z);
        sint32 right = (sint32)((uint32)x + (uint32)carObj->N.dimension.x);
        sint32 nearZ = (sint32)((uint32)carZ - (uint32)carObj->N.dimension.z);
        if (left < 0)
            left = (sint32)(0U - (uint32)left);
        if (farZ < left)
        {
            if (right < 0)
                right = (sint32)(0U - (uint32)right);
            if (farZ < right && nearZ < left && nearZ < right)
                goto notVisible;
        }
        if (inCarCam != 0)
            carObj->render.detail = 3;
        return carZ;
    }
notVisible:
    carObj->render.detail = -1;
    return (sint32)0x80000000U;
}

void R3DCar::TurnHeadLightOn(Car::tObj *carObj, sint32 tail)
{
    FUNCTION_MARKER(0x800AFAF4, "NFS4.EXE");
    carObj->render.headLight = (sint16)((uint16)carObj->render.headLight | 0x11);
    if (tail != 0)
        carObj->render.brakeLight = (sint16)((uint16)carObj->render.brakeLight | 2);
    uint16 lights = (uint16)carObj->render.headLight;
    if ((lights & 8) == 0)
    {
        carObj->render.headLight = (sint16)(lights | 2);
        lights = (uint16)carObj->render.headLight;
    }
    if ((lights & 0x80) == 0)
        carObj->render.headLight = (sint16)(lights | 0x20);
}

void R3DCar::TurnHeadLightOff(Car::tObj *carObj, sint32 head)
{
    FUNCTION_MARKER(0x800AFB54, "NFS4.EXE");
    carObj->render.brakeLight = (sint16)((uint16)carObj->render.brakeLight & 0xFD);
    if (head != 0)
    {
        uint16 lights = (uint16)carObj->render.headLight;
        carObj->render.headLight = (sint16)(lights & 0xEE);
        if ((lights & 8) == 0)
            carObj->render.headLight = (sint16)(lights & 0xEC);
        lights = (uint16)carObj->render.headLight;
        if ((lights & 0x80) == 0)
            carObj->render.headLight = (sint16)(lights & 0xDF);
    }
}

void R3DCar::MATRIX3DT_Copy(sint32 *from, sint32 *to)
{
    FUNCTION_MARKER(0x800AFBA8, "NFS4.EXE");
    for (sint32 i = 8; i != -1; --i)
    {
        sint32 value = *from++;
        *to++ = value;
    }
}

void R3DCar::InsertCarFacet(Car::tObj *carObj, DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800AFBD0, "NFS4.EXE");
    sint32 rideHeight = carObj->render.rideHeight;
    sint32 carType = carObj->render.currentCarType;
    sint32 countryFlag = (uint8)carObj->render.currentCountry >> 7;
    sint32 rightHandDrive = 0;
    if (carType < 28)
    {
        rightHandDrive = carType == -1 ? (sint8)RecessedLight[27] : (sint8)ForceDriveSide[carType];
        if (rightHandDrive < 0)
            rightHandDrive = (((sint32)((uint32)AITune::trackInfo[GameSetup::gData.track].driveSide + 1U)) >> 1) ^ 1;
    }
    if (GameSetup::gData.mirrorTrack != 0)
        rightHandDrive ^= 1;
    R3DCar::rightHandDrive = rightHandDrive;
    bool cop_flag = (uint32)carType - 22U < 6U;
    if (carObj->render.detail < 0)
        return;
    uint16 headLight = (uint16)carObj->render.headLight;
    if ((headLight & 8) == 0 && carObj->N.damage[0] > 0x1E0000)
    {
        if ((headLight & 0x44) == 0)
            carObj->render.headLight = (sint16)(headLight | 0xC);
        else if ((headLight & 2) != 0)
            carObj->render.headLight = (sint16)(headLight | 8);
        headLight = (uint16)carObj->render.headLight;
    }
    if ((headLight & 0x80) == 0 && carObj->N.damage[2] > 0x1E0000)
    {
        if ((headLight & 0x44) == 0)
            carObj->render.headLight = (sint16)(headLight | 0xC0);
        else if ((headLight & 0x20) != 0)
            carObj->render.headLight = (sint16)(headLight | 0x80);
    }
    if (cop_flag)
    {
        if ((carObj->AIFlags & 2) != 0)
        {
            if ((carObj->render.signalLight[0] & 0x80) == 0)
            {
                carObj->render.signalLight[0] = 0x80;
                carObj->render.signalLight[1] = 0x88;
                if ((carObj->control.lights & 2) == 0)
                    TurnHeadLightOn(carObj, 1);
            }
        }
        else if ((carObj->render.signalLight[0] & 0x80) != 0)
        {
            uint8 lights = carObj->control.lights;
            carObj->render.signalLight[0] = 0;
            carObj->render.signalLight[1] = 0;
            if ((lights & 2) == 0)
                TurnHeadLightOff(carObj, (lights & 4) == 0);
        }
    }
    if (Clock != 0)
    {
        if ((carObj->render.signalLight[0] & 0x80) != 0)
            carObj->render.signalLight[0] = (sint16)((carObj->render.signalLight[0] + 1) & 0x8F);
        if ((carObj->render.signalLight[1] & 0x80) != 0)
            carObj->render.signalLight[1] = (sint16)((carObj->render.signalLight[1] + 1) & 0x8F);
    }
    if (carType < 28)
    {
        if ((carObj->render.upgradeFlags & 2) != 0)
            rideHeight = carObj->render.upgradeHeight;
    }
    else
        rideHeight = 0;
    if (carObj->render.detail == 0 && (carType >= 28 || carObj->render.medOnly != 0))
        carObj->render.detail = 1;
    if (carObj->render.detail == 2 && carType == 28)
        carObj->render.detail = 1;
    sint32 detailIndex = carObj->render.detail + 2;
    coorddef car;
    car.x = (sint32)((uint32)carObj->N.position.x - (uint32)Vi->cview.translation.x);
    car.y = (sint32)((uint32)carObj->N.position.y - (uint32)Vi->cview.translation.y);
    car.z = (sint32)((uint32)carObj->N.position.z - (uint32)Vi->cview.translation.z);
    coorddef pos;
    uint32 dotX = (uint32)(car.x / 256) * (uint32)(carObj->N.orientMat.m[0] / 256);
    dotX += (uint32)(car.y / 256) * (uint32)(carObj->N.orientMat.m[1] / 256);
    dotX += (uint32)(car.z / 256) * (uint32)(carObj->N.orientMat.m[2] / 256);
    pos.x = (sint32)dotX;
    uint32 dotZ = (uint32)(car.x / 256) * (uint32)(carObj->N.orientMat.m[6] / 256);
    dotZ += (uint32)(car.y / 256) * (uint32)(carObj->N.orientMat.m[7] / 256);
    dotZ += (uint32)(car.z / 256) * (uint32)(carObj->N.orientMat.m[8] / 256);
    pos.z = (sint32)dotZ;
    yawCam = (sint16)(0x1000U - (uint32)(fixedatan(pos.x, pos.z) >> 4));
    matrixtdef bodyMat;
    matrixtdef orientMat;
    matrixtdef insideMat;
    matrixtdef orientIMat;
    matrixtdef steerMat;
    if (carType < 28)
    {
        MATRIX3DT_Copy(carObj->N.orientMat.m, insideMat.m);
        if (rightHandDrive != 0)
        {
            insideMat.m[0] = (sint32)(0U - (uint32)insideMat.m[0]);
            insideMat.m[2] = (sint32)(0U - (uint32)insideMat.m[2]);
            insideMat.m[1] = (sint32)(0U - (uint32)insideMat.m[1]);
        }
        sint32 roll = carObj->render.bodyRoll;
        if (rightHandDrive != 0)
            roll = (sint32)(0U - (uint32)roll);
        matrixtdef matP;
        fixedxformz(&bodyMat, carObj->render.bodyRoll);
        fixedxformx(&matP, (sint32)((uint32)carObj->render.bodyPitch + 100U));
        Math::fasttransmult(&bodyMat, &matP, &bodyMat);
        Math::fasttransmult(&bodyMat, &carObj->N.orientMat, &bodyMat);
        matrixtdef bodyIMat;
        fixedxformz(&bodyIMat, roll);
        Math::fasttransmult(&bodyIMat, &matP, &bodyIMat);
        Math::fasttransmult(&bodyIMat, &insideMat, &insideMat);
        Math::fasttransmult(&bodyMat, &Vi->cview.mrotationInv, &orientMat);
        Math::fasttransmult(&insideMat, &Vi->cview.mrotationInv, &orientIMat);
    }
    else
    {
        matrixtdef matP;
        fixedxformz(&bodyMat, carObj->render.bodyRoll);
        fixedxformx(&matP, carObj->render.bodyPitch);
        Math::fasttransmult(&bodyMat, &matP, &bodyMat);
        Math::fasttransmult(&bodyMat, &carObj->N.orientMat, &bodyMat);
        Math::fasttransmult(&bodyMat, &Vi->cview.mrotationInv, &orientMat);
    }
    if (simVar.pauseSim == 0 && simVar.quickPauseSim == 0)
    {
        if (carType == 28)
        {
            sint32 rear = carObj->N.wheelRot[1];
            carObj->N.wheelRot[0] = (sint32)(((uint32)carObj->N.wheelRot[0] + 0x1800U) & 0xFFFFU);
            carObj->N.wheelRot[1] = (sint32)(((uint32)rear + 0x1800U) & 0xFFFFU);
        }
        else
        {
            sint32 wheelSpin = carObj->wheelSpin;
            if (wheelSpin < 0)
                wheelSpin = (sint32)(0U - (uint32)wheelSpin);
            bool rear = (uint32)wheelSpin - 1U < 2U;
            sint32 replayMode = Replay::ReplayMode;
            for (sint32 i = 0; i < 2; ++i)
            {
                sint32 vel = carObj->linearVel_ch.z >> (replayMode == 2 ? ((8U - (uint32)Replay::ReplayInterface.speed) & 31U) : 6U);
                if ((i == 0 ? carObj->frontWheelSpin != 0 : rear) && (uint8)carObj->control.gear != 1)
                {
                    sint32 denominator = carObj->specs->velToRpmRatio[(uint8)carObj->control.gear];
                    sint32 numerator = (sint32)((uint32)carObj->flywheelRpm << 16);
                    if (denominator == 0 || (denominator == -1 && (uint32)numerator == 0x80000000U))
                        abort();
                    sint32 spin = numerator / denominator;
                    vel = (sint32)((uint32)spin << (replayMode == 2 ? (((uint32)Replay::ReplayInterface.speed + 7U) & 31U) : 9U));
                }
                if (carObj->wheelLock != 0 && ((uint32)carObj->wheelLock & (uint32)(i + 1)) != 0)
                    vel = 0;
                if (vel < -0x1800)
                    vel = -0x1800;
                if (vel > 0x1800)
                    vel = 0x1800;
                carObj->N.wheelRot[i] = (sint32)(((uint32)carObj->N.wheelRot[i] + (uint32)vel) & 0xFFFFU);
            }
        }
    }
    sint32 steeringAngle = carObj->control.steering;
    if (carObj->linearVel_ch.z > 0x1E0000)
        steeringAngle = fixeddiv((sint32)((uint32)steeringAngle << 16), fixedmult(carObj->linearVel_ch.z, 0x888)) / 65536;
    fixedxformy(&steerMat, (sint32)((uint32)steeringAngle << 5));
    if (carObj->control.desiredBrakeLevel != 0 && carObj->control.hanno == 0)
        carObj->render.brakeLight = (sint16)((uint16)carObj->render.brakeLight | 1U);
    else
        carObj->render.brakeLight = (sint16)((uint16)carObj->render.brakeLight & 0xFEU);
    for (sint32 i = 0; i < 57; ++i)
    {
        sint32 code = (sint8)ObjectInfo[i][detailIndex];
        if (carType < 28)
        {
            bool signals = (carObj->render.signalLight[0] & 0x80) != 0 || (carObj->render.signalLight[1] & 0x80) != 0;
            switch (code)
            {
            case 2: case 9: if ((carObj->render.inside & 1) != 0) code = 0; break;
            case 3: case 4: if (carObj->render.inside == 0) code = 0; break;
            case 5: if ((carObj->render.damageParts & 1) != 0) code = 0; break;
            case 6: if ((carObj->render.damageParts & 2) != 0) code = 0; break;
            case 7: if ((carObj->render.headLight & 2) == 0) code = 0; break;
            case 8: if ((carObj->render.headLight & 0x20) == 0) code = 0; break;
            case 11: if ((carObj->render.upgradeFlags & 4) != 0) code = 0; break;
            case 12: if ((carObj->render.upgradeFlags & 4) == 0) code = 0; break;
            case 13: if ((carObj->render.upgradeFlags & 1) != 0) code = 0; break;
            case 14: if ((carObj->render.upgradeFlags & 1) == 0) code = 0; break;
            case 15: if (cop_flag && (carObj->render.damageParts & 4) != 0) code = 0; break;
            case 16: if (!cop_flag || (carObj->render.damageParts & 4) != 0) code = 0; break;
            case 17: code = 0; break;
            case 18: if (carObj->render.brakeLight == 0 && (!cop_flag || (carObj->AIFlags & 2) == 0)) code = 0; break;
            case 19: if (carObj->control.gear != 0 && (!cop_flag || (carObj->AIFlags & 2) == 0)) code = 0; break;
            case 20: if (!signals || (cop_flag && (carObj->render.damageParts & 4) != 0)) code = 0; break;
            case 21:
                if (cop_flag)
                {
                    if (!signals || (carObj->render.damageParts & 4) != 0) code = 0;
                }
                else if (!signals && (SignalBrakeFlare[carType] == 0 || carObj->render.brakeLight == 0))
                    code = 0;
                break;
            case 22: if ((carObj->render.headLight & 0x11) == 0) code = 0; break;
            }
            if ((i == 6 || i == 7) && (carObj->render.damageParts & 0x18) != 0)
                code = 0;
            else if ((i == 8 || i == 9) && (carObj->render.damageParts & 4) != 0)
                code = (DamageSpoiler[carType] & ((carObj->render.upgradeFlags & 4) != 0 ? 1 : 0x80)) != 0 ? 11 : 0;
            else if ((i == 10 || i == 11) && (carObj->render.damageParts & 4) != 0)
                code = 0;
        }
        else
        {
            if (code == 18)
            {
                if (carObj->render.brakeLight == 0) code = 0;
            }
            else if (code == 22)
            {
                if ((carObj->render.headLight & 0x11) == 0) code = 0;
            }
            else if (code != 1)
                code = 0;
            if (carType == 28 && i == 32)
                code = 1;
        }
        ObjectVisible[i] = (char)code;
    }
    coorddef parent = LoadedScenePointer[countryFlag][carType]->obj[0]->translation;
    for (sint32 i = 0; i < 57; ++i)
    {
        Transformer::zObj *obj = LoadedScenePointer[countryFlag][carType]->obj[i];
        if (obj->numFacet == 0 || ObjectVisible[i] == 0)
            continue;
        sint32 suspensionOffset;
        if (i < 47)
            suspensionOffset = (sint32)((uint32)rideHeight - (uint32)carObj->render.currentHeight);
        else
        {
            suspensionOffset = carObj->wheel[Suspension[i - 47]].impactCompression;
            if (suspensionOffset < -0x1EB8) suspensionOffset = -0x1EB8;
            if (suspensionOffset > 0x1EB8) suspensionOffset = 0x1EB8;
        }
        coorddef translation;
        translation.x = (sint32)((uint32)obj->translation.x - (uint32)parent.x);
        translation.y = (sint32)((uint32)obj->translation.y - (uint32)parent.y - (uint32)suspensionOffset);
        translation.z = (sint32)((uint32)obj->translation.z - (uint32)parent.z);
        matrixtdef *matrix = i >= 47 ? &carObj->N.orientMat : (carType < 28 && i >= 35 && i <= 40 ? &insideMat : &bodyMat);
        coorddef tmp;
        transform(&translation, matrix, &tmp);
        translation.x = (sint32)((uint32)carObj->N.position.x + (uint32)tmp.x - (uint32)Vi->cview.translation.x);
        translation.y = (sint32)((uint32)carObj->N.position.y + (uint32)tmp.y - (uint32)Vi->cview.translation.y);
        translation.z = (sint32)((uint32)carObj->N.position.z + (uint32)tmp.z - (uint32)Vi->cview.translation.z);
        transform(&translation, &Vi->cview.mrotationInv, &position[i]);
        matrixtdef tmpMat;
        if (carType == 28 && (i == 31 || i == 35))
        {
            if (i == 31) fixedxformy(&tmpMat, carObj->N.wheelRot[0]);
            else fixedxformx(&tmpMat, carObj->N.wheelRot[1]);
            Math::fasttransmult(&tmpMat, &bodyMat, &tmpMat);
            Math::fasttransmult(&tmpMat, &Vi->cview.mrotationInv, &R3DCar::orientMat[i]);
            continue;
        }
        bool compose = false;
        matrix = &orientMat;
        if (carType != 28)
        {
            switch (i)
            {
            case 35: case 36: matrix = &orientIMat; break;
            case 37:
            {
                sint32 angle = carObj->control.steering;
                if (rightHandDrive != 0) angle = (sint32)(0U - (uint32)angle);
                fixedxformz(&tmpMat, (sint32)((uint32)angle * (uint32)-56));
                matrix = &insideMat; compose = true; break;
            }
            case 38:
            {
                sint32 roll = (sint32)((uint32)carObj->render.bodyRoll * 3U) >> 1;
                sint32 pitch = (sint32)((uint32)carObj->render.bodyPitch << 3);
                if (rightHandDrive != 0) roll = (sint32)(0U - (uint32)roll);
                matrixtdef matR, matP;
                fixedxformz(&matR, roll);
                fixedxformx(&matP, pitch);
                Math::fasttransmult(&matR, &matP, &tmpMat);
                matrix = &insideMat; compose = true; break;
            }
            case 39: case 40:
            {
                sint32 angle = carObj->control.steering;
                if (rightHandDrive != 0) angle = (sint32)(0U - (uint32)angle);
                sint32 maxAngleFactor = i == 39 ? 12 : -12;
                matrixtdef matX, matY;
                fixedxformx(&matX, (sint32)((uint32)angle * (uint32)maxAngleFactor));
                if (angle > 0) angle = (sint32)(0U - (uint32)angle);
                fixedxformy(&matY, (sint32)((uint32)angle * (uint32)(maxAngleFactor >> 1)));
                Math::fasttransmult(&matX, &matY, &tmpMat);
                matrix = &insideMat; compose = true; break;
            }
            case 47:
                fixedxformx(&tmpMat, carObj->N.wheelRot[0]);
                Math::fasttransmult(&tmpMat, &steerMat, &tmpMat);
                matrix = &carObj->N.orientMat; compose = true; break;
            case 48: case 49: case 50: case 51: case 52: matrix = &R3DCar::orientMat[47]; break;
            case 53: case 55:
                fixedxformx(&tmpMat, carObj->N.wheelRot[1]);
                matrix = &carObj->N.orientMat; compose = true; break;
            case 54: matrix = &R3DCar::orientMat[53]; break;
            case 56: matrix = &R3DCar::orientMat[55]; break;
            }
        }
        if (compose)
        {
            Math::fasttransmult(&tmpMat, matrix, &tmpMat);
            Math::fasttransmult(&tmpMat, &Vi->cview.mrotationInv, &R3DCar::orientMat[i]);
        }
        else
            MATRIX3DT_Copy(matrix->m, R3DCar::orientMat[i].m);
    }
    TrsProj::TransformProjectVertex(&Vi->cview.mrotationInv, &Vi->cview.translationInv, 1, &carObj->N.position, &center);
    if (shadowColour.r != 0 && carObj->N.simOptz == 0)
    {
        Newton::CalcRealShadowCoordinates(carObj, simGlobal.gameTicks);
        TrsProj::TransformProjectVertex(&Vi->cview.mrotationInv, &Vi->cview.translationInv, 4, carObj->N.shadowCoord, shadowVertex);
        shadowFlag = 1;
    }
    else
        shadowFlag = 0;
    if (simVar.pauseSim == 0 && simVar.quickPauseSim == 0)
    {
        sint32 delta = Replay::ReplayMode == 2 ? carObj->linearVel_ch.z >> ((18U - (uint32)Replay::ReplayInterface.speed) & 31U) : carObj->linearVel_ch.z >> 16;
        carObj->N.positionXZ = (sint16)((uint32)(uint16)carObj->N.positionXZ + (uint32)delta);
    }
    sint32 i = 0;
    if (DrawC::gEnvMapMax > 0)
        while (i < DrawC::gEnvMapMax && carObj->N.simRoadInfo.slice >= DrawC::gEnvMap[i].slice) ++i;
    carObj->N.eIndexEnvMap = (sint16)i;
    i = 0;
    if (DrawC::gShadowMax > 0)
        while (i < DrawC::gShadowMax && carObj->N.simRoadInfo.slice >= DrawC::gShadow[i].slice) ++i;
    carObj->N.eIndexShadow = (sint16)i;
}

void R3DCar::ReadInCarTextureMenu(Car::tObj *carObj, char *bigfile, sint32 reload, sint32 player)
{
    FUNCTION_MARKER(0x800B0DB0, "NFS4.EXE");
    char filename[10];
    sint32 carType = carObj->render.currentCarType;
    GetCarName(filename, carType, carObj->carInfo->Country);
    if (reload != 0)
        CarIO::ReleaseCarCluts(carObj);
    if (carObj->render.palCopy == 0)
        carObj->render.palCopy = static_cast<char *>(reservememadr("palCopy", 0x28A0, 0));
    DrawSync(0);
    Texture::palNum = 0;
    if (carType < 28)
    {
        char infilenames[3][15];
        char *shpfiles[3];
        sint32 index = 1;
        GetFileName(infilenames[0], filename, (carObj->render.inside & 1) != 0 ? "h" : "");
        if ((carObj->render.inside & 1) != 0)
        {
            GetFileName(infilenames[index], filename, "i");
            index = 2;
        }
        GetFileName(infilenames[index], filename, carObj->render.colorIndex < 8 ? "l" : "d");
        for (sint32 i = 0; i < index + 1; ++i)
            shpfiles[i] = locatebig(bigfile, infilenames[i]);
        carObj->render.textureOffsetV = 0;
        carObj->render.textureOffsetU = 0;
        if (reload != 0)
            reload = 0x11;
        index = 1;
        CarIO::ReadInCarTextureData(shpfiles[0], carObj, reload, player);
        if ((carObj->render.inside & 1) != 0)
        {
            index = 2;
            CarIO::ReadInCarTextureData(shpfiles[1], carObj, 0x19, player);
        }
        if (reload == 0)
            reload = 0x91;
        CarIO::ReadInCarTextureData(shpfiles[index], carObj, reload, player);
        carObj->render.palNum = (sint16)Texture::palNum;
        DrawSync(0);
        Texture::CarColor = ((uint16)carObj->render.colorIndex & 7) +
            ((uint8)carObj->render.upgradeFlags & 2) * 4;
        Texture::ProcessPaletteCopy(reinterpret_cast<Texture::pal8bit *>(carObj->render.palCopy), 0, 1);
        Texture::pal8bit *palCopy = reinterpret_cast<Texture::pal8bit *>(carObj->render.palCopy);
        sint16 palNum = carObj->render.palNum;
        Texture::CarColor = ((uint16)carObj->render.colorIndex & 7) +
            ((uint8)carObj->render.upgradeFlags & 1) * 8;
        Texture::ProcessPaletteCopy(palCopy, 1, palNum);
    }
    else
    {
        char infilename[15];
        strcpy(infilename, filename);
        strcat(infilename, ".psh");
        char *shpfile = locatebig(bigfile, infilename);
        carObj->render.textureOffsetV = 0;
        carObj->render.textureOffsetU = 0;
        CarIO::ReadInCarTextureData(shpfile, carObj, reload == 0 ? 0x80 : 0x11, 0);
        carObj->render.palNum = (sint16)Texture::palNum;
        DrawSync(0);
        Texture::pal8bit *palCopy = reinterpret_cast<Texture::pal8bit *>(carObj->render.palCopy);
        sint16 palNum = carObj->render.palNum;
        Texture::CarColor = (uint16)carObj->render.colorIndex & 7;
        Texture::ProcessPaletteCopy(palCopy, 0, palNum);
    }
    purgememadr(bigfile);
}

void R3DCar::InsertCarFacetMenu(Car::tObj *carObj, DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800B10FC, "NFS4.EXE");
    sint32 rideHeight = carObj->render.rideHeight;
    sint32 carType = carObj->render.currentCarType;
    sint32 countryFlag = (uint8)carObj->render.currentCountry >> 7;
    sint32 rightHandDrive = 0;
    bool changeCar = false;
    if (carType < 28)
    {
        rightHandDrive = carType == -1 ? (sint8)RecessedLight[27] : (sint8)ForceDriveSide[carType];
        if (rightHandDrive < 0)
            rightHandDrive = (((sint32)((uint32)AITune::trackInfo[GameSetup::gData.track].driveSide + 1U)) >> 1) ^ 1;
    }
    carType = carObj->carInfo->carType;
    R3DCar::rightHandDrive = rightHandDrive;
    bool cop_flag = (uint32)carType - 22U < 6U;
    if (!cop_flag)
        carObj->carInfo->Country = 0;
    carObj->render.sub_ot = subOtStart[gFlip][(InMenu & 0x80) != 0] + ((uint32)carObj->N.objID & 15U) * 512U;
    if ((InMenu & 0x80) == 0)
    {
        do
        {
            carObj->render.sub_otSize = 512;
            carObj->render.sub_otOffset = 256;
            carObj->render.sort_carObj = 0;
            carObj->render.sort_flag = 0;
            sint32 loading = aSyncLoading;
            DrawC::gScreenMat.m[0][0] = 0x1000;
            DrawC::gScreenMat.m[1][0] = 0;
            DrawC::gScreenMat.m[2][0] = 0;
            DrawC::gScreenMat.m[0][1] = 0;
            DrawC::gScreenMat.m[1][1] = -0x1000;
            DrawC::gScreenMat.m[2][1] = 0;
            DrawC::gScreenMat.m[0][2] = 0;
            DrawC::gScreenMat.m[1][2] = 0;
            DrawC::gScreenMat.m[2][2] = 0x1000;
            DrawC::gScreenMat.t[0] = 0;
            DrawC::gScreenMat.t[1] = 0;
            DrawC::gScreenMat.t[2] = 0;
            carObj->render.detail = 0;
            if (loading >= 0 && loading != Vi->player)
                break;
            if (carObj->async_handle == 0)
            {
                if ((uint8)carObj->render.newCountry != carObj->carInfo->Country)
                    carObj->render.newCarType = (sint16)((uint16)carObj->render.newCarType | 0x80U);
                if (carObj->async_handle == 0 && carObj->render.newCarType != carType)
                {
                    sint32 buffered = AudioMus::Buffered();
                    sint32 threshold = AudioMus::Threshold();
                    if (buffered >= threshold)
                    {
                        if (carType < 28)
                            carObj->render.inside = (sint16)((uint16)carObj->render.inside | 0x10U);
                        else
                            carObj->render.inside = (sint16)((uint16)carObj->render.inside & 0xEFU);
                        char filename[10];
                        char bigname[100];
                        sprintf(filename, "zz%s", GameSetup::gCarNames[carType]);
                        if (cop_flag)
                        {
                            sint32 index = (uint8)CopIndex[carType - 22][carObj->carInfo->Country];
                            filename[2] = CopCountry[index];
                        }
                        strcpy(bigname, Paths_Paths[24]);
                        strcat(bigname, filename);
                        if ((carObj->render.inside & 0x10) != 0)
                            strcat(bigname, "h");
                        strcat(bigname, ".viv");
                        carObj->async_handle = (sint32)asyncloadfile(bigname, 0x10);
                        aSyncLoading = Vi->player;
                        carObj->render.newCarType = (sint16)carType;
                        carObj->render.newCountry = (char)carObj->carInfo->Country;
                    }
                }
            }
            if (carObj->async_handle != 0)
            {
                sint32 status = getasyncreadstatus((uint32)carObj->async_handle);
                if (status > 0 || status == -1)
                {
                    if (carObj->render.newCarType == carType && (uint8)carObj->render.newCountry == carObj->carInfo->Country)
                    {
                        BigFile = getasyncreadadr((uint32)carObj->async_handle);
                        if (BigFile == 0)
                            break;
                        carObj->async_handle = 0;
                        if (status == -1)
                        {
                            purgememadr(BigFile);
                            BigFile = 0;
                            carObj->render.newCarType = (sint16)((uint16)carObj->render.newCarType | 0x80U);
                        }
                        else
                            aSyncLoading = -1;
                    }
                    else
                    {
                        char *bigFile = getasyncreadadr((uint32)carObj->async_handle);
                        if (bigFile != 0)
                        {
                            purgememadr(bigFile);
                            carObj->async_handle = 0;
                            carObj->render.newCarType = (sint16)((uint16)carObj->render.newCarType | 0x80U);
                        }
                    }
                }
                else if (status == -2)
                {
                    carObj->async_handle = 0;
                    carObj->render.newCarType = (sint16)((uint16)carObj->render.newCarType | 0x80U);
                }
            }
            if (BigFile != 0)
            {
                sint32 currentCarType = carObj->render.currentCarType;
                sint32 reload = 0;
                if (currentCarType >= 0)
                {
                    uint8 count = (uint8)((uint8)LoadedSceneCounter[countryFlag][currentCarType] - 1U);
                    LoadedSceneCounter[countryFlag][currentCarType] = (char)count;
                    reload = 1;
                    if (count == 0)
                    {
                        purgememadr(LoadedScenePointer[countryFlag][currentCarType]);
                        LoadedScenePointer[countryFlag][currentCarType] = 0;
                    }
                }
                carObj->render.currentCarType = carObj->render.newCarType;
                carObj->render.currentCountry = carObj->render.newCountry;
                carType = carObj->render.currentCarType;
                if (Vi->player != 0)
                    carObj->render.currentCountry = (char)((uint8)carObj->render.currentCountry | 0x80U);
                countryFlag = (uint8)carObj->render.currentCountry >> 7;
                carObj->render.inside = (sint16)(carObj->render.inside >> 4);
                char filename[10];
                char workFile[10];
                sprintf(filename, "zz%s", GameSetup::gCarNames[carType]);
                if (cop_flag)
                {
                    sint32 index = (uint8)CopIndex[carType - 22][(uint8)carObj->render.currentCountry & 0x7F];
                    filename[2] = CopCountry[index];
                }
                strcpy(workFile, filename);
                if ((carObj->render.inside & 1) != 0)
                    strcat(workFile, "h");
                if (LoadedScenePointer[countryFlag][carType] != 0)
                {
                    purgememadr(LoadedScenePointer[countryFlag][carType]);
                    LoadedScenePointer[countryFlag][carType] = 0;
                }
                LoadedScenePointer[countryFlag][carType] = ReadInCarData(workFile, carObj);
                LoadedSceneCounter[countryFlag][carType] = (char)((uint8)LoadedSceneCounter[countryFlag][carType] + 1U);
                CalcCarDimensions(carObj, LoadedScenePointer[countryFlag][carType], carType);
                ReadInCarTextureMenu(carObj, BigFile, reload, Vi->player);
                BigFile = 0;
                changeCar = true;
            }
        } while (false);
    }
    carType = carObj->render.currentCarType;
    countryFlag = (uint8)carObj->render.currentCountry >> 7;
    cop_flag = (uint32)carType - 22U < 6U;
    if ((InMenu & 0x80) == 0 && carType < 0)
        carObj->render.detail = -1;
    if (carObj->render.detail < 0)
        return;
    if ((InMenu & 0x80) == 0)
    {
        if (cop_flag)
        {
            if ((carObj->render.signalLight[0] & 0x80) == 0)
            {
                carObj->render.signalLight[0] = 0x80;
                carObj->render.signalLight[1] = 0x88;
            }
            if (Clock != 0)
            {
                if ((carObj->render.signalLight[0] & 0x80) != 0)
                    carObj->render.signalLight[0] = (sint16)((carObj->render.signalLight[0] + 1) & 0x8F);
                if ((carObj->render.signalLight[1] & 0x80) != 0)
                    carObj->render.signalLight[1] = (sint16)((carObj->render.signalLight[1] + 1) & 0x8F);
            }
            if (changeCar)
            {
                carObj->render.headLight = 0x33;
                carObj->render.brakeLight = 2;
            }
        }
        else if (changeCar)
            carObj->render.brakeLight = 0;
    }
    if (carType < 28)
    {
        if ((carObj->render.upgradeFlags & 2) != 0)
            rideHeight = carObj->render.upgradeHeight;
    }
    else
        rideHeight = 0;
    if (carObj->render.detail == 0 && carType >= 28)
        carObj->render.detail = 1;
    if (carObj->render.detail == 2 && carType == 28)
        carObj->render.detail = 1;
    sint32 detailIndex = carObj->render.detail + 2;
    if ((InMenu & 0x80) == 0)
        carObj->N.position.y = (sint32)((uint32)carObj->N.position.y + (uint32)carObj->N.dimension.y);
    else
        carObj->N.position.y = (sint32)((uint32)carObj->N.position.y - ((uint32)carObj->N.dimension.y << 1));
    coorddef car;
    car.x = (sint32)((uint32)carObj->N.position.x - (uint32)Vi->cview.translation.x);
    car.y = (sint32)((uint32)carObj->N.position.y - (uint32)Vi->cview.translation.y);
    car.z = (sint32)((uint32)carObj->N.position.z - (uint32)Vi->cview.translation.z);
    coorddef pos;
    uint32 dotX = (uint32)(car.x / 256) * (uint32)(carObj->N.orientMat.m[0] / 256);
    dotX += (uint32)(car.y / 256) * (uint32)(carObj->N.orientMat.m[1] / 256);
    dotX += (uint32)(car.z / 256) * (uint32)(carObj->N.orientMat.m[2] / 256);
    pos.x = (sint32)dotX;
    uint32 dotZ = (uint32)(car.x / 256) * (uint32)(carObj->N.orientMat.m[6] / 256);
    dotZ += (uint32)(car.y / 256) * (uint32)(carObj->N.orientMat.m[7] / 256);
    dotZ += (uint32)(car.z / 256) * (uint32)(carObj->N.orientMat.m[8] / 256);
    pos.z = (sint32)dotZ;
    yawCam = (sint16)(0x1000U - (uint32)(fixedatan(pos.x, pos.z) / 16));
    matrixtdef bodyMat, orientMat, insideMat, orientIMat;
    MATRIX3DT_Copy(carObj->N.orientMat.m, bodyMat.m);
    MATRIX3DT_Copy(carObj->N.orientMat.m, insideMat.m);
    if ((InMenu & 0x80) == 0)
    {
        if (rightHandDrive != 0)
        {
            insideMat.m[0] = (sint32)(0U - (uint32)insideMat.m[0]);
            insideMat.m[2] = (sint32)(0U - (uint32)insideMat.m[2]);
            insideMat.m[1] = (sint32)(0U - (uint32)insideMat.m[1]);
        }
    }
    else
    {
        bodyMat.m[3] = (sint32)(0U - (uint32)bodyMat.m[3]);
        bodyMat.m[5] = (sint32)(0U - (uint32)bodyMat.m[5]);
        bodyMat.m[4] = (sint32)(0U - (uint32)bodyMat.m[4]);
    }
    Math::fasttransmult(&bodyMat, &Vi->cview.mrotationInv, &orientMat);
    Math::fasttransmult(&insideMat, &Vi->cview.mrotationInv, &orientIMat);
    for (sint32 i = 0; i < 57; ++i)
    {
        sint32 code = (sint8)ObjectInfo[i][detailIndex];
        if (carType < 28)
        {
            switch (code)
            {
            case 2: if ((carObj->render.inside & 1) != 0 || (InMenu & 0x80) != 0) code = 0; break;
            case 3: if ((InMenu & 0x80) != 0) code = 0; break;
            case 4: case 10: code = 0; break;
            case 7: if ((carObj->render.headLight & 2) == 0) code = 0; break;
            case 8: if ((carObj->render.headLight & 0x20) == 0) code = 0; break;
            case 9: if ((carObj->render.inside & 1) != 0) code = 0; break;
            case 11: if ((carObj->render.upgradeFlags & 4) != 0) code = 0; break;
            case 12: if ((carObj->render.upgradeFlags & 4) == 0) code = 0; break;
            case 13: if ((carObj->render.upgradeFlags & 1) != 0) code = 0; break;
            case 14: if ((carObj->render.upgradeFlags & 1) == 0) code = 0; break;
            case 16: case 19: if (!cop_flag) code = 0; break;
            case 22: if ((carObj->render.headLight & 0x11) == 0) code = 0; break;
            }
        }
        else
        {
            if (code != 1 && code != 18 && code != 22)
                code = 0;
            if (carType == 28 && i == 32)
                code = 1;
        }
        ObjectVisible[i] = (char)code;
    }
    coorddef parent = LoadedScenePointer[countryFlag][carType]->obj[0]->translation;
    for (sint32 i = 0; i < 57; ++i)
    {
        Transformer::zObj *obj = LoadedScenePointer[countryFlag][carType]->obj[i];
        if (obj->numFacet == 0 || ObjectVisible[i] == 0)
            continue;
        sint32 suspensionOffset = i == 15 || i >= 47 ? 0 : rideHeight;
        coorddef translation;
        translation.x = (sint32)((uint32)obj->translation.x - (uint32)parent.x);
        translation.y = (sint32)((uint32)obj->translation.y - (uint32)parent.y - (uint32)suspensionOffset);
        translation.z = (sint32)((uint32)obj->translation.z - (uint32)parent.z);
        bool inside = carType < 28 && i >= 35 && i <= 40;
        coorddef tmp;
        transform(&translation, inside ? &insideMat : &bodyMat, &tmp);
        translation.x = (sint32)((uint32)carObj->N.position.x + (uint32)tmp.x - (uint32)Vi->cview.translation.x);
        translation.y = (sint32)((uint32)carObj->N.position.y + (uint32)tmp.y - (uint32)Vi->cview.translation.y);
        translation.z = (sint32)((uint32)carObj->N.position.z + (uint32)tmp.z - (uint32)Vi->cview.translation.z);
        transform(&translation, &Vi->cview.mrotationInv, &position[i]);
        MATRIX3DT_Copy(inside ? orientIMat.m : orientMat.m, R3DCar::orientMat[i].m);
    }
    TrsProj::TransformProjectVertex(&Vi->cview.mrotationInv, &Vi->cview.translationInv, 1, &carObj->N.position, &center);
    if ((InMenu & 0x80) == 0)
    {
        coorddef dimension = carObj->N.dimension;
        coorddef lengthVector, widthVector, frontWidthVector;
        lengthVector.x = fixedmult(dimension.z, carObj->N.orientMat.m[6]);
        lengthVector.y = fixedmult(dimension.z, carObj->N.orientMat.m[7]);
        lengthVector.z = fixedmult(dimension.z, carObj->N.orientMat.m[8]);
        widthVector.x = fixedmult(dimension.x, carObj->N.orientMat.m[0]);
        widthVector.y = fixedmult(dimension.x, carObj->N.orientMat.m[1]);
        widthVector.z = fixedmult(dimension.x, carObj->N.orientMat.m[2]);
        if (carObj->render.currentCarType == 20)
        {
            sint32 front = (sint32)((uint32)dimension.x * 192U) >> 8;
            frontWidthVector.x = fixedmult(front, carObj->N.orientMat.m[0]);
            frontWidthVector.y = fixedmult(front, carObj->N.orientMat.m[1]);
            frontWidthVector.z = fixedmult(front, carObj->N.orientMat.m[2]);
        }
        else
            frontWidthVector = widthVector;
        coorddef temp1;
        temp1.x = (sint32)((uint32)carObj->N.position.x + (uint32)lengthVector.x);
        temp1.y = lengthVector.y;
        temp1.z = (sint32)((uint32)carObj->N.position.z + (uint32)lengthVector.z);
        carObj->N.shadowCoord[0].x = (sint32)((uint32)temp1.x - (uint32)frontWidthVector.x);
        carObj->N.shadowCoord[0].y = (sint32)((uint32)temp1.y - (uint32)frontWidthVector.y);
        carObj->N.shadowCoord[0].z = (sint32)((uint32)temp1.z - (uint32)frontWidthVector.z);
        carObj->N.shadowCoord[1].x = (sint32)((uint32)temp1.x + (uint32)frontWidthVector.x);
        carObj->N.shadowCoord[1].y = (sint32)((uint32)temp1.y + (uint32)frontWidthVector.y);
        carObj->N.shadowCoord[1].z = (sint32)((uint32)temp1.z + (uint32)frontWidthVector.z);
        coorddef temp2;
        temp2.x = (sint32)((uint32)carObj->N.position.x - (uint32)lengthVector.x);
        temp2.y = (sint32)(0U - (uint32)lengthVector.y);
        temp2.z = (sint32)((uint32)carObj->N.position.z - (uint32)lengthVector.z);
        carObj->N.shadowCoord[2].x = (sint32)((uint32)temp2.x - (uint32)widthVector.x);
        carObj->N.shadowCoord[2].y = (sint32)((uint32)temp2.y - (uint32)widthVector.y);
        carObj->N.shadowCoord[2].z = (sint32)((uint32)temp2.z - (uint32)widthVector.z);
        carObj->N.shadowCoord[3].x = (sint32)((uint32)temp2.x + (uint32)widthVector.x);
        carObj->N.shadowCoord[3].y = (sint32)((uint32)temp2.y + (uint32)widthVector.y);
        carObj->N.shadowCoord[3].z = (sint32)((uint32)temp2.z + (uint32)widthVector.z);
        TrsProj::TransformProjectVertex(&Vi->cview.mrotationInv, &Vi->cview.translationInv, 4, carObj->N.shadowCoord, shadowVertex);
    }
}

void R3DCar::InsertCarFacetII(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800B2174, "NFS4.EXE");
    sint32 carType = carObj->render.currentCarType;
    sint32 countryFlag = (uint8)carObj->render.currentCountry >> 7;
    sint32 rightHandDrive = R3DCar::rightHandDrive;
    Transformer::zOverlay *overlay = LoadedScenePointer[countryFlag][carType]->overlay;
    sint32 reflect = 0;
    sint32 inAir = carObj->wheel[0].wheelInAir | carObj->wheel[1].wheelInAir |
        carObj->wheel[2].wheelInAir | carObj->wheel[3].wheelInAir;
    if (inAir == 0)
    {
        if (GameSetup::gData.Time == 0)
            reflect = DrawC::gWetRoad != 0;
        else if (GameSetup::gData.commMode == 1)
            reflect = DrawC::gWetRoad != 0 ? 1 : -1;
        else if (DrawC::gWetRoad != 0)
            reflect = -2;
    }
    sint32 light = BWorldSm::QuadLight(&carObj->N.simRoadInfo);
    carObj->render.light = light & 0xFFFFFF;
    if (carObj->render.detail < 0)
        return;
    if (gNight_renderNight != 0)
        DrawC::NightHeadlight(carObj);
    uint32 rgb = (uint32)carObj->render.light;
    sint32 lightAvg = (sint32)((rgb & 255U) + ((rgb >> 8) & 255U) + ((rgb >> 16) & 255U)) / 3;
    if (lightAvg < 24)
        lightAvg = 24;
    Draw::CarCache *sd = &DrawC::carCache;
    sd->color = lightAvg * 0x10101;
    sint32 worldZ = DrawC::PrimStart(&center, carObj, lightAvg, sd);
    if (worldZ < 0)
        return;
    for (sint32 i = 0; i < 57; ++i)
    {
        Transformer::zObj *obj = LoadedScenePointer[countryFlag][carType]->obj[i];
        sint32 visible = (uint8)ObjectVisible[i];
        if (obj->numFacet == 0 || visible == 0)
            continue;
        sint32 envmap = (sint8)ObjectInfo[i][1];
        sint32 offset = (sint8)ObjectInfo[i][0];
        if (carType < 28 && (uint32)i - 28U < 2U && RecessedLight[carType] != 0)
            offset = -offset;
        bool mirror = rightHandDrive != 0 && i >= 35 && i < 41;
        if (mirror)
            sd->head.mirror ^= 1;
        if (visible < 17)
        {
            sint32 clip = 32;
            if (carType < 28)
            {
                if (eMapColour.r == 0)
                    envmap &= 0xFE;
                if ((envmap & 0x10) != 0 && carType >= 22)
                    offset += 12;
            }
            else
            {
                clip = 64;
                envmap = 0x22;
                if (carType == 28 && i == 35)
                    offset = 4;
            }
            if (carObj->render.detail == 3)
            {
                envmap |= 0x20;
                offset = -offset;
                if ((uint32)i - 39U < 2U)
                    offset -= 16;
            }
            sd->sub_otz = (sint32)((uint32)carObj->render.sub_otz + (uint32)offset);
            if (worldZ < clip)
                DrawC::PrimClip(&orientMat[i], &position[i], obj, overlay, envmap, sd);
            else
                DrawC::Prim(&orientMat[i], &position[i], obj, overlay, envmap, sd);
        }
        else
        {
            sint32 index = visible - 17;
            sint32 type = index;
            sint32 overlayIndex = -1;
            if (carType < 28)
            {
                if (carType < 22)
                {
                    if (type == 4 && SignalBrakeFlare[carType] != 0)
                        type = 0x41;
                }
                else
                {
                    sint32 copIndex = (uint8)CopIndex[carType - 22][(uint8)carObj->render.currentCountry & 0x7F];
                    sint32 cop_type = FlareCopSirenType[copIndex][index];
                    if (cop_type != 0)
                        type = cop_type;
                }
                if (((carObj->render.brakeLight & 2) != 0 || (carType >= 22 && (carObj->AIFlags & 2) != 0)) && (type & 0xBF) == 1)
                    type |= 0x600;
                overlayIndex = FlareOverlayIndex[index];
            }
            sd->sub_otz = (sint32)((uint32)carObj->render.sub_otz + (uint32)offset);
            DrawC::PrimHalo(&orientMat[i], &position[i], obj, type, overlayIndex, reflect, sd);
        }
        if (mirror)
            sd->head.mirror ^= 1;
    }
    DrawC::PrimStop(carObj, sd);
    if (shadowFlag != 0)
    {
        sint32 lightR = (lightAvg * shadowColour.r) >> 8;
        sint32 lightG = (lightAvg * shadowColour.g) >> 8;
        sint32 lightB = (lightAvg * shadowColour.b) >> 8;
        sd->color = (lightB << 16) + (lightG << 8) + lightR;
        if (worldZ < 32)
            DrawC::ShadowPrimClip(shadowVertex, sd);
        else
            DrawC::ShadowPrim(shadowVertex, sd);
    }
}

void R3DCar::InsertCarFacetMenuII(Car::tObj *carObj, sint32 light)
{
    FUNCTION_MARKER(0x800B2760, "NFS4.EXE");
    sint32 carType = carObj->render.currentCarType;
    sint32 countryFlag = (uint8)carObj->render.currentCountry >> 7;
    Transformer::zOverlay *overlay = LoadedScenePointer[countryFlag][carType]->overlay;
    sint32 rightHandDrive = R3DCar::rightHandDrive;
    if ((InMenu & 0x80) != 0)
        light >>= 1;
    Draw::CarCache *sd = &DrawC::carCache;
    sd->color = (sint32)((uint32)light * 0x10101U);
    if (carObj->render.detail < 0)
        return;
    if (DrawC::PrimStart(&center, carObj, light, sd) < 0)
        return;
    for (sint32 i = 0; i < 57; ++i)
    {
        Transformer::zObj *obj = LoadedScenePointer[countryFlag][carType]->obj[i];
        sint32 visible = (uint8)ObjectVisible[i];
        if (obj->numFacet == 0 || visible == 0)
            continue;
        sint32 envmap = (sint8)ObjectInfo[i][1];
        sint32 offset = (sint8)ObjectInfo[i][0];
        if (carType < 28 && (uint32)i - 28U < 2U && RecessedLight[carType] != 0)
            offset = -offset;
        bool mirror = rightHandDrive != 0 && i >= 35 && i < 41;
        if (mirror)
            sd->head.mirror ^= 1;
        if (visible < 17)
        {
            if (carType < 28)
            {
                if ((envmap & 0x10) != 0 && carType >= 22)
                {
                    offset += 12;
                    if ((InMenu & 0x80) != 0)
                        offset = -offset;
                }
            }
            else
            {
                envmap &= 0x80;
                if (carType == 28 && i == 35)
                    offset = 4;
            }
            if ((envmap & 0x80) != 0 && (InMenu & 0x80) != 0)
                offset = -offset;
            sd->sub_otz = (sint32)((uint32)carObj->render.sub_otz + (uint32)offset);
            DrawC::PrimMenu(&orientMat[i], &position[i], obj, overlay, envmap, sd);
        }
        else
        {
            sint32 originalType = visible - 17;
            sint32 type = originalType;
            sint32 index = -1;
            if (carType < 28)
            {
                if (carType < 22)
                {
                    if (type == 4 && SignalBrakeFlare[carType] != 0)
                        type = 1;
                }
                else
                {
                    sint32 copIndex = (uint8)CopIndex[carType - 22][(uint8)carObj->render.currentCountry & 0x7F];
                    sint32 cop_type = FlareCopSirenType[copIndex][originalType];
                    if (cop_type != 0)
                        type = cop_type;
                    if ((envmap & 0x10) != 0 && (InMenu & 0x80) != 0)
                        offset = 0;
                    index = FlareOverlayIndex[originalType];
                }
                if (carType >= 22 && type == 1)
                    type = 0x601;
            }
            if ((InMenu & 0x80) != 0)
                type |= 0x8080;
            sd->sub_otz = (sint32)((uint32)carObj->render.sub_otz + (uint32)offset);
            DrawC::PrimHalo(&orientMat[i], &position[i], obj, type, index, 0, sd);
        }
        if (mirror)
            sd->head.mirror ^= 1;
    }
    DrawC::PrimStop(carObj, sd);
    if ((InMenu & 0x80) == 0)
        DrawC::ShadowPrimClip(shadowVertex, sd);
}

void R3DCar::Showroom(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800B2B88, "NFS4.EXE");
    DrawC::ShowroomPrims(&Vi->cview.mrotationInv, &Vi->cview.translationInv, &DrawC::carCache);
    DrawC::SpotPrims(&Vi->cview.mrotationInv, &Vi->cview.translationInv, &DrawC::carCache);
}

void R3DCar::InsertAllListFacet(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800B2BF8, "NFS4.EXE");
    sint32 sort_min[sizeof(Cars::gList) / sizeof(Cars::gList[0])];
    sint32 sort_max[sizeof(Cars::gList) / sizeof(Cars::gList[0])];
    stackSpeedUpEnbabledFlag = 1;
    if (Vi->player == 0)
    {
        if (ClockLast < generic128HzClock)
        {
            Clock = 1;
            ClockLast = (sint32)((uint32)generic128HzClock + 6);
        }
        else
            Clock = 0;
    }
    for (sint32 i = 0; i < Cars::gNumCars; ++i)
    {
        Car::tObj *carObj = Cars::gList[i];
        PositionZ[i] = Visibilty(carObj, Vi);
        carObj->render.sort_carObj = 0;
        carObj->render.sort_flag = 0;
        sort_min[i] = 0;
        sort_max[i] = carObj->render.currentCarType == 28 ? 416 : carObj->N.dimension.z >> 9;
    }
    sint32 count = Cars::gNumCars;
    sint32 last = (sint32)((uint32)count - 1);
    for (sint32 i = 0, sort = 1; i < last; ++i, ++sort)
    {
        if (PositionZ[i] == (sint32)0x80000000)
            continue;
        Car::tObj *carObj = Cars::gList[i];
        for (sint32 j = sort; j < count; ++j)
        {
            Car::tObj *sort_carObj = Cars::gList[j];
            sint32 diffZ = (sint32)((uint32)PositionZ[i] - (uint32)PositionZ[j]);
            sint32 size = (sint32)((uint32)carObj->N.dimension.z + (uint32)sort_carObj->N.dimension.z);
            if (carObj->render.sort_carObj != 0 || sort_carObj->render.sort_carObj != 0)
                continue;
            sint32 distance = diffZ < 0 ? (sint32)(0 - (uint32)diffZ) : diffZ;
            if (distance < size)
            {
                sort_carObj->render.sort_carObj = carObj;
                carObj->render.sort_flag = (sint16)((uint16)carObj->render.sort_flag + 1);
                if (diffZ < 0)
                    sort_min[i] = (sint32)((uint32)sort_min[i] - (uint32)(sort_carObj->N.dimension.z >> 9));
                else
                    sort_max[i] = (sint32)((uint32)sort_max[i] + (uint32)(sort_carObj->N.dimension.z >> 9));
            }
        }
    }
    subOt = subOtStart[gFlip][gCView.player];
    for (sint32 i = 0; i < count; ++i)
    {
        if (PositionZ[i] == (sint32)0x80000000)
            continue;
        Car::tObj *carObj = Cars::gList[i];
        sort_min[i] = (sint32)((uint32)sort_min[i] - 48);
        sort_max[i] = (sint32)((uint32)sort_max[i] + 48);
        if (carObj->render.sort_carObj == 0)
        {
            sint32 sub_otSize = (sint32)((uint32)sort_max[i] - (uint32)sort_min[i]);
            carObj->render.sub_ot = subOt;
            subOt += sub_otSize;
            carObj->render.sub_otSize = sub_otSize;
            carObj->render.sub_otOffset = (sint32)((uint32)sort_max[i] - (uint32)(carObj->N.dimension.z >> 10));
        }
        else
        {
            carObj->render.sub_ot = 0;
            carObj->render.sub_otSize = 0;
            carObj->render.sub_otOffset = 0;
        }
    }
    DrawC::gScreenMat.m[0][0] = 4096;
    DrawC::gScreenMat.m[1][0] = 0;
    DrawC::gScreenMat.m[2][0] = 0;
    DrawC::gScreenMat.m[0][1] = 0;
    DrawC::gScreenMat.m[1][1] = -4096;
    DrawC::gScreenMat.m[2][1] = 0;
    DrawC::gScreenMat.m[0][2] = 0;
    DrawC::gScreenMat.m[1][2] = 0;
    DrawC::gScreenMat.m[2][2] = 4096;
    DrawC::gScreenMat.t[0] = 0;
    DrawC::gScreenMat.t[1] = 0;
    DrawC::gScreenMat.t[2] = 0;
    stackSpeedUpEnbabledFlag = 0;
    for (sint32 i = 0; i < Cars::gNumCars; ++i)
    {
        Car::tObj *carObj = Cars::gList[i];
        stackSpeedUpEnbabledFlag = 1;
        InsertCarFacet(carObj, Vi);
        stackSpeedUpEnbabledFlag = 0;
        InsertCarFacetII(carObj);
    }
}
