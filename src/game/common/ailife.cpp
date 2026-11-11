#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/ai.h"
#include "game/common/aiinit.h"
#include "game/common/ailife.h"
#include "game/common/aiphysic.h"
#include "game/common/aispeeds.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/fastrand.h"
#include "game/common/gmesetup.h"
#include "game/common/mathnfs.h"
#include "game/common/newton.h"
#include "game/common/physics.h"
#include "game/common/r3dcar.h"
#include "game/psx/draww.h"
#include "lib/psx/fixdmult.h"

sint32 AILife::EvaluateLife(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80067650, "NFS4.EXE");
    if ((carObj->carFlags & 0x400) != 0)
        return 0;
    if (carObj->basisCar != 0 && IsCoordInThisLiveArea(&carObj->N.position, carObj->basisCar) != 0)
        return 0;
    Car::tObj *basisCar = IsTrafficCarInAnyLiveArea(carObj);
    carObj->basisCar = basisCar;
    return basisCar == 0;
}

void AILife::RCSetSpeeds(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800676B4, "NFS4.EXE");
    AISpeeds::CalcDesiredSpeed(carObj);
    carObj->currentSpeed = carObj->desiredSpeed;
}

void AILife::RCPickSliceAndDirection(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800676E4, "NFS4.EXE");
    uint32 seed = randSeed;
    uint32 firstRandom = fastRandom * seed;
    uint32 secondRandom = (firstRandom & 0xFFFFU) * seed;
    uint32 basisIndex = ((uint32)Cars::gNumLifeBasisCars * ((firstRandom >> 8) & 0xFFFFU)) >> 16;
    randtemp = firstRandom;
    fastRandom = firstRandom & 0xFFFFU;
    randtemp = secondRandom;
    fastRandom = secondRandom & 0xFFFFU;
    carObj->basisCar = Cars::gLifeBasisCarList[basisIndex];
    sint32 approachSide = (((secondRandom >> 8) & 0xFFFFU) * 1000U >> 16) < 500U ? -1 : 1;
    sint32 basisSpeed = carObj->basisCar->currentSpeed;
    if (basisSpeed < 0)
        basisSpeed = (sint32)(0U - (uint32)basisSpeed);
    if (basisSpeed > 0x1E0000)
        approachSide = carObj->basisCar->direction;
    sint32 search = carObj->basisCar->sortIndex;
    sint32 offset = (sint32)((uint32)approachSide * 36U);
    sint32 count = 0;
    for (;;)
    {
        sint32 numCars = Cars::gNumCars;
        if (count >= numCars)
            break;
        sint32 sortedIndex = (sint32)((uint32)search + (uint32)numCars) % numCars;
        Car::tObj *checkCar = Cars::gSortedList[sortedIndex];
        if (checkCar != carObj && checkCar != carObj->basisCar && (checkCar->carFlags & 0x100) != 0)
        {
            sint32 basisOuterSlice = (sint32)((uint32)carObj->basisCar->N.simRoadInfo.slice + (uint32)offset);
            if (offset >= 0)
            {
                if (basisOuterSlice >= gNumSlices)
                    basisOuterSlice = (sint32)((uint32)basisOuterSlice - (uint32)gNumSlices);
            }
            else if (basisOuterSlice < 0)
                basisOuterSlice = (sint32)((uint32)basisOuterSlice + (uint32)gNumSlices);
            coorddef basisOuterCoord;
            basisOuterCoord.x = BWorldSm::slices[basisOuterSlice].center[0];
            basisOuterCoord.y = BWorldSm::slices[basisOuterSlice].center[1];
            basisOuterCoord.z = BWorldSm::slices[basisOuterSlice].center[2];
            bool continuesOutward = false;
            if (IsCoordInThisVisibleArea(&basisOuterCoord, checkCar) != 0)
            {
                sint32 distance = AIWorld::ApxSplineDistance(checkCar, carObj->basisCar);
                if (distance < 0)
                    distance = (sint32)((uint32)distance + 0xFFFFU);
                sint32 directedDistance = (sint32)((uint32)approachSide * (uint32)(distance >> 16));
                continuesOutward = directedDistance > 0;
            }
            if (!continuesOutward)
                break;
            carObj->basisCar = checkCar;
        }
        search = (sint32)((uint32)search + (uint32)approachSide);
        count = (sint32)((uint32)count + 1U);
    }
    uint32 product = fastRandom * randSeed;
    randtemp = product;
    fastRandom = product & 0xFFFFU;
    carObj->direction = (((product >> 8) & 0xFFFFU) * 1000U >> 16) < 500U ? -1 : 1;
    if (AITune::oneWay != 0)
        carObj->direction = GameSetup::gData.reverseTrack == 0 ? 1 : -1;
    product = fastRandom * randSeed;
    offset = (sint32)((((product >> 21) & 7U) + 28U) * (uint32)approachSide);
    randtemp = product;
    fastRandom = product & 0xFFFFU;
    carObj->desiredDirection = carObj->direction;
    sint32 basisSlice = carObj->basisCar->N.simRoadInfo.slice;
    uint32 newSlice = (uint32)(uint16)basisSlice + (uint32)offset;
    sint32 signedSlice = (sint32)((uint32)basisSlice + (uint32)offset);
    if (offset >= 0)
    {
        if (signedSlice >= gNumSlices)
            newSlice -= (uint16)gNumSlices;
    }
    else if (signedSlice < 0)
        newSlice += (uint16)gNumSlices;
    carObj->N.simRoadInfo.slice = (sint16)newSlice;
    char format[] = " psad checked group, basis now %d(s=%d) new slice=%d\n";
    Debug(format, carObj->basisCar->carIndex, carObj->basisCar->N.simRoadInfo.slice, carObj->N.simRoadInfo.slice);
}

void AILife::RCPickDesiredLatPosition(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80067B1C, "NFS4.EXE");
    sint32 newSlice = carObj->N.simRoadInfo.slice;
    bool rightSide = carObj->direction == AITune::driveSide;
    uint32 product = fastRandom * randSeed;
    sint32 width = (rightSide ? BWorldSm::slices[newSlice].avgPavedWidthRt : BWorldSm::slices[newSlice].avgPavedWidthLf) * 32768;
    randtemp = product;
    fastRandom = product & 0xFFFFU;
    uint32 laneCount = BWorldSm::slices[newSlice].laneCount;
    laneCount = rightSide ? laneCount & 15U : laneCount >> 4;
    sint32 randNumLanes = (sint32)((laneCount * ((product >> 8) & 0xFFFFU)) >> 16) + 1;
    sint32 lateralPosition;
    if (rightSide)
        lateralPosition = width * randNumLanes - width / 2;
    else
        lateralPosition = -width * randNumLanes + width / 2;
    carObj->desiredLatPos = lateralPosition;
    lateralPosition = (sint32)((uint32)carObj->desiredLatPos + (uint32)carObj->laneSlack);
    carObj->desiredLatPos = lateralPosition;
    carObj->rampDesiredLatPos = lateralPosition;
}

void AILife::PlaceCarAtLocation(Car::tObj *carObj, sint32 slice, sint32 desiredLatPos, sint32 direction, sint32 currentSpeed, sint32 rotation1024)
{
    FUNCTION_MARKER(0x80067C50, "NFS4.EXE");
    carObj->desiredLatPos = desiredLatPos;
    carObj->direction = direction;
    carObj->desiredDirection = direction;
    carObj->N.simRoadInfo.slice = (sint16)slice;
    carObj->currentSpeed = currentSpeed;
    PlaceCarAtLocation(carObj, rotation1024);
}

void AILife::SetInitialSlicePositionOrientationEtc(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80067C8C, "NFS4.EXE");
    coorddef offset = { 0, 0, 0 };
    offset.x = carObj->desiredLatPos;
    offset.y = 65536;
    Newton::SetInitialSlicePositionOrientationEtc(&carObj->N, carObj->N.simRoadInfo.slice, &offset, carObj->direction);
}

void AILife::PlaceCarAtLocation(Car::tObj *carObj, sint32 rotation1024)
{
    FUNCTION_MARKER(0x80067CE0, "NFS4.EXE");
    carObj->N.active = 1;
    carObj->rampDesiredLatPos = carObj->desiredLatPos;
    carObj->desiredSpeed = carObj->currentSpeed;
    AIPhysic::ResetCar(carObj);
    if (stackSpeedUpEnbabledFlag != 0) {
        stackSpeedUpEnbabledFlag = 0;
        SetInitialSlicePositionOrientationEtc(carObj);
        stackSpeedUpEnbabledFlag = 1;
    } else {
        SetInitialSlicePositionOrientationEtc(carObj);
    }
    coorddef targetDirection = { 0, 0, 0 };
    if (carObj->currentSpeed != 0) {
        targetDirection.x = carObj->N.orientMat.m[6];
        targetDirection.y = carObj->N.orientMat.m[7];
        targetDirection.z = carObj->N.orientMat.m[8];
        sint32 speed = carObj->currentSpeed;
        if (speed < 0)
            speed = (sint32)(0U - (uint32)speed);
        targetDirection.x = fixedmult(speed, targetDirection.x);
        targetDirection.y = fixedmult(speed, targetDirection.y);
        targetDirection.z = fixedmult(speed, targetDirection.z);
    }
    carObj->N.linearVel = targetDirection;
    if ((carObj->carFlags & 4) != 0)
        Physics::ResetCar(carObj);
    matrixtdef rotMatrix;
    xformy(&rotMatrix, rotation1024);
    Math::fasttransmult(&carObj->N.orientMat, &rotMatrix, &carObj->N.orientMat);
    for (sint32 i = 0; i < 8; i += 4) {
        sint32 a = carObj->N.orientMat.m[i];
        sint32 b = carObj->N.orientMat.m[i + 1];
        sint32 c = carObj->N.orientMat.m[i + 2];
        sint32 d = carObj->N.orientMat.m[i + 3];
        carObj->N.shadowMat.m[i] = a;
        carObj->N.shadowMat.m[i + 1] = b;
        carObj->N.shadowMat.m[i + 2] = c;
        carObj->N.shadowMat.m[i + 3] = d;
    }
    carObj->N.shadowMat.m[8] = carObj->N.orientMat.m[8];
    AIInit::ClearAICar(carObj);
    sint32 roadPosition = Newton::CalculateRoadPosition(&carObj->N);
    carObj->rampDesiredLatPos = roadPosition;
    carObj->desiredLatPos = roadPosition;
    carObj->roadPosition = roadPosition;
    carObj->roadSpan = Cars::CalculateRoadSpan(carObj);
    AIWorld::CalculateLaneInfo(carObj);
}

void AILife::ReencarnateTraffic(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80067EE4, "NFS4.EXE");
    uint32 product = fastRandom * randSeed;
    randtemp = product;
    fastRandom = product & 0xFFFFU;
    if ((carObj->carFlags & 0x10) != 0)
        R3DCar::ChangeTrafficColor(carObj, (sint32)((((product >> 8) & 0xFFFFU) * 3U) >> 16));
    AI::ChooseNewLaneSlack(carObj);
    AISpeeds::SetTrafficSpeedRandomFactor(carObj);
    RCPickSliceAndDirection(carObj);
    RCPickDesiredLatPosition(carObj);
    RCSetSpeeds(carObj);
    PlaceCarAtLocation(carObj, 0);
}

void AILife::ReencarnateTrafficByPosition(Car::tObj *carObj, sint32 slice, sint32 travelDirection, coorddef *pos, matrixtdef *ori)
{
    FUNCTION_MARKER(0x80067F94, "NFS4.EXE");
    coorddef zero = { 0, 0, 0 };
    coorddef offset = { 0, 0, 0 };
    uint32 product = fastRandom * randSeed;
    randtemp = product;
    fastRandom = product & 0xFFFFU;
    if ((carObj->carFlags & 0x10) != 0)
        R3DCar::ChangeTrafficColor(carObj, (sint32)((((product >> 8) & 0xFFFFU) * 3U) >> 16));
    AI::ChooseNewLaneSlack(carObj);
    AISpeeds::SetTrafficSpeedRandomFactor(carObj);
    carObj->basisCar = 0;
    carObj->direction = travelDirection;
    carObj->desiredDirection = travelDirection;
    carObj->N.simRoadInfo.slice = (sint16)slice;
    RCSetSpeeds(carObj);
    sint32 initialSlice = carObj->N.simRoadInfo.slice;
    sint32 initialDirection = carObj->direction;
    carObj->currentSpeed = 0;
    Newton::SetInitialSlicePositionOrientationEtc(&carObj->N, initialSlice, &offset, initialDirection);
    coorddef position = *pos;
    carObj->N.position = position;
    for (sint32 i = 0; i < 8; i += 4)
    {
        sint32 a = ori->m[i];
        sint32 b = ori->m[i + 1];
        sint32 c = ori->m[i + 2];
        sint32 d = ori->m[i + 3];
        carObj->N.orientMat.m[i] = a;
        carObj->N.orientMat.m[i + 1] = b;
        carObj->N.orientMat.m[i + 2] = c;
        carObj->N.orientMat.m[i + 3] = d;
    }
    carObj->N.orientMat.m[8] = ori->m[8];
    for (sint32 i = 0; i < 8; i += 4)
    {
        sint32 a = ori->m[i];
        sint32 b = ori->m[i + 1];
        sint32 c = ori->m[i + 2];
        sint32 d = ori->m[i + 3];
        carObj->N.shadowMat.m[i] = a;
        carObj->N.shadowMat.m[i + 1] = b;
        carObj->N.shadowMat.m[i + 2] = c;
        carObj->N.shadowMat.m[i + 3] = d;
    }
    carObj->N.shadowMat.m[8] = ori->m[8];
    carObj->N.linearVel = zero;
    carObj->N.speedXZ = 0;
    AIInit::ClearAICar(carObj);
    sint32 roadPosition = Newton::CalculateRoadPosition(&carObj->N);
    carObj->rampDesiredLatPos = roadPosition;
    carObj->desiredLatPos = roadPosition;
    carObj->roadPosition = roadPosition;
    carObj->roadSpan = Cars::CalculateRoadSpan(carObj);
    AIWorld::CalculateLaneInfo(carObj);
    RCPickDesiredLatPosition(carObj);
}

void AILife::ReencarnateCopBySlice(Car::tObj *carObj, sint32 slice, sint32 travelDirection, sint32 roadSide, sint32 moving)
{
    FUNCTION_MARKER(0x800681A0, "NFS4.EXE");
    sint32 oneWay = AITune::oneWay;
    carObj->N.simRoadInfo.slice = (sint16)slice;
    if (oneWay != 0)
        travelDirection = GameSetup::gData.reverseTrack == 0 ? 1 : -1;
    carObj->direction = travelDirection;
    carObj->desiredDirection = travelDirection;
    sint32 lateralPosition;
    if (moving == 0)
    {
        if (roadSide == -1)
        {
            sint32 width = BWorldSm::slices[slice].avgPavedWidthLf * 32768;
            sint32 numLanes = BWorldSm::slices[slice].laneCount >> 4;
            lateralPosition = -0x20000 - width * numLanes;
        }
        else
        {
            sint32 width = BWorldSm::slices[slice].avgPavedWidthRt * 32768;
            sint32 numLanes = BWorldSm::slices[slice].laneCount & 15;
            lateralPosition = 0x20000 + width * numLanes;
        }
    }
    else
    {
        sint32 numLanes = 0;
        if (carObj->direction != 1)
            numLanes = BWorldSm::slices[slice].laneCount >> 4;
        if (numLanes != 0)
        {
            sint32 width = BWorldSm::slices[slice].avgPavedWidthLf * 32768;
            lateralPosition = -width * numLanes + width / 2;
        }
        else
        {
            sint32 width = BWorldSm::slices[slice].avgPavedWidthRt * 32768;
            numLanes = BWorldSm::slices[slice].laneCount & 15;
            lateralPosition = width * numLanes - width / 2;
        }
    }
    carObj->desiredLatPos = lateralPosition;
    if (moving != 0)
        RCSetSpeeds(carObj);
    else
    {
        carObj->desiredSpeed = 0;
        carObj->currentSpeed = 0;
    }
    sint32 desiredLatPos = carObj->desiredLatPos;
    carObj->rampDesiredLatPos = desiredLatPos;
    carObj->roadPosition = desiredLatPos;
    PlaceCarAtLocation(carObj, 0);
}

void AILife::ReencarnateCopByPosition(Car::tObj *carObj, sint32 slice, sint32 travelDirection, coorddef *pos, matrixtdef *ori)
{
    FUNCTION_MARKER(0x80068324, "NFS4.EXE");
    coorddef zero = { 0, 0, 0 };
    coorddef offset = { 0, 0, 0 };
    sint32 oneWay = AITune::oneWay;
    carObj->N.simRoadInfo.slice = (sint16)slice;
    if (oneWay != 0)
        travelDirection = GameSetup::gData.reverseTrack == 0 ? 1 : -1;
    carObj->direction = travelDirection;
    carObj->desiredDirection = travelDirection;
    carObj->desiredSpeed = 0;
    carObj->currentSpeed = 0;
    AIPhysic::ResetCar(carObj);
    Newton::SetInitialSlicePositionOrientationEtc(&carObj->N, carObj->N.simRoadInfo.slice, &offset, carObj->direction);
    coorddef position = *pos;
    carObj->N.position = position;
    for (sint32 i = 0; i < 8; i += 4)
    {
        sint32 a = ori->m[i];
        sint32 b = ori->m[i + 1];
        sint32 c = ori->m[i + 2];
        sint32 d = ori->m[i + 3];
        carObj->N.orientMat.m[i] = a;
        carObj->N.orientMat.m[i + 1] = b;
        carObj->N.orientMat.m[i + 2] = c;
        carObj->N.orientMat.m[i + 3] = d;
    }
    carObj->N.orientMat.m[8] = ori->m[8];
    for (sint32 i = 0; i < 8; i += 4)
    {
        sint32 a = ori->m[i];
        sint32 b = ori->m[i + 1];
        sint32 c = ori->m[i + 2];
        sint32 d = ori->m[i + 3];
        carObj->N.shadowMat.m[i] = a;
        carObj->N.shadowMat.m[i + 1] = b;
        carObj->N.shadowMat.m[i + 2] = c;
        carObj->N.shadowMat.m[i + 3] = d;
    }
    carObj->N.shadowMat.m[8] = ori->m[8];
    carObj->N.linearVel = zero;
    AIInit::ClearAICar(carObj);
    sint32 roadPosition = Newton::CalculateRoadPosition(&carObj->N);
    carObj->rampDesiredLatPos = roadPosition;
    carObj->desiredLatPos = roadPosition;
    carObj->roadPosition = roadPosition;
    carObj->roadSpan = Cars::CalculateRoadSpan(carObj);
    AIWorld::CalculateLaneInfo(carObj);
}

void AILife::ReencarnateCopByLatPosAndRotation(Car::tObj *carObj, sint32 slice, sint32 travelDirection, sint32 latPos, sint32 rotation1024)
{
    FUNCTION_MARKER(0x800684D4, "NFS4.EXE");
    coorddef zero = { 0, 0, 0 };
    coorddef offset = { 0, 0, 0 };
    carObj->N.active = 1;
    sint32 oneWay = AITune::oneWay;
    carObj->N.simRoadInfo.slice = (sint16)slice;
    if (oneWay != 0)
        travelDirection = GameSetup::gData.reverseTrack == 0 ? 1 : -1;
    carObj->direction = travelDirection;
    carObj->desiredDirection = travelDirection;
    carObj->desiredSpeed = 0;
    carObj->currentSpeed = 0;
    AIPhysic::ResetCar(carObj);
    offset.x = (sint32)((uint32)latPos * (uint32)carObj->direction);
    Newton::SetInitialSlicePositionOrientationEtc(&carObj->N, carObj->N.simRoadInfo.slice, &offset, carObj->direction);
    matrixtdef rotMatrix;
    xformy(&rotMatrix, rotation1024);
    Math::fasttransmult(&carObj->N.orientMat, &rotMatrix, &carObj->N.orientMat);
    for (sint32 i = 0; i < 8; i += 4)
    {
        sint32 a = carObj->N.orientMat.m[i];
        sint32 b = carObj->N.orientMat.m[i + 1];
        sint32 c = carObj->N.orientMat.m[i + 2];
        sint32 d = carObj->N.orientMat.m[i + 3];
        carObj->N.shadowMat.m[i] = a;
        carObj->N.shadowMat.m[i + 1] = b;
        carObj->N.shadowMat.m[i + 2] = c;
        carObj->N.shadowMat.m[i + 3] = d;
    }
    carObj->N.shadowMat.m[8] = carObj->N.orientMat.m[8];
    carObj->N.linearVel = zero;
    AIInit::ClearAICar(carObj);
    sint32 roadPosition = Newton::CalculateRoadPosition(&carObj->N);
    carObj->rampDesiredLatPos = roadPosition;
    carObj->desiredLatPos = roadPosition;
    carObj->roadPosition = roadPosition;
    carObj->roadSpan = Cars::CalculateRoadSpan(carObj);
    AIWorld::CalculateLaneInfo(carObj);
}

sint32 AILife::IsCoordInThisLiveArea(coorddef *tPos, Car::tObj *racer)
{
    FUNCTION_MARKER(0x80068658, "NFS4.EXE");
    sint32 zD = (sint32)((uint32)tPos->z - (uint32)racer->N.position.z);
    sint32 xD = (sint32)((uint32)tPos->x - (uint32)racer->N.position.x);
    if (zD < 0)
        zD = (sint32)(0U - (uint32)zD);
    if (xD < 0)
        xD = (sint32)(0U - (uint32)xD);
    sint32 dist;
    if (zD < xD)
        dist = (sint32)((uint32)xD + (uint32)(zD >> 2));
    else
        dist = (sint32)((uint32)zD + (uint32)(xD >> 2));
    if (dist <= 0xD80000)
        return 1;
    sint64 product = (sint64)dist * (sint32)0x80008001U;
    sint32 highProduct = (sint32)(product >> 32);
    sint32 numerator = (sint32)((uint32)highProduct + (uint32)dist);
    sint32 displayDistance = (sint32)((uint32)(numerator >> 15) - (uint32)(dist >> 31));
    char format[] = "dist=%d";
    Debug(format, displayDistance);
    return 0;
}

Car::tObj * AILife::IsTrafficCarInAnyLiveArea(Car::tObj *traffic)
{
    FUNCTION_MARKER(0x80068704, "NFS4.EXE");
    sint32 racerLoop = 0;
    coorddef *tPos = &traffic->N.position;
    if (Cars::gNumLifeBasisCars <= 0)
        return 0;
    Car::tObj **basisCar = Cars::gLifeBasisCarList;
    do
    {
        sint32 inLiveArea = IsCoordInThisLiveArea(tPos, *basisCar);
        racerLoop = (sint32)((uint32)racerLoop + 1U);
        if (inLiveArea != 0)
            return *basisCar;
        ++basisCar;
    } while (racerLoop < Cars::gNumLifeBasisCars);
    return 0;
}

sint32 AILife::IsCoordInThisVisibleArea(coorddef *tPos, Car::tObj *racer)
{
    FUNCTION_MARKER(0x80068788, "NFS4.EXE");
    sint32 zD = (sint32)((uint32)tPos->z - (uint32)racer->N.position.z);
    sint32 xD = (sint32)((uint32)tPos->x - (uint32)racer->N.position.x);
    if (zD < 0)
        zD = (sint32)(0U - (uint32)zD);
    if (xD < 0)
        xD = (sint32)(0U - (uint32)xD);
    sint32 dist;
    if (zD < xD)
        dist = (sint32)((uint32)xD + (uint32)(zD >> 2));
    else
        dist = (sint32)((uint32)zD + (uint32)(xD >> 2));
    return dist <= 0xAC0000;
}

Car::tObj * AILife::IsCarInAnyVisibleArea(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800687EC, "NFS4.EXE");
    return IsPositionInAnyVisibleArea(&carObj->N.position);
}

Car::tObj * AILife::IsSliceInAnyVisibleArea(sint32 slice)
{
    FUNCTION_MARKER(0x8006880C, "NFS4.EXE");
    sint32 racerLoop = 0;
    Car::tObj **car = Cars::gHumanRaceCarList;
    while (racerLoop < Cars::gNumHumanRaceCars)
    {
        sint32 sliceDist = AIWorld::ApxSplineDistance((*car)->N.simRoadInfo.slice, slice);
        if (sliceDist < 0)
            sliceDist = (sint32)(0U - (uint32)sliceDist);
        if (sliceDist < 0xAC0000)
            return *car;
        ++car;
        racerLoop = (sint32)((uint32)racerLoop + 1U);
    }
    return 0;
}

Car::tObj * AILife::IsSliceCloseToAnyCopCar(sint32 slice)
{
    FUNCTION_MARKER(0x800688AC, "NFS4.EXE");
    sint32 copLoop = 0;
    Car::tObj **car = Cars::gCopCarList;
    while (copLoop < Cars::gNumCopCars)
    {
        sint32 sliceDist = AIWorld::ApxSplineDistance((*car)->N.simRoadInfo.slice, slice);
        if (sliceDist < 0)
            sliceDist = (sint32)(0U - (uint32)sliceDist);
        if (sliceDist < 0x320000)
            return *car;
        ++car;
        copLoop = (sint32)((uint32)copLoop + 1U);
    }
    return 0;
}

Car::tObj * AILife::IsPositionInAnyVisibleArea(coorddef *pos)
{
    FUNCTION_MARKER(0x8006894C, "NFS4.EXE");
    sint32 racerLoop = 0;
    if (Cars::gNumHumanRaceCars <= 0)
        return 0;
    Car::tObj **racer = Cars::gHumanRaceCarList;
    do
    {
        sint32 inVisibleArea = IsCoordInThisVisibleArea(pos, *racer);
        racerLoop = (sint32)((uint32)racerLoop + 1U);
        if (inVisibleArea != 0)
            return *racer;
        ++racer;
    } while (racerLoop < Cars::gNumHumanRaceCars);
    return 0;
}

void AILife::Debug(char *format, ...)
{
    FUNCTION_MARKER(0x800689D0, "NFS4.EXE");
    (void)format;
}
