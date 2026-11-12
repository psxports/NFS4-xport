#ifndef NFS3_H
#define NFS3_H

#include "xport.h"
#include "eaclib/psx/nfile.h"
#include "psx.h"
#include "frontend/common/fecars.h"
#include "game/common/car.h"

class NFS3
{
public:
    static void CheckForFileOperations(void);
};

class Nfs2
{
public:
    static void SystemNLibStartUp(void);
    static void BefuddleCode(void);
    static void ResetGame(void);
    static void GameModuleStartUp(sint32 *FrontEndDataStream);
    static void CleanUpGameModule(void);
};

class NFS4
{
public:
    static void LoadPerps(void);
    static void LoadingIcon(void);
};

struct tfrontEnd
{
    sint16 randomSeed;
    char currentPlayer;
    char gameMode;
    char raceType;
    char style;
    char skillLevel;
    char catchup;
    char ghostcar;
    char tractionassist;
    char bestline;
    sint16 pinkSlipsWinner[5];
    char timeOfDay[5];
    char weather[5];
    char fog;
    char localSpeech;
    char opponentUpgrades;
    char track[5];
    sint32 recordlaptime;
    char laps;
    char lapind[5];
    char trackdirection[5];
    char trackmirrored[5];
    char traffic[5];
    char situations[5];
    char pinkSlipsTrackIndex;
    char pinkSlipsNumTracks;
    char pinkSlipsWins[2];
    sint32 pinkSlipsCash[2];
    sint16 cheatFlags;
    sint16 cheatFlagsActive;
    sint16 secretFlags;
    sint16 pinkSlipsForfeit;
    char playerCar[2];
    char transmission[2];
    char carColors[2][48];
    char carCountry[2][48];
    char tournament;
    char specialevent;
    char tier;
    char garageCar[2];
    char pinkSlipsCar[2];
    char upgrade;
    char dealerCar;
    char sellerCar;
    char carListType;
    char policeTier;
    char policeMission;
    char congratsCopCar;
    char congratsCopCountry;
    sint16 settingsActive[2];
    char steeringSensitivity[2][48];
    char tireType[2][48];
    char suspension[2][48];
    char carDownforce[2][48];
    char engineTuning[2][48];
    char ABS[2];
    char damage;
    char rampSteer[2];
    char rampGas[2];
    char rampBrake[2];
    char oppCar;
    char oppNumber;
    char measurement;
    char displaySpeed[2];
    char displayTach[2];
    char displayMap[2];
    char displayOpponentID[2];
    char displayTime[2];
    char displayPosition[2];
    char displayLapNumber[2];
    char displayMirror[2];
    char cameraType[2][4];
    char cameraSway[2][4];
    char language;
    char musicVolume;
    char sfxVolume;
    char narrationVolume;
    char engineVolume;
    char ambientVolume;
    char audioMode;
    char song;
    char sensitivity;
    char steeringRange[2];
    char deadSpot[2];
    char ImaxRange[2];
    char IImaxRange[2];
    char J1MAX[2];
    char J1MIN[2];
    char J2MAX[2];
    char J2MIN[2];
    char shockMode[2];
    char shockImpact[2];
    sint16 controlType[2];
    char controlConfig[2];
    char playerNameList[2][8];
    char allUpperCasedPlayerNameList[2][8];
    char licensePlate[2][8];
    char headstart;
    char numBTracksActivated;
    char checkPointType;
    char checkPointDisplay[2];
    char defaultedPlayerName[2];

    sint32 FEPlayList[40];
    uint32 gPinkSlipsNoCheat[2];

    sint32 GotAPlayList;

    sint32 AnalogOn[2];
};

struct tNfsSystemInfo
{
    sint32 userRam;
};

sint32 Nfs4Main(void);

extern tNfsSystemInfo nfs_sysInfo;
extern NativeFileSlot *gFEBigHandle;
extern tfrontEnd frontEnd;
extern sint32 gUseFrontend;
extern char gCalculateVictory;
extern bool MEMCARDFRONTENDISINITTED;

extern tCarManager carManager;
extern uint32 gFEBonus;
extern uint32 gFECheats;

#endif
