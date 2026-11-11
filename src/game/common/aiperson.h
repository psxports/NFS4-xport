#ifndef AIPERSON_H
#define AIPERSON_H

#include "xport.h"
#include "game/common/aiscript.h"
#include "game/common/car.h"
#include "game/common/udff.h"

class AIPerson
{
public:
    static AIScript::tReactionDetails ScriptData[9][7];

    struct AIPerson_t
    {
        sint32 blockMaxDistance;
        sint32 blockMinDistance;
        sint32 oncomingLookAhead;
        sint32 laneSwerve[4];
        sint32 swerveChangeProb;
        sint32 brakeMultiplier;
        sint32 attackActivationHits;
        sint32 attackTime;
        sint32 fishtailAngle;
        sint32 minimumBetweenWipeoutTicks;
        sint32 randomBetweenWipeoutTicks;
        sint32 gripLossProbPerSecond;
        sint32 gripLossMinFactor;
        sint32 gripLossRecoveryPerTick;
        sint32 bestLineAbility;
        uint32 rearBumpProbMask;
        uint32 smackProbMask;
        uint32 copCollisionFirmness;
    };

    static AIPerson_t PersonalityData[9];
    static sint32 glueTable[21];
    static sint32 blockMaxDistance[4];
    static sint32 blockMinDistance[4];
    static sint32 oncomingLookAhead[4];
    static sint32 laneSwerve[4][4];
    static sint32 swerveChangeProb[4];
    static sint32 brakeMultiplier[4];
    static sint32 attackActivationHits[4];
    static sint32 attackTimes[4];
    static sint32 fishtailAngles[4];
    static sint32 gripLossProbPerSecond[4];
    static sint32 gripLossMinFactor[4];
    static sint32 gripLossRecoveryPerTick[4];
    static sint32 minimumWipeOutTicks[5];
    static sint32 randomWipeOutTicks[5];
    static uint32 rearEndProbMask[4];
    static uint32 smackProbMask[4];
    static sint32 bestLineAbilities[4];

    static void LoadGridAndSetPersonalityIndexes(void);
    static void SetPersonality(Car::tObj *carObj, sint32 personalityIndex);
    static void SetPersonalityPointers(void);
    static void LoadPersonalityData(Udff::tInfo *handle);
    static void LoadScriptData(Udff::tInfo *handle);
    static void LoadGlue(Udff::tInfo *handle);
    static void Startup(void);
    static void Cleanup(void);
};

#endif
