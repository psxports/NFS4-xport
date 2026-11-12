#include <stddef.h>
#include "xport.h"
#include "game/common/ai.h"
#include "game/common/aidatarecord.h"
#include "game/common/ailife.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/audioclc.h"
#include "game/common/bworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/fastrand.h"
#include "game/common/group.h"
#include "game/common/object.h"
#include "game/common/sim.h"
#include "game/common/trk.h"
#include "lib/psx/fixdmult.h"

typedef sint32 fixed88;

typedef sint32 fixed824;

typedef sint32 fixed248;

class CarLogic
{
public:
    typedef sint32 tObservations[1][3];
    static tObservations gObs;
};

typedef struct coorddef LIBCOORD;

sint32 curveynessLevelStarts[4] = { 0, 10, 15, 25 };
sint32 AI::time;
sint32 AI::elapsedTime;
sint32 AI::iTime;
AI::tInfo AI::Info;
sint32 CarLogic::gObs[1][3];

void AI::CarAvoidance(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80057994, "NFS4.EXE");
    SetupOncomingLaneDemerits(carObj);
    CalculateLaneSpeeds(carObj);
    CalcMeritsBasedOnSpeed(carObj);
    CheckForClearLanes(carObj);
    CheckForCarsOnSide(carObj);
}

void AI::WorldAvoidance(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800579D8, "NFS4.EXE");
    CheckForBarriers(carObj);
    HandleChangeInNumLanes(carObj);
    HandleShouldersAndOffRoad(carObj);
    CalcBestLineMerits(carObj);
    AvoidObjects(carObj);
    AvoidSpikeBelt(carObj);
}

void AI::InitAIInfo(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80057A24, "NFS4.EXE");
    Info.blockingCars[2] = 0;
    Info.blockingCars[1] = 0;
    Info.blockingCars[0] = 0;
    Info.blockingCarsDist[2] = 0;
    Info.blockingCarsDist[1] = 0;
    Info.blockingCarsDist[0] = 0;
    Info.deltaYaw = AIWorld::CalculateDeltaRoadYaw(carObj);
    Info.desiredLane = 0;
}

void AI::GenericBeginCycle(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80057A6C, "NFS4.EXE");
    InitAIInfo(carObj);
    ClearLaneMerits();
}

void AI::GenericCycle(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80057A94, "NFS4.EXE");
    MaybeChangeLaneSlack(carObj);
    DoReactionsAndBehavior(carObj);
    PushFinishedCarsToSide(carObj);
    KeepCarsInLane(carObj);
    HandleTrafficHonking(carObj);
    CarAvoidance(carObj);
    WorldAvoidance(carObj);
}

void AI::GenericEndCycle(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80057AE8, "NFS4.EXE");
    ProcessObservationsAndChooseLane(carObj);
    CalculateDesiredLatPosition(carObj);
    CalculateAdjustedDesiredSpeed(carObj);
}

void AI::TargetLane(Car::tObj *carObj, sint32 lane)
{
    FUNCTION_MARKER(0x80057B1C, "NFS4.EXE");
    sint32 currentLane = carObj->laneIndex;
    if (currentLane < lane)
        CarLogic::gObs[0][2] = CarLogic::gObs[0][2] + 0x50000;
    else if (lane < currentLane)
        CarLogic::gObs[0][0] = CarLogic::gObs[0][0] + 0x50000;
}

void AI::ClearLaneMerits(void)
{
    FUNCTION_MARKER(0x80057B6C, "NFS4.EXE");
    for (sint32 o = 0; o < 1; ++o) {
        CarLogic::gObs[o][2] = 0;
        CarLogic::gObs[o][1] = 0;
        CarLogic::gObs[o][0] = 0;
    }
}

void AI::DoReactions(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80057B94, "NFS4.EXE");
    sint32 target = AIScript::DoReAction(&carObj->script, AIScript::AISCRIPT_AIREACTION_HONK_HORN);
    if (target != -1 && GameSetup::gData.raceType != 1)
        AudioClc::SetHorn(carObj, 1);
    else if (carObj->control.horn != 0)
        AudioClc::SetHorn(carObj, 0);
    target = AIScript::DoReAction(&carObj->script, AIScript::AISCRIPT_AIREACTION_CRAZY_HORN);
    if (target != -1 && GameSetup::gData.raceType != 1) {
        switch ((simGlobal.gameTicks & 0x18) >> 3) {
        case 0: AudioClc::HonkHorn(carObj, 4, 16, 8); break;
        case 1: AudioClc::HonkHorn(carObj, 2, 32, 12); break;
        case 2: AudioClc::HonkHorn(carObj, 7, 40, 16); break;
        case 3: AudioClc::HonkHorn(carObj, 1, 32, 0); break;
        }
    }
    target = AIScript::DoReAction(&carObj->script, AIScript::AISCRIPT_AIREACTION_SWERVE);
    if (target != -1) {
        sint32 absDistance = AIWorld::ApxSplineDistance(carObj, Cars::gList[target]);
        if (absDistance < 0)
            absDistance = -absDistance;
        if (absDistance > 0xC0000) {
            sint32 seconds = AIScript::GetReactionTicksLeft(&carObj->script) / 32;
            if (((uint32)seconds & 1) != 0)
                CarLogic::gObs[0][0] = CarLogic::gObs[0][0] + 0x1E0000;
            else
                CarLogic::gObs[0][2] = CarLogic::gObs[0][2] + 0x1E0000;
        }
    }
    target = AIScript::DoReAction(&carObj->script, AIScript::AISCRIPT_AIREACTION_SWIPE);
    if (target != -1) {
        Car::tObj *otherCarObj = Cars::gList[target];
        sint32 metersDistance = AIWorld::SplineDistance(carObj, otherCarObj) * carObj->direction;
        if (metersDistance < 0x40000)
            TargetLane(carObj, otherCarObj->laneIndex);
    }
}

void AI::CheckForPlayerActions(Car::tObj *carObj, Car::tObj *otherCarObj)
{
    FUNCTION_MARKER(0x80057DD4, "NFS4.EXE");
    sint32 absDistance = AIWorld::ApxSplineDistance(carObj, otherCarObj);
    if (absDistance < 0)
        absDistance = -absDistance;
    if (AIWorld::GameOdometer(carObj) < 0x3200000)
        return;
    sint32 collisionTime = simGlobal.gameTicks;
    sint32 sinceCollision = collisionTime - otherCarObj->N.collision.lastTime;
    if (sinceCollision < 15 && otherCarObj->N.collision.lastOtherObj != 0 && otherCarObj->N.collision.lastOtherObj == &carObj->N)
        AIScript::SubmitPlayerAction(&carObj->script, otherCarObj->carIndex, AIScript::AISCRIPT_PLAYACTION_COLLISION, collisionTime);
    sint32 direction = carObj->direction;
    sint32 otherSpeed = (otherCarObj->currentSpeed * direction) + 0x280000;
    sint32 currentSpeed = carObj->currentSpeed * direction;
    if (otherSpeed < currentSpeed && absDistance < 0xC0000)
        AIScript::SubmitPlayerAction(&carObj->script, otherCarObj->carIndex, AIScript::AISCRIPT_PLAYACTION_OPP_WIZZED_BY, simGlobal.gameTicks);
    if (absDistance < 0xC0000 && otherCarObj->laneIndex == carObj->laneIndex) {
        sint32 otherOdometer = AIWorld::GameOdometer(otherCarObj);
        sint32 odometer = AIWorld::GameOdometer(carObj);
        AIScript::tPlayAction action = odometer < otherOdometer ? AIScript::AISCRIPT_PLAYACTION_HUMAN_BLOCK_OPP : AIScript::AISCRIPT_PLAYACTION_OPP_BLOCK_HUMAN;
        AIScript::SubmitPlayerAction(&carObj->script, otherCarObj->carIndex, action, simGlobal.gameTicks);
    }
    if (otherCarObj->swapCar == carObj) {
        sint32 sinceSwap = simGlobal.gameTicks - carObj->swapTime;
        if (sinceSwap < 15) {
            sint32 odometer = AIWorld::GameOdometer(carObj);
            sint32 otherOdometer = AIWorld::GameOdometer(otherCarObj);
            AIScript::tPlayAction action = otherOdometer < odometer ? AIScript::AISCRIPT_PLAYACTION_OPP_PASS_HUMAN : AIScript::AISCRIPT_PLAYACTION_HUMAN_PASS_OPP;
            AIScript::SubmitPlayerAction(&carObj->script, otherCarObj->carIndex, action, simGlobal.gameTicks);
        }
    }
    if (absDistance < 0x1E0000 && otherCarObj->control.horn != 0 && GameSetup::gData.Time == 0)
        AIScript::SubmitPlayerAction(&carObj->script, otherCarObj->carIndex, AIScript::AISCRIPT_PLAYACTION_HUMAN_HONK, simGlobal.gameTicks);
}

void AI::DoReactionsAndBehavior(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80058014, "NFS4.EXE");
    DoReactions(carObj);
    for (sint32 t = 0; t < Cars::gNumCars; ++t) {
        Car::tObj *otherCarObj = Cars::gList[t];
        if (otherCarObj == carObj || otherCarObj->N.active == 0 || (otherCarObj->carFlags & 4) == 0)
            continue;
        CheckForPlayerActions(carObj, otherCarObj);
        if ((carObj->carFlags & 8) != 0)
            OpponentBlockPlayer(carObj, otherCarObj);
    }
}

void AI::OpponentBlockPlayer(Car::tObj *carObj, Car::tObj *otherCarObj)
{
    FUNCTION_MARKER(0x800580D8, "NFS4.EXE");
    if (IsMellowZone(carObj, 0x1900000) != 0 || Info.deltaYaw >= curveynessLevelStarts[2])
        return;
    sint32 otherCarObjSlice = otherCarObj->N.simRoadInfo.slice;
    sint32 distance = AIWorld::SplineDistance(carObj, otherCarObj) * carObj->direction;
    sint32 reaction = AIScript::DoReAction(&carObj->script, AIScript::AISCRIPT_AIREACTION_MOVE_AWAY);
    sint32 merit;
    sint32 otherLane;
    sint32 currentLane;
    if (reaction != -1 && distance >= 0 && distance < 0x320000) {
        merit = -0x40000;
        otherLane = otherCarObj->laneIndex;
        currentLane = carObj->laneIndex;
    } else {
        if (distance < -0x60000)
            return;
        AIPerson::AIPerson_t *personality = carObj->personality;
        if (distance >= personality->blockMaxDistance || distance < personality->blockMinDistance)
            return;
        uint8 laneCount = BWorldSm::slices[otherCarObjSlice].laneCount;
        otherLane = otherCarObj->laneIndex;
        if (otherLane < 7 - (laneCount >> 4) || otherLane > (laneCount & 15) + 6)
            return;
        sint32 otherSpeed = otherCarObj->currentSpeed * otherCarObj->direction;
        if (otherSpeed <= 0x140000)
            return;
        currentLane = carObj->laneIndex;
        merit = 0x40000;
    }
    sint32 observation = otherLane < currentLane ? 0 : (otherLane == currentLane ? 1 : 2);
    CarLogic::gObs[0][observation] = CarLogic::gObs[0][observation] + merit;
}

void AI::CheckForBarriers(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800582D4, "NFS4.EXE");
    sint32 laneNotChecked[3] = { 1, 1, 1 };
    sint32 masks[3];
    sint32 lane = carObj->laneIndex - 1;
    masks[0] = AIWorld::GetProfileMask(lane);
    masks[1] = AIWorld::GetProfileMask(carObj->laneIndex);
    lane = carObj->laneIndex + 1;
    masks[2] = AIWorld::GetProfileMask(lane);
    sint32 dir = carObj->direction;
    sint32 speed = carObj->currentSpeed / 65536;
    sint32 slice = carObj->N.simRoadInfo.slice;
    if (speed < 0)
        speed = -speed;
    sint32 scaledSpeed = (uint32)speed << 18;
    sint32 slicesAhead = scaledSpeed / 0x60000;
    sint32 offset = slicesAhead * dir;
    sint32 forwardSlice0 = slice + offset;
    if (offset >= 0) {
        sint32 numSlices = gNumSlices;
        if (forwardSlice0 >= numSlices)
            forwardSlice0 = forwardSlice0 - numSlices;
    } else if (forwardSlice0 < 0) {
        forwardSlice0 = forwardSlice0 + gNumSlices;
    }
    offset = (slicesAhead + 1) * dir;
    sint32 forwardSlice1 = slice + offset;
    if (offset >= 0) {
        sint32 numSlices = gNumSlices;
        if (forwardSlice1 >= numSlices)
            forwardSlice1 = forwardSlice1 - numSlices;
    } else if (forwardSlice1 < 0) {
        forwardSlice1 = forwardSlice1 + gNumSlices;
    }
    sint32 profileHere = BWorldSm::slices[slice].pavedProfile;
    for (sint32 i = 0; i < 3; ++i) {
        if (AIWorld::IsDriveableLane_UsingMask(slice, masks[i]) == 0)
            CarLogic::gObs[0][i] = CarLogic::gObs[0][i] - 0xA0000;
    }
    if (profileHere != BWorldSm::slices[forwardSlice0].pavedProfile || profileHere != BWorldSm::slices[forwardSlice1].pavedProfile)
        carObj->barrierThinkHarder = 0x49;
    sint32 barrierThinkHarder = carObj->barrierThinkHarder;
    if (barrierThinkHarder <= 0)
        return;
    sint32 product = slicesAhead * 0xA3D;
    sint32 interval = product / 65536;
    if (interval == 0)
        interval = 1;
    carObj->barrierThinkHarder = barrierThinkHarder - elapsedTime;
    for (sint32 sliceLoop = 0; sliceLoop < slicesAhead; sliceLoop = sliceLoop + interval) {
        offset = sliceLoop * dir;
        sint32 checkSlice = slice + offset;
        if (offset >= 0) {
            sint32 numSlices = gNumSlices;
            if (checkSlice >= numSlices)
                checkSlice = checkSlice - numSlices;
        } else if (checkSlice < 0) {
            checkSlice = checkSlice + gNumSlices;
        }
        for (sint32 i = 0; i < 3; ++i) {
            if (laneNotChecked[i] != 0 && AIWorld::IsDriveableLane_UsingMask(checkSlice, masks[i]) == 0) {
                laneNotChecked[i] = 0;
                CarLogic::gObs[0][i] = CarLogic::gObs[0][i] - 0x280000;
            }
        }
    }
}

void AI::SetupOncomingLaneDemerits(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80058694, "NFS4.EXE");
    if (IsMellowZone(carObj, 0x3C0000) != 0)
        return;
    sint32 opposingLaneMerit = (carObj->carFlags & 0x10) != 0 ? -0xC0000 : -0x40000;
    if ((Cars::gNumTrafficCars == 0 && Cars::gNumCopCars == 0) || AITune::oneWay != 0 || (carObj->AIFlags & 2) != 0)
        return;
    if (carObj->direction == AITune::driveSide) {
        sint32 adjacentLane = carObj->laneIndex - 1;
        if (adjacentLane < 7)
            CarLogic::gObs[0][0] = CarLogic::gObs[0][0] + opposingLaneMerit;
        if (carObj->laneIndex < 7)
            CarLogic::gObs[0][1] = CarLogic::gObs[0][1] + opposingLaneMerit;
    } else {
        if (carObj->laneIndex >= 7)
            CarLogic::gObs[0][1] = CarLogic::gObs[0][1] + opposingLaneMerit;
        sint32 adjacentLane = carObj->laneIndex + 1;
        if (adjacentLane >= 7)
            CarLogic::gObs[0][2] = CarLogic::gObs[0][2] + opposingLaneMerit;
    }
}

void AI::HandleChangeInNumLanes(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800587EC, "NFS4.EXE");
    sint32 speed = carObj->currentSpeed;
    if (speed < 0)
        speed = -speed;
    sint32 absLaneLookAhead = fixedmult(speed, 0x6AAA) / 65536;
    if (absLaneLookAhead < 5)
        absLaneLookAhead = 5;
    sint32 lookAhead = absLaneLookAhead * carObj->direction;
    sint32 lookAheadSlice;
    if (lookAhead >= 0) {
        sint32 numSlices = gNumSlices;
        lookAheadSlice = carObj->N.simRoadInfo.slice + lookAhead;
        if (lookAheadSlice >= numSlices)
            lookAheadSlice = lookAheadSlice - numSlices;
    } else {
        lookAheadSlice = carObj->N.simRoadInfo.slice + lookAhead;
        if (lookAheadSlice < 0)
            lookAheadSlice = lookAheadSlice + gNumSlices;
    }
    uint8 laneCount = BWorldSm::slices[lookAheadSlice].laneCount;
    sint32 laneIndex = carObj->laneIndex;
    if (laneIndex >= 7 - (laneCount >> 4) && laneIndex <= (laneCount & 15) + 6)
        return;
    laneCount = BWorldSm::slices[carObj->N.simRoadInfo.slice].laneCount;
    if (laneIndex < 7 - (laneCount >> 4) || laneIndex > (laneCount & 15) + 6)
        return;
    if (laneIndex < 7)
        CarLogic::gObs[0][2] = CarLogic::gObs[0][2] + 0x280000;
    else
        CarLogic::gObs[0][0] = CarLogic::gObs[0][0] + 0x280000;
}

void AI::HandleShouldersAndOffRoad(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8005895C, "NFS4.EXE");
    sint32 slice = carObj->N.simRoadInfo.slice;
    sint32 shoulder_merit = -0x4E666;
    if ((carObj->carFlags & 0x10) != 0) {
        shoulder_merit = -0x260000;
        Car::tObj *blockingCar = Info.blockingCars[1];
        if (blockingCar && (blockingCar->carFlags & 1) != 0 && blockingCar->speed > 0x8E38E)
            shoulder_merit = 0x50000;
    }
    uint8 laneCount = BWorldSm::slices[slice].laneCount;
    sint32 leftShoulder = 6 - (laneCount >> 4);
    sint32 rightShoulder = (laneCount & 15) + 7;
    sint32 laneIndex = carObj->laneIndex;
    sint32 leftLane = laneIndex - 1;
    sint32 rightLane = laneIndex + 1;
    if (leftLane == leftShoulder || leftLane == rightShoulder) {
        CarLogic::gObs[0][0] = CarLogic::gObs[0][0] + shoulder_merit;
        Trk::NewSlice *road = &BWorldSm::slices[carObj->N.simRoadInfo.slice];
        sint32 width = ((uint32)road->leftDrive << 8) - ((uint32)road->avgPavedWidthLf << 15) * (road->laneCount >> 4);
        if (width < carObj->N.dimension.x)
            CarLogic::gObs[0][0] = CarLogic::gObs[0][0] - 0x640000;
    }
    if (rightLane == leftShoulder || rightLane == rightShoulder) {
        CarLogic::gObs[0][2] = CarLogic::gObs[0][2] + shoulder_merit;
        Trk::NewSlice *road = &BWorldSm::slices[carObj->N.simRoadInfo.slice];
        sint32 width = ((uint32)road->rightDrive << 8) - ((uint32)road->avgPavedWidthRt << 15) * (road->laneCount & 15);
        if (width < carObj->N.dimension.x)
            CarLogic::gObs[0][2] = CarLogic::gObs[0][2] - 0x640000;
    }
    if (laneIndex == leftShoulder || laneIndex == rightShoulder) {
        CarLogic::gObs[0][1] = CarLogic::gObs[0][1] + shoulder_merit;
        sint32 isRight = laneIndex >= 7;
        Trk::NewSlice *road = &BWorldSm::slices[carObj->N.simRoadInfo.slice];
        sint32 width;
        if (isRight)
            width = ((uint32)road->rightDrive << 8) - ((uint32)road->avgPavedWidthRt << 15) * (road->laneCount & 15);
        else
            width = ((uint32)road->leftDrive << 8) - ((uint32)road->avgPavedWidthLf << 15) * (road->laneCount >> 4);
        if (width < carObj->N.dimension.x)
            CarLogic::gObs[0][1] = CarLogic::gObs[0][1] - 0x640000;
    }
    if (leftLane < leftShoulder || leftLane > rightShoulder)
        CarLogic::gObs[0][0] = CarLogic::gObs[0][0] - 0x3E80000;
    if (laneIndex < leftShoulder || laneIndex > rightShoulder)
        CarLogic::gObs[0][1] = CarLogic::gObs[0][1] - 0x3E80000;
    if (rightLane < leftShoulder || rightLane > rightShoulder)
        CarLogic::gObs[0][2] = CarLogic::gObs[0][2] - 0x3E80000;
}

void AI::CalculateLaneSpeeds(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80058D20, "NFS4.EXE");
    uint32 lanesFilled = 0;
    sint32 carsFound = 0;
    sint32 maxDistanceToCheck = carObj->personality->oncomingLookAhead;
    for (sint32 lane = 0; lane < 3; ++lane) {
        sint32 desiredSpeed = carObj->desiredSpeed;
        Info.laneSpeedsAhead[lane] = desiredSpeed;
        Info.laneSpeeds[lane] = desiredSpeed;
    }
    sint32 laneIndex = carObj->laneIndex;
    sint32 carObjIndexInSortedList = carObj->sortIndex;
    uint32 carObjLeftLaneBits = (uint32)1 << (((uint32)laneIndex - 1) & 31);
    uint32 carObjThisLaneBits = (uint32)1 << (laneIndex & 31);
    uint32 carObjRightLaneBits = (uint32)1 << (((uint32)laneIndex + 1) & 31);
    for (sint32 ci = 0; carsFound != 3; ++ci) {
        sint32 numCars = Cars::gNumCars;
        if (ci >= numCars)
            break;
        sint32 sortedIndex = (carObjIndexInSortedList + (ci * carObj->direction)) + numCars;
        Car::tObj *otherCarObj = Cars::gSortedList[sortedIndex % numCars];
        if (otherCarObj == carObj || otherCarObj->N.active == 0)
            continue;
        sint32 distanceFixedMeters = AIWorld::SplineDistance(otherCarObj, carObj);
        sint32 distanceFixedMetersSignChecked = distanceFixedMeters * carObj->direction;
        sint32 absSpeed = carObj->currentSpeed;
        if (absSpeed < 0)
            absSpeed = -absSpeed;
        sint32 inverseCollisionTime = 0xD555;
        sint32 inverseAheadCollisionTime = 0x6666;
        if (absSpeed > 0x1638E2) {
            inverseCollisionTime = 0x14000;
            inverseAheadCollisionTime = 0x8E38;
        }
        if (distanceFixedMetersSignChecked < 0xF0000) {
            inverseCollisionTime = 0x2AAA;
            inverseAheadCollisionTime = 0x1999;
        }
        if (distanceFixedMetersSignChecked <= carObj->N.dimension.z || distanceFixedMetersSignChecked >= maxDistanceToCheck)
            continue;
        sint32 distanceIntMeters = distanceFixedMeters / 65536;
        if (carObj->direction == 1) {
            distanceIntMeters -= 2;
            if ((carObj->carFlags & 0x28) != 0 && distanceIntMeters <= 0)
                distanceIntMeters = 1;
            if (distanceIntMeters < 0)
                distanceIntMeters = 0;
        } else {
            distanceIntMeters += 2;
            if ((carObj->carFlags & 0x28) != 0 && distanceIntMeters >= 2)
                distanceIntMeters = -1;
            if (distanceIntMeters > 0)
                distanceIntMeters = 0;
        }
        sint32 otherSpeed = otherCarObj->currentSpeed;
        sint32 collisionSpeed = otherSpeed + (distanceIntMeters * inverseCollisionTime);
        sint32 aheadCollisionSpeed = otherSpeed + (distanceIntMeters * inverseAheadCollisionTime);
        if ((otherCarObj->carInLane & carObjThisLaneBits) != 0 && (lanesFilled & carObjThisLaneBits) == 0) {
            lanesFilled |= carObjThisLaneBits;
            ++carsFound;
            Info.laneSpeeds[1] = collisionSpeed;
            Info.laneSpeedsAhead[1] = aheadCollisionSpeed;
            Info.blockingCars[1] = otherCarObj;
            Info.blockingCarsDist[1] = distanceFixedMetersSignChecked;
        }
        if ((uint32)carObj->laneIndex - 1 < 14 && (otherCarObj->carInLane & carObjLeftLaneBits) != 0 && (lanesFilled & carObjLeftLaneBits) == 0) {
            lanesFilled |= carObjLeftLaneBits;
            ++carsFound;
            Info.laneSpeeds[0] = collisionSpeed;
            Info.laneSpeedsAhead[0] = aheadCollisionSpeed;
            Info.blockingCars[0] = otherCarObj;
            Info.blockingCarsDist[0] = distanceFixedMetersSignChecked;
        }
        if ((uint32)carObj->laneIndex + 1 < 14 && (otherCarObj->carInLane & carObjRightLaneBits) != 0 && (lanesFilled & carObjRightLaneBits) == 0) {
            lanesFilled |= carObjRightLaneBits;
            ++carsFound;
            Info.laneSpeeds[2] = collisionSpeed;
            Info.laneSpeedsAhead[2] = aheadCollisionSpeed;
            Info.blockingCars[2] = otherCarObj;
            Info.blockingCarsDist[2] = distanceFixedMetersSignChecked;
        }
    }
}

void AI::CalcMeritsBasedOnSpeed(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800590B4, "NFS4.EXE");
    sint32 dSpeed = carObj->desiredSpeed;
    sint32 cSpeed = carObj->currentSpeed;
    sint32 considerDesired = AIWorld::GameOdometer(carObj) >= 0x2580000;
    for (sint32 lane = 0; lane < 3; ++lane) {
        if (Info.blockingCars[lane] && Info.blockingCarsDist[lane] < 0x1E0000)
            CarLogic::gObs[0][lane] = CarLogic::gObs[0][lane] - 0xE0000;
    }
    if (carObj->direction == 1) {
        for (sint32 lane = 0; lane < 3; ++lane) {
            sint32 laneSpeed = Info.laneSpeeds[lane];
            if (laneSpeed <= cSpeed && Info.blockingCars[lane]) {
                sint32 difference = cSpeed - laneSpeed;
                sint32 merit = fixedmult(difference, -0x14CCC);
                CarLogic::gObs[0][lane] = CarLogic::gObs[0][lane] + merit;
                laneSpeed = Info.laneSpeeds[lane];
            }
            if (laneSpeed <= dSpeed && Info.blockingCars[lane] && considerDesired) {
                sint32 difference = dSpeed - laneSpeed;
                sint32 merit = fixedmult(difference, -0x8000);
                CarLogic::gObs[0][lane] = CarLogic::gObs[0][lane] + merit;
            }
            laneSpeed = Info.laneSpeedsAhead[lane];
            if (laneSpeed <= cSpeed && Info.blockingCars[lane]) {
                sint32 difference = cSpeed - laneSpeed;
                sint32 merit = fixedmult(difference, -0x8000);
                CarLogic::gObs[0][lane] = CarLogic::gObs[0][lane] + merit;
                laneSpeed = Info.laneSpeedsAhead[lane];
            }
            if (laneSpeed <= dSpeed && Info.blockingCars[lane] && considerDesired) {
                sint32 difference = dSpeed - laneSpeed;
                sint32 merit = fixedmult(difference, -0x1999);
                CarLogic::gObs[0][lane] = CarLogic::gObs[0][lane] + merit;
            }
        }
    } else {
        for (sint32 lane = 0; lane < 3; ++lane) {
            sint32 laneSpeed = Info.laneSpeeds[lane];
            if (cSpeed <= laneSpeed) {
                if (Info.blockingCars[lane]) {
                    sint32 difference = laneSpeed - cSpeed;
                    sint32 merit = fixedmult(difference, -0x14CCC);
                    CarLogic::gObs[0][lane] = CarLogic::gObs[0][lane] + merit;
                }
                laneSpeed = Info.laneSpeeds[lane];
            }
            if (dSpeed <= laneSpeed && Info.blockingCars[lane]) {
                sint32 difference = laneSpeed - dSpeed;
                sint32 merit = fixedmult(difference, -0x8000);
                CarLogic::gObs[0][lane] = CarLogic::gObs[0][lane] + merit;
            }
            laneSpeed = Info.laneSpeedsAhead[lane];
            if (cSpeed <= laneSpeed) {
                if (Info.blockingCars[lane]) {
                    sint32 difference = laneSpeed - cSpeed;
                    sint32 merit = fixedmult(difference, -0x8000);
                    CarLogic::gObs[0][lane] = CarLogic::gObs[0][lane] + merit;
                }
                laneSpeed = Info.laneSpeedsAhead[lane];
            }
            if (dSpeed <= laneSpeed && Info.blockingCars[lane]) {
                sint32 difference = laneSpeed - dSpeed;
                sint32 merit = fixedmult(difference, -0x1999);
                CarLogic::gObs[0][lane] = CarLogic::gObs[0][lane] + merit;
            }
        }
    }
}

void AI::CheckForClearLanes(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80059434, "NFS4.EXE");
    sint32 carObjIndexInSortedList = carObj->sortIndex;
    if ((carObj->carFlags & 8) == 0 || IsMellowZone(carObj, 0) != 0)
        return;
    sint32 clearAheadMerits[3] = { 0x40000, 0x40000, 0x40000 };
    uint32 lanesFilled = 0;
    sint32 carsFound = 0;
    sint32 dir = carObj->direction;
    sint32 sortedIndex = (dir * dir) + carObjIndexInSortedList;
    for (sint32 ci = dir; carsFound != 3; ++ci) {
        sint32 numCars = Cars::gNumCars;
        if (ci >= numCars)
            break;
        sint32 index = sortedIndex + numCars;
        Car::tObj *otherCarObj = Cars::gSortedList[index % numCars];
        if (otherCarObj != carObj && otherCarObj->N.active != 0) {
            uint32 distance = (uint32)AIWorld::SplineDistance(otherCarObj, carObj) + 0x40000;
            if (distance < 0x6E0000) {
                sint32 lane = carObj->laneIndex;
                uint32 bit = (uint32)1 << (lane & 31);
                if ((otherCarObj->carInLane & bit) != 0 && (lanesFilled & bit) == 0) {
                    clearAheadMerits[1] = 0;
                    ++carsFound;
                    lanesFilled |= (uint32)1 << ((uint32)carObj->laneIndex & 31);
                    lane = carObj->laneIndex;
                }
                if (lane > 0) {
                    bit = (uint32)1 << (((uint32)lane - 1) & 31);
                    if ((otherCarObj->carInLane & bit) != 0 && (lanesFilled & bit) == 0) {
                        clearAheadMerits[0] = 0;
                        ++carsFound;
                        lanesFilled |= (uint32)1 << (((uint32)carObj->laneIndex - 1) & 31);
                    }
                }
                uint32 rightLane = (uint32)carObj->laneIndex + 1;
                if (rightLane < 14) {
                    bit = (uint32)1 << (rightLane & 31);
                    if ((otherCarObj->carInLane & bit) != 0 && (lanesFilled & bit) == 0) {
                        clearAheadMerits[2] = 0;
                        ++carsFound;
                        lanesFilled |= (uint32)1 << (((uint32)carObj->laneIndex + 1) & 31);
                    }
                }
            }
        }
        sortedIndex = sortedIndex + dir;
    }
    CarLogic::gObs[0][0] = CarLogic::gObs[0][0] + clearAheadMerits[0];
    CarLogic::gObs[0][1] = CarLogic::gObs[0][1] + clearAheadMerits[1];
    CarLogic::gObs[0][2] = CarLogic::gObs[0][2] + clearAheadMerits[2];
}

void AI::CalcBestLineMerits(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800596A4, "NFS4.EXE");
    sint32 checkSlice = carObj->lookAheadSlice;
    if ((carObj->carFlags & 8) == 0)
        return;
    sint32 latPos = static_cast<sint8>(AIDataRecord::BestLine->dataBuffer_[checkSlice]);
    sint32 position = fixedmult(carObj->personality->bestLineAbility, latPos * 16384);
    sint32 slack = carObj->laneSlack * carObj->direction;
    carObj->preferredLateralPosition = position;
    carObj->preferredLateralPositionPower = 0x50000;
    carObj->preferredLateralPosition = position - slack;
}

void AI::AddCollidableObjects(Car::tObj *carObj, Group *groupSimObjs)
{
    FUNCTION_MARKER(0x8005972C, "NFS4.EXE");
    static BWorldSm::Pos spos;
    static char firstTime = 1;
    Trk::SimObject *simObjs = reinterpret_cast<Trk::SimObject *>(groupSimObjs + 1);
    if (firstTime != 0) {
        firstTime = 0;
        BWorldSm::SetSlice(0, &spos);
    }
    for (sint32 objectIndex = 0; objectIndex < groupSimObjs->m_num_elements; ++objectIndex, ++simObjs) {
        coorddef pt;
        pt.x = simObjs->point[0];
        pt.y = simObjs->point[1];
        pt.z = simObjs->point[2];
        BWorldSm::FindClosestSlice(&pt, &spos);
        uint32 distance = AIWorld::ApxSplineDistance(spos.slice, carObj) * (uint32)carObj->direction;
        if (distance - 1 >= 0x63FFFF)
            continue;
        Trk::NewSlice *road = &BWorldSm::slices[carObj->N.simRoadInfo.slice];
        coorddef centerSlice;
        centerSlice.x = road->center[0];
        centerSlice.y = road->center[1];
        centerSlice.z = road->center[2];
        coorddef centerToPt;
        centerToPt.x = pt.x - centerSlice.x;
        centerToPt.y = pt.y - centerSlice.y;
        centerToPt.z = pt.z - centerSlice.z;
        uint32 product = (carObj->N.roadMatrix.m[0] / 256) * (uint32)(centerToPt.x / 256);
        product += (carObj->N.roadMatrix.m[1] / 256) * (uint32)(centerToPt.y / 256);
        product += (carObj->N.roadMatrix.m[2] / 256) * (uint32)(centerToPt.z / 256);
        sint32 latPos = product;
        sint32 avoidance = simObjs->type == 1 ? -0x280000 : -0xD0000;
        sint32 radius = simObjs->radius * 512;
        sint32 left = latPos - radius;
        sint32 right = latPos + radius;
        SubmitObstacle(carObj, avoidance, left, right, spos.slice);
    }
}

void AI::AvoidObjects(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80059928, "NFS4.EXE");
    if (Object::customObjInst != 0)
        AddCollidableObjects(carObj, Object::customSimObjs);
}

void AI::AvoidSpikeBelt(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8005995C, "NFS4.EXE");
    sint32 spikeSlice;
    sint32 leftLatPos;
    sint32 width;
    if (BWorld::GetSpikeBelt(&spikeSlice, &leftLatPos, &width) == 0)
        return;
    uint32 distance = AIWorld::ApxSplineDistance(spikeSlice, carObj) * (uint32)carObj->direction;
    if (distance - 1 < 0x63FFFF) {
        sint32 rightLatPos = leftLatPos + width;
        SubmitObstacle(carObj, -0x280000, leftLatPos, rightLatPos, spikeSlice);
    }
}

void AI::SubmitObstacle(Car::tObj *carObj, sint32 importance, sint32 leftLatPosition, sint32 rightLatPosition, sint32 slice)
{
    FUNCTION_MARKER(0x800599E4, "NFS4.EXE");
    sint32 observations[3] = { 0, 0, 0 };
    sint32 leftEdgeIndex = AIWorld::LaneIndex(slice, leftLatPosition);
    sint32 rightEdgeIndex = AIWorld::LaneIndex(slice, rightLatPosition);
    if ((uint32)rightEdgeIndex >= 14 || (uint32)leftEdgeIndex >= 14)
        return;
    sint32 lane = carObj->laneIndex - 1;
    if (leftEdgeIndex <= lane && lane <= rightEdgeIndex)
        observations[0] = importance;
    lane = carObj->laneIndex;
    if (leftEdgeIndex <= lane && lane <= rightEdgeIndex)
        observations[1] = importance;
    lane = carObj->laneIndex + 1;
    if (leftEdgeIndex <= lane && lane <= rightEdgeIndex)
        observations[2] = importance;
    if (observations[0] != 0 && observations[1] != 0 && observations[2] != 0) {
        sint32 position = carObj->roadPosition;
        sint32 leftDistance = position - leftLatPosition;
        sint32 rightDistance = position - rightLatPosition;
        if (leftDistance < 0)
            leftDistance = -leftDistance;
        if (rightDistance < 0)
            rightDistance = -rightDistance;
        if (leftDistance < rightDistance)
            observations[0] = 0;
        else
            observations[2] = 0;
    }
    CarLogic::gObs[0][0] = CarLogic::gObs[0][0] + observations[0];
    CarLogic::gObs[0][1] = CarLogic::gObs[0][1] + observations[1];
    CarLogic::gObs[0][2] = CarLogic::gObs[0][2] + observations[2];
}

void AI::HandleTrafficHonking(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80059B88, "NFS4.EXE");
    Car::tObj *visibleCar = AILife::IsCarInAnyVisibleArea(carObj);
    if ((carObj->carFlags & 0x10) == 0)
        return;
    sint32 direction = carObj->direction;
    uint32 opposing = ~direction;
    if (GameSetup::gData.reverseTrack == 0)
        opposing = direction ^ 1;
    if (opposing == 0 || visibleCar == 0 || Info.laneSpeeds[1] <= -0x30000)
        return;
    uint32 product = fastRandom * randSeed;
    randtemp = product;
    fastRandom = product & 0xFFFF;
    uint32 choice = (((product >> 8) & 0xFFFF) * 1000) >> 16;
    if (GameSetup::gData.commMode != 1 && choice < 5 && carObj->currentSpeed != 0)
        AudioClc::HonkHorn(carObj, 2, 32, 8);
}

void AI::CheckForCarsOnSide(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80059C8C, "NFS4.EXE");
    if ((carObj->carFlags & 0x10) != 0)
        return;
    sint32 length = carObj->N.dimension.z;
    sint32 blockDistance = ((uint32)length << 1) + (length / 2);
    for (sint32 ci = 0; ci < Cars::gNumCars; ++ci) {
        Car::tObj *otherCarObj = Cars::gList[ci];
        if (otherCarObj == carObj || otherCarObj->N.active == 0)
            continue;
        sint32 absDistance = AIWorld::SplineDistance(otherCarObj, carObj);
        if (absDistance < 0)
            absDistance = -absDistance;
        if (absDistance > 0xA0000 || absDistance >= blockDistance)
            continue;
        sint32 lane = carObj->laneIndex;
        sint32 otherLane = otherCarObj->laneIndex;
        sint32 leftLane = lane - 1;
        sint32 rightLane = lane + 1;
        if (otherLane == leftLane)
            CarLogic::gObs[0][0] = CarLogic::gObs[0][0] - 0x60000;
        else if (otherLane == rightLane)
            CarLogic::gObs[0][2] = CarLogic::gObs[0][2] - 0x60000;
    }
}

void AI::ProcessObservationsAndChooseLane(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80059DDC, "NFS4.EXE");
    sint32 left = CarLogic::gObs[0][0];
    sint32 center = CarLogic::gObs[0][1];
    sint32 right = CarLogic::gObs[0][2];
    Info.laneWeights[0] = left;
    Info.laneWeights[1] = center;
    Info.laneWeights[2] = right;
    ChooseBestLane(carObj);
}

void AI::ChooseBestLane(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80059E20, "NFS4.EXE");
    sint32 best = Info.laneWeights[1];
    sint32 lane = carObj->laneIndex;
    sint32 left = Info.laneWeights[0];
    Info.desiredLaneSide = 1;
    Info.desiredLane = lane;
    if (best < left) {
        best = left;
        Info.desiredLaneSide = 0;
        Info.desiredLane = carObj->laneIndex - 1;
    }
    if (best < Info.laneWeights[2]) {
        Info.desiredLaneSide = 2;
        Info.desiredLane = carObj->laneIndex + 1;
    }
    if (Info.desiredLane < 0)
        Info.desiredLane = 0;
    else if (Info.desiredLane >= 14)
        Info.desiredLane = 13;
}

sint32 AI::CheckPreferredLateralPosition(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80059EAC, "NFS4.EXE");
    sint32 position = (carObj->preferredLateralPosition - carObj->N.dimension.x) - 0x4000;
    sint32 carSideLane = AIWorld::LaneIndex(carObj->N.simRoadInfo.slice, position);
    sint32 lane = carObj->laneIndex;
    sint32 carLeftSideLaneWRTcurrent = carSideLane == lane ? 1 : (carSideLane < lane ? 0 : 2);
    sint32 tempLanePowers[3] = { Info.laneWeights[0], Info.laneWeights[1], Info.laneWeights[2] };
    tempLanePowers[carLeftSideLaneWRTcurrent] = tempLanePowers[carLeftSideLaneWRTcurrent] + carObj->preferredLateralPositionPower;
    sint32 bestLanePower = tempLanePowers[0];
    sint32 leftBestLane = 0;
    if (bestLanePower < tempLanePowers[1]) {
        leftBestLane = 1;
        bestLanePower = tempLanePowers[1];
    }
    if (bestLanePower < tempLanePowers[2])
        leftBestLane = 2;
    position = (carObj->preferredLateralPosition + carObj->N.dimension.x) + 0x4000;
    carSideLane = AIWorld::LaneIndex(carObj->N.simRoadInfo.slice, position);
    lane = carObj->laneIndex;
    sint32 carRightSideLaneWRTcurrent = carSideLane == lane ? 1 : (carSideLane < lane ? 0 : 2);
    tempLanePowers[0] = Info.laneWeights[0];
    tempLanePowers[1] = Info.laneWeights[1];
    tempLanePowers[2] = Info.laneWeights[2];
    tempLanePowers[carRightSideLaneWRTcurrent] = tempLanePowers[carRightSideLaneWRTcurrent] + carObj->preferredLateralPositionPower;
    bestLanePower = tempLanePowers[0];
    sint32 rightBestLane = 0;
    if (bestLanePower < tempLanePowers[1]) {
        rightBestLane = 1;
        bestLanePower = tempLanePowers[1];
    }
    if (bestLanePower < tempLanePowers[2])
        rightBestLane = 2;
    if (leftBestLane != carLeftSideLaneWRTcurrent || rightBestLane != carRightSideLaneWRTcurrent)
        return 0;
    carObj->desiredLatPos = carObj->preferredLateralPosition;
    return 1;
}

sint32 AI::TryToShareLanes(Car::tObj *carObj, Car::tObj *carInWay)
{
    FUNCTION_MARKER(0x8005A060, "NFS4.EXE");
    sint32 width = carObj->N.dimension.x;
    sint32 absLaneIndex = Info.desiredLane;
    sint32 minGapSize = width + (width / 2);
    sint32 leftRoadEdge;
    sint32 rightRoadEdge;
    if (absLaneIndex >= 7) {
        uint32 laneWidth = BWorldSm::slices[carInWay->N.simRoadInfo.slice].avgPavedWidthRt << 15;
        leftRoadEdge = ((uint32)absLaneIndex - 7) * laneWidth;
        rightRoadEdge = leftRoadEdge + laneWidth;
    } else {
        uint32 laneWidth = BWorldSm::slices[carInWay->N.simRoadInfo.slice].avgPavedWidthLf << 15;
        rightRoadEdge = ((uint32)absLaneIndex - 6) * laneWidth;
        leftRoadEdge = rightRoadEdge - laneWidth;
    }
    sint32 position = carInWay->roadPosition;
    sint32 span = carInWay->roadSpan;
    sint32 leftCarEdge = position - span;
    sint32 rightCarEdge = position + span;
    sint32 gapLeft = leftCarEdge - leftRoadEdge;
    sint32 gapRight = rightRoadEdge - rightCarEdge;
    if (gapRight <= minGapSize && gapLeft <= minGapSize)
        return 0;
    if (gapLeft < gapRight)
        carObj->desiredLatPos = rightCarEdge + (gapRight / 2);
    else
        carObj->desiredLatPos = leftCarEdge - (gapLeft / 2);
    return 1;
}

void AI::CalculateDesiredLatPosition(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8005A15C, "NFS4.EXE");
    Car::tObj *carInWay = Info.blockingCars[Info.desiredLaneSide];
    sint32 slice = carObj->N.simRoadInfo.slice;
    if (carObj->preferredLateralPositionPower > 0 && CheckPreferredLateralPosition(carObj) == 1)
        return;
    sint32 share = 0;
    if (carInWay != 0) {
        sint32 firstDistance = AIWorld::ApxSplineDistance(carInWay, carObj);
        sint32 distance = AIWorld::ApxSplineDistance(carInWay, carObj);
        if (firstDistance <= 0)
            distance = -distance;
        if (distance < 0x140000 && IsMellowZone(carObj, 0x3E80000) == 0)
            share = 1;
    }
    if (share && TryToShareLanes(carObj, carInWay) == 1)
        return;
    Trk::NewSlice *road = &BWorldSm::slices[slice];
    uint8 laneCount = road->laneCount;
    sint32 lane = Info.desiredLane;
    sint32 shoulder = lane == 6 - (laneCount >> 4) || lane == (laneCount & 15) + 7;
    if (lane < 7) {
        uint32 width = road->avgPavedWidthLf << 15;
        uint32 position = ((uint32)lane - 6) * width;
        carObj->desiredLatPos = position - (shoulder ? (uint32)carObj->N.dimension.x : width >> 1);
    } else {
        uint32 width = road->avgPavedWidthRt << 15;
        uint32 position = ((uint32)lane - 7) * width;
        carObj->desiredLatPos = position + (shoulder ? (uint32)carObj->N.dimension.x : width >> 1);
    }
    sint32 maxPosition = ((uint32)BWorldSm::slices[slice].rightDrive << 8) - carObj->N.dimension.x;
    if (maxPosition < carObj->desiredLatPos)
        carObj->desiredLatPos = maxPosition;
    sint32 minPosition = carObj->N.dimension.x - ((uint32)BWorldSm::slices[slice].leftDrive << 8);
    if (carObj->desiredLatPos < minPosition)
        carObj->desiredLatPos = minPosition;
}

void AI::CalculateAdjustedDesiredSpeed(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8005A390, "NFS4.EXE");
    sint32 finalLaneSpeed;
    if (Info.desiredLane != carObj->laneIndex && Info.blockingCars[1] != 0 && Info.blockingCars[1]->N.deadTimer == 0) {
        sint32 firstDistance = AIWorld::SplineDistance(carObj, Info.blockingCars[1]);
        sint32 Drel_hit = AIWorld::SplineDistance(carObj, Info.blockingCars[1]);
        if (firstDistance <= 0)
            Drel_hit = -Drel_hit;
        sint32 speed = carObj->currentSpeed;
        sint32 otherSpeed = Info.blockingCars[1]->currentSpeed;
        sint32 Vrel_hit = speed - otherSpeed;
        if (Vrel_hit <= 0)
            Vrel_hit = otherSpeed - speed;
        sint32 position = carObj->desiredLatPos;
        sint32 roadPosition = carObj->roadPosition;
        sint32 Dlane = position - roadPosition;
        if (Dlane <= 0)
            Dlane = roadPosition - position;
        sint32 firstVelocity = AIWorld::CalcLateralVelocity(carObj);
        sint32 Vlane = AIWorld::CalcLateralVelocity(carObj);
        if (firstVelocity <= 0)
            Vlane = -Vlane;
        sint32 a = fixedmult(Dlane, Vrel_hit);
        sint32 b = fixedmult(Drel_hit, Vlane);
        sint32 adjusted = fixedmult(a, 0x13333);
        if (adjusted < b)
            finalLaneSpeed = carObj->laneIndex < Info.desiredLane ? Info.laneSpeeds[2] : Info.laneSpeeds[0];
        else
            finalLaneSpeed = Info.laneSpeeds[1];
    } else {
        finalLaneSpeed = Info.laneSpeeds[1];
    }
    if (carObj->direction == 1) {
        if (finalLaneSpeed < carObj->desiredSpeed)
            carObj->desiredSpeed = finalLaneSpeed;
        if (carObj->desiredSpeed < -0x14CCC)
            carObj->desiredSpeed = -0x14CCC;
    } else {
        if (carObj->desiredSpeed < finalLaneSpeed)
            carObj->desiredSpeed = finalLaneSpeed;
        if (carObj->desiredSpeed > 0x14CCC)
            carObj->desiredSpeed = 0x14CCC;
    }
    if (Info.desiredLane != carObj->laneIndex && Info.blockingCars[1] != 0) {
        sint32 otherSpeed = Info.blockingCars[1]->currentSpeed;
        if (otherSpeed < 0)
            otherSpeed = -otherSpeed;
        if (otherSpeed < 0xA0000) {
            sint32 speed = carObj->desiredSpeed;
            if (carObj->direction == 1) {
                if (speed < 0x6AAAA)
                    speed = 0x6AAAA;
            } else if (speed > -0x6AAAA) {
                speed = -0x6AAAA;
            }
            carObj->desiredSpeed = speed;
        }
    }
}

sint32 AI::IsMellowZone(Car::tObj *carObj, sint32 delay)
{
    FUNCTION_MARKER(0x8005A628, "NFS4.EXE");
    sint32 index = carObj->sortIndex;
    sint32 odom = AIWorld::GameOdometer(carObj);
    if ((carObj->carFlags & 8) == 0)
        return 0;
    uint8 laneCount = BWorldSm::slices[carObj->N.simRoadInfo.slice].laneCount;
    sint32 lane = carObj->laneIndex;
    if (lane == 6 - (laneCount >> 4) || lane == (laneCount & 15) + 7)
        return 0;
    sint32 numCars = Cars::gNumAIRaceCars;
    sint32 endMellow = 0x6400000;
    if (numCars < 5)
        endMellow = 0x3200000;
    if (numCars < 2)
        endMellow = 0xC80000;
    if (numCars == 0)
        endMellow = 0;
    sint32 distance = odom + (index * 0x540000);
    sint32 limit = endMellow + delay;
    return distance <= limit;
}

void AI::KeepCarsInLane(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8005A714, "NFS4.EXE");
}

void AI::PushFinishedCarsToSide(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8005A724, "NFS4.EXE");
    if ((carObj->carFlags & 1) == 0 || carObj->stats.finishType != 2)
        return;
    sint32 raceType = GameSetup::gData.raceType;
    if (raceType == 1 || raceType == 5) {
        if ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0)
            return;
        if (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0)
            return;
    }
    sint32 absDistancePastFinish = AIWorld::ApxSplineDistance(carObj, 0);
    if (absDistancePastFinish < 0)
        absDistancePastFinish = -absDistancePastFinish;
    sint32 numCars = Cars::gNumCars;
    sint32 totalSortIndex = 0;
    while (totalSortIndex < numCars && Cars::gTotalSortedList[totalSortIndex] != carObj)
        ++totalSortIndex;
    sint32 minimumDistance = ((uint32)totalSortIndex * 5) << 19;
    if (absDistancePastFinish < minimumDistance)
        return;
    uint8 laneCount = BWorldSm::slices[carObj->N.simRoadInfo.slice].laneCount;
    sint32 lane = carObj->laneIndex;
    if (lane == 6 - (laneCount >> 4) || lane == (laneCount & 15) + 7)
        CarLogic::gObs[0][1] = CarLogic::gObs[0][1] + 0x960000;
    else if (lane < 7)
        CarLogic::gObs[0][0] = CarLogic::gObs[0][0] + 0x960000;
    else
        CarLogic::gObs[0][2] = CarLogic::gObs[0][2] + 0x960000;
}

void AI::MaybeChangeLaneSlack(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8005A8C0, "NFS4.EXE");
    sint32 adaptedSlice = (uint32)carObj->carIndex << 1;
    if (adaptedSlice >= 0) {
        sint32 numSlices = gNumSlices;
        adaptedSlice = carObj->N.simRoadInfo.slice + adaptedSlice;
        if (adaptedSlice >= numSlices)
            adaptedSlice = adaptedSlice - numSlices;
    } else {
        adaptedSlice = carObj->N.simRoadInfo.slice + adaptedSlice;
        if (adaptedSlice < 0)
            adaptedSlice = adaptedSlice + gNumSlices;
    }
    uint32 flags = carObj->carFlags;
    if ((flags & 4) != 0)
        return;
    if ((flags & 8) != 0 && ((uint32)adaptedSlice & 15) != 0)
        return;
    if ((flags & 0x10) != 0 && ((uint32)adaptedSlice & 1) != 0)
        return;
    uint32 product = fastRandom * randSeed;
    sint32 range = carObj->personality->swerveChangeProb;
    uint32 choice = range * ((product >> 8) & 0xFFFF);
    randtemp = product;
    fastRandom = product & 0xFFFF;
    if ((choice >> 16) == 1)
        ChooseNewLaneSlack(carObj);
}

void AI::ChooseNewLaneSlack(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8005A9DC, "NFS4.EXE");
    uint32 product = fastRandom * randSeed;
    sint32 laneSlack = carObj->personality->laneSwerve[(product >> 8) & 3U];
    randtemp = product;
    fastRandom = product & 0xFFFF;
    carObj->laneSlack = laneSlack;
}
