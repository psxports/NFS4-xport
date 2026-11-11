#include <stddef.h>
#include <stdlib.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/aicop.h"
#include "game/common/aiphysic.h"
#include "game/common/bo.h"
#include "game/common/bworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/collide.h"
#include "game/common/fastrand.h"
#include "game/common/gmesetup.h"
#include "game/common/mathnfs.h"
#include "game/common/newton.h"
#include "game/common/physics.h"
#include "game/common/schedule.h"
#include "game/common/sim.h"
#include "game/common/trk.h"
#include "game/psx/draww.h"
#include "game/psx/force.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/fixdsqrt.h"

BWorldSm::Pos testSimRoadInfo;
BWorldSm::Pos newtestSimRoadInfo;
sint32 divTable[50] = { 65536, 66052, 66576, 67108, 67650, 68200, 68759, 69327, 69905, 70492, 71089, 71697, 72315, 72944, 73584, 74235, 74898, 75573, 76260, 76959, 77672, 78398, 79137, 79891, 80659, 81442, 82241, 83055, 83886, 84733, 85598, 86480, 87381, 88301, 89240, 90200, 91180, 92182, 93206, 94254, 95325, 96420, 97541, 98689, 99864, 101067, 102300, 103563, 104857, 106184 };
sint16 fudgeTable[32] = { 0, 20, 36, 44, 30, 44, 52, 46, 36, 20, 6, -20, -30, -16, 0, 18, 26, 38, 52, 40, 30, 15, 0, -20, -32, -16, -8, 8, 16, 20, 16, 8 };
sint32 swap[4] = { 0, 1, 2, 3 };

sint32 Netwon_CheckForBadQuad(BO::tNewtonObj *newtonObj, BWorldSm::Pos *testSimRoadInfo, sint32 wheel)
{
    FUNCTION_MARKER(0x800A2E24, "NFS4.EXE");
    if (testSimRoadInfo->simQuad != 0 && (testSimRoadInfo->simQuad->surface & 15) == 0)
        return 1;
    Car::tObj *car = reinterpret_cast<Car::tObj *>(newtonObj);
    sint32 height = (sint32)((uint32)car->wheel[wheel].actualHeight - (uint32)car->wheel[wheel].currentPos.y);
    return height > 0x20000;
}

void Newton::AddDamageZone(BO::tNewtonObj *newtonObj, sint32 impulse, sint32 zone, sint32 type)
{
    FUNCTION_MARKER(0x8009EF2C, "NFS4.EXE");
    Car::tObj *carObj = reinterpret_cast<Car::tObj *>(newtonObj);
    if (Force::IsForceOn(carObj) != 0)
        Force::HitWall(newtonObj->collision.impulse);
    if (GameSetup::gData.Damage != 0)
    {
        sint32 imp = impulse / 2;
        if (imp > 0x640000)
            imp = 0x640000;
        if ((carObj->carFlags & 0x200) != 0)
            imp /= 2;
        if (imp < newtonObj->damage[zone])
            imp = newtonObj->damage[zone];
        newtonObj->damage[zone] = imp;
        if (zone < 8)
        {
            sint32 *damage = newtonObj->damage;
            sint32 temp;
            if (zone == 0)
            {
                temp = (sint32)((uint32)damage[0] + (uint32)damage[2]) / 2;
                if (temp < damage[1])
                    temp = damage[1];
                damage[1] = temp;
            }
            else if (zone == 1)
            {
                temp = (sint32)((uint32)damage[7] + (uint32)damage[1]) / 2;
                if (temp < damage[0])
                    temp = damage[0];
                damage[0] = temp;
                temp = (sint32)((uint32)damage[1] + (uint32)damage[3]) / 2;
                if (temp < damage[2])
                    temp = damage[2];
                damage[2] = temp;
            }
            else if (zone == 6)
            {
                temp = (sint32)((uint32)damage[4] + (uint32)damage[6]) / 2;
                if (temp < damage[5])
                    temp = damage[5];
                damage[5] = temp;
            }
            else if (zone == 7)
            {
                temp = (sint32)((uint32)damage[7] + (uint32)damage[1]) / 2;
                if (temp < damage[0])
                    temp = damage[0];
                damage[0] = temp;
                temp = (sint32)((uint32)damage[5] + (uint32)damage[7]) / 2;
                if (temp < damage[6])
                    temp = damage[6];
                damage[6] = temp;
            }
            else
            {
                temp = (sint32)((uint32)damage[zone] + (uint32)damage[zone + 2]) / 2;
                if (temp < damage[zone + 1])
                    temp = damage[zone + 1];
                damage[zone + 1] = temp;
                temp = (sint32)((uint32)damage[zone] + (uint32)damage[zone - 2]) / 2;
                if (temp < damage[zone - 1])
                    temp = damage[zone - 1];
                damage[zone - 1] = temp;
            }
            if (zone == 0 || zone == 6)
            {
                temp = (sint32)((uint32)damage[0] + (uint32)damage[6]) / 2;
                if (temp < damage[7])
                    temp = damage[7];
                damage[7] = temp;
            }
        }
    }
    if (type == 0)
        return;
    sint32 xMult = 0x20000;
    sint32 yMult = 0;
    sint32 zMult = 0x20000;
    sint32 intensity;
    if (impulse > 0x5a0000)
    {
        impulse /= 4;
        sint32 altitude = newtonObj->objAltitude;
        newtonObj->flightTime = 1;
        newtonObj->collision.lastCollision = 0;
        carObj->collision.smoking = 1;
        if (altitude < 0x20000)
            newtonObj->position.y = (sint32)((uint32)newtonObj->position.y + 0x10000U);
        sint32 verticalVelocity = (sint32)((uint32)newtonObj->linearVel.y + (uint32)(impulse / 3));
        if (verticalVelocity > 0xc0000)
            verticalVelocity = 0xc0000;
        newtonObj->linearVel.y = verticalVelocity;
        intensity = impulse / 32;
        if (intensity > 0x9999)
            intensity = 0x9999;
        if (BWorldSm::TunnelFlagSm(&newtonObj->simRoadInfo) && newtonObj->linearVel.y > 0x90000)
            newtonObj->linearVel.y = 0x90000;
        if (impulse > 0x140000)
        {
            randtemp = fastRandom * randSeed;
            fastRandom = randtemp & 0xffff;
            uint32 randomX = (randtemp & 0x00ffff00) >> 8;
            randtemp = fastRandom * randSeed;
            fastRandom = randtemp & 0xffff;
            uint32 randomY = (randtemp & 0x00ffff00) >> 8;
            randtemp = fastRandom * randSeed;
            fastRandom = randtemp & 0xffff;
            uint32 randomZ = (randtemp & 0x00ffff00) >> 8;
            randtemp = fastRandom * randSeed;
            fastRandom = randtemp & 0xffff;
            if (((randtemp & 0x00ffff00) >> 8) < 0x3333)
                intensity = (sint32)(0U - (uint32)intensity);
            xMult = (sint32)(randomX * 4);
            yMult = (sint32)(randomY * 3);
            zMult = (sint32)(randomZ * 3);
            if (xMult + yMult + zMult < 0x40000)
            {
                if (xMult < 0x10000)
                    xMult = (sint32)(randomX * 8);
                if (yMult < 0x10000)
                    yMult *= 2;
                if (zMult < 0x10000)
                    zMult *= 2;
            }
        }
        if (type == 2)
        {
            xMult *= 2;
            yMult *= 2;
            zMult *= 2;
        }
    }
    else
    {
        intensity = impulse / 64;
        if (intensity > 0x8000)
            intensity = 0x8000;
    }
    sint32 *localAngular[3] = {&carObj->angularVel_ch.x, &carObj->angularVel_ch.y, &carObj->angularVel_ch.z};
    for (sint32 row = 0; row < 3; ++row)
    {
        uint32 x = fixedmult(newtonObj->angularVel.x, newtonObj->orientMat.m[row * 3]);
        uint32 y = fixedmult(newtonObj->angularVel.y, newtonObj->orientMat.m[row * 3 + 1]);
        uint32 z = fixedmult(newtonObj->angularVel.z, newtonObj->orientMat.m[row * 3 + 2]);
        *localAngular[row] = (sint32)(x + y + z);
    }
    sint32 delta = fixedmult(intensity, xMult) / 2;
    if ((uint32)zone < 3)
        carObj->angularVel_ch.x = (sint32)((uint32)carObj->angularVel_ch.x - (uint32)delta);
    else
        carObj->angularVel_ch.x = (sint32)((uint32)carObj->angularVel_ch.x + (uint32)delta);
    bool add = (uint32)zone - 2U < 3;
    delta = fixedmult(intensity, yMult) / 2;
    carObj->angularVel_ch.y = (sint32)((uint32)carObj->angularVel_ch.y + (add ? (uint32)delta : 0U - (uint32)delta));
    delta = fixedmult(intensity, zMult) / 2;
    carObj->angularVel_ch.z = (sint32)((uint32)carObj->angularVel_ch.z + (add ? (uint32)delta : 0U - (uint32)delta));
    matrixtdef transposeMat;
    transpose(&newtonObj->orientMat, &transposeMat);
    sint32 *worldAngular[3] = {&newtonObj->angularVel.x, &newtonObj->angularVel.y, &newtonObj->angularVel.z};
    for (sint32 row = 0; row < 3; ++row)
    {
        uint32 x = fixedmult(carObj->angularVel_ch.x, transposeMat.m[row * 3]);
        uint32 y = fixedmult(carObj->angularVel_ch.y, transposeMat.m[row * 3 + 1]);
        uint32 z = fixedmult(carObj->angularVel_ch.z, transposeMat.m[row * 3 + 2]);
        *worldAngular[row] = (sint32)(x + y + z);
    }
}

void Newton::FindClosestQuad(BO::tNewtonObj *n)
{
    FUNCTION_MARKER(0x8009F704, "NFS4.EXE");
    BWorldSm::FindClosestQuadRez(&n->position, &n->simRoadInfo, 1);
}

sint32 Newton::CalculateSliceYaw(sint32 slice)
{
    FUNCTION_MARKER(0x8009F730, "NFS4.EXE");
    sint32 s = slice + 1;
    if (s >= gNumSlices)
        s = (sint32)((uint32)slice - (uint32)gNumSlices + 1U);
    sint32 x1 = BWorldSm::slices[slice].center[0];
    sint32 z1 = BWorldSm::slices[slice].center[2];
    sint32 x2 = BWorldSm::slices[s].center[0];
    sint32 z2 = BWorldSm::slices[s].center[2];
    return intatan((sint32)((uint32)x2 - (uint32)x1), (sint32)((uint32)z2 - (uint32)z1));
}

void Newton::UpdateRoadGeometry(BO::tNewtonObj *n)
{
    FUNCTION_MARKER(0x8009F7A0, "NFS4.EXE");
    if (n->active == 0)
        return;
    sint32 slice = n->simRoadInfo.slice;
    sint32 hiRez = n->simOptz < 2;
    if (hiRez != 0)
    {
        n->roadCenterPoint.z = 0;
        n->roadCenterPoint.y = 0;
        n->roadCenterPoint.x = 0;
        for (sint32 i = 0; i < 4; ++i)
        {
            coorddef temp;
            if (n->simRoadInfo.simQuad != 0)
                temp = n->simRoadInfo.quadPts[i];
            else
            {
                Trk::NewSlice *roadSlice = &BWorldSm::slices[n->simRoadInfo.slice];
                temp.x = roadSlice->center[0];
                temp.y = roadSlice->center[1];
                temp.z = roadSlice->center[2];
            }
            n->roadCenterPoint.x = (sint32)((uint32)n->roadCenterPoint.x + (uint32)temp.x);
            n->roadCenterPoint.y = (sint32)((uint32)n->roadCenterPoint.y + (uint32)temp.y);
            n->roadCenterPoint.z = (sint32)((uint32)n->roadCenterPoint.z + (uint32)temp.z);
        }
        n->roadCenterPoint.x /= 4;
        n->roadCenterPoint.y /= 4;
        n->roadCenterPoint.z /= 4;
    }
    else if (n->simRoadInfo.simQuad != 0)
        n->roadCenterPoint = n->simRoadInfo.quadPts[0];
    else
    {
        Trk::NewSlice *roadSlice = &BWorldSm::slices[slice];
        n->roadCenterPoint.x = roadSlice->center[0];
        n->roadCenterPoint.y = roadSlice->center[1];
        n->roadCenterPoint.z = roadSlice->center[2];
    }
    if (hiRez == 0)
    {
        Trk::NewSlice *roadSlice = &BWorldSm::slices[slice];
        n->roadMatrix.m[0] = (sint8)roadSlice->right[0] * 512;
        n->roadMatrix.m[2] = (sint8)roadSlice->right[2] * 512;
        n->roadMatrix.m[1] = (sint8)roadSlice->right[1] * 512;
        n->roadMatrix.m[3] = (sint8)roadSlice->normal[0] * 512;
        n->roadMatrix.m[5] = (sint8)roadSlice->normal[2] * 512;
        n->roadMatrix.m[4] = (sint8)roadSlice->normal[1] * 512;
        n->roadMatrix.m[6] = (sint8)roadSlice->forward[0] * 512;
        n->roadMatrix.m[7] = (sint8)roadSlice->forward[1] * 512;
        n->roadMatrix.m[8] = (sint8)roadSlice->forward[2] * 512;
    }
    else if (n->simRoadInfo.quadChanged != 0)
    {
        coorddef normal = *BWorldSm::UNormal(&n->simRoadInfo);
        n->roadMatrix.m[3] = normal.x;
        n->roadMatrix.m[4] = normal.y;
        n->roadMatrix.m[5] = normal.z;
        coorddef forward = *BWorldSm::UForward(&n->simRoadInfo);
        n->roadMatrix.m[6] = forward.x;
        n->roadMatrix.m[7] = forward.y;
        n->roadMatrix.m[8] = forward.z;
        sint32 r1 = n->roadMatrix.m[3];
        sint32 r2 = n->roadMatrix.m[4];
        sint32 r3 = n->roadMatrix.m[5];
        sint32 r4 = n->roadMatrix.m[6];
        sint32 r5 = n->roadMatrix.m[7];
        sint32 r6 = n->roadMatrix.m[8];
        sint32 x1 = fixedmult(r2, r6);
        sint32 x2 = fixedmult(r3, r5);
        n->roadMatrix.m[0] = (sint32)((uint32)x1 - (uint32)x2);
        x1 = fixedmult(r3, r4);
        x2 = fixedmult(r1, r6);
        n->roadMatrix.m[1] = (sint32)((uint32)x1 - (uint32)x2);
        x1 = fixedmult(r1, r5);
        x2 = fixedmult(r2, r4);
        n->roadMatrix.m[2] = (sint32)((uint32)x1 - (uint32)x2);
    }
    sint32 nextSlice = slice + 1;
    if (nextSlice >= gNumSlices)
        nextSlice = (sint32)((uint32)slice - (uint32)gNumSlices + 1U);
    sint32 x1 = BWorldSm::slices[slice].center[0];
    sint32 z1 = BWorldSm::slices[slice].center[2];
    sint32 x2 = BWorldSm::slices[nextSlice].center[0];
    sint32 z2 = BWorldSm::slices[nextSlice].center[2];
    n->roadYaw = intatan((sint32)((uint32)x2 - (uint32)x1), (sint32)((uint32)z2 - (uint32)z1));
}

sint32 Newton::FindGroundElevationGeneral(coorddef *point, coorddef *normal, coorddef *pointOnQuad)
{
    FUNCTION_MARKER(0x8009FB30, "NFS4.EXE");
    sint32 x = (sint32)((uint32)point->x - (uint32)pointOnQuad->x);
    sint32 z = (sint32)((uint32)point->z - (uint32)pointOnQuad->z);
    sint32 numerator = (sint32)(0U -
        (uint32)(normal->x / 256) * (uint32)(x / 256) -
        (uint32)(normal->z / 256) * (uint32)(z / 256));
    sint32 result;
    if (normal->y < 0x9eb9)
        result = fixeddiv(numerator, normal->y);
    else
    {
        sint32 index = (sint32)(0x10000U - (uint32)normal->y) >> 9;
        result = fixedmult(numerator, divTable[index]);
    }
    return (sint32)((uint32)result + (uint32)pointOnQuad->y);
}

sint32 Newton::FindGroundElevationRough(coorddef *point, coorddef *normal, coorddef *pointOnQuad)
{
    FUNCTION_MARKER(0x8009FC8C, "NFS4.EXE");
    sint32 x = (sint32)((uint32)point->x - (uint32)pointOnQuad->x);
    sint32 z = (sint32)((uint32)point->z - (uint32)pointOnQuad->z);
    sint32 numerator = (sint32)(0U -
        (uint32)(normal->x / 256) * (uint32)(x / 256) -
        (uint32)(normal->z / 256) * (uint32)(z / 256));
    sint32 fudgeHeight = 0;
    for (sint32 i = 0; i < 2; ++i)
    {
        sint32 first = i == 0 ? point->x : point->x >> 2;
        sint32 second = i == 0 ? point->z >> 1 : point->z;
        if (first < 0)
            first = (sint32)(0U - (uint32)first);
        if (second < 0)
            second = (sint32)(0U - (uint32)second);
        sint32 fudgeDist = second < first ?
            (sint32)((uint32)first + (uint32)(second >> 2)) :
            (sint32)((uint32)second + (uint32)(first >> 2));
        fudgeDist >>= 15;
        if (fudgeDist < 0)
            fudgeDist = -fudgeDist;
        sint32 fudgeIndex = fudgeDist % 32;
        fudgeHeight = (sint32)((uint32)fudgeHeight + (uint32)((sint32)fudgeTable[fudgeIndex] * 128));
    }
    sint32 result;
    if (normal->y < 0x9eb9)
        result = fixeddiv(numerator, normal->y);
    else
    {
        sint32 index = (sint32)(0x10000U - (uint32)normal->y) >> 9;
        result = fixedmult(numerator, divTable[index]);
    }
    return (sint32)((uint32)result + (uint32)pointOnQuad->y + (uint32)fudgeHeight);
}

sint32 Newton::FindGroundElevationAndNormalFast(BO::tNewtonObj *newtonObj, coorddef *normal)
{
    FUNCTION_MARKER(0x8009FE60, "NFS4.EXE");
    sint32 nx = newtonObj->roadMatrix.m[3];
    sint32 ny = newtonObj->roadMatrix.m[4];
    sint32 nz = newtonObj->roadMatrix.m[5];
    normal->y = ny;
    normal->x = nx;
    normal->z = nz;
    if (ny < 0x199a)
        return (sint32)0x83000000U;
    sint32 elevation = FindGroundElevationGeneral(&newtonObj->position, normal, &newtonObj->roadCenterPoint);
    sint32 x = newtonObj->position.x;
    sint32 z = newtonObj->position.z;
    Car::tObj *car = reinterpret_cast<Car::tObj *>(newtonObj);
    for (sint32 i = 0; i < 4; ++i)
    {
        car->wheel[i].wheelInAir = 0;
        car->wheel[i].rebound = 0;
        car->wheel[i].currentPos.y = elevation;
        car->wheel[i].actualHeight = elevation;
        car->wheel[i].currentPos.x = x;
        car->wheel[i].currentPos.z = z;
    }
    nx = normal->x;
    ny = normal->y;
    nz = normal->z;
    for (sint32 i = 0; i < 4; ++i)
    {
        car->wheel[i].roadSurfaceType = 1;
        car->wheel[i].roadNormal.x = nx;
        car->wheel[i].roadNormal.y = ny;
        car->wheel[i].roadNormal.z = nz;
    }
    return elevation;
}

sint32 Newton::FindGroundElevationAndNormal(BO::tNewtonObj *newtonObj, coorddef *normal)
{
    FUNCTION_MARKER(0x8009FF80, "NFS4.EXE");
    Car::tObj *car = reinterpret_cast<Car::tObj *>(newtonObj);
    coorddef elevation = { 0, 0, 0 };
    coorddef carNormal;
    carNormal.x = newtonObj->orientMat.m[3];
    carNormal.y = newtonObj->orientMat.m[4];
    carNormal.z = newtonObj->orientMat.m[5];
    sint32 length = (sint32)((uint32)newtonObj->dimension.z * 13U) >> 12;
    sint32 width = (sint32)((uint32)newtonObj->dimension.x * 7U) >> 11;
    sint32 height = (sint32)(0U - (uint32)newtonObj->dimension.y) >> 8;
    coorddef lengthVector;
    coorddef widthVector;
    coorddef vecOffset;
    coorddef carGroundCoord;
    lengthVector.x = (sint32)((uint32)length * (uint32)(newtonObj->orientMat.m[6] >> 8));
    widthVector.x = (sint32)((uint32)width * (uint32)(newtonObj->orientMat.m[0] >> 8));
    vecOffset.x = (sint32)((uint32)height * (uint32)(newtonObj->orientMat.m[3] >> 8));
    carGroundCoord.x = (sint32)((uint32)newtonObj->position.x + (uint32)vecOffset.x);
    lengthVector.y = (sint32)((uint32)length * (uint32)(newtonObj->orientMat.m[7] >> 8));
    widthVector.y = (sint32)((uint32)width * (uint32)(newtonObj->orientMat.m[1] >> 8));
    vecOffset.y = (sint32)((uint32)height * (uint32)(newtonObj->orientMat.m[4] >> 8));
    carGroundCoord.y = (sint32)((uint32)newtonObj->position.y + (uint32)vecOffset.y);
    lengthVector.z = (sint32)((uint32)length * (uint32)(newtonObj->orientMat.m[8] >> 8));
    widthVector.z = (sint32)((uint32)width * (uint32)(newtonObj->orientMat.m[2] >> 8));
    vecOffset.z = (sint32)((uint32)height * (uint32)(newtonObj->orientMat.m[5] >> 8));
    carGroundCoord.z = (sint32)((uint32)newtonObj->position.z + (uint32)vecOffset.z);
    coorddef tireCoord[4];
    tireCoord[0].x = (sint32)((uint32)carGroundCoord.x + (uint32)lengthVector.x - (uint32)widthVector.x);
    tireCoord[0].y = (sint32)((uint32)carGroundCoord.y + (uint32)lengthVector.y - (uint32)widthVector.y);
    tireCoord[0].z = (sint32)((uint32)carGroundCoord.z + (uint32)lengthVector.z - (uint32)widthVector.z);
    tireCoord[1].x = (sint32)((uint32)carGroundCoord.x + (uint32)lengthVector.x + (uint32)widthVector.x);
    tireCoord[1].y = (sint32)((uint32)carGroundCoord.y + (uint32)lengthVector.y + (uint32)widthVector.y);
    tireCoord[1].z = (sint32)((uint32)carGroundCoord.z + (uint32)lengthVector.z + (uint32)widthVector.z);
    tireCoord[2].x = (sint32)((uint32)carGroundCoord.x - (uint32)lengthVector.x - (uint32)widthVector.x);
    tireCoord[2].y = (sint32)((uint32)carGroundCoord.y - (uint32)lengthVector.y - (uint32)widthVector.y);
    tireCoord[2].z = (sint32)((uint32)carGroundCoord.z - (uint32)lengthVector.z - (uint32)widthVector.z);
    tireCoord[3].x = (sint32)((uint32)carGroundCoord.x - (uint32)lengthVector.x + (uint32)widthVector.x);
    tireCoord[3].y = (sint32)((uint32)carGroundCoord.y - (uint32)lengthVector.y + (uint32)widthVector.y);
    tireCoord[3].z = (sint32)((uint32)carGroundCoord.z - (uint32)lengthVector.z + (uint32)widthVector.z);
    sint32 wheelsInAir = 0;
    sint32 bounce = 0;
    BWorldSm::Pos testSimRoadInfo = newtonObj->simRoadInfo;
    coorddef wheelHeight[4];
    for (sint32 i = 0; i < 4; ++i)
    {
        wheelHeight[i] = tireCoord[i];
        car->wheel[i].actualHeight = tireCoord[i].y;
        BWorldSm::FindClosestTriangleRez(&tireCoord[i], &testSimRoadInfo, 1);
        coorddef roadNormal = *BWorldSm::UNormal(&testSimRoadInfo);
        sint32 roadSurfaceType = testSimRoadInfo.simQuad ? testSimRoadInfo.simQuad->surface : 14;
        car->wheel[i].roadSurfaceType = roadSurfaceType;
        roadSurfaceType &= 15;
        if (roadNormal.y < 0x1999 || roadSurfaceType == 14 || roadSurfaceType == 0)
        {
            roadNormal.x = 0;
            roadNormal.y = 0x10000;
            roadNormal.z = 0;
            elevation.x = (sint32)((uint32)elevation.x + (uint32)tireCoord[i].x);
            elevation.y = (sint32)((uint32)elevation.y + (uint32)tireCoord[i].y - (uint32)newtonObj->objAltitude);
            elevation.z = (sint32)((uint32)elevation.z + (uint32)tireCoord[i].z);
        }
        else
        {
            coorddef roadCenterPoint;
            if (testSimRoadInfo.simQuad != 0)
                roadCenterPoint = testSimRoadInfo.quadPts[0];
            else
            {
                Trk::NewSlice *slice = &BWorldSm::slices[testSimRoadInfo.slice];
                roadCenterPoint.x = slice->center[0];
                roadCenterPoint.y = slice->center[1];
                roadCenterPoint.z = slice->center[2];
            }
            if ((uint32)roadSurfaceType - 2U < 2U)
                wheelHeight[i].y = FindGroundElevationRough(&tireCoord[i], &roadNormal, &roadCenterPoint);
            else
                wheelHeight[i].y = FindGroundElevationGeneral(&tireCoord[i], &roadNormal, &roadCenterPoint);
            car->wheel[i].actualHeight = wheelHeight[i].y;
            if ((sint32)((uint32)wheelHeight[i].y - (uint32)tireCoord[i].y) > 0x20000)
            {
                roadNormal.x = 0;
                roadNormal.y = 0x10000;
                roadNormal.z = 0;
                wheelHeight[i].y = tireCoord[i].y;
                elevation.x = (sint32)((uint32)elevation.x + (uint32)tireCoord[i].x);
                elevation.y = (sint32)((uint32)elevation.y + (uint32)tireCoord[i].y);
                elevation.z = (sint32)((uint32)elevation.z + (uint32)tireCoord[i].z);
            }
            else
            {
                elevation.x = (sint32)((uint32)elevation.x + (uint32)wheelHeight[i].x);
                elevation.y = (sint32)((uint32)elevation.y + (uint32)wheelHeight[i].y);
                elevation.z = (sint32)((uint32)elevation.z + (uint32)wheelHeight[i].z);
            }
        }
        car->wheel[i].currentPos = wheelHeight[i];
        car->wheel[i].roadNormal = roadNormal;
    }
    coorddef wheelVec = { 0, 0, 0 };
    sint32 compressionValue[4];
    for (sint32 i = 0; i < 4; ++i)
    {
        wheelVec.y = (sint32)((uint32)car->wheel[i].currentPos.y - (uint32)tireCoord[i].y) / 2;
        car->wheel[i].rebound = 0;
        if (wheelVec.y < -0x2665)
        {
            car->wheel[i].wheelInAir = 1;
            ++wheelsInAir;
        }
        else if (car->wheel[i].wheelInAir == 1)
        {
            if (wheelVec.y > 0)
            {
                ++bounce;
                car->wheel[i].wheelInAir = 0;
                car->wheel[i].rebound = 1;
            }
            else if (newtonObj->objAltitude < 0x3333 && newtonObj->linearVel.y > -0x1999 &&
                     newtonObj->orientationToGround.y > 0xf333)
                car->wheel[i].wheelInAir = 0;
        }
        sint32 limit = newtonObj->speedXZ > 0x358000 ? -0x2666 : -0x3333;
        if (wheelVec.y < limit)
            wheelVec.y = limit;
        compressionValue[i] = (sint32)((uint32)(wheelVec.y / 256) * (uint32)(carNormal.y / 256));
        car->wheel[i].wheelAcc = (sint32)((uint32)(compressionValue[i] / 256) * (uint32)(car->specs->suspensionStiffness / 256));
        if (car->carInfo->WeightTransfer == 1)
            car->wheel[i].wheelAcc = (sint32)((uint32)car->wheel[i].wheelAcc * 2U);
    }
    sint32 count = 0;
    if (bounce != 0)
        count = (sint32)((uint32)car->wheel[0].rebound + (uint32)car->wheel[1].rebound +
            (uint32)car->wheel[2].rebound + (uint32)car->wheel[3].rebound);
    for (sint32 i = 0; i < 4; ++i)
    {
        if (bounce == 0)
        {
            if (car->wheel[i].wheelAcc < 0 && newtonObj->orientationToGround.y > 0xdc28)
                car->wheel[i].wheelAcc >>= 1;
        }
        else if (car->wheel[i].rebound != 0 && newtonObj->linearVel.y < 0)
        {
            sint32 speed = (sint32)(0U - (uint32)newtonObj->linearVel.y + (uint32)newtonObj->groundVel);
            if (speed >= 0)
            {
                if (count == 0 || (count == -1 && speed == (-2147483647 - 1)))
                    abort();
                sint32 wheelBounce = (speed / count) >> 1;
                if (speed < 0x100000)
                {
                    sint32 ratio = fixedmult(speed, 0x1000);
                    if (ratio < 0x4ccd)
                        ratio = 0x4ccc;
                    else
                        ratio = fixedmult(speed, 0x1000);
                    wheelBounce = fixedmult(ratio, wheelBounce);
                }
                car->wheel[i].wheelAcc = (sint32)((uint32)car->wheel[i].wheelAcc + (uint32)wheelBounce);
            }
        }
    }
    for (sint32 i = 0; i < 4; ++i)
    {
        sint32 desiredCompression = fixedmult((sint32)((uint32)tireCoord[i].y - (uint32)wheelHeight[i].y) >> 1,
            newtonObj->orientMat.m[4]);
        if (desiredCompression > 0)
        {
            if (desiredCompression >= car->specs->tireRange)
                desiredCompression = car->specs->tireRange;
        }
        else
        {
            sint32 minimum = (sint32)(0U - (uint32)car->specs->tireRange);
            if (desiredCompression < minimum)
                desiredCompression = minimum;
        }
        car->wheel[swap[i]].impactCompression = desiredCompression;
    }
    elevation.x >>= 2;
    elevation.z >>= 2;
    elevation.y >>= 2;
    coorddef tempVecZ;
    coorddef tempVecX;
    coorddef tempVecY;
    tempVecZ.x = (sint32)((uint32)wheelHeight[0].x + (uint32)wheelHeight[1].x - (uint32)wheelHeight[2].x - (uint32)wheelHeight[3].x) >> 1;
    tempVecZ.y = (sint32)((uint32)wheelHeight[0].y + (uint32)wheelHeight[1].y - (uint32)wheelHeight[2].y - (uint32)wheelHeight[3].y) >> 1;
    tempVecZ.z = (sint32)((uint32)wheelHeight[0].z + (uint32)wheelHeight[1].z - (uint32)wheelHeight[2].z - (uint32)wheelHeight[3].z) >> 1;
    Math::NormalizeShortVector(&tempVecZ);
    tempVecX.x = (sint32)((uint32)wheelHeight[1].x + (uint32)wheelHeight[3].x - (uint32)wheelHeight[0].x - (uint32)wheelHeight[2].x) >> 1;
    tempVecX.y = (sint32)((uint32)wheelHeight[1].y + (uint32)wheelHeight[3].y - (uint32)wheelHeight[0].y - (uint32)wheelHeight[2].y) >> 1;
    tempVecX.z = (sint32)((uint32)wheelHeight[1].z + (uint32)wheelHeight[3].z - (uint32)wheelHeight[0].z - (uint32)wheelHeight[2].z) >> 1;
    Math::NormalizeShortVector(&tempVecX);
    sint32 x1 = fixedmult(tempVecZ.y, tempVecX.z);
    sint32 x2 = fixedmult(tempVecZ.z, tempVecX.y);
    tempVecY.x = (sint32)((uint32)x1 - (uint32)x2);
    sint32 y1 = fixedmult(tempVecZ.z, tempVecX.x);
    sint32 y2 = fixedmult(tempVecZ.x, tempVecX.z);
    tempVecY.y = (sint32)((uint32)y1 - (uint32)y2);
    sint32 z1 = fixedmult(tempVecZ.x, tempVecX.y);
    sint32 z2 = fixedmult(tempVecZ.y, tempVecX.x);
    tempVecY.z = (sint32)((uint32)z1 - (uint32)z2);
    Math::NormalizeShortVector(&tempVecY);
    if (tempVecY.y < 0)
    {
        wheelsInAir = 4;
        normal->x = 0;
        normal->y = 0x10000;
        normal->z = 0;
    }
    else
    {
        *normal = tempVecY;
        if (tempVecY.y < 0x8000)
            wheelsInAir = 4;
    }
    if (newtonObj->orientationToGround.y < -0xe666)
    {
        newtonObj->angularVel.x = (sint32)((uint32)newtonObj->angularVel.x * 253U) / 256;
        newtonObj->angularVel.y = (sint32)((uint32)newtonObj->angularVel.y * 253U) / 256;
        newtonObj->angularVel.z = (sint32)((uint32)newtonObj->angularVel.z * 253U) / 256;
    }
    else if (wheelsInAir < 4)
    {
        sint32 xDot1 = fixedmult(newtonObj->angularVel.x, newtonObj->orientMat.m[0]);
        sint32 xDot2 = fixedmult(newtonObj->angularVel.y, newtonObj->orientMat.m[1]);
        sint32 xDot3 = fixedmult(newtonObj->angularVel.z, newtonObj->orientMat.m[2]);
        car->angularVel_ch.x = (sint32)((uint32)xDot1 + (uint32)xDot2 + (uint32)xDot3);
        sint32 yDot1 = fixedmult(newtonObj->angularVel.x, newtonObj->orientMat.m[3]);
        sint32 yDot2 = fixedmult(newtonObj->angularVel.y, newtonObj->orientMat.m[4]);
        sint32 yDot3 = fixedmult(newtonObj->angularVel.z, newtonObj->orientMat.m[5]);
        car->angularVel_ch.y = (sint32)((uint32)yDot1 + (uint32)yDot2 + (uint32)yDot3);
        sint32 zDot1 = fixedmult(newtonObj->angularVel.x, newtonObj->orientMat.m[6]);
        sint32 zDot2 = fixedmult(newtonObj->angularVel.y, newtonObj->orientMat.m[7]);
        sint32 zDot3 = fixedmult(newtonObj->angularVel.z, newtonObj->orientMat.m[8]);
        car->angularVel_ch.z = (sint32)((uint32)zDot1 + (uint32)zDot2 + (uint32)zDot3);
        sint32 pitch = (sint32)((uint32)car->wheel[2].wheelAcc + (uint32)car->wheel[3].wheelAcc -
            (uint32)car->wheel[0].wheelAcc - (uint32)car->wheel[1].wheelAcc) >> 2;
        if (pitch > 0)
        {
            if (pitch > car->specs->pitchAngularVelCap)
                pitch = car->specs->pitchAngularVelCap;
        }
        else
        {
            sint32 minimum = (sint32)(0U - (uint32)car->specs->pitchAngularVelCap);
            if (minimum < pitch)
                minimum = pitch;
            pitch = minimum;
        }
        sint32 roll = (sint32)((uint32)car->wheel[0].wheelAcc + (uint32)car->wheel[2].wheelAcc -
            (uint32)car->wheel[1].wheelAcc - (uint32)car->wheel[3].wheelAcc) >> 2;
        if (roll > 0)
        {
            if (roll > car->specs->rollAngularVelCap)
                roll = car->specs->rollAngularVelCap;
        }
        else
        {
            sint32 minimum = (sint32)(0U - (uint32)car->specs->rollAngularVelCap);
            if (minimum < roll)
                minimum = roll;
            roll = minimum;
        }
        sint32 absPitchRate = car->angularVel_ch.x;
        if (absPitchRate < 0)
            absPitchRate = (sint32)(0U - (uint32)absPitchRate);
        sint32 pitchDamping = absPitchRate < 0x13333 ? car->specs->dampingPitchRate : 0xd999;
        car->angularVel_ch.x = fixedmult(car->angularVel_ch.x, pitchDamping);
        car->angularVel_ch.x = (sint32)((uint32)car->angularVel_ch.x + (uint32)pitch);
        sint32 absRollRate = car->angularVel_ch.z;
        if (absRollRate < 0)
            absRollRate = (sint32)(0U - (uint32)absRollRate);
        sint32 rollDamping = absRollRate < 0x13333 ? car->specs->dampingRollRate : 0xd999;
        car->angularVel_ch.z = fixedmult(car->angularVel_ch.z, rollDamping);
        car->angularVel_ch.z = (sint32)((uint32)car->angularVel_ch.z - (uint32)roll);
        matrixtdef transposeMat;
        transpose(&newtonObj->orientMat, &transposeMat);
        sint32 transformX1 = fixedmult(car->angularVel_ch.x, transposeMat.m[0]);
        sint32 transformX2 = fixedmult(car->angularVel_ch.y, transposeMat.m[1]);
        sint32 transformX3 = fixedmult(car->angularVel_ch.z, transposeMat.m[2]);
        newtonObj->angularVel.x = (sint32)((uint32)transformX1 + (uint32)transformX2 + (uint32)transformX3);
        sint32 transformY1 = fixedmult(car->angularVel_ch.x, transposeMat.m[3]);
        sint32 transformY2 = fixedmult(car->angularVel_ch.y, transposeMat.m[4]);
        sint32 transformY3 = fixedmult(car->angularVel_ch.z, transposeMat.m[5]);
        newtonObj->angularVel.y = (sint32)((uint32)transformY1 + (uint32)transformY2 + (uint32)transformY3);
        sint32 transformZ1 = fixedmult(car->angularVel_ch.x, transposeMat.m[6]);
        sint32 transformZ2 = fixedmult(car->angularVel_ch.y, transposeMat.m[7]);
        sint32 transformZ3 = fixedmult(car->angularVel_ch.z, transposeMat.m[8]);
        newtonObj->angularVel.z = (sint32)((uint32)transformZ1 + (uint32)transformZ2 + (uint32)transformZ3);
    }
    coorddef *samplePoint = simGlobal.gameTicks < 64 ? &newtonObj->roadCenterPoint : &elevation;
    newtonObj->objAltitude = CalcPerpenHeightOfCenterPointFromGround(newtonObj, normal, samplePoint);
    return elevation.y;
}

void Newton::LimitCarsToDrivableDist(BO::tNewtonObj *newtonObj)
{
    FUNCTION_MARKER(0x800A0DA4, "NFS4.EXE");
    if (Physics::DoBarrierCheck(reinterpret_cast<Car::tObj *>(newtonObj)) != 0)
    {
        newtonObj->collision.impulse = 0;
        newtonObj->collision.otherObj = 0;
        FindClosestQuad(newtonObj);
        UpdateRoadGeometry(newtonObj);
    }
}

sint32 Newton::CalcPerpenHeightOfLowestPointFromGround(BO::tNewtonObj *newtonObj, coorddef *normal, coorddef *samplePoint)
{
    FUNCTION_MARKER(0x800A0DE8, "NFS4.EXE");
    coorddef basisDots;
    sint32 *basis[3] = {&basisDots.x, &basisDots.y, &basisDots.z};
    for (sint32 row = 0; row < 3; ++row)
    {
        uint32 x = (uint32)(normal->x / 256) * (uint32)(newtonObj->orientMat.m[row * 3] / 256);
        uint32 y = (uint32)(normal->y / 256) * (uint32)(newtonObj->orientMat.m[row * 3 + 1] / 256);
        uint32 z = (uint32)(normal->z / 256) * (uint32)(newtonObj->orientMat.m[row * 3 + 2] / 256);
        *basis[row] = (sint32)(x + y + z);
    }
    basisDots.x = (sint32)((uint32)(basisDots.x / 256) * (uint32)(newtonObj->dimension.x / 256));
    basisDots.y = (sint32)((uint32)(basisDots.y / 256) * (uint32)(newtonObj->dimension.y / 256));
    basisDots.z = (sint32)((uint32)(basisDots.z / 256) * (uint32)(newtonObj->dimension.z / 256));
    sint32 xDir = basisDots.x < 0 ? 1 : -1;
    sint32 yDir = basisDots.y < 0 ? 1 : -1;
    sint32 zDir = basisDots.z < 0 ? 1 : -1;
    coorddef relativePos;
    relativePos.x = (sint32)((uint32)newtonObj->position.x - (uint32)samplePoint->x);
    relativePos.y = (sint32)((uint32)newtonObj->position.y - (uint32)samplePoint->y);
    relativePos.z = (sint32)((uint32)newtonObj->position.z - (uint32)samplePoint->z);
    uint32 x = (uint32)(normal->x / 256) * (uint32)(relativePos.x / 256);
    uint32 y = (uint32)(normal->y / 256) * (uint32)(relativePos.y / 256);
    uint32 z = (uint32)(normal->z / 256) * (uint32)(relativePos.z / 256);
    sint32 relativeDot = (sint32)(x + y + z);
    return (sint32)((uint32)xDir * (uint32)basisDots.x +
                    (uint32)yDir * (uint32)basisDots.y +
                    (uint32)zDir * (uint32)basisDots.z + (uint32)relativeDot);
}

sint32 Newton::CalcPerpenHeightOfCenterPointFromGround(BO::tNewtonObj *newtonObj, coorddef *normal, coorddef *samplePoint)
{
    FUNCTION_MARKER(0x800A116C, "NFS4.EXE");
    coorddef relativePos;
    relativePos.x = (sint32)((uint32)newtonObj->position.x - (uint32)samplePoint->x);
    relativePos.y = (sint32)((uint32)newtonObj->position.y - (uint32)samplePoint->y);
    relativePos.z = (sint32)((uint32)newtonObj->position.z - (uint32)samplePoint->z);
    sint32 xDot = fixedmult(normal->x, relativePos.x);
    sint32 yDot = fixedmult(normal->y, relativePos.y);
    sint32 zDot = fixedmult(normal->z, relativePos.z);
    sint32 relativeDot = (sint32)((uint32)xDot + (uint32)yDot + (uint32)zDot);
    sint32 orientation = newtonObj->orientationToGround.y;
    if (orientation < 0)
        orientation = (sint32)(0U - (uint32)orientation);
    sint32 dimension = orientation < 0xb334 ? newtonObj->dimension.x : newtonObj->dimension.y;
    return (sint32)((uint32)relativeDot - (uint32)dimension);
}

void Newton::CalcDistToClosestPlayerCar(BO::tNewtonObj *n)
{
    FUNCTION_MARKER(0x800A1248, "NFS4.EXE");
    sint32 whichPlayer = 0;
    sint32 forcedSimOptz = 0;
    Car::tObj *player = Cars::gHumanRaceCarList[GameSetup::gData.localCar];
    sint32 x = (sint32)((uint32)n->position.x - (uint32)player->N.position.x);
    if (x <= 0)
        x = (sint32)((uint32)player->N.position.x - (uint32)n->position.x);
    sint32 z = (sint32)((uint32)n->position.z - (uint32)player->N.position.z);
    if (z <= 0)
        z = (sint32)((uint32)player->N.position.z - (uint32)n->position.z);
    n->distToPlayer = z < x ? (sint32)((uint32)x + (uint32)(z >> 2)) :
        (sint32)((uint32)z + (uint32)(x >> 2));
    if (Cars::gNumHumanRaceCars == 2)
    {
        sint32 otherPlayer = (sint32)(1u - (uint32)GameSetup::gData.localCar);
        player = Cars::gHumanRaceCarList[otherPlayer];
        x = (sint32)((uint32)n->position.x - (uint32)player->N.position.x);
        if (x <= 0)
            x = (sint32)((uint32)player->N.position.x - (uint32)n->position.x);
        z = (sint32)((uint32)n->position.z - (uint32)player->N.position.z);
        if (z <= 0)
            z = (sint32)((uint32)player->N.position.z - (uint32)n->position.z);
        sint32 dist = z < x ? (sint32)((uint32)x + (uint32)(z >> 2)) :
            (sint32)((uint32)z + (uint32)(x >> 2));
        if (dist < n->distToPlayer)
        {
            whichPlayer = 1;
            n->distToPlayer = dist;
        }
    }
    if (n->distToPlayer <= 0x600000)
    {
        if (BWorld::CheckChunkVisible(&Cars::gHumanRaceCarList[whichPlayer]->N.simRoadInfo, &n->simRoadInfo) == 0)
            forcedSimOptz = 1;
    }
    Car::tObj *car = reinterpret_cast<Car::tObj *>(n);
    if ((n->distToPlayer > 0x600000 || forcedSimOptz != 0) &&
        (car->forceNoSimOptz == 0 || simGlobal.gameTicks < 3))
    {
        if (n->simOptz != 2)
        {
            n->groundSurfaceType = 1;
            n->driveSurfaceType = 1;
        }
        n->simOptz = 2;
        n->angularVel.x = 0;
        n->angularVel.z = 0;
        return;
    }
    sint32 oldOptz = n->simOptz;
    if (((uint32)car->carFlags & 0x30u) == 0 &&
        (car->forceNoSimOptz == 0 || simGlobal.gameTicks < 3) && n->distToPlayer > 0x480000)
        n->simOptz = 1;
    else
        n->simOptz = 0;
    if (oldOptz >= 2)
    {
        Cars::SetCarUpForHiRezSim(car);
        FindClosestQuad(n);
        UpdateRoadGeometry(n);
        LimitCarsToDrivableDist(n);
        static coorddef dummy;
        if (stackSpeedUpEnbabledFlag != 0)
        {
            stackSpeedUpEnbabledFlag = 0;
            n->groundElevation = FindGroundElevationAndNormal(n, &dummy);
            stackSpeedUpEnbabledFlag = 1;
        }
        else
            n->groundElevation = FindGroundElevationAndNormal(n, &dummy);
        n->groundSurfaceType = 1;
        n->driveSurfaceType = 1;
        n->groundVel = 0;
        n->collision.impulse = 0;
        n->collision.otherObj = 0;
        n->objAltitude = 0;
        n->position.y = (sint32)((uint32)n->groundElevation + (uint32)n->dimension.y);
    }
}

void Newton::UpdateRoadInfo(BO::tNewtonObj *n)
{
    FUNCTION_MARKER(0x800A1568, "NFS4.EXE");
    if (n->active == 0)
        return;
    CalcDistToClosestPlayerCar(n);
    FindClosestQuad(n);
    if (n->simRoadInfo.quadChanged != 0)
        UpdateRoadGeometry(n);
    if (n->simOptz == 0)
    {
        uint32 surface = n->simRoadInfo.simQuad ? n->simRoadInfo.simQuad->surface : 14U;
        n->groundSurfaceType = surface;
        n->driveSurfaceType = surface & 15U;
    }
}

void Newton::CopyRoadMatrixToOrientMat(BO::tNewtonObj *n, sint32 backwards)
{
    FUNCTION_MARKER(0x800A15F4, "NFS4.EXE");
    matrixtdef *ori = &n->orientMat;
    matrixtdef *road = &n->roadMatrix;
    if (backwards == 0)
        *ori = *road;
    else
    {
        ori->m[0] = (sint32)(0U - (uint32)road->m[0]);
        ori->m[1] = (sint32)(0U - (uint32)road->m[1]);
        ori->m[2] = (sint32)(0U - (uint32)road->m[2]);
        ori->m[3] = road->m[3];
        ori->m[4] = road->m[4];
        ori->m[5] = road->m[5];
        ori->m[6] = (sint32)(0U - (uint32)road->m[6]);
        ori->m[7] = (sint32)(0U - (uint32)road->m[7]);
        ori->m[8] = (sint32)(0U - (uint32)road->m[8]);
    }
}

void Newton::CopyRoadMatrixToShadowMat(BO::tNewtonObj *n, sint32 backwards)
{
    FUNCTION_MARKER(0x800A16C8, "NFS4.EXE");
    matrixtdef *shad = &n->shadowMat;
    matrixtdef *road = &n->roadMatrix;
    if (backwards == 0)
        *shad = *road;
    else
    {
        shad->m[0] = (sint32)(0U - (uint32)road->m[0]);
        shad->m[1] = (sint32)(0U - (uint32)road->m[1]);
        shad->m[2] = (sint32)(0U - (uint32)road->m[2]);
        shad->m[3] = road->m[3];
        shad->m[4] = road->m[4];
        shad->m[5] = road->m[5];
        shad->m[6] = (sint32)(0U - (uint32)road->m[6]);
        shad->m[7] = (sint32)(0U - (uint32)road->m[7]);
        shad->m[8] = (sint32)(0U - (uint32)road->m[8]);
    }
}

void Newton::SetInitialSlicePositionOrientationEtc(BO::tNewtonObj *n, sint32 slice, coorddef *offset, sint32 direction)
{
    FUNCTION_MARKER(0x800A179C, "NFS4.EXE");
    sint32 backwards = direction != 1;
    BWorldSm::SetSlice(slice, &n->simRoadInfo);
    Trk::NewSlice *roadSlice = &BWorldSm::slices[slice];
    n->totalSlice = (uint16)slice;
    n->roadMatrix.m[0] = (sint8)roadSlice->right[0] * 512;
    n->roadMatrix.m[1] = (sint8)roadSlice->right[1] * 512;
    n->roadMatrix.m[2] = (sint8)roadSlice->right[2] * 512;
    n->roadMatrix.m[3] = (sint8)roadSlice->normal[0] * 512;
    n->roadMatrix.m[4] = (sint8)roadSlice->normal[1] * 512;
    n->roadMatrix.m[5] = (sint8)roadSlice->normal[2] * 512;
    n->roadMatrix.m[6] = (sint8)roadSlice->forward[0] * 512;
    n->roadMatrix.m[7] = (sint8)roadSlice->forward[1] * 512;
    n->roadMatrix.m[8] = (sint8)roadSlice->forward[2] * 512;
    for (sint32 row = 0; row < 9; row += 3) {
        coorddef basis = { n->roadMatrix.m[row], n->roadMatrix.m[row + 1], n->roadMatrix.m[row + 2] };
        Math::NormalizeShortVector(&basis);
        n->roadMatrix.m[row] = basis.x;
        n->roadMatrix.m[row + 1] = basis.y;
        n->roadMatrix.m[row + 2] = basis.z;
    }
    matrixtdef transposeMat;
    transpose(&n->roadMatrix, &transposeMat);
    CopyRoadMatrixToOrientMat(n, backwards);
    CopyRoadMatrixToShadowMat(n, backwards);
    coorddef rOffset;
    sint32 first = fixedmult(offset->x, transposeMat.m[0]);
    sint32 second = fixedmult(offset->y, transposeMat.m[1]);
    sint32 third = fixedmult(offset->z, transposeMat.m[2]);
    rOffset.x = (sint32)((uint32)first + (uint32)second + (uint32)third);
    first = fixedmult(offset->x, transposeMat.m[3]);
    second = fixedmult(offset->y, transposeMat.m[4]);
    third = fixedmult(offset->z, transposeMat.m[5]);
    rOffset.y = (sint32)((uint32)first + (uint32)second + (uint32)third);
    first = fixedmult(offset->x, transposeMat.m[6]);
    second = fixedmult(offset->y, transposeMat.m[7]);
    third = fixedmult(offset->z, transposeMat.m[8]);
    rOffset.z = (sint32)((uint32)first + (uint32)second + (uint32)third);
    roadSlice = &BWorldSm::slices[slice];
    n->position.x = (sint32)((uint32)rOffset.x + (uint32)roadSlice->center[0]);
    n->position.y = (sint32)((uint32)rOffset.y + (uint32)roadSlice->center[1]);
    sint32 carFlags = reinterpret_cast<Car::tObj *>(n)->carFlags;
    n->position.z = (sint32)((uint32)rOffset.z + (uint32)roadSlice->center[2]);
    n->simOptz = (carFlags & 4) != 0 ? 0 : 2;
    FindClosestQuad(n);
    UpdateRoadInfo(n);
    UpdateRoadGeometry(n);
    CopyRoadMatrixToOrientMat(n, backwards);
    CopyRoadMatrixToShadowMat(n, backwards);
    if (n->simOptz == 2) {
        n->groundElevation = BWorldSm::slices[slice].center[1];
    } else if (n->simOptz == 1) {
        uint32 sum = 0;
        for (sint32 i = 0; i < 4; ++i) {
            coorddef temp;
            if (n->simRoadInfo.simQuad != 0) {
                temp = n->simRoadInfo.quadPts[i];
            } else {
                Trk::NewSlice *quadSlice = &BWorldSm::slices[n->simRoadInfo.slice];
                temp.x = quadSlice->center[0];
                temp.y = quadSlice->center[1];
                temp.z = quadSlice->center[2];
            }
            sum += (uint32)temp.y;
        }
        sint32 quadCenterY = (sint32)sum;
        if (quadCenterY < 0)
            quadCenterY += 3;
        n->groundElevation = quadCenterY >> 2;
    } else {
        static coorddef dummy;
        if (stackSpeedUpEnbabledFlag != 0) {
            stackSpeedUpEnbabledFlag = 0;
            n->groundElevation = FindGroundElevationAndNormal(n, &dummy);
            stackSpeedUpEnbabledFlag = 1;
        } else {
            n->groundElevation = FindGroundElevationAndNormal(n, &dummy);
        }
        n->position.y = (sint32)((uint32)n->groundElevation + (uint32)n->dimension.y);
    }
    coorddef normal = { n->roadMatrix.m[3], n->roadMatrix.m[4], n->roadMatrix.m[5] };
    sint32 altitude = CalcPerpenHeightOfLowestPointFromGround(n, &normal, &n->roadCenterPoint);
    sint32 oldY = n->position.y;
    uint8 simOptz = n->simOptz;
    n->objAltitude = altitude;
    n->objAltitude = 0;
    n->position.y = (sint32)((uint32)oldY - (uint32)altitude);
    if (simOptz == 0) {
        Trk::NewSimQuad *simQuad = n->simRoadInfo.simQuad;
        n->groundSurfaceType = simQuad != 0 ? simQuad->surface : 14;
    } else {
        n->groundSurfaceType = 1;
    }
    n->driveSurfaceType = n->groundSurfaceType & 15;
}

void Newton::InitBaseNewtonObj(BO::tNewtonObj *newtonObj, sint32 index, sint32 mass, sint32 moInertia, sint32 dimX, sint32 dimY, sint32 dimZ)
{
    FUNCTION_MARKER(0x800A1BE8, "NFS4.EXE");
    (void)moInertia;
    newtonObj->objID = index;
    newtonObj->simRoadInfo.simSlice = 0;
    newtonObj->simOptz = 0;
    newtonObj->position.x = 0;
    newtonObj->position.y = 0;
    newtonObj->position.z = 0;
    newtonObj->linearVel.x = 0;
    newtonObj->linearVel.y = 0;
    newtonObj->linearVel.z = 0;
    newtonObj->mass = mass;
    if ((GameSetup::gData.sgge & 2) != 0 && CollisionHumanFlag(newtonObj))
        newtonObj->mass = (sint32)((uint32)mass * 5U);
    if (CollisionCopFlag(newtonObj)) {
        if (GameSetup::gData.commMode == 1 &&
            (Cars::gHumanRaceCarList[0]->carInfo->carType < 22 ||
             Cars::gHumanRaceCarList[1]->carInfo->carType < 22))
            newtonObj->mass = (sint32)((uint32)newtonObj->mass << 1);
        else
            newtonObj->mass = (sint32)((uint32)newtonObj->mass * 3U) / 2;
    }
    newtonObj->massInv = fixeddiv(0x10000, newtonObj->mass);
    newtonObj->moInertia = (sint32)((uint32)newtonObj->mass << 3);
    newtonObj->moInertiaInv = fixeddiv(0x10000, newtonObj->moInertia);
    newtonObj->angularVel.x = 0;
    newtonObj->angularVel.y = 0;
    newtonObj->angularVel.z = 0;
    newtonObj->orientationToGround.x = 0;
    newtonObj->orientationToGround.y = 0x10000;
    newtonObj->orientationToGround.z = 0;
    newtonObj->dimension.x = dimX;
    newtonObj->dimension.y = dimY;
    newtonObj->dimension.z = dimZ;
    uint32 x = (uint32)(dimX / 256);
    uint32 y = (uint32)(dimY / 256);
    newtonObj->dimensionRadius = fixedsqrt((sint32)(x * x + y * y));
    uint32 radius = (uint32)(newtonObj->dimensionRadius / 256);
    uint32 z = (uint32)(dimZ / 256);
    newtonObj->dimensionRadius = fixedsqrt((sint32)(radius * radius + z * z));
    newtonObj->flightTime = 0;
    newtonObj->groundSurfaceType = 1;
    newtonObj->driveSurfaceType = 1;
    newtonObj->groundVel = 0;
    newtonObj->objAltitude = 0;
    newtonObj->gravityMult = 0x10000;
    newtonObj->xRelRoadCenter = 0;
    newtonObj->roadGravityModifier = 0;
    newtonObj->collision.collided = 0;
    newtonObj->collision.impulse = 0;
    newtonObj->collision.otherObj = 0;
    newtonObj->collision.sfxType = 0;
    newtonObj->collision.disableCollisionTimer = 0;
    newtonObj->collision.lastOtherObj = 0;
    newtonObj->collision.lastImpulse = 0;
    newtonObj->collision.lastTime = 0;
    for (sint32 i = 9; i >= 0; --i)
        newtonObj->damage[i] = 0;
    newtonObj->reOrthoCounter = 0;
    newtonObj->deadTimer = 0;
    newtonObj->active = 1;
    newtonObj->cumulatedRot = 0;
}

void Newton::QDUpdateVel(BO::tNewtonObj *newtonObj)
{
    FUNCTION_MARKER(0x800A1E68, "NFS4.EXE");
    if (newtonObj->active == 0)
        return;
    if ((GameSetup::gData.sgge & 4) == 0)
    {
        newtonObj->position.x = (sint32)((uint32)newtonObj->position.x + (uint32)(newtonObj->linearVel.x >> 6));
        newtonObj->position.y = (sint32)((uint32)newtonObj->position.y + (uint32)(newtonObj->linearVel.y >> 6));
        newtonObj->position.z = (sint32)((uint32)newtonObj->position.z + (uint32)(newtonObj->linearVel.z >> 6));
    }
    else
    {
        sint32 t1 = newtonObj->linearVel.x >> 6;
        sint32 t2 = newtonObj->linearVel.y >> 6;
        sint32 t3 = newtonObj->linearVel.z >> 6;
        t1 = fixedmult(t1, 0xcccc);
        t3 = fixedmult(t3, 0xcccc);
        newtonObj->position.x = (sint32)((uint32)newtonObj->position.x + (uint32)t1);
        newtonObj->position.z = (sint32)((uint32)newtonObj->position.z + (uint32)t3);
        newtonObj->position.y = (sint32)((uint32)newtonObj->position.y + (uint32)t2);
    }
}

sint32 Newton::OptzRotxform(matrixtdef *m, sint32 ax, sint32 ay, sint32 az, sint32 *reOrthoNeeded, sint32 reOrthoLimit, sint32 *cumulatedRot)
{
    FUNCTION_MARKER(0x800A1F4C, "NFS4.EXE");
    (void)reOrthoLimit;
    sint32 absx = ax < 0 ? (sint32)(0U - (uint32)ax) : ax;
    sint32 absy = ay < 0 ? (sint32)(0U - (uint32)ay) : ay;
    sint32 absz = az < 0 ? (sint32)(0U - (uint32)az) : az;
    *reOrthoNeeded = 0;
    *cumulatedRot = (sint32)((uint32)*cumulatedRot + (uint32)absx + (uint32)absy + (uint32)absz);
    matrixtdef mx;
    matrixtdef my;
    matrixtdef mz;
    matrixtdef mt;
    fixedxformy(&my, ay);
    sint32 changed = absy >= 20;
    if (absx < 14)
        mt = my;
    else
    {
        fixedxformx(&mx, ax);
        Math::fasttransmult(&mx, &my, &mt);
        changed = 1;
    }
    if (absz < 14)
        *m = mt;
    else
    {
        fixedxformz(&mz, az);
        Math::fasttransmult(&mt, &mz, m);
        changed = 1;
    }
    return changed;
}

void Newton::QDUpdateRot64Hz(BO::tNewtonObj *newtonObj)
{
    FUNCTION_MARKER(0x800A20E8, "NFS4.EXE");
    if (newtonObj->active == 0)
        return;
    coorddef angularVel;
    angularVel.x = newtonObj->angularVel.x / 64;
    angularVel.y = newtonObj->angularVel.y / 64;
    angularVel.z = newtonObj->angularVel.z / 64;
    matrixtdef m;
    sint32 reOrthoNeeded;
    if (OptzRotxform(&m, angularVel.x, angularVel.y, angularVel.z, &reOrthoNeeded,
            0x1000, &newtonObj->cumulatedRot) != 0)
    {
        Math::fasttransmult(&newtonObj->orientMat, &m, &newtonObj->orientMat);
        newtonObj->reOrthoCounter = (uint8)((uint32)newtonObj->reOrthoCounter - 1U);
        if (newtonObj->reOrthoCounter == 0 || reOrthoNeeded != 0)
        {
            reorthogonalize(&newtonObj->orientMat);
            newtonObj->reOrthoCounter = 32;
            newtonObj->cumulatedRot = 0;
        }
    }
}

void Newton::QDUpdateRot32Hz(BO::tNewtonObj *newtonObj)
{
    FUNCTION_MARKER(0x800A21F8, "NFS4.EXE");
    if (newtonObj->active == 0 || newtonObj->simOptz != 0)
        return;
    coorddef angularVel;
    angularVel.x = newtonObj->angularVel.x / 16;
    angularVel.y = newtonObj->angularVel.y / 16;
    angularVel.z = newtonObj->angularVel.z / 16;
    matrixtdef m;
    sint32 reOrthoNeeded;
    if (OptzRotxform(&m, angularVel.x, angularVel.y, angularVel.z, &reOrthoNeeded,
            0x2000, &newtonObj->cumulatedRot) != 0)
    {
        Math::fasttransmult(&newtonObj->orientMat, &m, &newtonObj->orientMat);
        sint32 restore = reOrthoNeeded != 0;
        if (!restore)
        {
            newtonObj->reOrthoCounter = (uint8)((uint32)newtonObj->reOrthoCounter - 1U);
            restore = newtonObj->reOrthoCounter == 0;
        }
        if (restore)
        {
            reorthogonalize(&newtonObj->orientMat);
            newtonObj->reOrthoCounter = 64;
            newtonObj->cumulatedRot = 0;
        }
    }
}

void Newton::CalculateGroundShadowMatrix(BO::tNewtonObj *newtonObj, coorddef *normal, sint32 orientToGround)
{
    FUNCTION_MARKER(0x800A2318, "NFS4.EXE");
    Car::tObj *car = reinterpret_cast<Car::tObj *>(newtonObj);
    if ((car->carFlags & 4) == 0 && orientToGround > 0xe666)
    {
        newtonObj->shadowMat = newtonObj->orientMat;
        coorddef up = *normal;
        newtonObj->shadowMat.m[3] = up.x;
        newtonObj->shadowMat.m[4] = up.y;
        newtonObj->shadowMat.m[5] = up.z;
        return;
    }
    coorddef up = *normal;
    newtonObj->shadowMat.m[3] = up.x;
    newtonObj->shadowMat.m[4] = up.y;
    newtonObj->shadowMat.m[5] = up.z;
    sint32 rightFirst = 0;
    if (orientToGround < 0x8000)
    {
        sint32 dot = (sint32)(
            (uint32)(newtonObj->orientMat.m[0] / 256) * (uint32)(normal->x / 256) +
            (uint32)(newtonObj->orientMat.m[1] / 256) * (uint32)(normal->y / 256) +
            (uint32)(newtonObj->orientMat.m[2] / 256) * (uint32)(normal->z / 256));
        if (dot < 0)
            dot = (sint32)(0U - (uint32)dot);
        if (dot == (-2147483647 - 1))
        {
            for (;;) {}
        }
        rightFirst = (double)dot > 0.5;
    }
    if (rightFirst != 0)
    {
        sint32 right0a = fixedmult(newtonObj->shadowMat.m[4], newtonObj->orientMat.m[8]);
        sint32 right0b = fixedmult(newtonObj->shadowMat.m[5], newtonObj->orientMat.m[7]);
        newtonObj->shadowMat.m[0] = (sint32)((uint32)right0a - (uint32)right0b);
        sint32 right1a = fixedmult(newtonObj->shadowMat.m[5], newtonObj->orientMat.m[6]);
        sint32 right1b = fixedmult(newtonObj->shadowMat.m[3], newtonObj->orientMat.m[8]);
        newtonObj->shadowMat.m[1] = (sint32)((uint32)right1a - (uint32)right1b);
        sint32 right2a = fixedmult(newtonObj->shadowMat.m[3], newtonObj->orientMat.m[7]);
        sint32 right2b = fixedmult(newtonObj->shadowMat.m[4], newtonObj->orientMat.m[6]);
        newtonObj->shadowMat.m[2] = (sint32)((uint32)right2a - (uint32)right2b);
        coorddef rightVector;
        rightVector.x = newtonObj->shadowMat.m[0];
        rightVector.y = newtonObj->shadowMat.m[1];
        rightVector.z = newtonObj->shadowMat.m[2];
        Math::NormalizeShortVector(&rightVector);
        newtonObj->shadowMat.m[0] = rightVector.x;
        newtonObj->shadowMat.m[1] = rightVector.y;
        newtonObj->shadowMat.m[2] = rightVector.z;
        sint32 forward0a = fixedmult(newtonObj->shadowMat.m[1], newtonObj->shadowMat.m[5]);
        sint32 forward0b = fixedmult(newtonObj->shadowMat.m[2], newtonObj->shadowMat.m[4]);
        newtonObj->shadowMat.m[6] = (sint32)((uint32)forward0a - (uint32)forward0b);
        sint32 forward1a = fixedmult(newtonObj->shadowMat.m[2], newtonObj->shadowMat.m[3]);
        sint32 forward1b = fixedmult(newtonObj->shadowMat.m[0], newtonObj->shadowMat.m[5]);
        newtonObj->shadowMat.m[7] = (sint32)((uint32)forward1a - (uint32)forward1b);
        sint32 forward2a = fixedmult(newtonObj->shadowMat.m[0], newtonObj->shadowMat.m[4]);
        sint32 forward2b = fixedmult(newtonObj->shadowMat.m[1], newtonObj->shadowMat.m[3]);
        newtonObj->shadowMat.m[8] = (sint32)((uint32)forward2a - (uint32)forward2b);
        coorddef forwardVector;
        forwardVector.x = newtonObj->shadowMat.m[6];
        forwardVector.y = newtonObj->shadowMat.m[7];
        forwardVector.z = newtonObj->shadowMat.m[8];
        Math::NormalizeShortVector(&forwardVector);
        newtonObj->shadowMat.m[6] = forwardVector.x;
        newtonObj->shadowMat.m[7] = forwardVector.y;
        newtonObj->shadowMat.m[8] = forwardVector.z;
    }
    else
    {
        sint32 forward0a = fixedmult(newtonObj->orientMat.m[1], newtonObj->shadowMat.m[5]);
        sint32 forward0b = fixedmult(newtonObj->orientMat.m[2], newtonObj->shadowMat.m[4]);
        newtonObj->shadowMat.m[6] = (sint32)((uint32)forward0a - (uint32)forward0b);
        sint32 forward1a = fixedmult(newtonObj->orientMat.m[2], newtonObj->shadowMat.m[3]);
        sint32 forward1b = fixedmult(newtonObj->orientMat.m[0], newtonObj->shadowMat.m[5]);
        newtonObj->shadowMat.m[7] = (sint32)((uint32)forward1a - (uint32)forward1b);
        sint32 forward2a = fixedmult(newtonObj->orientMat.m[0], newtonObj->shadowMat.m[4]);
        sint32 forward2b = fixedmult(newtonObj->orientMat.m[1], newtonObj->shadowMat.m[3]);
        newtonObj->shadowMat.m[8] = (sint32)((uint32)forward2a - (uint32)forward2b);
        coorddef forwardVector;
        forwardVector.x = newtonObj->shadowMat.m[6];
        forwardVector.y = newtonObj->shadowMat.m[7];
        forwardVector.z = newtonObj->shadowMat.m[8];
        Math::NormalizeShortVector(&forwardVector);
        newtonObj->shadowMat.m[6] = forwardVector.x;
        newtonObj->shadowMat.m[7] = forwardVector.y;
        newtonObj->shadowMat.m[8] = forwardVector.z;
        sint32 right0a = fixedmult(newtonObj->shadowMat.m[4], newtonObj->shadowMat.m[8]);
        sint32 right0b = fixedmult(newtonObj->shadowMat.m[5], newtonObj->shadowMat.m[7]);
        newtonObj->shadowMat.m[0] = (sint32)((uint32)right0a - (uint32)right0b);
        sint32 right1a = fixedmult(newtonObj->shadowMat.m[5], newtonObj->shadowMat.m[6]);
        sint32 right1b = fixedmult(newtonObj->shadowMat.m[3], newtonObj->shadowMat.m[8]);
        newtonObj->shadowMat.m[1] = (sint32)((uint32)right1a - (uint32)right1b);
        sint32 right2a = fixedmult(newtonObj->shadowMat.m[3], newtonObj->shadowMat.m[7]);
        sint32 right2b = fixedmult(newtonObj->shadowMat.m[4], newtonObj->shadowMat.m[6]);
        newtonObj->shadowMat.m[2] = (sint32)((uint32)right2a - (uint32)right2b);
        coorddef rightVector;
        rightVector.x = newtonObj->shadowMat.m[0];
        rightVector.y = newtonObj->shadowMat.m[1];
        rightVector.z = newtonObj->shadowMat.m[2];
        Math::NormalizeShortVector(&rightVector);
        newtonObj->shadowMat.m[0] = rightVector.x;
        newtonObj->shadowMat.m[1] = rightVector.y;
        newtonObj->shadowMat.m[2] = rightVector.z;
    }
}

void Newton::CalcRealShadowCoordinates(Car::tObj *carObj, sint32 currentTick)
{
    FUNCTION_MARKER(0x800A268C, "NFS4.EXE");
    sint32 diff = (sint32)((uint32)currentTick - (uint32)carObj->N.lastUpdated);
    coorddef dimension = carObj->N.dimension;
    dimension.x = (sint32)((uint32)dimension.x + 0xcccU);
    dimension.z = (sint32)((uint32)dimension.z + 0xcccU);
    coorddef lengthVector;
    lengthVector.x = fixedmult(dimension.z, carObj->N.orientMat.m[6]);
    lengthVector.y = fixedmult(dimension.z, carObj->N.orientMat.m[7]);
    lengthVector.z = fixedmult(dimension.z, carObj->N.orientMat.m[8]);
    coorddef widthVector;
    widthVector.x = fixedmult(dimension.x, carObj->N.orientMat.m[0]);
    widthVector.y = fixedmult(dimension.x, carObj->N.orientMat.m[1]);
    widthVector.z = fixedmult(dimension.x, carObj->N.orientMat.m[2]);
    coorddef frontWidthVector;
    if (carObj->render.currentCarType == 0x14)
    {
        sint32 front = (sint32)((uint32)dimension.x * 0xc0U) >> 8;
        frontWidthVector.x = fixedmult(front, carObj->N.orientMat.m[0]);
        frontWidthVector.y = fixedmult(front, carObj->N.orientMat.m[1]);
        frontWidthVector.z = fixedmult(front, carObj->N.orientMat.m[2]);
    }
    else
        frontWidthVector = widthVector;
    coorddef carGroundCoord;
    if (carObj->N.orientationToGround.y > 0xe666)
    {
        coorddef vecOffset;
        sint32 height = (sint32)(0U - (uint32)dimension.y - (uint32)carObj->N.objAltitude);
        vecOffset.x = fixedmult(height, carObj->N.roadMatrix.m[3]);
        vecOffset.y = fixedmult(height, carObj->N.roadMatrix.m[4]);
        vecOffset.z = fixedmult(height, carObj->N.roadMatrix.m[5]);
        carGroundCoord.x = (sint32)((uint32)carObj->N.position.x + (uint32)vecOffset.x);
        carGroundCoord.y = (sint32)((uint32)carObj->N.position.y + (uint32)vecOffset.y);
        carGroundCoord.z = (sint32)((uint32)carObj->N.position.z + (uint32)vecOffset.z);
    }
    else
    {
        carGroundCoord = carObj->N.position;
        carGroundCoord.y = carObj->N.groundElevation;
    }
    coorddef temp;
    temp.x = (sint32)((uint32)carGroundCoord.x + (uint32)lengthVector.x);
    temp.y = (sint32)((uint32)carGroundCoord.y + (uint32)lengthVector.y);
    temp.z = (sint32)((uint32)carGroundCoord.z + (uint32)lengthVector.z);
    carObj->N.shadowCoord[0].x = (sint32)((uint32)temp.x - (uint32)frontWidthVector.x);
    carObj->N.shadowCoord[0].y = (sint32)((uint32)temp.y - (uint32)frontWidthVector.y);
    carObj->N.shadowCoord[0].z = (sint32)((uint32)temp.z - (uint32)frontWidthVector.z);
    carObj->N.shadowCoord[1].x = (sint32)((uint32)temp.x + (uint32)frontWidthVector.x);
    carObj->N.shadowCoord[1].y = (sint32)((uint32)temp.y + (uint32)frontWidthVector.y);
    carObj->N.shadowCoord[1].z = (sint32)((uint32)temp.z + (uint32)frontWidthVector.z);
    temp.x = (sint32)((uint32)carGroundCoord.x - (uint32)lengthVector.x);
    temp.y = (sint32)((uint32)carGroundCoord.y - (uint32)lengthVector.y);
    temp.z = (sint32)((uint32)carGroundCoord.z - (uint32)lengthVector.z);
    carObj->N.shadowCoord[2].x = (sint32)((uint32)temp.x - (uint32)widthVector.x);
    carObj->N.shadowCoord[2].y = (sint32)((uint32)temp.y - (uint32)widthVector.y);
    carObj->N.shadowCoord[2].z = (sint32)((uint32)temp.z - (uint32)widthVector.z);
    carObj->N.shadowCoord[3].x = (sint32)((uint32)temp.x + (uint32)widthVector.x);
    carObj->N.shadowCoord[3].y = (sint32)((uint32)temp.y + (uint32)widthVector.y);
    carObj->N.shadowCoord[3].z = (sint32)((uint32)temp.z + (uint32)widthVector.z);
    for (sint32 i = 0; i < 4; ++i)
    {
        sint32 wheelHeight = carObj->wheel[i].currentPos.y;
        carObj->N.shadowCoord[i].y = wheelHeight;
        sint32 verticalVelocity = carObj->N.linearVel.y / 64;
        carObj->N.shadowCoord[i].y = (sint32)((uint32)wheelHeight + (uint32)verticalVelocity * (uint32)diff);
    }
}

void Newton::CheckForSpikeBelts(BO::tNewtonObj *newtonObj)
{
    FUNCTION_MARKER(0x800A2A2C, "NFS4.EXE");
    sint32 active = AICop::spikeBelt.active_;
    if (active == 0)
        return;
    sint32 slice = AICop::spikeBelt.slice_;
    sint32 leftLatPos = AICop::spikeBelt.leftLatPos_;
    sint32 rightLatPos = AICop::spikeBelt.rightLatPos_;
    if (newtonObj->simRoadInfo.slice != slice)
        return;
    Car::tObj *car = reinterpret_cast<Car::tObj *>(newtonObj);
    sint32 latPos = car->roadPosition;
    if ((car->carFlags & 0x230) != 0 || leftLatPos >= latPos || latPos >= rightLatPos)
        return;
    car->blowout = (sint32)((uint32)car->blowout + 1U);
    if (car->blowout == 1)
    {
        newtonObj->collision.impulse = 0xf0000;
        newtonObj->collision.sfxType = 0x50007;
        newtonObj->collision.collisionPoint = newtonObj->position;
    }
}

void Newton::DoPostBarrierCollisionHandling(BO::tNewtonObj *newtonObj, coorddef normal)
{
    FUNCTION_MARKER(0x800A2AF0, "NFS4.EXE");
    coorddef barrierVec;
    barrierVec.x = (sint32)(0U - ((uint32)(normal.z / 256) << 8));
    barrierVec.y = 0;
    barrierVec.z = (sint32)((uint32)(normal.x / 256) << 8);
    uint32 x = (uint32)(normal.x / 256) * (uint32)(newtonObj->linearVel.x / 256);
    uint32 y = (uint32)(normal.y / 256) * (uint32)(newtonObj->linearVel.y / 256);
    uint32 z = (uint32)(normal.z / 256) * (uint32)(newtonObj->linearVel.z / 256);
    sint32 impactVel = (sint32)(x + y + z);
    if (impactVel < 0)
        impactVel = (sint32)(0U - (uint32)impactVel);
    impactVel = (sint32)(0U - (uint32)impactVel);
    sint32 distRetreat = impactVel / 16;
    if (distRetreat >= -0x7ad)
        distRetreat = -0x7ae;
    coorddef upVec = {0, 0x10000, 0};
    matrixtdef islandMatrix = {{normal.x, normal.y, normal.z,
                               upVec.x, upVec.y, upVec.z,
                               barrierVec.x, barrierVec.y, barrierVec.z}};
    Car::tObj *carObj = reinterpret_cast<Car::tObj *>(newtonObj);
    impactVel = Physics::AttenuateVelocity(carObj, distRetreat, &islandMatrix);
    Physics::SetCurrentWallType(4);
    Physics::CorrectPostCollisionYaw(carObj, impactVel, normal);
}

void Newton::GenerateVector(sint32 type, coorddef *vector, BWorldSm::Pos *testSimRoadInfo)
{
    FUNCTION_MARKER(0x800A2C98, "NFS4.EXE");
    coorddef upVec = {0, 0x10000, 0};
    coorddef fwdVec;
    fwdVec.y = 0;
    if (type == 1)
    {
        fwdVec.z = (sint32)((uint32)testSimRoadInfo->quadPts[3].z - (uint32)testSimRoadInfo->quadPts[2].z);
        fwdVec.x = (sint32)((uint32)testSimRoadInfo->quadPts[3].x - (uint32)testSimRoadInfo->quadPts[2].x);
    }
    else if (type == 4)
    {
        fwdVec.z = (sint32)((uint32)testSimRoadInfo->quadPts[2].z - (uint32)testSimRoadInfo->quadPts[1].z);
        fwdVec.x = (sint32)((uint32)testSimRoadInfo->quadPts[2].x - (uint32)testSimRoadInfo->quadPts[1].x);
    }
    else if (type == 2)
    {
        fwdVec.z = (sint32)((uint32)testSimRoadInfo->quadPts[1].z - (uint32)testSimRoadInfo->quadPts[0].z);
        fwdVec.x = (sint32)((uint32)testSimRoadInfo->quadPts[1].x - (uint32)testSimRoadInfo->quadPts[0].x);
    }
    else if (type == 8)
    {
        fwdVec.z = (sint32)((uint32)testSimRoadInfo->quadPts[0].z - (uint32)testSimRoadInfo->quadPts[3].z);
        fwdVec.x = (sint32)((uint32)testSimRoadInfo->quadPts[0].x - (uint32)testSimRoadInfo->quadPts[3].x);
    }
    Math::NormalizeShortVector(&fwdVec);
    coorddef result;
    sint32 first = fixedmult(fwdVec.y, upVec.z);
    sint32 second = fixedmult(fwdVec.z, upVec.y);
    result.x = (sint32)((uint32)first - (uint32)second);
    first = fixedmult(fwdVec.z, upVec.x);
    second = fixedmult(fwdVec.x, upVec.z);
    result.y = (sint32)((uint32)first - (uint32)second);
    first = fixedmult(fwdVec.x, upVec.y);
    second = fixedmult(fwdVec.y, upVec.x);
    result.z = (sint32)((uint32)first - (uint32)second);
    *vector = result;
}

static coorddef OffsetUndrivableProbe(const coorddef &point, const coorddef &offset)
{
    coorddef result = {
        (sint32)((uint32)point.x + (uint32)offset.x),
        (sint32)((uint32)point.y + (uint32)offset.y),
        (sint32)((uint32)point.z + (uint32)offset.z)
    };
    return result;
}

static coorddef UndrivableCenterNormal(const coorddef &origin, const BWorldSm::Pos &road)
{
    uint32 x = 0;
    uint32 z = 0;
    for (sint32 j = 0; j < 4; ++j)
    {
        if (road.simQuad != 0)
        {
            x += (uint32)road.quadPts[j].x;
            z += (uint32)road.quadPts[j].z;
        }
        else
        {
            const Trk::NewSlice &slice = BWorldSm::slices[road.slice];
            x += (uint32)slice.center[0];
            z += (uint32)slice.center[2];
        }
    }
    coorddef normal = {
        (sint32)((uint32)origin.x - (uint32)((sint32)x / 4)),
        0,
        (sint32)((uint32)origin.z - (uint32)((sint32)z / 4))
    };
    return normal;
}

void Newton::TestForUndrivableSurfaces(BO::tNewtonObj *newtonObj)
{
    FUNCTION_MARKER(0x800A2E84, "NFS4.EXE");

    Car::tObj *car = reinterpret_cast<Car::tObj *>(newtonObj);
    const coorddef cautionaryCenter = newtonObj->roadCenterPoint;
    sint32 newHeight = 0;
    coorddef speedVec = { 0, 0, 0 };
    testSimRoadInfo = newtonObj->simRoadInfo;
    if (newtonObj->speedXZ > 0x280000)
    {
        speedVec.x = (sint32)((uint32)newtonObj->linearVel.x * 5u) / 256;
        speedVec.y = (sint32)((uint32)newtonObj->linearVel.y * 5u) / 256;
        speedVec.z = (sint32)((uint32)newtonObj->linearVel.z * 5u) / 256;
    }

    for (sint32 i = 0; i < 4; ++i)
    {
        coorddef testPoint = OffsetUndrivableProbe(car->wheel[i].currentPos, speedVec);
        BWorldSm::FindClosestQuadRez(&testPoint, &testSimRoadInfo, 1);
        coorddef normal;
        if (testSimRoadInfo.offEdge != 0)
        {
            GenerateVector((sint8)testSimRoadInfo.offEdge, &normal, &newtonObj->simRoadInfo);
        }
        else
        {
            if (Netwon_CheckForBadQuad(newtonObj, &testSimRoadInfo, i) == 0)
                continue;

            newtestSimRoadInfo = testSimRoadInfo;
            coorddef newTestPoint = testPoint;
            sint32 check = BWorldSm::FindEdgeOff(&testPoint, &newtonObj->simRoadInfo, &testSimRoadInfo, &newHeight);
            if (check == 1 || check == 2 || check == 4 || check == 8)
                GenerateVector(check, &normal, &newtonObj->simRoadInfo);

            if ((check & 3) != 0)
            {
                GenerateVector((check & 1) != 0 ? 1 : 2, &normal, &newtonObj->simRoadInfo);
                newTestPoint = OffsetUndrivableProbe(newTestPoint, normal);
                BWorldSm::FindClosestQuadRez(&newTestPoint, &newtestSimRoadInfo, 1);
                if (Netwon_CheckForBadQuad(newtonObj, &newtestSimRoadInfo, i) != 0)
                {
                    if ((check & 12) != 0)
                    {
                        GenerateVector((check & 4) != 0 ? 4 : 8, &normal, &newtonObj->simRoadInfo);
                        newtestSimRoadInfo = testSimRoadInfo;
                        newTestPoint = OffsetUndrivableProbe(testPoint, normal);
                        BWorldSm::FindClosestQuadRez(&newTestPoint, &newtestSimRoadInfo, 1);
                        if (Netwon_CheckForBadQuad(newtonObj, &newtestSimRoadInfo, i) != 0)
                            normal = UndrivableCenterNormal(cautionaryCenter, testSimRoadInfo);
                    }
                    else
                    {
                        normal = UndrivableCenterNormal(newtonObj->position, testSimRoadInfo);
                    }
                }
            }
            else if (check != 4 && check != 8)
            {
                normal = UndrivableCenterNormal(newtonObj->position, testSimRoadInfo);
                if (normal.x == 0 && normal.z == 0)
                {
                    normal.x = (sint32)(0u - (uint32)newtonObj->linearVel.x);
                    normal.z = (sint32)(0u - (uint32)newtonObj->linearVel.z);
                    if (normal.x == 0 && normal.z == 0)
                        continue;
                }
            }
        }

        normal.y = 0;
        Math::NormalizeShortVector(&normal);
        DoPostBarrierCollisionHandling(newtonObj, normal);
        newtonObj->collision.collisionPoint = car->wheel[i].currentPos;
        AIPhysic::ProcessBarrierCollision(car);
        if ((car->carFlags & 4u) != 0)
            Physics::FixEngineRpm(car);
        sint32 impulse = newtonObj->collision.impulse;
        if (impulse > 0xA0000)
        {
            const sint32 damageZones[4] = { 0, 2, 6, 4 };
            AddDamageZone(newtonObj, impulse, damageZones[i], 1);
        }
        return;
    }
}

void Newton::LimitAngularVelocity(BO::tNewtonObj *newtonObj)
{
    FUNCTION_MARKER(0x800A35DC, "NFS4.EXE");
    if (newtonObj->angularVel.x > 0x18000)
        newtonObj->angularVel.x = 0x18000;
    else if (newtonObj->angularVel.x < -0x18000)
        newtonObj->angularVel.x = -0x18000;
    if (newtonObj->angularVel.y > 0x18000)
        newtonObj->angularVel.y = 0x18000;
    else if (newtonObj->angularVel.y < -0x18000)
        newtonObj->angularVel.y = -0x18000;
    if (newtonObj->angularVel.z > 0x18000)
        newtonObj->angularVel.z = 0x18000;
    else if (newtonObj->angularVel.z < -0x18000)
        newtonObj->angularVel.z = -0x18000;
}

void Newton::ApplyTheLawOfGravity(BO::tNewtonObj *newtonObj)
{
    FUNCTION_MARKER(0x800A3674, "NFS4.EXE");
    if (newtonObj->active == 0)
        return;
    Car::tObj *car = reinterpret_cast<Car::tObj *>(newtonObj);
    sint32 timeCount;
    sint32 elapsedTime;
    sint32 iTimeCount;
    if (Sched::ExecuteCheck(1, 3, newtonObj->distToPlayer, newtonObj->objID,
            &timeCount, &elapsedTime, &iTimeCount, car->forceNoSimOptz) == 0)
        return;
    coorddef normal;
    if (newtonObj->simOptz >= 2)
    {
        newtonObj->linearVel.y = 0;
        newtonObj->flightTime = 0;
        sint32 elevationOfGround = FindGroundElevationAndNormalFast(newtonObj, &normal);
        newtonObj->position.y = (sint32)((uint32)elevationOfGround + (uint32)newtonObj->dimension.y);
        return;
    }
    sint32 elevationOfGround = FindGroundElevationAndNormal(newtonObj, &normal);
    coorddef shadowNormal = normal;
    sint32 previousElevation = newtonObj->groundElevation;
    newtonObj->groundElevation = elevationOfGround;
    sint32 groundVel = (sint32)(((uint32)elevationOfGround - (uint32)previousElevation) * (uint32)timeCount);
    sint32 objAltitude = newtonObj->objAltitude;
    sint32 modifiedGravity = fixedmult((sint32)((uint32)elapsedTime * 0x4800U), newtonObj->gravityMult);
    newtonObj->linearVel.y = (sint32)((uint32)newtonObj->linearVel.y - (uint32)modifiedGravity);
    sint32 xDot = fixedmult(newtonObj->orientMat.m[3], normal.x);
    sint32 yDot = fixedmult(newtonObj->orientMat.m[4], normal.y);
    sint32 zDot = fixedmult(newtonObj->orientMat.m[5], normal.z);
    sint32 orientation = (sint32)((uint32)xDot + (uint32)yDot + (uint32)zDot);
    newtonObj->orientationToGround.y = orientation;
    CalculateGroundShadowMatrix(newtonObj, &shadowNormal, orientation);
    sint32 absX = newtonObj->linearVel.x;
    sint32 absZ = newtonObj->linearVel.z;
    if (absX < 0)
        absX = (sint32)(0U - (uint32)absX);
    if (absZ < 0)
        absZ = (sint32)(0U - (uint32)absZ);
    newtonObj->speedXZ = Math::BetterDist(absX, absZ);
    sint32 gravityModifier = (sint32)((uint32)newtonObj->roadGravityModifier * 7U +
        (uint32)groundVel - (uint32)newtonObj->groundVel);
    newtonObj->roadGravityModifier = gravityModifier / 8;
    newtonObj->groundVel = groundVel;
    if (objAltitude < 0x3333)
    {
        if (newtonObj->flightTime == 0)
            CheckForSpikeBelts(newtonObj);
        else
        {
            if ((car->carFlags & 4) != 0)
                Physics::FixEngineRpm(car);
            sint32 relativeClosingVelocity = (sint32)((uint32)groundVel - (uint32)newtonObj->linearVel.y);
            if (newtonObj->orientationToGround.y < 0xb334)
            {
                coorddef collisionPoint = newtonObj->roadCenterPoint;
                collisionPoint.y = (sint32)((uint32)collisionPoint.y - 0x1999U);
                Collide::TestWithPlane(newtonObj, &normal, &collisionPoint);
                sint32 impulse = newtonObj->collision.impulse;
                if (impulse > 0x50000)
                    newtonObj->collision.impulse = impulse >= 0x140000 ? impulse : 0x140000;
                if (newtonObj->orientationToGround.y < 0x3333)
                    car->collision.smoking = 1;
            }
            else
            {
                sint32 bounceVel = 0;
                for (sint32 k = 0; k < 4; ++k)
                    bounceVel = (sint32)((uint32)bounceVel + (uint32)car->wheel[k].wheelAcc);
                if (bounceVel > 0)
                {
                    sint32 scaledBounce = (sint32)((uint32)bounceVel * 3U);
                    newtonObj->linearVel.y = (sint32)((uint32)groundVel + (uint32)(scaledBounce / 4));
                    newtonObj->collision.impulse = (sint32)((uint32)relativeClosingVelocity * 2U);
                    newtonObj->flightTime = 0;
                    newtonObj->position.y = (sint32)((uint32)newtonObj->position.y - (uint32)objAltitude);
                    objAltitude = 0xccc;
                    newtonObj->objAltitude = 0xccc;
                    if (newtonObj->collision.impulse > 0x140000)
                    {
                        newtonObj->collision.sfxType = 0x10000;
                        newtonObj->collision.otherObj = 0;
                        newtonObj->collision.collisionPoint = newtonObj->position;
                        AddDamageZone(newtonObj, newtonObj->collision.impulse, 9, 0);
                    }
                    if (Force::IsForceOn(car) != 0)
                        Force::HitWall(newtonObj->collision.impulse);
                }
            }
            LimitAngularVelocity(newtonObj);
        }
        if (objAltitude < 0xa3d)
        {
            sint32 previousY = newtonObj->position.y;
            sint32 carFlags = car->carFlags;
            newtonObj->objAltitude = 0;
            newtonObj->position.y = (sint32)((uint32)previousY - (uint32)objAltitude);
            if ((carFlags & 0x400) == 0 && newtonObj->orientationToGround.y > 0xe666)
                newtonObj->flightTime = 0;
            if (groundVel > 0x5ffff)
            {
                sint32 scale = groundVel > 0xc0000 ? 0xffbe : 0xffdf;
                newtonObj->linearVel.x = fixedmult(newtonObj->linearVel.x, scale);
                newtonObj->linearVel.z = fixedmult(newtonObj->linearVel.z, scale);
            }
            if (newtonObj->speedXZ < 0x50000)
                newtonObj->linearVel.y = groundVel / 4;
            else if (newtonObj->speedXZ < 0xa0000)
                newtonObj->linearVel.y = groundVel / 2;
            else
                newtonObj->linearVel.y = groundVel;
        }
    }
    else
    {
        newtonObj->roadGravityModifier = 0;
        newtonObj->flightTime = (uint16)((uint32)newtonObj->flightTime + (uint32)(uint16)elapsedTime);
        modifiedGravity = fixedmult((sint32)((uint32)elapsedTime * 0x4800U), newtonObj->gravityMult);
        newtonObj->linearVel.y = (sint32)((uint32)newtonObj->linearVel.y - (uint32)modifiedGravity);
    }
    newtonObj->lastUpdated = simGlobal.gameTicks;
    if (BWorldSm::TunnelFlagSm(&newtonObj->simRoadInfo) && newtonObj->linearVel.y > 0 &&
        (sint32)((uint32)newtonObj->position.y - (uint32)newtonObj->roadCenterPoint.y) > 0x80000)
        newtonObj->linearVel.y = (sint32)(0U - (uint32)newtonObj->linearVel.y);
}

sint32 Newton::CalculateRoadPositionFromSliceAndPosition(sint32 slice, coorddef *position, matrixtdef *matrix)
{
    FUNCTION_MARKER(0x800A3B60, "NFS4.EXE");
    coorddef centerBack;
    centerBack.x = BWorldSm::slices[slice].center[0];
    centerBack.y = BWorldSm::slices[slice].center[1];
    centerBack.z = BWorldSm::slices[slice].center[2];
    coorddef carRelative;
    carRelative.x = (sint32)((uint32)position->x - (uint32)centerBack.x);
    carRelative.y = (sint32)((uint32)position->y - (uint32)centerBack.y);
    carRelative.z = (sint32)((uint32)position->z - (uint32)centerBack.z);
    return (sint32)(
        (uint32)(matrix->m[0] / 256) * (uint32)(carRelative.x / 256) +
        (uint32)(matrix->m[1] / 256) * (uint32)(carRelative.y / 256) +
        (uint32)(matrix->m[2] / 256) * (uint32)(carRelative.z / 256));
}

sint32 Newton::CalculateRoadPosition(BO::tNewtonObj *newtonObj)
{
    FUNCTION_MARKER(0x800A3C54, "NFS4.EXE");
    sint32 slice = newtonObj->simRoadInfo.slice;
    coorddef carPos = newtonObj->position;
    coorddef *position = &carPos;
    matrixtdef *matrix = &newtonObj->roadMatrix;
    coorddef centerBack;
    centerBack.x = BWorldSm::slices[slice].center[0];
    centerBack.y = BWorldSm::slices[slice].center[1];
    centerBack.z = BWorldSm::slices[slice].center[2];
    coorddef carRelative;
    carRelative.x = (sint32)((uint32)position->x - (uint32)centerBack.x);
    carRelative.y = (sint32)((uint32)position->y - (uint32)centerBack.y);
    carRelative.z = (sint32)((uint32)position->z - (uint32)centerBack.z);
    return (sint32)(
        (uint32)(matrix->m[0] / 256) * (uint32)(carRelative.x / 256) +
        (uint32)(matrix->m[1] / 256) * (uint32)(carRelative.y / 256) +
        (uint32)(matrix->m[2] / 256) * (uint32)(carRelative.z / 256));
}
