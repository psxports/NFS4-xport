#include <new>
#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/ai.h"
#include "game/common/aih_basiccop.h"
#include "game/common/aihigh.h"
#include "game/common/ailife.h"
#include "game/common/aiworld.h"
#include "game/common/bworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/fastrand.h"
#include "game/common/group.h"
#include "game/common/matrixtdef.h"
#include "game/common/object.h"
#include "game/common/scene.h"
#include "game/common/sim.h"
#include "game/common/speech.h"
#include "game/common/trk.h"
#include "lib/psx/fixdmult.h"

typedef matrixtdef MATRIX3DT;

static void PlacePointOnRoad(sint32 slice, coorddef *offset)
{
    FUNCTION_MARKER(0x8005C5E4, "NFS4.EXE");
    coorddef ioff;
    memcpy(&ioff, offset, sizeof(ioff));
    BWorldSm::Pos testSimRoadInfo;
    memcpy(&testSimRoadInfo, &Cars::gHumanRaceCarList[0]->N.simRoadInfo, sizeof(testSimRoadInfo));
    testSimRoadInfo.slice = slice;
    Trk::NewSlice *road = &BWorldSm::slices[slice];
    coorddef slicecenter;
    slicecenter.x = road->center[0];
    slicecenter.y = road->center[1];
    slicecenter.z = road->center[2];
    sint32 vecXx = static_cast<sint8>(road->right[0]) * 512;
    sint32 vecXz = static_cast<sint8>(road->right[2]) * 512;
    sint32 vecZx = static_cast<sint8>(road->forward[0]) * 512;
    sint32 vecZz = static_cast<sint8>(road->forward[2]) * 512;
    sint32 x = fixedmult(vecXx, ioff.x);
    sint32 z = fixedmult(vecZx, ioff.z);
    offset->x = (uint32)slicecenter.x + (uint32)x + (uint32)z;
    offset->y = slicecenter.y;
    x = fixedmult(vecXz, ioff.x);
    z = fixedmult(vecZz, ioff.z);
    offset->z = (uint32)slicecenter.z + (uint32)x + (uint32)z;
    BWorldSm::FindClosestQuadRez(offset, &testSimRoadInfo, 1);
    const coorddef *normal = BWorldSm::UNormal(&testSimRoadInfo);
    const coorddef *pointOnPlane;
    coorddef center;
    if (testSimRoadInfo.simQuad != 0) {
        pointOnPlane = &testSimRoadInfo.quadPts[0];
    } else {
        Trk::NewSlice *currentRoad = &BWorldSm::slices[testSimRoadInfo.slice];
        center.x = currentRoad->center[0];
        center.y = currentRoad->center[1];
        center.z = currentRoad->center[2];
        pointOnPlane = &center;
    }
    offset->y = GetPlaneY(normal, pointOnPlane, offset);
}

AIHigh_BasicCop::AIHigh_BasicCop(Car::tObj *carObj, sint32 copIndex) : AIHigh_Base(carObj)
{
    FUNCTION_MARKER(0x8005C164, "NFS4.EXE");
    copIndex_ = copIndex;
    type_ = (carObj->carFlags & 0x40) != 0 ? COP_SUPER : COP_REGULAR;
    blockade_.mode = BLOCKADEMODE_NONE;
    blockade_.target = 0;
    blockade_.requestSpikeBeltAtSlice = -1;
    blockade_.blockadeSpeechFlags = 0;
    driveAway_ = AIHigh::DRIVEAWAY_NONE;
}

void AIHigh_BasicCop::CheckSpikeBelt(void)
{
    FUNCTION_MARKER(0x8005C1EC, "NFS4.EXE");
    if (AICop::spikeBelt.active_) {
        sint32 timeNow = simGlobal.gameTicks;
        sint32 elapsed = (sint32)((uint32)timeNow - (uint32)AICop::spikeBelt.freshenTime_);
        if (elapsed >= 320) {
            if (!AILife::IsSliceInAnyVisibleArea(AICop::spikeBelt.slice_)) {
                BWorld::InitSpikeBelt();
                AICop::spikeBelt.active_ = 0;
            } else {
                AICop::spikeBelt.freshenTime_ = simGlobal.gameTicks;
            }
        }
    }
    if (AICop::gRoadBlockState == AICop::kAICop_RoadBlockState_PerpPassed &&
        !AILife::IsSliceInAnyVisibleArea(Object::customSliceNum)) {
        Object::ClearCustomObjects();
        AICop::gRoadBlockState = AICop::kAICop_RoadBlockState_None;
    }
}

sint32 AIHigh_BasicCop::ShouldIPerformCutOffBlock(sint32 chancePerSecond, Car::tObj *target)
{
    FUNCTION_MARKER(0x8005C2B4, "NFS4.EXE");
    sint32 chanceForElapsedTime = (sint32)((uint32)(chancePerSecond / 32) * (uint32)AI::elapsedTime);
    sint32 scaledChance = (sint32)((uint32)chanceForElapsedTime * 1000);
    sint32 chanceOutOf1000 = scaledChance / 65536;
    uint32 product = fastRandom * randSeed;
    randtemp = product;
    fastRandom = product & 0xFFFF;
    sint32 random1000 = (((product >> 8) & 0xFFFF) * 1000) >> 16;
    if (random1000 >= chanceOutOf1000)
        return 0;
    Car::tObj *car = carObj_;
    sint32 relLatPosition = (sint32)((uint32)car->roadPosition - (uint32)target->roadPosition);
    sint32 absRelLatPosition = relLatPosition;
    if (absRelLatPosition < 0)
        absRelLatPosition = (sint32)(0 - (uint32)absRelLatPosition);
    sint32 minimumLateralGap = (sint32)((uint32)target->N.dimension.x + 0x10000);
    if (absRelLatPosition <= minimumLateralGap || absRelLatPosition >= 0x40000)
        return 0;
    sint32 distance = AIWorld::SplineDistance(car, target);
    sint32 metersBetween = (sint32)((uint32)distance * (uint32)carObj_->direction);
    sint32 carLength = (sint32)(((uint32)target->N.dimension.z << 1) + 0x20000);
    return carLength < metersBetween && metersBetween < 0xC0000;
}

void Blockade::AddRoadFlare(coorddef *pos)
{
    FUNCTION_MARKER(0x8005C414, "NFS4.EXE");
    Group *group = Object::customSFXInst;
    uint8 *storage = reinterpret_cast<uint8 *>(group + 1) + ((uint32)group->m_num_elements << 4);
    Trk::SFX *sfxInstance = ::new (storage) Trk::SFX;
    sfxInstance->point[0] = pos->x;
    sfxInstance->point[1] = pos->y;
    sint32 z = pos->z;
    sfxInstance->type = 22;
    sfxInstance->pad = 0;
    sfxInstance->point[2] = z;
    group->m_num_elements = (uint32)group->m_num_elements + 1;
}

void Blockade::AddObject(sint32 slice, coorddef *pos, sint32 objectID)
{
    FUNCTION_MARKER(0x8005C468, "NFS4.EXE");
    SceneElem theObj;
    theObj.type = 0;
    theObj.subType = 1;
    theObj.scalar1 = Object::GetObjDefID(objectID);
    theObj.scalar2 = 0x23916;
    theObj.cp = *pos;
    theObj.subTypeIndex = objectID;
    BWorldSm::Pos slicePos;
    BWorldSm::SetSlice(slice, &slicePos);
    BWorldSm::FindClosestQuadRez(pos, &slicePos, 1);
    const coorddef *roty = BWorldSm::UNormal(&slicePos);
    theObj.orient.m[3] = roty->x;
    theObj.orient.m[4] = roty->y;
    theObj.orient.m[5] = roty->z;
    const coorddef *rotz = BWorldSm::UForward(&slicePos);
    theObj.orient.m[6] = rotz->x;
    theObj.orient.m[7] = rotz->y;
    theObj.orient.m[8] = rotz->z;
    sint32 first = fixedmult(theObj.orient.m[4], theObj.orient.m[8]);
    theObj.orient.m[0] = (sint32)((uint32)first - (uint32)fixedmult(theObj.orient.m[5], theObj.orient.m[7]));
    first = fixedmult(theObj.orient.m[5], theObj.orient.m[6]);
    theObj.orient.m[1] = (sint32)((uint32)first - (uint32)fixedmult(theObj.orient.m[3], theObj.orient.m[8]));
    first = fixedmult(theObj.orient.m[3], theObj.orient.m[7]);
    theObj.orient.m[2] = (sint32)((uint32)first - (uint32)fixedmult(theObj.orient.m[4], theObj.orient.m[6]));
    transpose(&theObj.orient, &theObj.orient);
    Object::AddCustomObject(&theObj, 1);
}

void AIHigh_BasicCop::SetupBlockadeElements(blockade_t *blockade)
{
    FUNCTION_MARKER(0x8005C790, "NFS4.EXE");
    if (blockade->flags == 0)
        return;
    Object::ClearCustomObjects();
    sint32 flags = blockade->flags;
    sint32 slice = blockade->slice;
    Object::customSliceNum = slice;
    coorddef pt;
    if ((flags & 1) != 0) {
        sint32 longitudinal = -0x100000;
        for (sint32 i = 0; i < 5; ++i) {
            pt.x = i * 0x20000;
            pt.y = 0;
            pt.z = (sint32)((uint32)blockade->direction * (uint32)longitudinal);
            PlacePointOnRoad(slice, &pt);
            Blockade::AddRoadFlare(&pt);
            longitudinal += 0x40000;
        }
        longitudinal = -0xC0000;
        for (sint32 i = 1; i < 5; ++i) {
            pt.x = -i * 0x20000;
            pt.y = 0;
            pt.z = (sint32)((uint32)blockade->direction * (uint32)longitudinal);
            PlacePointOnRoad(slice, &pt);
            Blockade::AddRoadFlare(&pt);
            longitudinal += 0x40000;
        }
    } else if ((flags & 2) != 0) {
        sint32 longitudinal = -0x180000;
        for (sint32 i = 0; i < 7; ++i) {
            pt.x = -0x60000 + i * 0x20000;
            pt.y = 0;
            pt.z = (sint32)((uint32)blockade->direction * (uint32)longitudinal);
            PlacePointOnRoad(slice, &pt);
            Blockade::AddRoadFlare(&pt);
            longitudinal += 0x40000;
        }
        longitudinal = -0x180000;
        for (sint32 i = 0; i < 7; ++i) {
            if (i != 3) {
                pt.x = 0x60000 - i * 0x20000;
                pt.y = 0;
                pt.z = (sint32)((uint32)blockade->direction * (uint32)longitudinal);
                PlacePointOnRoad(slice, &pt);
                Blockade::AddRoadFlare(&pt);
            }
            longitudinal += 0x40000;
        }
    }
    if ((blockade->flags & 4) != 0) {
        sint32 objId = Object::FindDefWithThisID(3);
        if (objId != -1) {
            pt.x = -0x28000;
            pt.y = 0;
            pt.z = (sint32)(0 - ((uint32)blockade->direction << 18));
            PlacePointOnRoad(slice, &pt);
            Blockade::AddObject(slice, &pt, objId);
            pt.x = 0x28000;
            pt.y = 0;
            pt.z = (sint32)(0 - ((uint32)blockade->direction << 18));
            PlacePointOnRoad(slice, &pt);
            Blockade::AddObject(slice, &pt, objId);
        }
    }
    blockade->flags = 0;
}

void AIHigh_BasicCop::HandleBlockadeSpeech(void)
{
    FUNCTION_MARKER(0x8005CA28, "NFS4.EXE");
    uint16 flags = (uint16)blockade_.blockadeSpeechFlags;
    if (flags == 0)
        return;
    Car::tObj *theCar = blockade_.target->carObj_;
    if (!theCar) {
        blockade_.blockadeSpeechFlags = 0;
        return;
    }
    sint32 carSlice = theCar->stats.slice;
    sint32 sliceDiff = (sint32)((uint32)carSlice - (uint32)blockade_.slice);
    if (theCar->blowout) {
        blockade_.blockadeSpeechFlags = 0;
        return;
    }
    sint32 distance = sliceDiff;
    if (distance < 0)
        distance = (sint32)(0 - (uint32)distance);
    if (distance >= 33)
        return;
    sint32 scaledSpeed = (sint32)((uint32)Cars::topSpeedCap[theCar->render.currentCarType] * 28);
    sint32 checkSlice = scaledSpeed / 0x640000;
    if (sliceDiff < checkSlice)
        blockade_.blockadeSpeechFlags = (sint16)(flags | 2);
    else if (checkSlice < sliceDiff)
        blockade_.blockadeSpeechFlags = (sint16)(flags | 4);
    if ((blockade_.blockadeSpeechFlags & 6) == 6) {
        Speech::Mobile(carObj_)->Lose();
        blockade_.blockadeSpeechFlags = 0;
        AICop::gRoadBlockState = AICop::kAICop_RoadBlockState_PerpPassed;
    }
}
