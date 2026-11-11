#include <stdio.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/ai.h"
#include "game/common/aidatarecord.h"
#include "game/common/aispeeds.h"
#include "game/common/aiworld.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/fastrand.h"
#include "game/common/paths.h"
#include "game/common/sim.h"
#include "game/common/udff.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

speedData_t Track2Speeds[9] = { { 50, 3982 }, { 196, 7395 }, { 293, 7395 }, { 332, 3982 }, { 390, 7395 }, { 624, 7395 }, { 665, 3982 }, { 1026, 7395 }, { 10000, 3982 } };
speedData_t Track3Speeds[3] = { { 7, 5688 }, { 236, 3555 }, { 10000, 5688 } };
speedData_t Track5Speeds[3] = { { 369, 7964 }, { 674, 5688 }, { 10000, 7964 } };
speedData_t Track6Speeds[5] = { { 26, 3555 }, { 327, 7111 }, { 393, 3555 }, { 627, 7111 }, { 10000, 3555 } };
speedData_t Track7Speeds[6] = { { 29, 5688 }, { 429, 7964 }, { 617, 7964 }, { 655, 5688 }, { 823, 7964 }, { 10000, 5688 } };
speedData_t Track0Speeds[1] = { { 10000, 7395 } };
speedData_t Track1Speeds[1] = { { 10000, 7395 } };
speedData_t Track4Speeds[1] = { { 10000, 5688 } };
speedData_t Track8Speeds[1] = { { 10000, 11377 } };
speedData_t Track9Speeds[1] = { { 10000, 11377 } };
speedData_t Track10Speeds[1] = { { 10000, 11377 } };
AISpeeds::tLeaderBoard AISpeeds::leaderBoard;
AISpeeds::Upgrade_t AISpeeds::engineUpgrade;
AISpeeds::Upgrade_t AISpeeds::suspensionUpgrade;
AISpeeds::Upgrade_t AISpeeds::aeroUpgrade;
speedData_t *AISpeeds::TrackSpeeds[11] = { Track0Speeds, Track1Speeds, Track2Speeds, Track3Speeds, Track4Speeds, Track5Speeds, Track6Speeds, Track7Speeds, Track8Speeds, Track9Speeds, Track10Speeds };
sint32 AISpeeds::trackAndNightMult;
sint32 *AISpeeds::WeatherMultFactors;
AISpeeds::tSlotInfo AISpeeds::CaravanInfo[6];

void AISpeeds::StartUp(void)
{
    FUNCTION_MARKER(0x8006D5B4, "NFS4.EXE");
    WeatherMultFactors = (sint32 *)reservememadr("aiwther", 65 * sizeof(sint32), 0);
    ReadTuningInfo();
}

void AISpeeds::ReadTuningInfo(void)
{
    FUNCTION_MARKER(0x8006D5EC, "NFS4.EXE");
    char filename[110];
    sprintf(filename, "%stuning.bin", Paths_Paths[2]);
    Udff::tInfo *handle = Udff::Opena(filename, 0, 1);
    Udff::GetInt(handle);
    for (sint32 slotLoop = 0; slotLoop < 6; ++slotLoop)
    {
        sint32 value = Udff::GetInt(handle);
        CaravanInfo[slotLoop].distanceMaintainTime32 = (sint32)((uint32)value << 5);
        CaravanInfo[slotLoop].minDistanceMeters = Udff::GetInt(handle);
        CaravanInfo[slotLoop].maxDistanceMeters = Udff::GetInt(handle);
        value = Udff::GetInt(handle);
        CaravanInfo[slotLoop].fallBackRandomTime_TickPercent = fixeddiv(65536, (sint32)((uint32)value << 21));
    }
    if (GameSetup::gData.raceType == 0 && Cars::gNumAIRaceCars > 0)
    {
        sint32 carType = Cars::gAIRaceCarList[0]->carInfo->carType;
        for (sint32 carModelLoop = 0; carModelLoop < 22; ++carModelLoop)
        {
            sint32 skillMult[3];
            skillMult[0] = Udff::GetInt(handle);
            skillMult[1] = Udff::GetInt(handle);
            skillMult[2] = Udff::GetInt(handle);
            if (carModelLoop == carType)
                GameSetup::gData.tournamentMultiplier = skillMult[GameSetup::gData.skill];
        }
    }
    else
    {
        for (sint32 carModelLoop = 0; carModelLoop < 22; ++carModelLoop)
        {
            Udff::GetInt(handle);
            Udff::GetInt(handle);
            Udff::GetInt(handle);
        }
    }
    trackAndNightMult = 0;
    for (sint32 trackLoop = 0; trackLoop < 12; ++trackLoop)
    {
        Udff::GetInt(handle);
        sint32 trackMult = Udff::GetInt(handle);
        sint32 nightMult = Udff::GetInt(handle);
        if (GameSetup::gData.Time == 0)
            nightMult = 65536;
        if (trackLoop == GameSetup::gData.track)
            trackAndNightMult = fixedmult(trackMult, nightMult);
    }
    sint32 weatherRamp = Udff::GetInt(handle);
    sint32 ramp = 0;
    sint32 *weather = WeatherMultFactors;
    for (sint32 curveLoop = 0; curveLoop < 65; ++curveLoop)
    {
        weather[curveLoop] = (sint32)(65536U - (uint32)(ramp / 64));
        ramp = (sint32)((uint32)ramp + (uint32)weatherRamp);
    }
    Udff::GetBuffer(handle, (char *)&engineUpgrade, sizeof(engineUpgrade));
    Udff::GetBuffer(handle, (char *)&suspensionUpgrade, sizeof(suspensionUpgrade));
    Udff::GetBuffer(handle, (char *)&aeroUpgrade, sizeof(aeroUpgrade));
    Udff::Close(handle);
}

void AISpeeds::CleanUp(void)
{
    FUNCTION_MARKER(0x8006D870, "NFS4.EXE");
    if (WeatherMultFactors != 0)
    {
        purgememadr(WeatherMultFactors);
        WeatherMultFactors = 0;
    }
}

sint32 AISpeeds::SuperDuperSpeedUpTheCarsAtTheStartBecauseWeCannotActuallyHandleRenderingTheseCars(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006D89C, "NFS4.EXE");
    sint32 numAI = Cars::gNumAIRaceCars;
    sint32 leadIndex = (sint32)((uint32)numAI - (uint32)carObj->AISlot);
    sint32 f_crappyFrameRateCompensatingSpeedup = 65536;
    if (numAI >= 2 && GameSetup::gData.raceType != 1 && simGlobal.gameTicks < 0x780 &&
        Cars::gNumHumanRaceCars == 1 && Cars::gHumanRaceCarList[0]->N.totalSlice < carObj->N.totalSlice)
        f_crappyFrameRateCompensatingSpeedup = (sint32)((uint32)leadIndex * 0x3333U + 65536U);
    return f_crappyFrameRateCompensatingSpeedup;
}

sint32 AISpeeds::CalcOpponentTopSpeed(Car::tObj *carObj, sint32 *unFetteredDesiredSpeed)
{
    FUNCTION_MARKER(0x8006D938, "NFS4.EXE");
    sint32 topSpeed = CalcOpponentCurveSpeed(carObj);
    sint32 f_glue;
    if (carObj->fallBehindCar != 0)
        f_glue = 65536;
    else if ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) &&
        ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0 ||
         (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0)))
        f_glue = BTCGetGlueFactor(carObj);
    else
        f_glue = GetGlueFactor(carObj);
    sint32 f_script = GetScriptFactor(carObj);
    sint32 f_nitrous = carObj->speedNitrous;
    sint32 f_damage = GetDamageFactor(carObj);
    sint32 f_caravan = 65536;
    if (GameSetup::gData.raceType != 1 && Cars::gNumAIRaceCars >= 2)
        f_caravan = GetCaravanFactor(carObj);
    sint32 f_unfettered = (sint32)((uint32)(GameSetup::gData.tournamentMultiplier / 256) * (uint32)(f_glue / 256));
    f_unfettered = (sint32)((uint32)(f_caravan / 256) * (uint32)(f_unfettered / 256));
    f_unfettered = (sint32)((uint32)(trackAndNightMult / 256) * (uint32)(f_unfettered / 256));
    f_unfettered = (sint32)((uint32)(f_nitrous / 256) * (uint32)(f_unfettered / 256));
    sint32 combinedDamage = (sint32)((uint32)(f_damage / 256) * (uint32)(f_script / 256));
    sint32 f_final = (sint32)((uint32)(f_unfettered / 256) * (uint32)(combinedDamage / 256));
    if (GameSetup::gData.raceType != 1 && carObj->N.totalSlice < 150)
    {
        sint32 product = (sint32)((65536U - (uint32)f_unfettered) * carObj->N.totalSlice);
        sint32 blended = fixedmult(product, 0x1b4);
        f_unfettered = (sint32)(65536U - (uint32)blended);
        if (carObj->N.totalSlice < 150)
        {
            product = (sint32)((65536U - (uint32)f_final) * carObj->N.totalSlice);
            blended = fixedmult(product, 0x1b4);
            f_final = (sint32)(65536U - (uint32)blended);
        }
    }
    sint32 f_crappyFrameRateCompensatingSpeedup = SuperDuperSpeedUpTheCarsAtTheStartBecauseWeCannotActuallyHandleRenderingTheseCars(carObj);
    if (f_crappyFrameRateCompensatingSpeedup != 65536)
    {
        sint32 boost = f_crappyFrameRateCompensatingSpeedup / 256;
        f_unfettered = (sint32)((uint32)(f_unfettered / 256) * (uint32)boost);
        f_final = (sint32)((uint32)(f_final / 256) * (uint32)boost);
    }
    carObj->aiGlue = LimitGlueMultiplier(carObj, f_final);
    f_unfettered = LimitGlueMultiplier(carObj, f_unfettered);
    carObj->N.gravityMult = carObj->aiGlue > 65536 ? carObj->aiGlue : 65536;
    sint32 topSpeed256 = topSpeed / 256;
    sint32 newDesired = (sint32)((uint32)topSpeed256 * (uint32)(carObj->aiGlue / 256));
    *unFetteredDesiredSpeed = (sint32)((uint32)topSpeed256 * (uint32)(f_unfettered / 256));
    if ((carObj->carFlags & 1) != 0 && carObj->stats.finishType == 2 &&
        ((GameSetup::gData.raceType != 1 && GameSetup::gData.raceType != 5) ||
         ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) == 0 &&
          (Cars::gNumHumanRaceCars != 2 || (Cars::gHumanRaceCarList[1]->carFlags & 0x200) == 0))))
    {
        sint32 metersPastFinish = AIWorld::ApxSplineDistance(carObj, 0);
        if (metersPastFinish < 0)
            metersPastFinish = (sint32)(0U - (uint32)metersPastFinish);
        sint32 latLeft = (sint32)((uint32)carObj->roadPosition - (uint32)carObj->N.dimension.x);
        sint32 latRight = (sint32)((uint32)carObj->roadPosition + (uint32)carObj->N.dimension.x);
        sint32 numCars = Cars::gNumCars;
        sint32 totalSortIndex = 0;
        while (totalSortIndex < numCars && Cars::gTotalSortedList[totalSortIndex] != carObj)
            ++totalSortIndex;
        if (metersPastFinish >= (sint32)((uint32)totalSortIndex * 0x280000U))
        {
            Trk::NewSlice *slice = &BWorldSm::slices[carObj->N.simRoadInfo.slice];
            sint32 leftEdge = -(slice->avgPavedWidthLf * 32768 * (slice->laneCount >> 4));
            sint32 rightEdge = slice->avgPavedWidthRt * 32768 * (slice->laneCount & 15);
            newDesired = 0;
            if (latLeft >= leftEdge && latLeft <= rightEdge && latRight >= leftEdge && latRight <= rightEdge &&
                metersPastFinish <= 0x1900000)
                newDesired = 0x11c71c;
        }
    }
    *unFetteredDesiredSpeed = (sint32)((uint32)*unFetteredDesiredSpeed * (uint32)carObj->direction);
    return (sint32)((uint32)newDesired * (uint32)carObj->direction);
}

sint32 AISpeeds::NeedToSlowDownForCurve(Car::tObj *carObj, sint32 distanceMeters, sint32 currentSpeed, sint32 futureCurveSpeed)
{
    FUNCTION_MARKER(0x8006DE90, "NFS4.EXE");
    if (currentSpeed < futureCurveSpeed)
        return 0;
    AIPhysic::BrakeInfo *info = carObj->brakeInfo;
    sint32 speed = currentSpeed / 65536;
    if (speed < 0)
        speed = -speed;
    sint32 sIndex = speed < 128 ? speed : 128;
    sint32 currentDistance = sIndex == 128 ? (sint32)((uint32)info->deceleration_ & 255U) : info->brakeTable_[sIndex];
    sint32 futureSpeed = futureCurveSpeed / 65536;
    if (futureSpeed < 0)
        futureSpeed = -futureSpeed;
    sIndex = futureSpeed < 128 ? futureSpeed : 128;
    sint32 futureDistance = sIndex == 128 ? (sint32)((uint32)info->deceleration_ & 255U) : info->brakeTable_[sIndex];
    sint32 neededDistance = (sint32)(((uint32)currentDistance - (uint32)futureDistance) << 17);
    neededDistance = (sint32)((uint32)neededDistance + (uint32)(neededDistance >> 3));
    return neededDistance >= distanceMeters;
}

sint32 AISpeeds::CalcOpponentCurveSpeed(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006DF34, "NFS4.EXE");
    sint32 speedHere = carObj->currentSpeed;
    if (speedHere < 0)
        speedHere = (sint32)(0U - (uint32)speedHere);
    for (sint32 scanMetersDistanceInt = 200; scanMetersDistanceInt >= 0; scanMetersDistanceInt -= 25)
    {
        sint32 offset = (sint32)((uint32)(scanMetersDistanceInt / 6) * (uint32)carObj->direction);
        sint32 scanSlice = (sint32)((uint32)carObj->N.simRoadInfo.slice + (uint32)offset);
        if (offset >= 0)
        {
            if (scanSlice >= gNumSlices)
                scanSlice = (sint32)((uint32)scanSlice - (uint32)gNumSlices);
        }
        else if (scanSlice < 0)
            scanSlice = (sint32)((uint32)scanSlice + (uint32)gNumSlices);
        sint32 curve = AIDataRecord::TrackCurve->Get(scanSlice);
        sint32 curveSpeed = carObj->curveSpeedTable->Get(curve);
        if (GameSetup::gData.Weather != 0)
            curveSpeed = fixedmult(curveSpeed, WeatherMultFactors[curve / 4]);
        if (scanMetersDistanceInt == 0)
            return curveSpeed;
        if (NeedToSlowDownForCurve(carObj, scanMetersDistanceInt * 65536, speedHere, curveSpeed) != 0)
            return curveSpeed;
    }
    return 0;
}

sint32 AISpeeds::BTCGetGlueFactor(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006E09C, "NFS4.EXE");
    sint32 closestHumanDistance = 0x270f0000;
    Car::tObj *closestHumanCarObj = 0;
    if ((carObj->carFlags & 0x20) != 0)
        return 65536;
    for (sint32 humanLoop = 0; humanLoop < Cars::gNumHumanRaceCars; ++humanLoop)
    {
        Car::tObj *copCar = Cars::gHumanRaceCarList[humanLoop];
        if ((copCar->carFlags & 0x200) != 0)
        {
            sint32 longMetersBetween = AIWorld::ApxSplineDistance(carObj, copCar);
            sint32 distanceAbs = longMetersBetween;
            if (distanceAbs < 0)
                distanceAbs = (sint32)(0U - (uint32)distanceAbs);
            sint32 closestAbs = closestHumanDistance;
            if (closestAbs < 0)
                closestAbs = (sint32)(0U - (uint32)closestAbs);
            if (distanceAbs < closestAbs)
            {
                closestHumanDistance = longMetersBetween;
                closestHumanCarObj = copCar;
            }
        }
    }
    closestHumanDistance = (sint32)((uint32)closestHumanDistance * (uint32)carObj->direction);
    if (closestHumanCarObj->RSControl != 0)
        return 65536;
    if ((uint32)closestHumanCarObj->currentSpeed + 0x9ffffU <= 0x13fffeU)
        return 65536;
    sint32 glueIndex = closestHumanDistance / 0x3c0000 + 10;
    if (glueIndex < 0)
        glueIndex = 0;
    if (glueIndex >= 21)
        glueIndex = 20;
    sint32 glue = AIPerson::glueTable[glueIndex];
    if (glue < 65536)
    {
        sint32 delta = fixedmult((sint32)(65536U - (uint32)glue), carObj->btcGlueModifier);
        glue = (sint32)(65536U - (uint32)delta);
    }
    if (glue < 0x6666)
        glue = 0x6666;
    return fixedmult(glue, carObj->speedFactor);
}

Car::tObj * AISpeeds::GetNextAICar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006E258, "NFS4.EXE");
    return carObj->nextAIRacer;
}

Car::tObj * AISpeeds::GetPrevAICar(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006E264, "NFS4.EXE");
    Car::tObj *prevCar = 0;
    const sint32 numCars = Cars::gNumCars;
    for (sint32 carLoop = 0; carLoop < numCars; ++carLoop)
    {
        Car::tObj *testCar = Cars::gTotalSortedList[carLoop];
        if (testCar == carObj)
            return prevCar;
        if ((testCar->carFlags & 8) != 0)
            prevCar = testCar;
    }
    return 0;
}

sint32 AISpeeds::GetCaravanFactor(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006E2D0, "NFS4.EXE");
    sint32 slot = carObj->AISlot;
    Car::tObj *nextAICar = GetNextAICar(carObj);
    sint32 f_caravan = 0x10000;
    if (carObj->fallBehindCar != 0)
    {
        if (carObj->fallBehindCar->AISlot < slot)
            carObj->fallBehindCar = 0;
        f_caravan = 0xE666;
    }
    else if (nextAICar != 0)
    {
        if (nextAICar->fallBehindCar == carObj)
            f_caravan = 0x11999;
        else if (nextAICar->fallBehindCar == 0 && nextAICar->speed > 0xD6491)
        {
            uint32 direction = (uint32)carObj->direction;
            sint32 originalSpeed = (sint32)((uint32)nextAICar->originalDesiredSpeed * direction);
            sint32 desiredSpeed = (sint32)((uint32)nextAICar->desiredSpeed * direction);
            if (desiredSpeed >= (originalSpeed / 256) * 0xB3)
            {
                sint32 leaderOdometer = AIWorld::GameOdometer(nextAICar);
                sint32 odometer = AIWorld::GameOdometer(carObj);
                sint32 leaderIsThisManyMetersAhead = (sint32)((uint32)leaderOdometer - (uint32)odometer);
                uint32 distance = (uint32)carObj->caravanFollowBehindDistanceMeters;
                if (leaderIsThisManyMetersAhead > (sint32)(distance + 0xA0000U))
                    f_caravan = leaderIsThisManyMetersAhead > (sint32)(distance + 0x3E80000U) ? 0x18000 : 0x13333;
                else if (leaderIsThisManyMetersAhead < (sint32)(distance - 0xA0000U))
                    f_caravan = leaderIsThisManyMetersAhead < (sint32)(distance - 0x3E80000U) ? 0x9999 : 0xCCCC;
            }
        }
    }

    const tSlotInfo &info = CaravanInfo[slot];
    if (info.distanceMaintainTime32 != 0)
        carObj->caravanTimer = (sint32)((uint32)carObj->caravanTimer - (uint32)AI::elapsedTime);
    if (carObj->caravanTimer < 0)
    {
        uint32 random = fastRandom * randSeed;
        sint32 halfTime = info.distanceMaintainTime32 / 2;
        carObj->caravanTimer = (sint32)((uint32)halfTime +
            (((uint32)halfTime * ((random >> 8) & 0xFFFFU)) >> 16));

        randtemp = random;
        fastRandom = randtemp & 0xFFFFU;
        randtemp = fastRandom * randSeed;
        fastRandom = randtemp & 0xFFFFU;

        uint32 minDistance = (uint32)info.minDistanceMeters;
        uint32 distanceRange = (uint32)info.maxDistanceMeters - minDistance;
        uint32 distanceOffset = (distanceRange * ((randtemp >> 8) & 0xFFFFU)) >> 16;
        carObj->caravanFollowBehindDistanceMeters = (sint32)((minDistance + distanceOffset) << 16);
    }

    if (slot < (sint32)((uint32)Cars::gNumAIRaceCars - 1U) && carObj->fallBehindCar == 0 &&
        carObj->N.totalSlice < (sint32)((uint32)GameSetup::gData.numLaps * (uint32)gNumSlices - 0x14DU) &&
        ((uint32)slot >= 2 || leaderBoard.leadRacer != Cars::gHumanRaceCarList[0]))
    {
        randtemp = fastRandom * randSeed;
        fastRandom = randtemp & 0xFFFFU;
        uint32 tempRandom = (randtemp >> 8) & 0xFFFFU;
        if ((nextAICar == 0 || nextAICar->fallBehindCar == 0) &&
            tempRandom < info.fallBackRandomTime_TickPercent * (uint32)AI::elapsedTime)
        {
            Car::tObj *prevAICar = GetPrevAICar(carObj);
            if (prevAICar != 0)
                carObj->fallBehindCar = prevAICar;
        }
    }
    if (nextAICar != 0 && nextAICar->damageMult < (sint32)((uint32)carObj->damageMult - 0x1999U) &&
        nextAICar->fallBehindCar == 0)
        nextAICar->fallBehindCar = carObj;
    return f_caravan;
}

sint32 AISpeeds::GetGlueFactor(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006E68C, "NFS4.EXE");
    sint32 packPositionGlueModifier = 65536;
    sint32 leadHumanRacerOdometer = AIWorld::GameOdometer(leaderBoard.leadHumanRacer);
    sint32 leadAIRacerOdometer = AIWorld::GameOdometer(leaderBoard.leadAIRacer);
    sint32 lastAIRacerOdometer = AIWorld::GameOdometer(leaderBoard.lastAIRacer);
    sint32 leadSpeed = leaderBoard.leadHumanRacer->currentSpeed;
    if (leadSpeed < 0)
        leadSpeed = (sint32)(0U - (uint32)leadSpeed);
    sint32 glue = 65536;
    if (leadSpeed >= 0xa0000)
    {
        sint32 distance = (sint32)((uint32)leadAIRacerOdometer - (uint32)leadHumanRacerOdometer);
        sint32 glueIndex = distance / 0x3c0000 + 10;
        if (glueIndex < 0)
            glueIndex = 0;
        if (glueIndex >= 21)
            glueIndex = 20;
        glue = AIPerson::glueTable[glueIndex];
        if (leadHumanRacerOdometer >= lastAIRacerOdometer && leadHumanRacerOdometer <= leadAIRacerOdometer)
            packPositionGlueModifier = GameSetup::gData.raceType == 3 ? 0x8000 : 0xe666;
    }
    sint32 odometer = AIWorld::GameOdometer(carObj);
    if (odometer < (sint32)((uint32)leadHumanRacerOdometer + 0x3c0000U) && glue < 65536)
        glue = 65536;
    if (glue > 65536)
    {
        sint32 delta = fixedmult((sint32)((uint32)glue - 65536U), packPositionGlueModifier);
        return (sint32)((uint32)delta + 65536U);
    }
    sint32 delta = fixedmult((sint32)(65536U - (uint32)glue), packPositionGlueModifier);
    return (sint32)(65536U - (uint32)delta);
}

sint32 AISpeeds::GetDamageFactor(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006E898, "NFS4.EXE");
    uint32 damage = (uint32)carObj->N.damage[0] + (uint32)carObj->N.damage[1] +
        (uint32)carObj->N.damage[2] + (uint32)carObj->N.damage[3];
    carObj->damageMult = 0;
    damage += (uint32)carObj->N.damage[4] + (uint32)carObj->N.damage[5] +
        (uint32)carObj->N.damage[6] + (uint32)carObj->N.damage[7];
    carObj->damageMult = (sint32)damage;
    sint32 factor = 0x48;
    if ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) &&
        ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0 ||
         (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0)))
        factor = 0x147;
    carObj->damageMult = fixedmult(carObj->damageMult, factor);
    carObj->damageMult = (sint32)(65536U - (uint32)carObj->damageMult);
    if (carObj->damageMult < 32768)
        carObj->damageMult = 32768;

    return carObj->damageMult;
}

sint32 AISpeeds::LimitGlueMultiplier(Car::tObj *carObj, sint32 f_final)
{
    FUNCTION_MARKER(0x8006E9B0, "NFS4.EXE");
    if ((uint32)f_final - 0x999aU > 0xcccbU)
    {
        sint32 bestDistanceAbsMeters = 0x27100000;
        for (sint32 playerLoop = 0; playerLoop < Cars::gNumHumanRaceCars; ++playerLoop)
        {
            sint32 thisDistanceAbsMeters = AIWorld::ApxSplineDistance(carObj, Cars::gHumanRaceCarList[playerLoop]);
            if (thisDistanceAbsMeters < 0)
                thisDistanceAbsMeters = (sint32)(0U - (uint32)thisDistanceAbsMeters);
            if (thisDistanceAbsMeters < bestDistanceAbsMeters)
                bestDistanceAbsMeters = thisDistanceAbsMeters;
        }
        if (bestDistanceAbsMeters <= 0x780000)
        {
            if (f_final > 0x16666)
                f_final = 0x16666;
            if (f_final < 0x9999)
                f_final = 0x9999;
        }
    }
    return f_final;
}

sint32 AISpeeds::CalcCopTopSpeed(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006EAA4, "NFS4.EXE");
    sint32 topSpeed = CalcOpponentCurveSpeed(carObj);
    sint32 newDesired = carObj->copTopSpeed;
    if (topSpeed < newDesired)
        newDesired = topSpeed;
    sint32 f_nitrous = carObj->speedNitrous;
    carObj->aiGlue = (sint32)((uint32)(trackAndNightMult / 256) * (uint32)(f_nitrous / 256));
    carObj->N.gravityMult = carObj->aiGlue > 65536 ? carObj->aiGlue : 65536;
    return (sint32)((uint32)(newDesired / 256) * (uint32)(carObj->aiGlue / 256) * (uint32)carObj->direction);
}

sint32 AISpeeds::CalcTrafficTopSpeed(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006EB6C, "NFS4.EXE");
    sint32 desired = GetLegalSpeed(carObj->N.simRoadInfo.slice);
    desired = fixedmult(desired, 0xc000);
    bool oncoming;
    if ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) &&
        ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0 ||
         (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0)))
        oncoming = carObj->direction != Cars::gHumanRaceCarList[0]->direction;
    else
        oncoming = GameSetup::gData.reverseTrack == 0 ? carObj->direction != 1 : carObj->direction != -1;
    if (oncoming)
    {
        sint32 oncomingSpeed = CalculateOncomingCarSpeed(carObj);
        if (oncomingSpeed <= desired)
            desired = CalculateOncomingCarSpeed(carObj);
    }
    if ((carObj->carFlags & 0x10) != 0)
    {
        desired = RandomizeTrafficSpeed(carObj, desired);
        desired = (sint32)((uint32)(desired / 256) * (uint32)(carObj->speedFactor / 256));
    }
    if (desired < 0x8e38e)
        desired = 0x8e38e;
    return (sint32)((uint32)desired * (uint32)carObj->direction);
}

sint32 AISpeeds::GetLegalSpeed(sint32 slice)
{
    FUNCTION_MARKER(0x8006ED0C, "NFS4.EXE");
    speedData_t *speedData = TrackSpeeds[GameSetup::gData.track];
    while ((sint32)speedData->endSlice < slice)
        ++speedData;
    return speedData->speedMPS * 256;
}

sint32 AISpeeds::RandomizeTrafficSpeed(Car::tObj *carObj, sint32 oldsafe)
{
    FUNCTION_MARKER(0x8006ED50, "NFS4.EXE");
    sint32 newsafe = (sint32)((uint32)(oldsafe / 256) * (uint32)(carObj->trafficSpeedRandomizingFactor / 256));
    if (oldsafe > 0)
    {
        sint32 safeminus30 = (sint32)((uint32)oldsafe - 0xd6666U);
        if (newsafe < safeminus30)
            newsafe = safeminus30;
        if (newsafe < 0x8e666)
            newsafe = 0x8e666;
    }
    return newsafe;
}

void AISpeeds::CalcDesiredSpeed(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006EDDC, "NFS4.EXE");
    sint32 flags = carObj->carFlags;
    carObj->desiredSpeed = 0;
    if ((flags & 8) != 0)
        carObj->desiredSpeed = CalcOpponentTopSpeed(carObj, &carObj->originalDesiredSpeed);
    else if ((flags & 0x20) != 0)
    {
        if ((carObj->AIFlags & 2) != 0)
            carObj->desiredSpeed = CalcCopTopSpeed(carObj);
        else
            carObj->desiredSpeed = CalcTrafficTopSpeed(carObj);
        carObj->originalDesiredSpeed = carObj->desiredSpeed;
    }
    else if ((flags & 0x10) != 0)
    {
        sint32 desired = CalcTrafficTopSpeed(carObj);
        carObj->desiredSpeed = desired;
        carObj->originalDesiredSpeed = desired;
    }
}

sint32 AISpeeds::CalculateOncomingCarSpeed(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006EE80, "NFS4.EXE");
    sint32 playerSpeed = carObj->basisCar->currentSpeed;
    if (playerSpeed < 0)
        playerSpeed = (sint32)(0U - (uint32)playerSpeed);
    if (playerSpeed < 0xd5555)
        return 0x1638e3;
    if (playerSpeed < 0x1aaaaa)
        return 0xd5555;
    if (playerSpeed < 0x280000)
        return 0x8e38e;
    if (playerSpeed < 0x378e38)
        return 0x471c7;
    if (playerSpeed <= 0x471c70)
        return 0x238e3;
    return 0x2c51e;
}

void AISpeeds::SetTrafficSpeedRandomFactor(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006EF28, "NFS4.EXE");
    uint32 product = fastRandom * randSeed;
    sint32 randomSpeedMultipliers[4] = { 65536, 58982, 52428, 45875 };
    sint32 selected = randomSpeedMultipliers[(product >> 8) & 3U];
    randtemp = product;
    fastRandom = product & 0xffffU;
    carObj->trafficSpeedRandomizingFactor = selected;
}

void AISpeeds::MaintainLeaderBoard(void)
{
    FUNCTION_MARKER(0x8006EFA4, "NFS4.EXE");
    sint32 slot = 0;
    Car::tObj *lastAI = 0;
    sint32 numCars = Cars::gNumCars;
    leaderBoard.leadRacer = 0;
    leaderBoard.leadHumanRacer = 0;
    leaderBoard.leadAIRacer = 0;
    leaderBoard.lastAIRacer = 0;
    for (sint32 carLoop = numCars - 1; carLoop >= 0; --carLoop)
    {
        Car::tObj *test = Cars::gTotalSortedList[carLoop];
        if ((test->carFlags & 1) != 0 && leaderBoard.leadRacer == 0 && test->fallBehindCar == 0)
            leaderBoard.leadRacer = test;
        if ((test->carFlags & 8) != 0)
        {
            test->nextAIRacer = lastAI;
            lastAI = test;
            test->AISlot = slot;
            ++slot;
            if (leaderBoard.leadAIRacer == 0)
                leaderBoard.leadAIRacer = test;
            leaderBoard.lastAIRacer = test;
        }
        if ((test->carFlags & 4) != 0 && leaderBoard.leadHumanRacer == 0)
            leaderBoard.leadHumanRacer = test;
    }
}

sint32 AISpeeds::GetScriptFactor(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006F0A4, "NFS4.EXE");
    AIScript::AIScript_t *script = &carObj->script;
    if (AIScript::DoReAction(script, AIScript::AISCRIPT_AIREACTION_SPEED_BURST) != -1)
        return 0x18000;
    if (AIScript::DoReAction(script, AIScript::AISCRIPT_AIREACTION_HARD_BRAKE) != -1)
        return 0x8000;
    return 0x10000;
}

sint32 AISpeeds::GetUpgradeAccMult(sint32 carIndex)
{
    FUNCTION_MARKER(0x8006F10C, "NFS4.EXE");
    Car::tObj *carObj = Cars::gList[carIndex];
    sint32 accMult = 65536;
    if (carObj->carInfo->EngineMods == 1)
        accMult = fixedmult(accMult, engineUpgrade.accMult);
    if (carObj->carInfo->WeightTransfer == 1)
        accMult = fixedmult(accMult, suspensionUpgrade.accMult);
    if (carObj->carInfo->GroundEffects == 1)
        accMult = fixedmult(accMult, aeroUpgrade.accMult);
    return accMult;
}

sint32 AISpeeds::GetUpgradeBrakeMult(sint32 carIndex)
{
    FUNCTION_MARKER(0x8006F1C8, "NFS4.EXE");
    Car::tObj *carObj = Cars::gList[carIndex];
    sint32 brakeMult = 65536;
    if (carObj->carInfo->EngineMods == 1)
        brakeMult = fixedmult(brakeMult, engineUpgrade.brakeMult);
    if (carObj->carInfo->WeightTransfer == 1)
        brakeMult = fixedmult(brakeMult, suspensionUpgrade.brakeMult);
    if (carObj->carInfo->GroundEffects == 1)
        brakeMult = fixedmult(brakeMult, aeroUpgrade.brakeMult);
    return brakeMult;
}

sint32 AISpeeds::GetUpgradeHandlingMult(sint32 carIndex)
{
    FUNCTION_MARKER(0x8006F284, "NFS4.EXE");
    Car::tObj *carObj = Cars::gList[carIndex];
    sint32 handlingMult = 65536;
    if (carObj->carInfo->EngineMods == 1)
        handlingMult = fixedmult(handlingMult, engineUpgrade.handlingMult);
    if (carObj->carInfo->WeightTransfer == 1)
        handlingMult = fixedmult(handlingMult, suspensionUpgrade.handlingMult);
    if (carObj->carInfo->GroundEffects == 1)
        handlingMult = fixedmult(handlingMult, aeroUpgrade.handlingMult);
    return handlingMult;
}

sint32 AISpeeds::GetUpgradeTopSpeedMult(sint32 carIndex)
{
    FUNCTION_MARKER(0x8006F340, "NFS4.EXE");
    Car::tObj *carObj = Cars::gList[carIndex];
    sint32 topSpeedMult = 65536;
    if (carObj->carInfo->EngineMods == 1)
        topSpeedMult = fixedmult(topSpeedMult, engineUpgrade.topSpeedMult);
    if (carObj->carInfo->WeightTransfer == 1)
        topSpeedMult = fixedmult(topSpeedMult, suspensionUpgrade.topSpeedMult);
    if (carObj->carInfo->GroundEffects == 1)
        topSpeedMult = fixedmult(topSpeedMult, aeroUpgrade.topSpeedMult);
    return topSpeedMult;
}

sint32 AISpeeds::CalcHumanTopSpeed(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006F3FC, "NFS4.EXE");
    return CalcHumanCurveSpeed(carObj);
}

sint32 AISpeeds::CalcHumanCurveSpeed(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8006F41C, "NFS4.EXE");
    sint32 sliceHere = carObj->N.simRoadInfo.slice;
    sint32 sliceAhead = sliceHere;
    if (sliceAhead >= gNumSlices)
        sliceAhead = (sint32)((uint32)sliceAhead - (uint32)gNumSlices);
    sint32 tightestCurve = AIDataRecord::TrackCurve->Get(sliceAhead);
    for (sint32 distance = 4; distance <= 20; distance += 4)
    {
        sint32 offset = (sint32)((uint32)carObj->direction * (uint32)distance);
        sliceAhead = (sint32)((uint32)sliceHere + (uint32)offset);
        if (offset >= 0)
        {
            if (sliceAhead >= gNumSlices)
                sliceAhead = (sint32)((uint32)sliceAhead - (uint32)gNumSlices);
        }
        else if (sliceAhead < 0)
            sliceAhead = (sint32)((uint32)sliceAhead + (uint32)gNumSlices);
        sint32 curveAhead = AIDataRecord::TrackCurve->Get(sliceAhead);
        if (tightestCurve < curveAhead)
            tightestCurve = curveAhead;
    }
    sint32 scaled = tightestCurve * 0x1a666;
    tightestCurve = scaled / 65536;
    if (tightestCurve >= 256)
        tightestCurve = 255;
    return carObj->curveSpeedTable->Get(tightestCurve);
}
