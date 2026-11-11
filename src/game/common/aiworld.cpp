#include "xport.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/newton.h"
#include "lib/psx/fixdmult.h"

sint32 inverseLaneWidthTable[80];

sint32 AIWorld::ZSplineDistance(Car::tObj *carObj, Car::tObj *otherCarObj)
{
    FUNCTION_MARKER(0x80072F90, "NFS4.EXE");
    coorddef relPos;
    relPos.x = (sint32)((uint32)carObj->N.position.x - (uint32)otherCarObj->N.position.x);
    relPos.y = (sint32)((uint32)carObj->N.position.y - (uint32)otherCarObj->N.position.y);
    relPos.z = (sint32)((uint32)carObj->N.position.z - (uint32)otherCarObj->N.position.z);
    coorddef forward;
    forward.x = carObj->N.roadMatrix.m[6];
    forward.y = carObj->N.roadMatrix.m[7];
    forward.z = carObj->N.roadMatrix.m[8];
    sint32 x = fixedmult(relPos.x, forward.x);
    sint32 z = fixedmult(relPos.z, forward.z);
    return (sint32)((uint32)x + (uint32)z);
}

sint32 AIWorld::ZSplineDistance(coorddef *pos1, coorddef *pos2, matrixtdef *roadMatrix)
{
    FUNCTION_MARKER(0x80073024, "NFS4.EXE");
    coorddef relPos;
    relPos.x = (sint32)((uint32)(*pos1).x - (uint32)(*pos2).x);
    relPos.y = (sint32)((uint32)(*pos1).y - (uint32)(*pos2).y);
    relPos.z = (sint32)((uint32)(*pos1).z - (uint32)(*pos2).z);
    coorddef forward;
    forward.x = (*roadMatrix).m[6];
    forward.y = (*roadMatrix).m[7];
    forward.z = (*roadMatrix).m[8];
    sint32 x = fixedmult(relPos.x, forward.x);
    sint32 z = fixedmult(relPos.z, forward.z);
    return (sint32)((uint32)x + (uint32)z);
}

sint32 AIWorld::ApxSplineDistance(Car::tObj *carObj, Car::tObj *otherCarObj)
{
    FUNCTION_MARKER(0x800730B8, "NFS4.EXE");
    sint32 diff = (sint32)((uint32)carObj->N.simRoadInfo.slice - (uint32)otherCarObj->N.simRoadInfo.slice);
    sint32 halfTrack = gNumSlices / 2;
    if (diff > 0 && diff > halfTrack)
        diff = (sint32)((uint32)diff - (uint32)gNumSlices);
    else if (diff < 0 && diff < -halfTrack)
        diff = (sint32)((uint32)diff + (uint32)gNumSlices);
    return (sint32)((uint32)diff * 0x60000U);
}

sint32 AIWorld::ApxSplineDistance(Car::tObj *carObj, sint32 location)
{
    FUNCTION_MARKER(0x80073128, "NFS4.EXE");
    sint32 diff = (sint32)((uint32)carObj->N.simRoadInfo.slice - (uint32)location);
    sint32 halfTrack = gNumSlices / 2;
    if (diff > 0 && diff > halfTrack)
        diff = (sint32)((uint32)diff - (uint32)gNumSlices);
    else if (diff < 0 && diff < -halfTrack)
        diff = (sint32)((uint32)diff + (uint32)gNumSlices);
    return (sint32)((uint32)diff * 0x60000U);
}

sint32 AIWorld::ApxSplineDistance(sint32 location, Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80073194, "NFS4.EXE");
    sint32 distance = ApxSplineDistance(carObj, location);
    return (sint32)(0U - (uint32)distance);
}

sint32 AIWorld::ApxSplineDistance(sint32 locationA, sint32 locationB)
{
    FUNCTION_MARKER(0x800731BC, "NFS4.EXE");
    sint32 diff = (sint32)((uint32)locationA - (uint32)locationB);
    sint32 halfTrack = gNumSlices / 2;
    if (diff > 0 && diff > halfTrack)
        diff = (sint32)((uint32)diff - (uint32)gNumSlices);
    else if (diff < 0 && diff < -halfTrack)
        diff = (sint32)((uint32)diff + (uint32)gNumSlices);
    return (sint32)((uint32)diff * 0x60000U);
}

sint32 AIWorld::SplineDistance(Car::tObj *carObj, Car::tObj *otherCarObj)
{
    FUNCTION_MARKER(0x80073224, "NFS4.EXE");
    sint32 distance = ApxSplineDistance(carObj, otherCarObj);
    if ((uint32)distance + 0xc0000U <= 0x180000U)
        distance = ZSplineDistance(carObj, otherCarObj);
    return distance;
}

sint32 AIWorld::SplineDistance(Car::tObj *carObj, sint32 location, coorddef *position)
{
    FUNCTION_MARKER(0x8007327C, "NFS4.EXE");
    sint32 distance = ApxSplineDistance(carObj, location);
    if ((uint32)distance + 0xc0000U <= 0x180000U)
        distance = ZSplineDistance(&carObj->N.position, position, &carObj->N.roadMatrix);
    return distance;
}

sint32 AIWorld::GameOdometer(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800732D8, "NFS4.EXE");
    return (sint32)((uint32)carObj->N.totalSlice * 0x60000U);
}

sint32 AIWorld::IsDriveableLaneInSliceRange(sint32 startSlice, sint32 numSlicesToCheck, sint32 direction, sint32 laneIndex)
{
    FUNCTION_MARKER(0x800732F0, "NFS4.EXE");
    sint32 laneOffset = (sint32)(7U - (uint32)laneIndex);
    sint32 profileIndex = (sint32)(8U - (uint32)laneOffset);
    if (profileIndex < 0)
        profileIndex = 0;
    if (profileIndex >= 16)
        profileIndex = 15;
    sint32 mask = 1 << (15 - profileIndex);
    sint32 offset = 0;
    const sint32 numSlices = gNumSlices;
    Trk::NewSlice *slices = BWorldSm::slices;
    for (sint32 checkSliceOffset = 0; checkSliceOffset < numSlicesToCheck; ++checkSliceOffset)
    {
        sint32 checkSlice = (sint32)((uint32)startSlice + (uint32)offset);
        if (offset >= 0)
        {
            if (checkSlice >= numSlices)
                checkSlice = (sint32)((uint32)checkSlice - (uint32)numSlices);
        }
        else if (checkSlice < 0)
            checkSlice = (sint32)((uint32)checkSlice + (uint32)numSlices);
        sint32 profile = slices[checkSlice].pavedProfile;
        offset = (sint32)((uint32)offset + (uint32)direction);
        if ((profile & mask) == 0)
            return 0;
    }
    return 1;
}

sint32 AIWorld::IsDriveableLane(sint32 slice, sint32 laneIndex)
{
    FUNCTION_MARKER(0x800733A8, "NFS4.EXE");
    sint32 laneOffset = (sint32)(7U - (uint32)laneIndex);
    sint32 profileIndex = (sint32)(8U - (uint32)laneOffset);
    if (profileIndex < 0)
        profileIndex = 0;
    if (profileIndex >= 16)
        profileIndex = 15;
    sint32 mask = 1 << (15 - profileIndex);
    return BWorldSm::slices[slice].pavedProfile & mask;
}

sint32 AIWorld::GetProfileMask(sint32 laneIndex)
{
    FUNCTION_MARKER(0x800733FC, "NFS4.EXE");
    sint32 laneOffset = (sint32)(7U - (uint32)laneIndex);
    sint32 profileIndex = (sint32)(8U - (uint32)laneOffset);
    if (profileIndex < 0)
        profileIndex = 0;
    if (profileIndex >= 16)
        profileIndex = 15;
    sint32 mask = 1 << (15 - profileIndex);
    return mask;
}

sint32 AIWorld::IsDriveableLane_UsingMask(sint32 slice, sint32 mask)
{
    FUNCTION_MARKER(0x8007343C, "NFS4.EXE");
    return BWorldSm::slices[slice].pavedProfile & mask;
}

sint32 AIWorld::CheckForBarrierBetweenLanes(sint32 slice, sint32 lane0, sint32 lane1)
{
    FUNCTION_MARKER(0x80073458, "NFS4.EXE");
    sint32 profileLane0 = (sint32)(14U - (uint32)lane0);
    sint32 profileLane1 = (sint32)(14U - (uint32)lane1);
    sint32 profile = BWorldSm::slices[slice].pavedProfile;
    sint32 low = profileLane0;
    sint32 high = profileLane1;
    if (profileLane1 < profileLane0)
    {
        low = profileLane1;
        high = profileLane0;
    }
    uint32 width = (uint32)high - (uint32)low + 1U;
    sint32 shifted = ~profile >> ((uint32)low & 31U);
    uint32 mask = ~(0xffffffffU << (width & 31U));
    return (sint32)((uint32)shifted & mask);
}

sint32 AIWorld::LaneIndex(sint32 slice, sint32 position)
{
    FUNCTION_MARKER(0x800734CC, "NFS4.EXE");
    sint32 li;
    sint32 laneWidth;
    if (position < 0)
    {
        laneWidth = BWorldSm::slices[slice].avgPavedWidthLf;
        li = 6;
    }
    else
    {
        laneWidth = BWorldSm::slices[slice].avgPavedWidthRt;
        li = 7;
    }
    sint32 scaled = fixedmult(position, inverseLaneWidthTable[laneWidth * 2]);
    li += scaled / 65536;
    if (li < 0)
        li = 0;
    if (li >= 14)
        li = 13;
    return li;
}

void AIWorld::CalculateLaneInfo(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80073594, "NFS4.EXE");
    sint32 flags = carObj->AIFlags;
    carObj->carInLane = 0;
    if ((flags & 4) != 0)
        return;
    carObj->laneIndex = LaneIndex(carObj->N.simRoadInfo.slice, carObj->roadPosition);
    sint32 leftEdgeIndex = LaneIndex(carObj->N.simRoadInfo.slice,
        (sint32)((uint32)carObj->roadPosition - (uint32)carObj->roadSpan + 0x8000U));
    sint32 rightEdgeIndex = LaneIndex(carObj->N.simRoadInfo.slice,
        (sint32)((uint32)carObj->roadPosition + (uint32)carObj->roadSpan - 0x8000U));
    if ((uint32)rightEdgeIndex < 14U && (uint32)leftEdgeIndex < 14U)
        for (sint32 laneLoop = leftEdgeIndex; laneLoop <= rightEdgeIndex; ++laneLoop)
            carObj->carInLane |= 1 << laneLoop;
}

sint32 AIWorld::CalculateDeltaRoadYaw(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80073658, "NFS4.EXE");
    sint32 delta = 0;
    if ((carObj->carFlags & 8) != 0)
    {
        sint32 thisSlice = carObj->N.simRoadInfo.slice;
        sint32 numSlices = gNumSlices;
        sint32 yaw0 = carObj->N.roadYaw;
        sint32 nextSlice = thisSlice + 1;
        if (nextSlice >= numSlices)
            nextSlice = (sint32)((uint32)thisSlice - ((uint32)numSlices - 1U));
        sint32 yaw = Newton::CalculateSliceYaw(nextSlice);
        delta = (sint32)((uint32)yaw - (uint32)yaw0);
        if (delta > 512)
            delta = (sint32)((uint32)delta - 1024U);
        if (delta < -512)
            delta = (sint32)((uint32)delta + 1024U);
    }
    return delta;
}

sint32 AIWorld::CalcRoadBend(Car::tObj *carObj, sint32 lookAhead)
{
    FUNCTION_MARKER(0x800736E0, "NFS4.EXE");
    sint32 thisSlice = carObj->N.simRoadInfo.slice;
    sint32 nextSlice = (sint32)((uint32)thisSlice + (uint32)lookAhead);
    if (lookAhead >= 0)
    {
        if (nextSlice >= gNumSlices)
            nextSlice = (sint32)((uint32)nextSlice - (uint32)gNumSlices);
    }
    else if (nextSlice < 0)
        nextSlice = (sint32)((uint32)nextSlice + (uint32)gNumSlices);
    Trk::NewSlice *slices = BWorldSm::slices;
    sint32 forwardX = (sint8)slices[nextSlice].forward[0] * 2;
    sint32 rightX = (sint8)slices[thisSlice].right[0] * 2;
    sint32 forwardZ = (sint8)slices[nextSlice].forward[2] * 2;
    sint32 rightZ = (sint8)slices[thisSlice].right[2] * 2;
    return forwardX * rightX + forwardZ * rightZ;
}

sint32 AIWorld::CalcFutureLateralVel(Car::tObj *carObj, sint32 slicesAhead)
{
    FUNCTION_MARKER(0x800737BC, "NFS4.EXE");
    if ((carObj->carFlags & 0x10) != 0)
    {
        sint32 speed = carObj->currentSpeed;
        if (speed < 0)
            speed = (sint32)(0U - (uint32)speed);
        if (speed < 0x140000)
            slicesAhead = 0;
    }
    sint32 currentSlice = carObj->N.simRoadInfo.slice;
    sint32 futureSlice = (sint32)((uint32)currentSlice + (uint32)slicesAhead);
    if (slicesAhead >= 0)
    {
        if (futureSlice >= gNumSlices)
            futureSlice = (sint32)((uint32)futureSlice - (uint32)gNumSlices);
    }
    else if (futureSlice < 0)
        futureSlice = (sint32)((uint32)futureSlice + (uint32)gNumSlices);
    Trk::NewSlice *slices = BWorldSm::slices;
    coorddef right;
    right.x = (sint8)slices[futureSlice].right[0] * 512;
    right.y = (sint8)slices[futureSlice].right[1] * 512;
    right.z = (sint8)slices[futureSlice].right[2] * 512;
    sint32 x = fixedmult(carObj->N.linearVel.x, right.x);
    sint32 y = fixedmult(carObj->N.linearVel.y, right.y);
    sint32 z = fixedmult(carObj->N.linearVel.z, right.z);
    return (sint32)((uint32)x + (uint32)y + (uint32)z);
}

void AIWorld::CalcSpeed(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800738D4, "NFS4.EXE");
    sint32 optVar1 = carObj->N.linearVel.x;
    sint32 optVar2 = carObj->N.linearVel.z;
    if (optVar1 < 0)
        optVar1 = (sint32)(0U - (uint32)optVar1);
    if (optVar2 < 0)
        optVar2 = (sint32)(0U - (uint32)optVar2);
    if (optVar2 < optVar1)
        carObj->speed = (sint32)((uint32)optVar1 + (uint32)(optVar2 >> 2));
    else
        carObj->speed = (sint32)((uint32)optVar2 + (uint32)(optVar1 >> 2));
}

sint32 AIWorld::CalcLateralVelocity(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8007391C, "NFS4.EXE");
    sint32 temp = fixedmult(carObj->N.linearVel.x, carObj->N.roadMatrix.m[0]);
    sint32 y = fixedmult(carObj->N.linearVel.y, carObj->N.roadMatrix.m[1]);
    temp = (sint32)((uint32)temp + (uint32)y);
    sint32 z = fixedmult(carObj->N.linearVel.z, carObj->N.roadMatrix.m[2]);
    return (sint32)((uint32)temp + (uint32)z);
}

void AIWorld::FindBarrierLessLaneAndPosition(Car::tObj *carObj, sint32 *goodLane, sint32 *goodPosition)
{
    FUNCTION_MARKER(0x80073978, "NFS4.EXE");
    sint32 roadSide = (sint32)((uint32)carObj->direction * (uint32)AITune::driveSide);
    sint32 laneLoop = 0;
    sint32 laneWidth;
    if (roadSide == 1)
    {
        *goodLane = 7;
        laneWidth = BWorldSm::slices[carObj->N.simRoadInfo.slice].avgPavedWidthRt * 32768;
    }
    else
    {
        *goodLane = 6;
        laneWidth = BWorldSm::slices[carObj->N.simRoadInfo.slice].avgPavedWidthLf * 32768;
    }
    do
    {
        if (IsDriveableLane(carObj->N.simRoadInfo.slice, *goodLane) != 0)
            break;
        ++laneLoop;
        *goodLane = (sint32)((uint32)*goodLane + (uint32)roadSide);
    } while (laneLoop < 3);
    *goodPosition = (sint32)((uint32)roadSide * ((uint32)laneWidth * (uint32)laneLoop + ((uint32)laneWidth >> 1)));
}
