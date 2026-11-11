#ifndef FRONT_H
#define FRONT_H

#include "xport.h"

struct tFEStream;

class tFront
{
public:
    enum ProcessingType
    {
        kFront_InitialLoad = 0,
        kFront_QuitToGameSetup = 1,
        kFront_QuitToPostGame = 2
    };
};

class Front
{
public:
    static void ConstructAll(void);
    static void DeleteAll(void);
    static void ResetOverlayScreenGraph(void);
    static void ResetOverlayMenuState(void);
    static void ResetOverlayCarSelect(void);
    static void ResetOverlayDemo(void);
    static void ResetOverlayCredits(void);
    static void ResetOverlayOptions(void);
    static void ResetOverlayAwards(void);
    static void ResetOverlayAudioScreen(void);
    static void ResetOverlayCongrats(void);
    static void ResetOverlayPinkSlips(void);
    static void ResetOverlayController(void);
    static void ResetSettingsForCar(sint32 player, sint32 car);
    static void ResetSettingsMenu(sint32 player);
    static void ResetPSXController(sint32 player, sint32 config);
    static void ResetPSXAnalogs(sint32 player);
    static void InitialMemCardCheck(void);
    static void SecondaryMemCardCheck(void);
    static void InitGraphics(void);
    static sint32 Menu(tFront::ProcessingType role);
    static sint32 GetLapsForType(void);
    static void InitStream(tFEStream &streamData);
    static void InitPlayerCars(tFEStream &streamData);
    static void InitTourneyTraffic(tFEStream &streamData);
    static void InitOpponentCars(tFEStream &streamData);
    static void InitMissions(tFEStream &streamData);
    static void InitCopCars(tFEStream &streamData);
    static void InitPerps(tFEStream &streamData);
    static void InitTrack(tFEStream &streamData);
    static void InitTraffic(tFEStream &streamData);
    static sint32 * AppendPlayerCarData(sint32 *stream, tFEStream &streamData);
    static sint32 * AppendOpponentData(sint32 *stream, tFEStream &streamData);
    static sint32 * AppendCopData(sint32 *stream, tFEStream &streamData);
    static sint32 * AppendPerpData(sint32 *stream, tFEStream &streamData);
    static sint32 * AppendTrafficData(sint32 *stream, tFEStream &streamData);
    static sint32 * AppendTrackData(sint32 *stream, tFEStream &streamData);
    static bool EnableLocalSpeech(void);
    static sint32 * BuildStream(sint32 *stream);
    static void GetInGameVars(void);
    static sint16 GetTrackRaced(void);
};

void SetPads(void);
bool PlayerNameExist(sint32 player);
sint32 GetPSXPadValue(sint32 value, sint32 player);
char *PlayerName(sint32 player);
char *PlayerNameMixedCase(sint32 player);
void SetLicensePlate(void);
void SetPlayList(sint32 ivealreadygotone);

extern sint32 ComingIntoTheFrontEndTheVeryFirstTime;

#endif
