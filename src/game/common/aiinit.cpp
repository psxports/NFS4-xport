#include <new>
#include <stdio.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/ai.h"
#include "game/common/aidatarecord.h"
#include "game/common/aiinit.h"
#include "game/common/aiphysic.h"
#include "game/common/aispeeds.h"
#include "game/common/aitriger.h"
#include "game/common/aitune.h"
#include "game/common/aiworld.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/paths.h"
#include "game/common/udff.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

void *AITraffic_rawTriggers;
char trafcfg[108] = { 4, 0, 0, 0, 12, 0, 0, 0, 30, 0, 0, 0, 51, 179, 0, 0, 0, 0, 11, 0, 0, 0, 8, 0, 0, 0, 32, 0, 0, 0, 8, 0, 0, 0, 24, 0, 0, 64, 1, 0, 204, 204, 0, 0, 0, 0, 5, 0, 204, 204, 0, 0, 0, 0, 20, 0, 71, 33, 0, 0, 83, 3, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 8, 0, 0, 0, 24, 0, 0, 160, 0, 0, 204, 204, 0, 0, 0, 0, 10, 0, 204, 204, 0, 0, 0, 0, 10, 0, 235, 81, 0, 0, 83, 3, 0, 0 };
sint32 AIInit::useSpreadForce;
sint32 AIInit::forceHumanHandBrake;

void AIInit::StartUp1(void)
{
    FUNCTION_MARKER(0x80066BB0, "NFS4.EXE");
    AI::TrafficStartUp();
    inverseLaneWidthTable[0] = 0;
    for (sint32 i = 1; i < 80; ++i)
        inverseLaneWidthTable[i] = fixeddiv(65536, i * 16384);
    AITune::StartUp1();
    AIDataRecord_t::StartUp1();
}

void AIInit::StartUp2(void)
{
    FUNCTION_MARKER(0x80066C24, "NFS4.EXE");
    AISpeeds::StartUp();
    LoadConfigs();
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop)
        AIScript::Startup(&Cars::gList[carLoop]->script);
    AIPerson::Startup();
    AIDataRecord_t::StartUp2();
    AIPhysic::StartUp();
    AITune::StartUp2();
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop)
    {
        Car::tObj *carObj = Cars::gList[carLoop];
        AIPhysic::InitCar(carObj);
        InitAICar2(carObj);
    }
}

void AIInit::Reset1(void)
{
    FUNCTION_MARKER(0x80066CFC, "NFS4.EXE");
    forceHumanHandBrake = 0;
}

void AIInit::Reset2(void)
{
    FUNCTION_MARKER(0x80066D08, "NFS4.EXE");
    useSpreadForce = GameSetup::gData.raceType != 3;
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop)
        AIScript::Startup(&Cars::gList[carLoop]->script);
    Car::tObj *human = Cars::gHumanRaceCarList[0];
    Car::tObj *ai = Cars::gAIRaceCarList[0];
    AISpeeds::leaderBoard.leadHumanRacer = human;
    AISpeeds::leaderBoard.leadRacer = human;
    AISpeeds::leaderBoard.lastAIRacer = ai;
    AISpeeds::leaderBoard.leadAIRacer = ai;
    AIPhysic::Reset();
    AI::Info.blockingCars[2] = 0;
    AI::Info.blockingCars[1] = 0;
    AI::Info.blockingCars[0] = 0;
    AI::Info.blockingCarsDist[2] = 0;
    AI::Info.blockingCarsDist[1] = 0;
    AI::Info.blockingCarsDist[0] = 0;
    AI::Info.laneSpeeds[2] = 0;
    AI::Info.laneSpeeds[1] = 0;
    AI::Info.laneSpeeds[0] = 0;
    AI::Info.laneSpeedsAhead[2] = 0;
    AI::Info.laneSpeedsAhead[1] = 0;
    AI::Info.laneSpeedsAhead[0] = 0;
    AI::Info.laneWeights[2] = 0;
    AI::Info.laneWeights[1] = 0;
    AI::Info.laneWeights[0] = 0;
    AI::Info.desiredLane = 0;
    AI::Info.deltaYaw = 0;
}

void AIInit::CleanUp1(void)
{
    FUNCTION_MARKER(0x80066E0C, "NFS4.EXE");
    AIDataRecord_t::CleanUp1();
    AITune::CleanUp1();
    AI::TrafficCleanUp();
}

void AIInit::CleanUp2(void)
{
    FUNCTION_MARKER(0x80066E44, "NFS4.EXE");
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop)
    {
        DeInitAICar2(Cars::gList[carLoop]);
        AIPhysic::DeInitCar(Cars::gList[carLoop]);
    }
    AITune::CleanUp2();
    AIPhysic::CleanUp();
    AIDataRecord_t::CleanUp2();
    AIPerson::Cleanup();
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop)
        AIScript::Cleanup();
    AISpeeds::CleanUp();
}

void AI::TrafficStartUp(void)
{
    FUNCTION_MARKER(0x80066F0C, "NFS4.EXE");
    char filename[100];
    if (GameSetup::gData.trafficDensity != 0)
    {
        void *memory = reservememadr(0, sizeof(AITrigger_TriggerManager), 0);
        triggerManagerTraffic = new (memory) AITrigger_TriggerManager;
        sprintf(filename, "%sTr%02d.trf", Paths_Paths[18], GameSetup::gData.track);
        AITraffic_rawTriggers = loadfileadrz(filename, 0);
        triggerManagerTraffic->Init((char *)AITraffic_rawTriggers);
    }
}

void AI::TrafficCleanUp(void)
{
    FUNCTION_MARKER(0x80066FA8, "NFS4.EXE");
    if (triggerManagerTraffic != 0)
    {
        purgememadr(triggerManagerTraffic);
        triggerManagerTraffic = 0;
    }
    if (AITraffic_rawTriggers != 0)
    {
        purgememadr(AITraffic_rawTriggers);
        AITraffic_rawTriggers = 0;
    }
}

void AIInit::LoadConfigs(void)
{
    FUNCTION_MARKER(0x80066FF8, "NFS4.EXE");
    char pathname[100];
    sprintf(pathname, "%strafcfg.dat", Paths_Paths[2]);
    Udff::tInfo *handle = Udff::Opena(0, trafcfg, 1);
    LoadPhysicsConfig(handle);
    Udff::Close(handle);
}

void AIInit::LoadPhysicsConfig(Udff::tInfo *handle)
{
    FUNCTION_MARKER(0x80067054, "NFS4.EXE");
    AIPhysicConfig.latvelcalc_lookahead = Udff::GetInt(handle);
    AIPhysicConfig.min_lookahead = Udff::GetInt(handle);
    AIPhysicConfig.max_lookahead = Udff::GetInt(handle);
    AIPhysicConfig.look_ahead_factor = Udff::GetInt(handle);
    AIPhysicConfig.skid_value = Udff::GetInt(handle);
    for (sint32 loop = 0; loop < 2; ++loop)
    {
        AIPhysic::ModelConfig_t *model = loop == 0 ? &AIPhysicConfig.ICModel : &AIPhysicConfig.OOCModel;
        model->dlpos_to_dlvel = Udff::GetInt(handle);
        model->max_dlvel = Udff::GetInt(handle);
        model->dlvel_to_clacc = Udff::GetInt(handle);
        model->max_clacc = Udff::GetInt(handle);
        model->dangle_to_dav = Udff::GetInt(handle);
        model->max_dav = Udff::GetInt(handle);
        model->dav_to_aa = Udff::GetInt(handle);
        model->max_aa = Udff::GetInt(handle);
        model->vel_limit_range = Udff::GetInt(handle);
        model->lat_vel_limit_factor = Udff::GetInt(handle);
        model->ang_vel_limit_factor = Udff::GetInt(handle);
    }
}

void AIInit::ClearAICar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80067174, "NFS4.EXE");
    coorddef zero = { 0, 0, 0 };
    carObj->N.angularVel = zero;
    carObj->N.flightTime = 0;
    carObj->frontSkid = 0;
    carObj->rearSkid = 0;
    carObj->tailgateTimer = 0;
    carObj->aiShiftTimer = 0;
    carObj->driveDirection = 1;
    carObj->driveDirectionTimer = 0;
    carObj->driveDirectionReverseTime = 0;
    carObj->wipeOutEndTick = 0;
    carObj->wipeOutStartTick = 0;
    carObj->AIFishtailEndTick = 0;
}

void AIInit::RestartAICar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800671EC, "NFS4.EXE");
    sint32 direction;

    direction = -1;
    if (GameSetup::gData.reverseTrack == 0) {
      direction = 1;
    }
    carObj->direction = direction;
    carObj->desiredDirection = direction;
    carObj->targetLatPos = 0;
    carObj->targetPos.z = 0;
    carObj->targetPos.y = 0;
    carObj->targetPos.x = 0;
    carObj->preferredLateralPosition = 0;
    carObj->preferredLateralPositionPower = 0;
    carObj->carInLane = 0;
    carObj->driveDirection = 1;
    carObj->driveDirectionTimer = 0;
    carObj->driveDirectionReverseTime = 0;
    carObj->barrierThinkHarder = 0;
    carObj->desiredLatPos = 0;
    carObj->desiredSpeed = 0;
    carObj->originalDesiredSpeed = 0;
    carObj->currentSpeed = 0;
    carObj->speed = 0;
    carObj->laneSlack = 0;
    carObj->roadPosition = 0;
    carObj->AIFlags = 0;
    carObj->roadSpan = 0;
    carObj->basisCar = carObj;
    carObj->aiShiftTimer = 0;
    carObj->tailgateTimer = 0;
    carObj->physicsModelTimer = 0;
    carObj->trafficSpeedRandomizingFactor = 0;
    carObj->laneChangeSpeed = 0;
    carObj->timeOffRoad = 0;
    carObj->rampDesiredLatPos = 0;
    carObj->accNitrous = 0x10000;
    carObj->speedNitrous = 0x10000;
    carObj->speechInfo.speechMode = SPEECH_MODE_NONE;
    carObj->wipeOutEndTick = 0;
    carObj->btcGlueModifier = 0x10000;
    carObj->copAccMult = 0x10000;
    carObj->copTopSpeed = 0x640000;
    carObj->donutMode = DONUTMODE_NONE;
    carObj->laneIndex = 7;
    carObj->AIFishtailEndTick = 0;
    carObj->lookAheadSlice = 0;
    if ((carObj->carFlags & 2U) == 0) {
      carObj->forceNoSimOptz = 1;
    }
    else {
      carObj->forceNoSimOptz = 0;
    }
    carObj->wipeOutStartTick = 0;
    if ((carObj->carFlags & 10U) == 2) {
      carObj->N.active = 0;
    }
    carObj->caravanTimer = -1;
    carObj->fallBehindCar = 0;
    carObj->nextAIRacer = 0;
    carObj->caravanFollowBehindDistanceMeters = 0;
    carObj->AISlot = 0;
    carObj->gripFactor = 0x10000;
    carObj->damageMult = 0x10000;
    carObj->extraWallCollisionAllowance = 0;
}

void AIInit::InitAICar(Car::tObj *carObj, Udff::tInfo *handle)
{
    FUNCTION_MARKER(0x80067314, "NFS4.EXE");
    sint32 accelerationScale = 65536;
    carObj->redLine = Udff::GetInt(handle);
    Udff::GetBuffer(handle, (char *)carObj->topSpeeds, sizeof(carObj->topSpeeds));
    for (sint32 gearLoop = 0; gearLoop < 7; ++gearLoop)
    {
        if (gearLoop == 0)
            carObj->invTopSpeeds[0] = fixeddiv(65536, carObj->topSpeeds[0]);
        else if (carObj->topSpeeds[gearLoop] == 0)
            carObj->invTopSpeeds[gearLoop] = 0;
        else
            carObj->invTopSpeeds[gearLoop] = fixeddiv(65536,
                (sint32)((uint32)carObj->topSpeeds[gearLoop] - (uint32)carObj->topSpeeds[gearLoop - 1]));
    }
    Udff::GetBuffer(handle, (char *)carObj->accTable, sizeof(carObj->accTable));
    carObj->aiShiftDuration = Udff::GetInt(handle);
    carObj->max_clacc = Udff::GetInt(handle);
    carObj->max_aa = Udff::GetInt(handle);
    if ((carObj->carFlags & 8) != 0)
    {
        sint32 carType = carObj->carInfo->carType;
        accelerationScale = AITune::accelerationScale[carType].scale;
    }
    carObj->accelerationRecord = new AIDataRecord_AccTable_t((char *)carObj->accTable,
        accelerationScale, AIDataRecord::ACCELERATION_R);
    if (IsNonStandardCarFile(carObj->carInfo->carType) != 0)
        carObj->curveSpeedTable = new AIDataRecord_CurveSpeedTable_t(carObj->carName, AIDataRecord::CURVESPEED_TABLE_R);
    else
        carObj->curveSpeedTable = 0;
    carObj->max_clacc = fixedmult(carObj->max_clacc, 0x13333);
    carObj->speedFactor = Udff::GetInt(handle);
    carObj->slackProb = Udff::GetInt(handle);
    carObj->gripFactor = 65536;
    carObj->topSpeedUpgradeMult = 0;
    carObj->accUpgradeMult = 0;
    carObj->damageMult = 65536;
}

void AIInit::DeInitAICar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800674E8, "NFS4.EXE");
    if (carObj->curveSpeedTable != 0)
    {
        delete carObj->curveSpeedTable;
        carObj->curveSpeedTable = 0;
    }
    if (carObj->accelerationRecord != 0)
    {
        delete carObj->accelerationRecord;
        carObj->accelerationRecord = 0;
    }
}

void AIInit::InitAICar2(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80067568, "NFS4.EXE");
    if ((carObj->carFlags & 8) != 0)
    {
        sint32 handlingMult = AISpeeds::GetUpgradeHandlingMult(carObj->carIndex);
        carObj->curveSpeedTable->Upgrade(handlingMult);
    }
    carObj->accUpgradeMult = AISpeeds::GetUpgradeAccMult(carObj->carIndex);
    carObj->topSpeedUpgradeMult = AISpeeds::GetUpgradeTopSpeedMult(carObj->carIndex);
}

void AIInit::DeInitAICar2(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800675D0, "NFS4.EXE");
    (void)carObj;
}

sint32 AIInit::IsNonStandardCarFile(sint32 index)
{
    FUNCTION_MARKER(0x800675D8, "NFS4.EXE");
    sint32 nonStandardList[50] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    return index < 50 ? nonStandardList[index] : 0;
}
