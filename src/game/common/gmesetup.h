#ifndef GMESETUP_H
#define GMESETUP_H

#include "xport.h"

class GameSetup
{
public:
    static char gPersonalityNames[15][8];
    static char gCarNames[51][5];
    static char gTrackNames[12][4];

    struct tCarData
    {
        sint32 carType;
        sint32 carClass;
        sint32 Transmission;
        sint32 Colour;
        sint32 ColourSV;
        sint32 ColourChange;
        sint32 RampSteering;
        sint32 RampGas;
        sint32 RampBrake;
        sint32 BrakeBias;
        sint32 GearRatio;
        sint32 SteeringQuickness;
        sint32 Traction;
        sint32 BrakeAssist;
        sint32 TireType;
        sint32 EngineMods;
        sint32 WeightTransfer;
        sint32 GroundEffects;
        sint32 ABS;
        sint32 Bestline;
        sint32 Personality;
        sint32 StartingPos;
        sint32 carNameIndex;
        char driver[8];
        sint32 pad;
        char license[8];
        sint32 HudSpeed;
        sint32 HudSpeedMult;
        sint32 HudTach;
        sint32 HudMap;
        sint32 HudOpponentID;
        sint32 HudTime;
        sint32 HudLapnum;
        sint32 HudPosition;
        sint32 HudMirror;
        sint32 HudColour;
        sint32 HudTachColour;
        sint32 SpeechColour;
        sint32 Country;
        sint32 Camera[4];
    };

    struct tUserSetting
    {
        sint32 language;
        sint32 display;
        sint32 bestlap;
        sint32 audioMode;
        sint32 musicLevel;
        sint32 sfxLevel;
        sint32 engineLevel;
        sint32 speechLevel;
        sint32 ambientLevel;
        sint32 interactive;
        sint32 numplaylistsongs;
        sint32 playlist[40];
    };

    struct tControllerData
    {
        sint32 controllerConfig[2];
        sint32 deadSpot[2];
        sint32 steeringRange[2];
        sint32 IImaxRange[2];
        sint32 ImaxRange[2];
        sint32 J1MIN[2];
        sint32 J1MAX[2];
        sint32 J2MIN[2];
        sint32 J2MAX[2];
        sint32 shockMode[2];
        sint32 shockImpact[2];
    };

    struct tPerpData
    {
        sint32 CarType;
        sint32 Colour;
        sint32 SpeechColour;
        sint32 HudColour;
        sint32 Personality;
        sint32 TimeLimit;
        sint32 WingmanTime;
        sint32 SpikeBeltTime;
        sint32 BlockadeCopTime;
        sint32 Distance;
        sint32 SpeedFactor;
        sint32 WeightFactor;
        sint32 GlueFactor;
    };

    struct tData
    {
        sint32 raceType;
        sint32 numLaps;
        sint32 skill;
        sint32 commMode;
        sint32 tournamentMultiplier;
        sint32 cops;
        sint32 trafficDensity;
        sint32 localCar;
        sint32 catchupLogic;
        sint32 replayMode;
        sint32 instantReplay;
        sint32 mirrorTrack;
        sint32 reverseTrack;
        sint32 measurement;
        sint32 sgge;
        sint32 track;
        sint32 trackSegment;
        sint32 song;
        sint32 Weather;
        sint32 Fog;
        sint32 Damage;
        sint32 Time;
        sint32 randSeed;
        sint32 easter;
        tControllerData controllerData;
        sint32 pinkSlipsForfeit;
        sint32 checkpointType;
        sint32 checkpointHUD[2];
        sint32 dispatchSpeech;
        sint32 reverseCallSpeech;
        sint32 languageSpeech;
        sint32 SceneNumber;
        sint32 SceneStartLap;
        sint32 SceneEndLap;
        tUserSetting userSetting;
        sint32 numPerps;
        sint32 stageOffset;
        sint32 perpArrests;
        sint32 finalPerpArrests;
        tPerpData perpInfo[10];
        sint32 numCars;
        sint32 numPlayerRaceCars;
        sint32 numOpponentRaceCars;
        sint32 opponentCarType;
        tCarData carInfo[9];
    };

    static tData gData;

    static void StartUp(sint32 *FrontEndDataStream);
    static void CleanUp(void);
};

#endif
