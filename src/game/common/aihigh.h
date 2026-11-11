#ifndef AIHIGH_H
#define AIHIGH_H

#include <stddef.h>
#include "xport.h"
#include "game/common/aicop.h"
#include "game/common/aistate.h"
#include "game/common/aitriger.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/scene.h"

class AIHigh
{
public:
    enum CopGameType_t
    {
        COP_GAME_NO = 0,
        COP_GAME_PURSUIT = 1,
        COP_GAME_BTC_1HC = 2,
        COP_GAME_BTC_2HC = 3,
        COP_GAME_BTC_1HC1HP = 4
    };

    static CopGameType_t CopGameType;
    static sint32 BTC_uTurnProb1000Skills[3];

    enum tAttackMode
    {
        NO_ATTACK = 0,
        REAR_END = 1,
        SCRIPT_ATTACK = 2,
        PROVOKED_ATTACK = 3
    };

    enum tDriveAwayMode
    {
        DRIVEAWAY_NONE = 0,
        DRIVEAWAY_RESET = 1,
        DRIVEAWAY_NORESET = 2
    };

    static void StartUp(void);
    static void Restart2(void);
    static void CleanUp(void);
    static void Execute(void);
};

enum stateType_t
{
    STATE_NONE = 0,
    STATE_PURGATORY = 1,
    STATE_NORMAL = 2,
    STATE_IDLE = 3,
    STATE_CHASE = 4,
    STATE_OFFROAD = 5,
    STATE_ROVING_TRAFFIC = 6,
    STATE_NONACTIVE = 7,
    STATE_DONUTS = 8,
    STATE_GOTOSLICE = 9,
    STATE_CRUISE = 10
};

enum copType
{
    COP_REGULAR = 0,
    COP_SUPER = 1
};

enum Wingman_Role
{
    ROLE_IDLE = 0,
    ROLE_WINGMAN = 1,
    ROLE_BLOCKADER = 2,
    ROLE_BLOCKADER_SPIKEBELT = 3
};

enum crimeType
{
    CRIME_NONE = 0,
    CRIME_SPEEDER = 1,
    CRIME_WRONGSIDE = 2,
    CRIME_BUMPCOP = 3,
    CRIME_SMASHCOP = 4
};

struct AICop::BasicPerpInfo
{
    sint32 copsAssigned_[2];
    crimeType crime_;
};

class AIHigh_Base
{
public:
    static void *operator new(size_t size) throw();
    static void operator delete(void *memory) throw();
    AIHigh_Base(Car::tObj *carObj);
    virtual void HighExecute(void) = 0;
    virtual ~AIHigh_Base(void);

    Car::tObj *carObj_;
    AIState_Base *state_;
    stateType_t stateType_;
    sint32 schedulingOff_;
    sint32 lastTrafficTriggerCheckSlice_;
};

class AIHigh_None : public AIHigh_Base
{
public:
    AIHigh_None(Car::tObj *carObj);
    virtual void HighExecute(void);

};

class AIHigh_BasicPerp : public AIHigh_Base
{
public:

    static sint32 PlayerCaughtSpeed[3];
    static sint32 PlayerCaughtDeltaY[3];
    static sint32 MinDeltaSpeed[3];
    static sint32 CopCaughtSpeed[3];
    static sint32 CaughtDistance[3];

    AIHigh_BasicPerp(Car::tObj *carObj);

    virtual ~AIHigh_BasicPerp(void);

    virtual void CheckForCrimes(void);
    sint32 CheckIfCaught(void);
    void RemoveCloseCops(void);
    void RemoveChaser(sint32 copIndex, sint32 carIndex, copType type);
    sint32 AddChaser(sint32 copIndex, sint32 carIndex, copType type);
    sint32 CheckChaserPosition(sint32 copIndex, sint32 carIndex);
    void Clear(void);

    enum
    {
        PULLOVER_NONE = 0,
        PULLOVER_WARNING = 1,
        PULLOVER_BUST = 2,
        PULLOVER_EOG = 3
    } pullOverMode_;

    struct
    {
        sint32 copIndex;
        sint32 carIndex;
    } positionVSCopList_[6];

    sint32 copVSPositionList_[6];
    sint32 beatingTicksLeft_;
    sint32 lastPullOverTime_;
    Car::tObj *lastArrestingCop_;
    AICop::BasicPerpInfo basicPerpInfo_;
};

struct AICop::PerpChaseInfo
{
    sint32 engagementTime_;
    copGame_t *copGameInfo_;
    sint32 chaseLevelIndex_;
    sint32 bestChaseLevelIndex_;
    copLevel_t *chaseLevel_;
    sint32 totalEngagementPercent_;
    sint32 blockadeDone_;
    sint32 engagementPercentIncreasePerTick_;
    sint32 copFreeTicks_;
};

class AIHigh_Player : public AIHigh_BasicPerp
{
public:

    static sint32 kNumArrestsByLap[3];

    AIHigh_Player(Car::tObj *carObj);

    virtual ~AIHigh_Player(void)
    {
        FUNCTION_MARKER(0x80063248, "NFS4.EXE");
    }

    sint32 CheckIfABlockadeCanBeSetup(void);
    void SetupBlockade(void);
    void CheckForNewLevel(sint32 force);
    void HandleSpeech(void);
    void MaintainAvailableCops(void);
    void HandleCops(void);
    void CleanupBlockaders(sint32 forceClearAll);
    void HandlePullOver(void);
    sint32 numWarnings_;
    sint32 numBusts_;
    sint32 newTriggerProb_;
    sint32 lastTriggerCheckSlice_;
    AICop::PerpChaseInfo perpChaseInfo_;
};

class AIHigh_Human : public AIHigh_Player
{
public:
    AIHigh_Human(Car::tObj *carObj);

    virtual ~AIHigh_Human(void)
    {
        FUNCTION_MARKER(0x800632E0, "NFS4.EXE");
    }

    virtual void HighExecute(void);
};

class AIHigh_Opponent : public AIHigh_Player
{
public:
    AIHigh_Opponent(Car::tObj *carObj);

    virtual ~AIHigh_Opponent(void)
    {
        FUNCTION_MARKER(0x80063BD8, "NFS4.EXE");
    }

    void CheckForWipeOut(void);
    sint32 DoRearEnder(void);
    virtual void HighExecute(void);
    sint32 DoProvokedAttack(void);
    AIHigh::tAttackMode attackMode_;
    Car::tObj *lastHumanHitter_;
    sint32 hitCount_;
    sint32 attackTicksLeft_;
};

class AIHigh_BTC_HumanCop;

class AIHigh_BTC_Perp : public AIHigh_BasicPerp
{
public:

    AIHigh_BTC_Perp(Car::tObj *carObj);

    virtual ~AIHigh_BTC_Perp(void);

    void ReleaseCops(void);
    void HandleCops(void);
    sint32 IsFalseArrest(void);
    sint32 CheckForControlsPressed(void);
    void HandlePullOver(void);
    void NotifyCopsOfArrest(void);
    void NotifyCopsOfArrestComplete(void);
    void NotifyCopsOfFalseArrest(void);
    void NotifyHumanCopsOfArrestHud(void);
    void ClearForNewStage(AIHigh_BTC_HumanCop *chaserCop);
    AIHigh_BTC_HumanCop *CheckForActivation(void);
    sint32 caught_;
    sint32 hudActivated_;
    AIHigh_BTC_HumanCop *originalActivationCop_;
};

class AIHigh_BTC_HumanPerp : public AIHigh_BTC_Perp
{
public:
    AIHigh_BTC_HumanPerp(Car::tObj *carObj);
    virtual ~AIHigh_BTC_HumanPerp(void)
    {
        FUNCTION_MARKER(0x80061320, "NFS4.EXE");
    }

    void NewStage(AIHigh_BTC_HumanCop *chaserCop);
    virtual void HighExecute(void);
};

class AIHigh_BTC_AIPerp : public AIHigh_BTC_Perp
{
public:
    AIHigh_BTC_AIPerp(Car::tObj *carObj);
    virtual ~AIHigh_BTC_AIPerp(void);
    void AvoidCops(void);
    void CalculateTimeTillContact(void);
    void FindClosestCop(void);
    virtual void HighExecute(void);
    void NewStage(AIHigh_BTC_HumanCop *chaserCop);

    enum
    {
        PERPMODE_CRUISE = 0,
        PERPMODE_FAST = 1,
        PERPMODE_MADECONTACT = 2,
        PERPMODE_TEASE = 3,
        PERPMODE_ESCAPE = 4,
        PERPMODE_CHASEON = 5
    } perpMode_;

    sint32 creationTime_;
    sint32 madeContactTime_;
    sint32 timeUntilContact_;
    sint32 escapeDuration_;
    sint32 originalMass_;
    sint32 originalMassInv_;
    Car::tObj *closestCopCarObj_;
    sint32 closestCopCarDistanceMeters_;
};

enum blockadeMode_t
{
    BLOCKADEMODE_NONE = 0,
    BLOCKADEMODE_SETUP = 1,
    BLOCKADEMODE_WAITING = 2,
    BLOCKADEMODE_CHASE = 3,
    BLOCKADEMODE_SETUP_FOR_OTHER = 4
};

struct blockade_t
{
    blockadeMode_t mode;
    AIHigh_BasicPerp *target;
    sint32 flags;
    sint32 chaseLevel;
    sint32 requestSpikeBeltAtSlice;
    sint32 slice;
    sint32 direction;
    sint32 latPos;
    sint32 rotation;
    sint32 reverse;
    sint32 releaseTime;
    sint32 initialPlayerDistanceMetersInt;
    sint16 blockadeSpeechFlags;
};

class AIHigh_BasicCop : public AIHigh_Base
{
public:
    AIHigh_BasicCop(Car::tObj *carObj, sint32 copIndex);

    void CheckSpikeBelt(void);
    sint32 ShouldIPerformCutOffBlock(sint32 chancePerSecond, Car::tObj *target);
    void SetupBlockadeElements(blockade_t *blockade);
    void HandleBlockadeSpeech(void);
    copType type_;
    sint32 copIndex_;
    blockade_t blockade_;
    AIHigh::tDriveAwayMode driveAway_;
};

class AIHigh_Cop : public AIHigh_BasicCop
{
public:

    struct AggressionEntry
    {
        sint32 ticksInChaseRegionForMurder;
        sint32 minLatMetersDistanceForMurder;
        sint32 minLongMetersDistanceForMurder;
        sint32 murderTicks;
        sint32 nitrousTicks;
    };

    static AggressionEntry AggressionData[3];

    AIHigh_Cop(Car::tObj *carObj, sint32 copIndex);

    void SetTuningLevers(void);
    virtual void HighExecute(void);
    sint32 CheckForNeedyPlayers(void);
    void CheckForWipeOut(void);
    sint32 CheckForNewTarget(void);
    void AssignToPlayer(AIHigh_Player *target);
    sint32 GetCheckChasePosition(coorddef *pos);
    trigger::trigger_t *CheckForNewTriggers(void);
    AIHigh_Player *perpTarget_;
    sint32 forcePurgatory_;
    sint32 chaseIndex_;
    sint32 requestSpikeBeltAtSlice_;
    sint32 aggressionLevel_;
};

class AIHigh_BTC_Cop : public AIHigh_BasicCop
{
public:
    AIHigh_BTC_Cop(Car::tObj *carObj, sint32 copIndex);

    void AssignToPlayer(AIHigh_BTC_Perp *target);
    sint32 GetCheckChasePosition(coorddef *pos);
    sint32 CheckForNewTarget(void);
    void StartArrest(AIHigh_BTC_Perp *arrestMe);
    void FinishArrest(AIHigh_BTC_Perp *arrestMe);
    void FalseArrest(AIHigh_BTC_Perp *cantArrestMe);
    virtual void FreezeAndEndChase(void);
    void HudOff(void);
    AIHigh_BTC_Perp *perpTarget_;
    sint32 chaseIndex_;

    enum
    {
        FREEZE_NONE = 0,
        FREEZE_NEWPERP_ROVE = 1,
        FREEZE_NEWPERP_DONE = 2,
        FREEZE_ARREST = 3,
        FREEZE_ARRESTDONE = 4
    } freezeMode_;
};

class AIHigh_BTC_HumanCop : public AIHigh_BTC_Cop
{
public:

    static sint32 lastInputRequestTick_;
    AIHigh_BTC_HumanCop(Car::tObj *carObj, sint32 copIndex);

    sint32 FindRandomBarrierFreeArea(sint32 startSlice, sint32 safetyZone, sint32 randomDistance);
    void ReleaseAndStartChase(AIHigh_BTC_Perp *newPerp);
    virtual void FreezeAndEndChase(void);
    void CheckConditionWithCop0(void);
    void NewStage(sint32 copSlice, sint32 direction, sint32 movement);
    void UpdateAndCheckTimeLeft(void);
    void UpdateFreezeModeAndPullOverMode(void);
    void RequestWingman(void);
    void RequestBlockader(sint32 spikeBeltRequest);
    sint32 CheckForWingmanRequest(void);
    sint32 CheckForBlockaderRequest(sint32 *spikeBeltRequest);
    void UpdateWingmanRole(Wingman_Role currentRole);
    void ClearTrafficToPurgatory(void);
    void ResetClearTrafficToPurgatory(void);
    void SetDesiredSpeed(void);
    virtual void HighExecute(void);
    void HudOn(AIHigh_BTC_Perp *arrestMe, sint32 gameOver, Car::tObj *arrestingCop);
    sint32 currentStage_;
    sint32 stageRepeatCount_;
    sint32 stageTimeMultiplier_;
    sint32 timeLeft_;
    sint32 chaseStartTime_;

    enum
    {
        WINGMAN_NONE = 0,
        WINGMAN_WINGMAN_REQUEST = 1,
        WINGMAN_BLOCKADER_REQUEST = 2,
        WINGMAN_BLOCKADER_SPIKEBELT_REQUEST = 3,
        WINGMAN_WINGMAN_ACTIVE = 4,
        WINGMAN_BLOCKADER_ACTIVE = 5
    } wingmanStatus_;

    sint32 needPerp_;
    sint32 initialDirection_;
    sint32 initialMovement_;
    sint32 requestedDesiredSpeed_;
};

class AIHigh_BTC_Wingman : public AIHigh_BTC_Cop
{
public:
    AIHigh_BTC_Wingman(Car::tObj *carObj, sint32 copIndex);

    virtual void HighExecute(void);
    sint32 CheckForActivation(void);
    sint32 UpdateFreezeModeAndPullOverMode(void);
    void SetupWingman(AIHigh_BTC_HumanCop *humanCop);
    void SetupBlockader(AIHigh_BTC_HumanCop *humanCop, sint32 spikeBeltRequest);
    Wingman_Role currentRole_;
    Wingman_Role newRole_;
    AIHigh_BTC_HumanCop *newHumanBoss_;
    sint32 spikeBeltPlaced_;
    sint32 spikeBeltSlice_;
    sint32 spikeBeltInterceptReleaseTime_;
};

class AIHigh_Traffic : public AIHigh_Base
{
public:
    AIHigh_Traffic(Car::tObj *carObj);

    Car::tObj *CheckForCops(sint32 *closestDistance);
    AIHigh_Cop *CopCheck(sint32 *blockade);
    virtual void HighExecute(void);
    trigger::trigger_t *CheckForNewTriggers(void);
    sint32 ignoreCops_;
    sint32 forcePurgatory_;
    SceneElem *accidentData_;
};

extern AIHigh_Base *highLevelAIObjs[9];

#endif
