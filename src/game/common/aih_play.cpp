#include <stddef.h>
#include "xport.h"
#include "game/common/ai.h"
#include "game/common/aihigh.h"
#include "game/common/ailife.h"
#include "game/common/aitriger.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/dashhud.h"
#include "game/common/fastrand.h"
#include "game/common/gmesetup.h"
#include "game/common/sim.h"
#include "game/common/speech.h"
#include "game/common/stats.h"
#include "game/psx/hud.h"
#include "lib/psx/fixdmult.h"

extern char gBlockadeTypes[5];
char gBlockadeTypes[5] = { 5, 6, 4, 2, 0 };
sint32 AIHigh_Player::kNumArrestsByLap[3] = { 3, 5, 8 };

sint32 AIHigh_Player::CheckIfABlockadeCanBeSetup(void)
{
    FUNCTION_MARKER(0x80061400, "NFS4.EXE");
    copLevel_t *pLevel = perpChaseInfo_.chaseLevel_;
    sint32 ready[2] = { 0, 0 };
    sint32 assigned[2] = { 0, 0 };
    sint32 split = Cars::gNumHumanRaceCars == 2;
    if (pLevel->numBlockaders == 0 || perpChaseInfo_.blockadeDone_ != 0 ||
        (basicPerpInfo_.copsAssigned_[0] < pLevel->copChasers[0] && !split) ||
        (basicPerpInfo_.copsAssigned_[1] < pLevel->copChasers[1] && !split))
        return 0;
    sint32 nCopsNeeded[2];
    nCopsNeeded[0] = pLevel->copBlockaders[0];
    nCopsNeeded[1] = pLevel->copBlockaders[1];
    sint32 numCops = Cars::gNumCopCars;
    for (sint32 copLoop = 0; copLoop < numCops; ++copLoop) {
        Car::tObj *cop = Cars::gCopCarList[copLoop];
        AIHigh_Cop *thisCop = static_cast<AIHigh_Cop *>(highLevelAIObjs[cop->carIndex]);
        if ((cop->AIFlags & 12) == 12) {
            sint32 type = thisCop->type_;
            if (assigned[type] < nCopsNeeded[type]) {
                ++ready[type];
                ++assigned[type];
                thisCop->blockade_.mode = BLOCKADEMODE_SETUP;
                thisCop->blockade_.target = this;
            }
        }
    }
    if (assigned[0] < nCopsNeeded[0] || assigned[1] < nCopsNeeded[1]) {
        numCops = Cars::gNumCopCars;
        for (sint32 copLoop = 0; copLoop < numCops; ++copLoop) {
            Car::tObj *cop = Cars::gCopCarList[copLoop];
            AIHigh_Cop *thisCop = static_cast<AIHigh_Cop *>(highLevelAIObjs[cop->carIndex]);
            if ((cop->AIFlags & 12) == 8 && thisCop->blockade_.mode != BLOCKADEMODE_WAITING) {
                sint32 type = thisCop->type_;
                if (assigned[type] < nCopsNeeded[type]) {
                    ++assigned[type];
                    thisCop->blockade_.mode = BLOCKADEMODE_SETUP;
                    thisCop->blockade_.target = this;
                }
            }
        }
    }
    if (Cars::gNumHumanRaceCars != 1 && assigned[1] < nCopsNeeded[1]) {
        numCops = Cars::gNumCopCars;
        for (sint32 copLoop = 0; copLoop < numCops; ++copLoop) {
            Car::tObj *cop = Cars::gCopCarList[copLoop];
            AIHigh_Cop *thisCop = static_cast<AIHigh_Cop *>(highLevelAIObjs[cop->carIndex]);
            if ((cop->AIFlags & 12) == 8 && thisCop->type_ == COP_REGULAR &&
                assigned[1] < nCopsNeeded[1] && assigned[1] == 0) {
                assigned[1] = 1;
                thisCop->blockade_.mode = BLOCKADEMODE_SETUP_FOR_OTHER;
                thisCop->blockade_.target = this;
            }
        }
    }
    return ready[0] >= nCopsNeeded[0] && ready[1] >= nCopsNeeded[1];
}

void AIHigh_Player::SetupBlockade(void)
{
    FUNCTION_MARKER(0x80061784, "NFS4.EXE");
    Car::tObj *car = carObj_;
    copLevel_t *pLevel = perpChaseInfo_.chaseLevel_;
    sint32 addToSlice = (sint32)((uint32)car->direction * 83);
    sint32 searchSlice = (sint32)((uint32)car->N.simRoadInfo.slice + (uint32)addToSlice);
    if (addToSlice < 0) {
        if (searchSlice < 0)
            searchSlice = (sint32)((uint32)searchSlice + (uint32)gNumSlices);
    } else if (searchSlice >= gNumSlices) {
        searchSlice = (sint32)((uint32)searchSlice - (uint32)gNumSlices);
    }
    sint32 nCopsNeeded[2];
    nCopsNeeded[0] = pLevel->copBlockaders[0];
    nCopsNeeded[1] = pLevel->copBlockaders[1];
    sint32 blockadeHandle = triggerManagerCops->CheckForClosestTriggerOfType(searchSlice,
        TRIGGER_COP_ROADBLOCK, carObj_->direction);
    if (blockadeHandle == -1) {
        CheckForNewLevel(1);
        return;
    }
    sint32 used;
    trigger::trigger_t *blockade = triggerManagerCops->GetTrigger(blockadeHandle, &used);
    sint32 loop = 0;
    do {
        if (!AILife::IsSliceInAnyVisibleArea(blockade->any.slice) &&
            !AILife::IsSliceCloseToAnyCopCar(blockade->any.slice))
            break;
        sint32 nextSlice = (sint32)((uint32)blockade->any.slice + 1);
        sint32 numSlices = gNumSlices;
        if (nextSlice >= numSlices)
            nextSlice = (sint32)((uint32)blockade->any.slice - ((uint32)numSlices - 1));
        blockadeHandle = triggerManagerCops->CheckForClosestTriggerOfType(nextSlice,
            TRIGGER_COP_ROADBLOCK, carObj_->direction);
        if (blockadeHandle == -1) {
            CheckForNewLevel(1);
            return;
        }
        ++loop;
        blockade = triggerManagerCops->GetTrigger(blockadeHandle, &used);
    } while (loop < 4);
    sint32 blockadeSlice = blockade->any.slice;
    sint32 requestSpikeBeltAtSlice = pLevel->spikeBelt != 0 ? blockadeSlice : -1;
    sint32 nCopsAvail[2] = { 0, 0 };
    sint32 needed[2] = { nCopsNeeded[0], nCopsNeeded[1] };
    sint32 numCops = Cars::gNumCopCars;
    for (sint32 copLoop = 0; copLoop < numCops; ++copLoop) {
        Car::tObj *cop = Cars::gCopCarList[copLoop];
        AIHigh_Cop *thisCop = static_cast<AIHigh_Cop *>(highLevelAIObjs[cop->carIndex]);
        if ((cop->AIFlags & 4) != 0 && thisCop->blockade_.mode == BLOCKADEMODE_SETUP &&
            needed[thisCop->type_] != 0) {
            sint32 type = thisCop->type_;
            needed[type] = (sint32)((uint32)needed[type] - 1);
            ++nCopsAvail[type];
        }
    }
    uint32 product = fastRandom * randSeed;
    randtemp = product;
    fastRandom = product & 0xFFFF;
    sint32 blockadeFlags = (uint8)gBlockadeTypes[((product >> 8) & 0xFFFF) % 5];
    sint32 posIndex = 0;
    bool saySpikeBelt = false;
    AIHigh_Cop *blockadeCar = 0;
    for (sint32 copLoop = 0; copLoop < Cars::gNumCopCars; ++copLoop) {
        Car::tObj *cop = Cars::gCopCarList[copLoop];
        AIHigh_Cop *thisCop = static_cast<AIHigh_Cop *>(highLevelAIObjs[cop->carIndex]);
        if ((cop->AIFlags & 4) == 0 || thisCop->blockade_.mode != BLOCKADEMODE_SETUP)
            continue;
        bool superPlacement = thisCop->type_ == COP_SUPER && nCopsNeeded[1] != 0;
        sint32 type = superPlacement ? 1 : 0;
        if (nCopsNeeded[type] == 0)
            continue;
        if (!blockadeCar)
            blockadeCar = thisCop;
        nCopsNeeded[type] = (sint32)((uint32)nCopsNeeded[type] - 1);
        blockade_t *copBlockade = &thisCop->blockade_;
        copBlockade->blockadeSpeechFlags = 0;
        copBlockade->flags = blockadeFlags;
        copBlockade->chaseLevel = perpChaseInfo_.chaseLevelIndex_;
        copBlockade->mode = BLOCKADEMODE_WAITING;
        addToSlice = (sint32)((uint32)((posIndex / 2) * 2 + 3) * (uint32)carObj_->direction);
        blockadeFlags = 0;
        sint32 slice = (sint32)((uint32)blockadeSlice + (uint32)addToSlice);
        if (addToSlice < 0) {
            if (slice < 0)
                slice = (sint32)((uint32)slice + (uint32)gNumSlices);
        } else if (slice >= gNumSlices) {
            slice = (sint32)((uint32)slice - (uint32)gNumSlices);
        }
        copBlockade->slice = slice;
        if (superPlacement) {
            copBlockade->direction = carObj_->direction;
            Trk::NewSlice *road = &BWorldSm::slices[copBlockade->slice];
            sint32 left = road->avgPavedWidthLf * 0x8000 * (road->laneCount >> 4);
            sint32 totalRoadWidth = left + road->avgPavedWidthRt * 0x8000 * (road->laneCount & 15);
            if (nCopsAvail[1] == 1 && nCopsAvail[0] == 0) {
                copBlockade->latPos = totalRoadWidth / 2 - left;
                copBlockade->rotation = 255;
            } else {
                road = &BWorldSm::slices[copBlockade->slice];
                left = road->avgPavedWidthLf * 0x8000 * (road->laneCount >> 4);
                sint32 quarter = totalRoadWidth / 4;
                if ((posIndex & 1) == 0) {
                    copBlockade->latPos = quarter - left;
                    copBlockade->rotation = 190;
                } else {
                    copBlockade->latPos = quarter * 3 - left;
                    copBlockade->rotation = -190;
                }
            }
            product = fastRandom * randSeed;
            slice = copBlockade->slice;
            copBlockade->target = this;
            copBlockade->reverse = 0;
            uint32 releaseProduct = ((product >> 8) & 0xFFFF) * 0x14CCD;
            copBlockade->releaseTime = (releaseProduct >> 16) + 0xD999;
            randtemp = product;
            fastRandom = product & 0xFFFF;
            sint32 distance = AIWorld::ApxSplineDistance(carObj_, slice);
            sint32 initialDistance = -(distance / 65536);
            copBlockade->initialPlayerDistanceMetersInt = initialDistance;
            sint32 directedDistance = (sint32)((uint32)initialDistance * (uint32)carObj_->direction);
            if (directedDistance < 0)
                copBlockade->initialPlayerDistanceMetersInt = 0;
        } else {
            product = fastRandom * randSeed;
            copBlockade->direction = carObj_->direction;
            randtemp = product;
            fastRandom = product & 0xFFFF;
            copBlockade->reverse = ((((product >> 8) & 0xFFFF) * 1000) >> 16) < 300 ? 1 : 0;
            product = fastRandom * randSeed;
            slice = copBlockade->slice;
            uint32 releaseProduct = ((product >> 8) & 0xFFFF) * 0x14CCD;
            copBlockade->releaseTime = (releaseProduct >> 16) + 0xD999;
            randtemp = product;
            fastRandom = product & 0xFFFF;
            sint32 distance = AIWorld::ApxSplineDistance(carObj_, slice);
            sint32 initialDistance = -(distance / 65536);
            copBlockade->initialPlayerDistanceMetersInt = initialDistance;
            sint32 directedDistance = (sint32)((uint32)initialDistance * (uint32)carObj_->direction);
            if (directedDistance < 0)
                copBlockade->initialPlayerDistanceMetersInt = 0;
            Trk::NewSlice *road = &BWorldSm::slices[copBlockade->slice];
            sint32 left = road->avgPavedWidthLf * 0x8000 * (road->laneCount >> 4);
            sint32 totalRoadWidth = left + road->avgPavedWidthRt * 0x8000 * (road->laneCount & 15);
            if (nCopsAvail[0] == 1 && nCopsAvail[1] == 0) {
                copBlockade->latPos = totalRoadWidth / 2 - left;
                copBlockade->rotation = 255;
            } else {
                road = &BWorldSm::slices[copBlockade->slice];
                left = road->avgPavedWidthLf * 0x8000 * (road->laneCount >> 4);
                sint32 quarter = totalRoadWidth / 4;
                if ((posIndex & 1) == 0) {
                    copBlockade->latPos = quarter - left;
                    copBlockade->rotation = 190;
                } else {
                    copBlockade->latPos = quarter * 3 - left;
                    copBlockade->rotation = -190;
                }
            }
            copBlockade->target = this;
        }
        if (requestSpikeBeltAtSlice != -1) {
            saySpikeBelt = true;
            copBlockade->requestSpikeBeltAtSlice = requestSpikeBeltAtSlice;
            requestSpikeBeltAtSlice = -1;
        }
        perpChaseInfo_.blockadeDone_ = 1;
        ++posIndex;
    }
    if (blockadeCar) {
        blockadeCar->blockade_.blockadeSpeechFlags = 1;
        if (saySpikeBelt)
            Speech::Mobile(blockadeCar->carObj_)->SpikeBelt();
        else
            Speech::Mobile(blockadeCar->carObj_)->RoadBlock();
        Speech::Mobile(blockadeCar->carObj_)->Engage(carObj_);
        Speech::Dispatch()->Grant();
        Speech::Dispatch()->Ready(blockadeCar->carObj_);
    }
}

void AIHigh_Player::CheckForNewLevel(sint32 force)
{
    FUNCTION_MARKER(0x8006220C, "NFS4.EXE");
    sint32 oldChaseLevel = perpChaseInfo_.chaseLevelIndex_;
    sint32 chaseLevel = oldChaseLevel;
    bool finished = carObj_->stats.finishType >= 2;
    bool resetTimer = finished;
    if (finished) {
        perpChaseInfo_.chaseLevelIndex_ = 0;
        if (perpChaseInfo_.bestChaseLevelIndex_ < 0)
            perpChaseInfo_.bestChaseLevelIndex_ = 0;
    } else {
        CheckForCrimes();
        if (basicPerpInfo_.crime_ != CRIME_NONE &&
            (force != 0 || perpChaseInfo_.engagementTime_ / 65536 <= 0)) {
            chaseLevel = (sint32)((uint32)chaseLevel + 1);
            sint32 numLevels = perpChaseInfo_.copGameInfo_->numLevels;
            if (chaseLevel >= numLevels)
                chaseLevel = (sint32)((uint32)numLevels - 2);
            perpChaseInfo_.chaseLevelIndex_ = chaseLevel;
            if (perpChaseInfo_.bestChaseLevelIndex_ < chaseLevel)
                perpChaseInfo_.bestChaseLevelIndex_ = chaseLevel;
            resetTimer = true;
        }
    }
    if (resetTimer) {
        copLevel_t *pLevel = perpChaseInfo_.copGameInfo_->levels + perpChaseInfo_.chaseLevelIndex_;
        perpChaseInfo_.chaseLevel_ = pLevel;
        sint32 product = (sint32)((uint32)pLevel->engagementLapFraction * (uint32)AITune::gRoughLapTime);
        sint32 seconds = product / 65536;
        sint32 divisor = (sint32)((uint32)seconds << 5);
        XASSERT(divisor > 0);
        sint32 increase = 65536 / divisor;
        perpChaseInfo_.engagementTime_ = (sint32)((uint32)seconds << 21);
        perpChaseInfo_.engagementPercentIncreasePerTick_ = increase;
        if (GameSetup::gData.numLaps == 2)
            perpChaseInfo_.engagementPercentIncreasePerTick_ = fixedmult(increase, 0x13333);
        else if (GameSetup::gData.numLaps == 4)
            perpChaseInfo_.engagementPercentIncreasePerTick_ = fixedmult(increase, 0xA8F5);
        perpChaseInfo_.blockadeDone_ = 0;
    }
    if (finished) {
        basicPerpInfo_.crime_ = CRIME_NONE;
        return;
    }
    if (oldChaseLevel != perpChaseInfo_.chaseLevelIndex_)
        newTriggerProb_ = (sint32)((uint32)triggerManagerCops->invNumTriggers_ *
            (uint32)perpChaseInfo_.chaseLevel_->copsPerLap);
}

void AIHigh_Player::HandleSpeech(void)
{
    FUNCTION_MARKER(0x800624EC, "NFS4.EXE");
    sint32 highestRankedCopIndex = positionVSCopList_[0].carIndex;
    if (highestRankedCopIndex == -1)
        highestRankedCopIndex = Cars::gCopCarList[0]->carIndex;
    sint32 carIndex = positionVSCopList_[1].carIndex;
    if (carIndex != -1 && carIndex < highestRankedCopIndex)
        highestRankedCopIndex = carIndex;
    carIndex = positionVSCopList_[2].carIndex;
    if (carIndex != -1 && carIndex < highestRankedCopIndex)
        highestRankedCopIndex = carIndex;
    sint32 player = carObj_->carIndex;
    sint32 arrestType = 4;
    switch (pullOverMode_) {
    case PULLOVER_WARNING:
        Hud::Perp_OverlayOn(player, 0);
        arrestType = 2;
        break;
    case PULLOVER_BUST: {
        Hud::Perp_OverlayOn(player, 1);
        sint32 busts = numBusts_;
        arrestType = busts < 3 ? 8 : (sint32)((uint32)busts + 6);
        break;
    }
    case PULLOVER_EOG:
        Hud::Perp_OverlayOn(player, 2);
        arrestType = 1;
        break;
    default:
        break;
    }
    if (AICop::gRoadBlockState != AICop::kAICop_RoadBlockState_None)
        AICop::gRoadBlockState = AICop::kAICop_RoadBlockState_PerpPassed;
    Speech::Mobile(Cars::gList[highestRankedCopIndex])->Catch(arrestType);
}

void AIHigh_Player::MaintainAvailableCops(void)
{
    FUNCTION_MARKER(0x80062638, "NFS4.EXE");
    sint32 need[2] = { 0, 0 };
    sint32 got[2] = { 0, 0 };
    sint32 availableCops = 3;
    if (Cars::gNumRaceCars != 1)
        availableCops = Cars::gNumHumanRaceCars == 2 ? 2 : 4;
    sint32 numPlayers = Cars::gNumRaceCars;
    for (sint32 playLoop = 0; playLoop < numPlayers; ++playLoop) {
        Car::tObj *playerCarObj = Cars::gRaceCarList[playLoop];
        AIHigh_Player *playerHighObj = static_cast<AIHigh_Player *>(highLevelAIObjs[playerCarObj->carIndex]);
        need[0] = (uint32)need[0] + (uint32)playerHighObj->perpChaseInfo_.chaseLevel_->copBlockaders[0];
        need[1] = (uint32)need[1] + (uint32)playerHighObj->perpChaseInfo_.chaseLevel_->copBlockaders[1];
        need[0] = (uint32)need[0] + (uint32)playerHighObj->perpChaseInfo_.chaseLevel_->copChasers[0];
        need[1] = (uint32)need[1] + (uint32)playerHighObj->perpChaseInfo_.chaseLevel_->copChasers[1];
    }
    sint32 numCops = Cars::gNumCopCars;
    for (sint32 copLoop = 0; copLoop < numCops; ++copLoop) {
        Car::tObj *copCarObj = Cars::gCopCarList[copLoop];
        AIHigh_Cop *copHighObj = static_cast<AIHigh_Cop *>(highLevelAIObjs[copCarObj->carIndex]);
        if ((copCarObj->AIFlags & 4) == 0 || copHighObj->blockade_.mode == BLOCKADEMODE_SETUP ||
            copHighObj->blockade_.mode == BLOCKADEMODE_WAITING) {
            ++got[copHighObj->type_];
            --availableCops;
            copCarObj->AIFlags |= 8;
        } else {
            copCarObj->AIFlags &= ~8;
        }
    }
    numCops = Cars::gNumCopCars;
    for (sint32 copLoop = 0; availableCops > 0 && copLoop < numCops; ++copLoop) {
        Car::tObj *copCarObj = Cars::gCopCarList[copLoop];
        if ((copCarObj->AIFlags & 8) == 0) {
            AIHigh_Cop *copHighObj = static_cast<AIHigh_Cop *>(highLevelAIObjs[copCarObj->carIndex]);
            sint32 type = copHighObj->type_;
            if (got[type] < need[type]) {
                ++got[type];
                --availableCops;
                copCarObj->AIFlags |= 8;
            }
        }
    }
}

AIHigh_Player::AIHigh_Player(Car::tObj *carObj) : AIHigh_BasicPerp(carObj)
{
    FUNCTION_MARKER(0x800628C0, "NFS4.EXE");
    sint32 lapIndex = GameSetup::gData.numLaps != 2;
    sint32 gameIndex = GameSetup::gData.commMode == 1 ? 2 : (Cars::gNumAIRaceCars > 0 ? 4 : 0);
    copGame_t *copGameInfo = &copGame[gameIndex + lapIndex];
    perpChaseInfo_.copGameInfo_ = copGameInfo;
    perpChaseInfo_.chaseLevelIndex_ = 0;
    sint32 level = perpChaseInfo_.chaseLevelIndex_;
    perpChaseInfo_.engagementTime_ = 0;
    perpChaseInfo_.bestChaseLevelIndex_ = 0;
    copLevel_t *levels = copGameInfo->levels;
    perpChaseInfo_.blockadeDone_ = 0;
    perpChaseInfo_.copFreeTicks_ = 0;
    perpChaseInfo_.totalEngagementPercent_ = 0;
    perpChaseInfo_.engagementPercentIncreasePerTick_ = 0;
    perpChaseInfo_.chaseLevel_ = levels + level;
    numWarnings_ = 0;
    numBusts_ = 0;
    if (GameSetup::gData.cops == 0)
        return;
    newTriggerProb_ = (sint32)((uint32)triggerManagerCops->invNumTriggers_ *
        (uint32)perpChaseInfo_.chaseLevel_->copsPerLap);
    lastTriggerCheckSlice_ = carObj_->N.simRoadInfo.slice;
    perpChaseInfo_.chaseLevelIndex_ = 0;
    if (perpChaseInfo_.bestChaseLevelIndex_ < 0)
        perpChaseInfo_.bestChaseLevelIndex_ = 0;
    copLevel_t *pLevel = perpChaseInfo_.copGameInfo_->levels + perpChaseInfo_.chaseLevelIndex_;
    perpChaseInfo_.chaseLevel_ = pLevel;
    sint32 product = (sint32)((uint32)pLevel->engagementLapFraction * (uint32)AITune::gRoughLapTime);
    sint32 seconds = product / 65536;
    sint32 divisor = (sint32)((uint32)seconds << 5);
    XASSERT(divisor > 0);
    sint32 increase = 65536 / divisor;
    perpChaseInfo_.engagementTime_ = (sint32)((uint32)seconds << 21);
    perpChaseInfo_.engagementPercentIncreasePerTick_ = increase;
    if (GameSetup::gData.numLaps == 2)
        perpChaseInfo_.engagementPercentIncreasePerTick_ = fixedmult(increase, 0x13333);
    else if (GameSetup::gData.numLaps == 4)
        perpChaseInfo_.engagementPercentIncreasePerTick_ = fixedmult(increase, 0xA8F5);
    perpChaseInfo_.blockadeDone_ = 0;
}

void AIHigh_Player::HandleCops(void)
{
    FUNCTION_MARKER(0x80062AC4, "NFS4.EXE");
    copLevel_t *pLevel = perpChaseInfo_.chaseLevel_;
    if (Cars::gNumCopCars == 0)
        return;
    MaintainAvailableCops();
    if (CheckIfABlockadeCanBeSetup())
        SetupBlockade();
    if (pLevel->numBlockaders == 0)
        CleanupBlockaders(0);
    Car::tObj *car = carObj_;
    sint32 travelSpeed = (sint32)((uint32)car->currentSpeed * (uint32)car->direction);
    sint32 totalCopsEngaged = (sint32)((uint32)basicPerpInfo_.copsAssigned_[0] +
        (uint32)basicPerpInfo_.copsAssigned_[1]);
    sint32 ticks = AI::elapsedTime;
    if (totalCopsEngaged > 0) {
        perpChaseInfo_.copFreeTicks_ = 0;
        sint32 engagementTime = perpChaseInfo_.engagementTime_;
        if (engagementTime / 65536 >= -1) {
            uint32 decrease = (uint32)ticks << (travelSpeed < 0 ? 15 : 16);
            perpChaseInfo_.engagementTime_ = (uint32)engagementTime - decrease;
            sint32 lapTime = (sint32)((uint32)perpChaseInfo_.chaseLevel_->engagementLapFraction *
                (uint32)AITune::gRoughLapTime);
            sint32 threshold = (lapTime / 65536) * 32 - 128;
            if (perpChaseInfo_.engagementTime_ / 65536 < threshold)
                perpChaseInfo_.totalEngagementPercent_ = (uint32)perpChaseInfo_.totalEngagementPercent_ +
                    (uint32)perpChaseInfo_.engagementPercentIncreasePerTick_ * (uint32)ticks;
        }
    } else {
        perpChaseInfo_.copFreeTicks_ = (uint32)perpChaseInfo_.copFreeTicks_ + (uint32)ticks;
    }
    CheckForNewLevel(0);
    HandlePullOver();
}

void AIHigh_Player::CleanupBlockaders(sint32 forceClearAll)
{
    FUNCTION_MARKER(0x80062C64, "NFS4.EXE");
    Car::tObj *car = carObj_;
    sint32 clearWaitingBlockaders = car->stats.numArrests > 0 || car->stats.finishType >= 2 || forceClearAll != 0;
    for (sint32 copLoop = 0; copLoop < Cars::gNumCopCars; ++copLoop) {
        AIHigh_Cop *thisCop = static_cast<AIHigh_Cop *>(highLevelAIObjs[Cars::gCopCarList[copLoop]->carIndex]);
        blockade_t *blockade = &thisCop->blockade_;
        blockadeMode_t mode = blockade->mode;
        if ((mode == BLOCKADEMODE_SETUP || mode == BLOCKADEMODE_SETUP_FOR_OTHER ||
                (mode == BLOCKADEMODE_WAITING && clearWaitingBlockaders)) && blockade->target == this) {
            blockade->mode = BLOCKADEMODE_NONE;
            thisCop->AssignToPlayer(0);
        }
    }
}

void AIHigh_Player::HandlePullOver(void)
{
    FUNCTION_MARKER(0x80062D7C, "NFS4.EXE");
    if (pullOverMode_ != PULLOVER_NONE) {
        beatingTicksLeft_ = (sint32)((uint32)beatingTicksLeft_ - (uint32)AI::elapsedTime);
        if (beatingTicksLeft_ > 0)
            return;
        sint32 player = carObj_->carIndex;
        if (player < 2)
            Hud::Perp_OverlayOff(player);
        lastPullOverTime_ = simGlobal.gameTicks;
        if (pullOverMode_ == PULLOVER_EOG) {
            if ((carObj_->carFlags & 4) != 0)
                AICop::numArrestedHumans = (sint32)((uint32)AICop::numArrestedHumans + 1);
            carObj_->stats.finishType = 3;
        }
        if (AICop::numArrestedHumans == Cars::gNumHumanRaceCars) {
            simVar.endSimGame = 1;
            Stats::ExtrapolateOpponentTimes(2);
        }
        perpChaseInfo_.chaseLevelIndex_ = 0;
        if (perpChaseInfo_.bestChaseLevelIndex_ < 0)
            perpChaseInfo_.bestChaseLevelIndex_ = 0;
        copLevel_t *pLevel = perpChaseInfo_.copGameInfo_->levels + perpChaseInfo_.chaseLevelIndex_;
        perpChaseInfo_.chaseLevel_ = pLevel;
        sint32 product = (sint32)((uint32)pLevel->engagementLapFraction * (uint32)AITune::gRoughLapTime);
        sint32 seconds = product / 65536;
        sint32 divisor = (sint32)((uint32)seconds << 5);
        XASSERT(divisor > 0);
        sint32 increase = 65536 / divisor;
        perpChaseInfo_.engagementTime_ = (sint32)((uint32)seconds << 21);
        perpChaseInfo_.engagementPercentIncreasePerTick_ = increase;
        if (GameSetup::gData.numLaps == 2)
            perpChaseInfo_.engagementPercentIncreasePerTick_ = fixedmult(increase, 0x13333);
        else if (GameSetup::gData.numLaps == 4)
            perpChaseInfo_.engagementPercentIncreasePerTick_ = fixedmult(increase, 0xA8F5);
        perpChaseInfo_.blockadeDone_ = 0;
        basicPerpInfo_.crime_ = CRIME_NONE;
        RemoveCloseCops();
        if (pullOverMode_ != PULLOVER_EOG || Cars::gNumHumanRaceCars != 1 || (carObj_->carFlags & 8) != 0)
            Cars::ResetCollidedCars(carObj_, 1, 1);
        if (pullOverMode_ != PULLOVER_EOG) {
            carObj_->pullOver = 0;
        } else if (Cars::gNumHumanRaceCars == 2 && AICop::numArrestedHumans != 2) {
            sint32 *hudSlots[9] = {
                &DashHUD::gInfo.showhud[0], &DashHUD::gInfo.showhud[1],
                &DashHUD::gInfo.showmap[0], &DashHUD::gInfo.showmap[1],
                &DashHUD::gInfo.wrongway[0], &DashHUD::gInfo.wrongway[1],
                &DashHUD::gInfo.laptime, &DashHUD::gInfo.lap, &DashHUD::gInfo.maxlaps
            };
            sint32 carIndex = carObj_->carIndex;
            XASSERT(carIndex >= 0 && carIndex < 9);
            *hudSlots[carIndex] = 0;
        }
        pullOverMode_ = PULLOVER_NONE;
        return;
    }
    if (!CheckIfCaught())
        return;
    carObj_->pullOver = 1;
    CleanupBlockaders(1);
    copLevel_t *pLevel = perpChaseInfo_.chaseLevel_;
    sint32 product = (sint32)((uint32)pLevel->engagementLapFraction * (uint32)AITune::gRoughLapTime);
    sint32 lapTicks = (sint32)((uint32)(product / 65536) << 5);
    sint32 engagementSeconds = perpChaseInfo_.engagementTime_ / 65536;
    beatingTicksLeft_ = pLevel->beatingTicks;
    lastPullOverTime_ = simGlobal.gameTicks;
    sint32 chaseTime = (sint32)((uint32)lapTicks - (uint32)engagementSeconds);
    bool warning = basicPerpInfo_.crime_ != CRIME_SMASHCOP && carObj_->stats.numFines == 0 &&
        perpChaseInfo_.copGameInfo_->levels[perpChaseInfo_.bestChaseLevelIndex_].numWarningsAdded != 0 &&
        chaseTime < perpChaseInfo_.chaseLevel_->warningTicks;
    if (warning && numWarnings_ < 2) {
        Car::tObj *car = carObj_;
        numWarnings_ = (sint32)((uint32)numWarnings_ + (uint32)perpChaseInfo_.chaseLevel_->numWarningsAdded);
        car->stats.numWarnings = (sint32)((uint32)car->stats.numWarnings + 1);
        pullOverMode_ = PULLOVER_WARNING;
    } else {
        Car::tObj *car = carObj_;
        numBusts_ = (sint32)((uint32)numBusts_ + 1);
        car->stats.numFines = (sint32)((uint32)car->stats.numFines + 1);
        sint32 lapIndex = 2;
        if (GameSetup::gData.numLaps == 2)
            lapIndex = 0;
        else if (GameSetup::gData.numLaps == 4)
            lapIndex = 1;
        if (numBusts_ >= kNumArrestsByLap[lapIndex] ||
            (perpChaseInfo_.bestChaseLevelIndex_ == (sint32)((uint32)perpChaseInfo_.copGameInfo_->numLevels - 1) &&
                Cars::gNumHumanRaceCars == 1)) {
            pullOverMode_ = PULLOVER_EOG;
            car = carObj_;
            beatingTicksLeft_ = (sint32)((uint32)beatingTicksLeft_ + 192);
            car->stats.numArrests = (sint32)((uint32)car->stats.numArrests + 1);
        } else {
            pullOverMode_ = PULLOVER_BUST;
        }
    }
    HandleSpeech();
}
