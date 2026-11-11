#ifndef AI_H
#define AI_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/group.h"

class AI
{
public:
    struct tInfo
    {
        Car::tObj *blockingCars[3];
        sint32 blockingCarsDist[3];
        sint32 laneSpeeds[3];
        sint32 laneSpeedsAhead[3];
        sint32 laneWeights[3];
        sint32 desiredLane;
        sint32 desiredLaneSide;
        sint32 deltaYaw;
    };

    static sint32 time;
    static sint32 elapsedTime;
    static sint32 iTime;
    static tInfo Info;

    static void CarAvoidance(Car::tObj *carObj);
    static void WorldAvoidance(Car::tObj *carObj);
    static void InitAIInfo(Car::tObj *carObj);
    static void GenericBeginCycle(Car::tObj *carObj);
    static void GenericCycle(Car::tObj *carObj);
    static void GenericEndCycle(Car::tObj *carObj);
    static void TargetLane(Car::tObj *carObj, sint32 lane);
    static void ClearLaneMerits(void);
    static void DoReactions(Car::tObj *carObj);
    static void CheckForPlayerActions(Car::tObj *carObj, Car::tObj *otherCarObj);
    static void DoReactionsAndBehavior(Car::tObj *carObj);
    static void OpponentBlockPlayer(Car::tObj *carObj, Car::tObj *otherCarObj);
    static void CheckForBarriers(Car::tObj *carObj);
    static void SetupOncomingLaneDemerits(Car::tObj *carObj);
    static void HandleChangeInNumLanes(Car::tObj *carObj);
    static void HandleShouldersAndOffRoad(Car::tObj *carObj);
    static void CalculateLaneSpeeds(Car::tObj *carObj);
    static void CalcMeritsBasedOnSpeed(Car::tObj *carObj);
    static void CheckForClearLanes(Car::tObj *carObj);
    static void CalcBestLineMerits(Car::tObj *carObj);
    static void AddCollidableObjects(Car::tObj *carObj, Group *groupSimObjs);
    static void AvoidObjects(Car::tObj *carObj);
    static void AvoidSpikeBelt(Car::tObj *carObj);
    static void SubmitObstacle(Car::tObj *carObj, sint32 importance, sint32 leftLatPosition, sint32 rightLatPosition, sint32 slice);
    static void HandleTrafficHonking(Car::tObj *carObj);
    static void CheckForCarsOnSide(Car::tObj *carObj);
    static void ProcessObservationsAndChooseLane(Car::tObj *carObj);
    static void ChooseBestLane(Car::tObj *carObj);
    static sint32 CheckPreferredLateralPosition(Car::tObj *carObj);
    static sint32 TryToShareLanes(Car::tObj *carObj, Car::tObj *carInWay);
    static void CalculateDesiredLatPosition(Car::tObj *carObj);
    static void CalculateAdjustedDesiredSpeed(Car::tObj *carObj);
    static sint32 IsMellowZone(Car::tObj *carObj, sint32 delay);
    static void KeepCarsInLane(Car::tObj *carObj);
    static void PushFinishedCarsToSide(Car::tObj *carObj);
    static void MaybeChangeLaneSlack(Car::tObj *carObj);
    static void ChooseNewLaneSlack(Car::tObj *carObj);
    static void TrafficStartUp(void);
    static void TrafficCleanUp(void);
};

#endif
