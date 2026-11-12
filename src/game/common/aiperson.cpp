#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "game/common/aiperson.h"
#include "game/common/aiscript.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/paths.h"
#include "game/common/udff.h"
#include "lib/psx/fixdmult.h"

typedef sint32 AIPerson_tGlueTable[21];

sint32 AIPerson::glueTable[21];
sint32 AIPerson::blockMaxDistance[4] = { 2293760, 3276800, 5242880, 7864320 };
sint32 AIPerson::blockMinDistance[4] = { 2293760, 1310720, 655360, 0 };
sint32 AIPerson::oncomingLookAhead[4] = { 5898240, 10485760, 13762560, 16384000 };

sint32 AIPerson::laneSwerve[4][4] =
{
    { 0, 0, 0, 0 },
    { -65536, -32768, 0, 65536 },
    { -131072, 0, 65536, 131072 },
    { -196608, -98304, 98304, 196608 }
};

sint32 AIPerson::swerveChangeProb[4] = { 400, 100, 40, 10 };
sint32 AIPerson::brakeMultiplier[4] = { 55705, 65536, 78643, 88473 };
sint32 AIPerson::attackActivationHits[4] = { 10000, 15, 40, 40 };
sint32 AIPerson::attackTimes[4] = { 0, 96, 320, 1280 };
sint32 AIPerson::fishtailAngles[4] = { 35, 40, 45, 50 };
sint32 AIPerson::gripLossProbPerSecond[4] = { 32768, 32768, 32768, 0 };
sint32 AIPerson::gripLossMinFactor[4] = { 19660, 26214, 32768, 64880 };
sint32 AIPerson::gripLossRecoveryPerTick[4] = { 71, 61, 40, 65536 };
sint32 AIPerson::minimumWipeOutTicks[5] = { 65536, 4096, 2048, 1024, 1024 };
sint32 AIPerson::randomWipeOutTicks[5] = { 65535, 8191, 4095, 2047, 2047 };
uint32 AIPerson::rearEndProbMask[4] = { 65535, 768, 1792, 512 };
uint32 AIPerson::smackProbMask[4] = { 65535, 768, 1792, 1536 };
sint32 AIPerson::bestLineAbilities[4] = { 19660, 32768, 52428, 65536 };
AIPerson::AIPerson_t AIPerson::PersonalityData[9];
AIScript::tReactionDetails AIPerson::ScriptData[9][7];

void AIPerson::LoadGridAndSetPersonalityIndexes(void)
{
    FUNCTION_MARKER(0x800689E8, "NFS4.EXE");
    const sint32 numCars = Cars::gNumCars;
    for (sint32 carLoop = 0; carLoop < numCars; ++carLoop)
        Cars::gList[carLoop]->personalityIndex = GameSetup::gData.carInfo[carLoop].Personality;
}

void AIPerson::SetPersonality(Car::tObj *carObj, sint32 personalityIndex)
{
    FUNCTION_MARKER(0x80068A34, "NFS4.EXE");
    if ((carObj->carFlags & 4) == 0)
        strcpy(carObj->carInfo->driver, GameSetup::gPersonalityNames[personalityIndex]);
    personalityIndex %= 5;
    carObj->personalityIndex = personalityIndex;
    carObj->personality = &PersonalityData[personalityIndex];
    AIScript::Assign(&carObj->script, &ScriptData[personalityIndex]);
}

void AIPerson::SetPersonalityPointers(void)
{
    FUNCTION_MARKER(0x80068AFC, "NFS4.EXE");
    for (sint32 carLoop = 0; carLoop < Cars::gNumCars; ++carLoop)
    {
        Car::tObj *carObj = Cars::gList[carLoop];
        SetPersonality(carObj, carObj->personalityIndex);
    }
}

void AIPerson::LoadPersonalityData(Udff::tInfo *handle)
{
    FUNCTION_MARKER(0x80068B60, "NFS4.EXE");
    Udff::GetInt(handle);
    for (sint32 perLoop = 0; perLoop < 9; ++perLoop)
    {
        sint32 blockingDistance = Udff::GetInt(handle);
        sint32 cutOffDistance = Udff::GetInt(handle);
        sint32 lookAhead = Udff::GetInt(handle);
        sint32 swervyness = Udff::GetInt(handle);
        sint32 braking = Udff::GetInt(handle);
        sint32 roadRage = Udff::GetInt(handle);
        sint32 fishtailControl = Udff::GetInt(handle);
        Udff::GetInt(handle);
        sint32 wipeOutFreq = Udff::GetInt(handle);
        sint32 corneringAbility = Udff::GetInt(handle);
        sint32 bestLineAbility = Udff::GetInt(handle);
        sint32 rearEndingTendency = Udff::GetInt(handle);
        sint32 aggression = Udff::GetInt(handle);
        uint32 copCollisionFirmness = Udff::GetInt(handle);
        AIPerson_t *personality = &PersonalityData[perLoop];
        personality->blockMaxDistance = blockMaxDistance[blockingDistance];
        personality->blockMinDistance = blockMinDistance[cutOffDistance];
        personality->oncomingLookAhead = oncomingLookAhead[lookAhead];
        personality->laneSwerve[0] = laneSwerve[swervyness][0];
        personality->laneSwerve[1] = laneSwerve[swervyness][1];
        personality->laneSwerve[2] = laneSwerve[swervyness][2];
        personality->laneSwerve[3] = laneSwerve[swervyness][3];
        personality->swerveChangeProb = swerveChangeProb[swervyness];
        personality->brakeMultiplier = brakeMultiplier[braking];
        personality->attackActivationHits = attackActivationHits[roadRage];
        personality->attackTime = attackTimes[roadRage];
        personality->fishtailAngle = fishtailAngles[fishtailControl];
        personality->minimumBetweenWipeoutTicks = minimumWipeOutTicks[wipeOutFreq + GameSetup::gData.Weather];
        personality->randomBetweenWipeoutTicks = randomWipeOutTicks[wipeOutFreq + GameSetup::gData.Weather];
        personality->gripLossProbPerSecond = gripLossProbPerSecond[corneringAbility];
        personality->gripLossMinFactor = gripLossMinFactor[corneringAbility];
        personality->gripLossRecoveryPerTick = gripLossRecoveryPerTick[corneringAbility];
        personality->bestLineAbility = bestLineAbilities[bestLineAbility];
        personality->rearBumpProbMask = rearEndProbMask[rearEndingTendency];
        uint32 smackMask = smackProbMask[aggression];
        personality->copCollisionFirmness = copCollisionFirmness;
        personality->smackProbMask = smackMask;
    }
}

void AIPerson::LoadScriptData(Udff::tInfo *handle)
{
    FUNCTION_MARKER(0x80068EA4, "NFS4.EXE");
    Udff::GetInt(handle);
    for (sint32 perLoop = 0; perLoop < 9; ++perLoop)
        for (sint32 actionLoop = 0; actionLoop < 7; ++actionLoop)
            for (sint32 reactionLoop = 0; reactionLoop < 4; ++reactionLoop)
            {
                ScriptData[perLoop][actionLoop].reaction[reactionLoop] = (char)Udff::GetInt(handle);
                ScriptData[perLoop][actionLoop].halfSeconds[reactionLoop] = (char)Udff::GetInt(handle);
            }
}

void AIPerson::LoadGlue(Udff::tInfo *handle)
{
    FUNCTION_MARKER(0x80068F80, "NFS4.EXE");
    Udff::GetInt(handle);
    Udff::GetBuffer(handle, (char *)glueTable, sizeof(glueTable));
    if (Cars::gNumCopCars != 0 && ((GameSetup::gData.raceType != 1 && GameSetup::gData.raceType != 5) || ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) == 0 && (Cars::gNumHumanRaceCars != 2 || (Cars::gHumanRaceCarList[1]->carFlags & 0x200) == 0))))
    {
        for (sint32 glueLoop = 0; glueLoop < 21; ++glueLoop)
        {
            sint32 value = glueTable[glueLoop];
            if (value > 0x10000)
                glueTable[glueLoop] = fixedmult(value, 0x12666);
            else if (value < 0x10000)
                glueTable[glueLoop] = fixedmult(value, 0xd999);
        }
    }
}

void AIPerson::Startup(void)
{
    FUNCTION_MARKER(0x8006908C, "NFS4.EXE");
    Udff::tInfo *handle;
    char filename[110];
    LoadGridAndSetPersonalityIndexes();
    sprintf(filename, "%sprsonal.bin", Paths_Paths[2]);
    handle = Udff::Opena(filename, 0, 1);
    LoadPersonalityData(handle);
    Udff::Close(handle);
    sprintf(filename, "%sscripts.bin", Paths_Paths[2]);
    handle = Udff::Opena(filename, 0, 1);
    LoadScriptData(handle);
    Udff::Close(handle);
    const char *format;
    if ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) && ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0 || (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0)))
        format = "%sbtcglue.bin";
    else if ((uint32)GameSetup::gData.raceType < 2 && Cars::gNumAIRaceCars == 1)
        format = "%shhglue.bin";
    else
        format = "%sglue.bin";
    sprintf(filename, format, Paths_Paths[2]);
    handle = Udff::Opena(filename, 0, 1);
    LoadGlue(handle);
    Udff::Close(handle);
    SetPersonalityPointers();
}

void AIPerson::Cleanup(void)
{
    FUNCTION_MARKER(0x80069230, "NFS4.EXE");
}
