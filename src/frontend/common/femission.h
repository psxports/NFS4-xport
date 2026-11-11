#ifndef FEMISSION_H
#define FEMISSION_H

#include "xport.h"

struct tMissionInfo
{
    uint8 fDescriptionID;
    uint8 fTrackNumber;
    uint8 fDirection;
    uint8 fMirrored;
    uint8 fTimeOfDay;
    uint8 fWeather;
    uint8 fTraffic;
    uint8 fPad;
    uint16 fStageOffset;
    uint8 fNumStages;
    uint8 fReserved[9];
};

struct tAcademyDefinition;

struct tStageInfo
{
    uint8 fCarModel;
    uint8 fColor;
    uint8 fAIPersonality;
    uint8 fDirection;
    sint16 fTimeLimit;
    sint16 fWingman;
    sint16 fSpikeBelt;
    sint16 fBlockadeCop;
    uint8 fPlacement;
    uint8 fStyle;
    uint16 fDistance;
    uint32 fSpeed;
    uint32 fWeight;
    uint32 fGlue;
    uint8 fSpeechColor;
    uint8 fReserved[15];
};

class tMissionManager
{
public:
    char fNumTiers;
    tAcademyDefinition *fDefinition;

    void Initialize(void);
    void LoadDescription(bool LoadHotPursuit);
    void ReleaseDescription(void);
    sint16 GetMissionStages(sint16 tier, sint16 mission, tStageInfo **pStages);
    void GetMissionToRace(tMissionInfo **mission);
};

extern tMissionManager missionManager;

#endif
