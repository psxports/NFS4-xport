#include <new>
#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/ai.h"
#include "game/common/aihigh.h"
#include "game/common/aistate.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/gmesetup.h"
#include "game/common/schedule.h"

AIHigh_Base *highLevelAIObjs[9];
AIHigh::CopGameType_t AIHigh::CopGameType = AIHigh::COP_GAME_NO;

void *AIHigh_Base::operator new(size_t size) throw()
{
    return reservememadr(0, (sint32)size, 0);
}

void AIHigh_Base::operator delete(void *memory) throw()
{
    purgememadr(memory);
}

void AIHigh::StartUp(void)
{
    FUNCTION_MARKER(0x8005AD94, "NFS4.EXE");
    sint32 copCounter = 0;
    sint32 humanCopCounter = 0;
    AIState::StartUp();
    sint32 raceType = GameSetup::gData.raceType;
    bool btc = false;
    if (raceType == 1 || raceType == 5) {
        btc = (Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0;
        if (!btc && Cars::gNumHumanRaceCars == 2)
            btc = (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0;
    }
    if (btc) {
        for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
            Car::tObj *carObj = Cars::gList[carLoop];
            sint32 flags = carObj->carFlags;
            AIHigh_Base *object;
            if ((flags & 0x200) != 0) {
                void *storage = reservememadr(0, sizeof(AIHigh_BTC_HumanCop), 0);
                sint32 copIndex = copCounter++;
                object = ::new (storage) AIHigh_BTC_HumanCop(carObj, copIndex);
            } else if ((flags & 4) != 0) {
                void *storage = reservememadr(0, sizeof(AIHigh_BTC_HumanPerp), 0);
                object = ::new (storage) AIHigh_BTC_HumanPerp(carObj);
            } else if ((flags & 8) != 0) {
                void *storage = reservememadr(0, sizeof(AIHigh_BTC_AIPerp), 0);
                object = ::new (storage) AIHigh_BTC_AIPerp(carObj);
            } else if ((flags & 0x10) != 0) {
                void *storage = reservememadr(0, sizeof(AIHigh_Traffic), 0);
                object = ::new (storage) AIHigh_Traffic(carObj);
            } else if ((flags & 0x20) != 0) {
                void *storage = reservememadr(0, sizeof(AIHigh_BTC_Wingman), 0);
                sint32 copIndex = copCounter++;
                object = ::new (storage) AIHigh_BTC_Wingman(carObj, copIndex);
            } else {
                void *storage = reservememadr(0, sizeof(AIHigh_None), 0);
                object = ::new (storage) AIHigh_None(carObj);
            }
            highLevelAIObjs[carLoop] = object;
            if ((carObj->carFlags & 0x200) != 0)
                ++humanCopCounter;
        }
        if (humanCopCounter == 2)
            CopGameType = COP_GAME_BTC_2HC;
        else if (humanCopCounter == 1 && copCounter == 1)
            CopGameType = COP_GAME_BTC_1HC1HP;
        else
            CopGameType = COP_GAME_BTC_1HC;
    } else {
        for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
            Car::tObj *carObj = Cars::gList[carLoop];
            sint32 flags = carObj->carFlags;
            AIHigh_Base *object;
            if ((flags & 4) != 0) {
                void *storage = reservememadr(0, sizeof(AIHigh_Human), 0);
                object = ::new (storage) AIHigh_Human(carObj);
            } else if ((flags & 8) != 0) {
                void *storage = reservememadr(0, sizeof(AIHigh_Opponent), 0);
                object = ::new (storage) AIHigh_Opponent(carObj);
            } else if ((flags & 0x10) != 0) {
                void *storage = reservememadr(0, sizeof(AIHigh_Traffic), 0);
                object = ::new (storage) AIHigh_Traffic(carObj);
            } else if ((flags & 0x20) != 0) {
                void *storage = reservememadr(0, sizeof(AIHigh_Cop), 0);
                sint32 copIndex = copCounter++;
                object = ::new (storage) AIHigh_Cop(carObj, copIndex);
            } else {
                void *storage = reservememadr(0, sizeof(AIHigh_None), 0);
                object = ::new (storage) AIHigh_None(carObj);
            }
            highLevelAIObjs[carLoop] = object;
        }
        CopGameType = copCounter > 0 ? COP_GAME_PURSUIT : COP_GAME_NO;
    }
}

void AIHigh::Restart2(void)
{
    FUNCTION_MARKER(0x8005B15C, "NFS4.EXE");
    AIState::Restart();
    StartUp();
}

void AIHigh::CleanUp(void)
{
    FUNCTION_MARKER(0x8005B184, "NFS4.EXE");
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
        AIHigh_Base *object = highLevelAIObjs[carLoop];
        if (object) {
            delete object;
            highLevelAIObjs[carLoop] = 0;
        }
    }
    AIState::CleanUp();
}

void AIHigh::Execute(void)
{
    FUNCTION_MARKER(0x8005B210, "NFS4.EXE");
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop) {
        AIHigh_Base *object = highLevelAIObjs[carLoop];
        Car::tObj *carObj = Cars::gList[carLoop];
        if (object) {
            bool execute = object->schedulingOff_ != 0;
            if (!execute)
                execute = Sched::ExecuteCheck(1, 0, carObj->N.distToPlayer, carObj->N.objID, &AI::time, &AI::elapsedTime, &AI::iTime, carObj->forceNoSimOptz) != 0;
            if (execute)
                highLevelAIObjs[carLoop]->HighExecute();
        }
    }
}

AIHigh_Base::AIHigh_Base(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8005B318, "NFS4.EXE");
    carObj_ = carObj;
    state_ = 0;
    stateType_ = STATE_NONE;
    void *storage = reservememadr(0, sizeof(AIState_None), 0);
    AIState_Base *nextState = ::new (storage) AIState_None(carObj_);
    AIState_Base *previous = state_;
    if (previous)
        delete previous;
    Car::tObj *car = carObj_;
    state_ = nextState;
    stateType_ = STATE_NONE;
    schedulingOff_ = 0;
    lastTrafficTriggerCheckSlice_ = car->N.simRoadInfo.slice;
}

AIHigh_Base::~AIHigh_Base(void)
{
    FUNCTION_MARKER(0x8005B3C4, "NFS4.EXE");
    AIState_Base *state = state_;
    if (state) {
        delete state;
        state_ = 0;
    }
}

AIHigh_None::AIHigh_None(Car::tObj *carObj) : AIHigh_Base(carObj)
{
}

void AIHigh_None::HighExecute(void)
{
    FUNCTION_MARKER(0x8005B460, "NFS4.EXE");
}
