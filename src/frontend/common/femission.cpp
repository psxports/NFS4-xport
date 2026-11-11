#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "frontend/common/femission.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "lib/psx/blkmov.h"

struct tMissionTierInfo;

struct tMissionTierInfo
{
    uint8 fNumMissions;
    uint8 fDescriptionID;
    uint8 fMissionOffset;
    uint8 fPad;
};

struct tAcademyDefinition;

struct tAcademyDefinition
{
    tMissionTierInfo fTiers[8];
    tMissionInfo fMissions[64];
    tStageInfo fStages[256];
};

void tMissionManager::Initialize(void)
{
    FUNCTION_MARKER(0x80034060, "FRONT.BIN");
    fNumTiers = 0;
    fDefinition = 0;
    frontEnd.policeTier = 0;
    frontEnd.policeMission = 0;
}

void tMissionManager::LoadDescription(bool LoadHotPursuit)
{
    FUNCTION_MARKER(0x8003407C, "FRONT.BIN");
    (void)LoadHotPursuit;
    char filename[80];
    sprintf(filename, "%s%s", Paths_Paths[37],
        (uint8)frontEnd.gameMode == 1 ? "zHPurs2.mis" : "zHPurs.mis");
    char *input = static_cast<char *>(loadfileadr(filename, 16));
    fNumTiers = input[0];
    uint32 numMissions;
    uint32 numStages;
    memcpy(&numMissions, input + 4, 4);
    memcpy(&numStages, input + 8, 4);
    char *data = input + 12;
    if (!fDefinition)
        fDefinition = static_cast<tAcademyDefinition *>(reservememadr("Missions", 0x3120, 0));
    blockmove(data, fDefinition, (uint8)fNumTiers * 4);
    data += (uint8)fNumTiers * 4;
    blockmove(data, fDefinition->fMissions, numMissions * 20);
    blockmove(data + numMissions * 20, fDefinition->fStages, numStages * 44);
    purgememadr(input);
}

void tMissionManager::ReleaseDescription(void)
{
    FUNCTION_MARKER(0x800341B8, "FRONT.BIN");
    if (fDefinition)
    {
        purgememadr(fDefinition);
        fDefinition = 0;
    }
}

sint16 tMissionManager::GetMissionStages(sint16 tier, sint16 mission, tStageInfo **pStages)
{
    FUNCTION_MARKER(0x800341F4, "FRONT.BIN");
    tMissionInfo *pMissionInfo = &fDefinition->fMissions[fDefinition->fTiers[tier].fMissionOffset + mission];
    *pStages = &fDefinition->fStages[pMissionInfo->fStageOffset];
    return pMissionInfo->fNumStages;
}

void tMissionManager::GetMissionToRace(tMissionInfo **mission)
{
    FUNCTION_MARKER(0x8003425C, "FRONT.BIN");
    tMissionTierInfo *currentTier = &fDefinition->fTiers[(uint8)frontEnd.policeTier];
    *mission = &fDefinition->fMissions[currentTier->fMissionOffset + (uint8)frontEnd.policeMission];
}
