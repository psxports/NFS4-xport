#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/pad.h"
#include "game/common/audiocmn.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/nfs3.h"
#include "game/psx/device.h"

class FEI
{
public:
    struct tList
    {
        sint32 v;
        sint32 *p;
    };

    static tList gList[114];
};

FEI::tList FEI::gList[114] =
{
    { 1, 0 },
    { 3, 0 },
    { 4, 0 },
    { 5, 0 },
    { 6, &gUseFrontend },
    { 7, &GameSetup::gData.raceType },
    { 8, &GameSetup::gData.numLaps },
    { 9, &GameSetup::gData.skill },
    { 10, &GameSetup::gData.commMode },
    { 11, &GameSetup::gData.tournamentMultiplier },
    { 12, &GameSetup::gData.cops },
    { 13, &GameSetup::gData.trafficDensity },
    { 14, &GameSetup::gData.localCar },
    { 15, &GameSetup::gData.catchupLogic },
    { 16, &GameSetup::gData.sgge },
    { 17, &GameSetup::gData.easter },
    { 18, &GameSetup::gData.track },
    { 19, &GameSetup::gData.song },
    { 20, &GameSetup::gData.Weather },
    { 21, &GameSetup::gData.Fog },
    { 22, &GameSetup::gData.Damage },
    { 23, &GameSetup::gData.Time },
    { 24, &GameSetup::gData.mirrorTrack },
    { 25, &GameSetup::gData.reverseTrack },
    { 26, &GameSetup::gData.measurement },
    { 27, &GameSetup::gData.randSeed },
    { 28, &GameSetup::gData.numCars },
    { 29, &GameSetup::gData.numPlayerRaceCars },
    { 30, &GameSetup::gData.numOpponentRaceCars },
    { 31, &GameSetup::gData.opponentCarType },
    { 32, &GameSetup::gData.checkpointType },
    { 33, &GameSetup::gData.pinkSlipsForfeit },
    { 34, &GameSetup::gData.dispatchSpeech },
    { 35, &GameSetup::gData.reverseCallSpeech },
    { 36, &GameSetup::gData.languageSpeech },
    { 37, &GameSetup::gData.numPerps },
    { 38, &GameSetup::gData.stageOffset },
    { 39, &GameSetup::gData.userSetting.language },
    { 40, &GameSetup::gData.userSetting.display },
    { 41, &GameSetup::gData.userSetting.bestlap },
    { 42, &GameSetup::gData.userSetting.audioMode },
    { 43, &GameSetup::gData.userSetting.musicLevel },
    { 44, &GameSetup::gData.userSetting.sfxLevel },
    { 45, &GameSetup::gData.userSetting.engineLevel },
    { 46, &GameSetup::gData.userSetting.speechLevel },
    { 47, &GameSetup::gData.userSetting.ambientLevel },
    { 48, &GameSetup::gData.userSetting.numplaylistsongs },
    { 49, &GameSetup::gData.userSetting.interactive },
    { 50, &GameSetup::gData.checkpointHUD[0] },
    { 51, &GameSetup::gData.checkpointHUD[1] },
    { 52, &GameSetup::gData.controllerData.controllerConfig[0] },
    { 53, &GameSetup::gData.controllerData.deadSpot[0] },
    { 54, &GameSetup::gData.controllerData.steeringRange[0] },
    { 55, &GameSetup::gData.controllerData.IImaxRange[0] },
    { 56, &GameSetup::gData.controllerData.ImaxRange[0] },
    { 57, &GameSetup::gData.controllerData.J1MAX[0] },
    { 58, &GameSetup::gData.controllerData.J1MIN[0] },
    { 59, &GameSetup::gData.controllerData.J2MAX[0] },
    { 60, &GameSetup::gData.controllerData.J2MIN[0] },
    { 61, &GameSetup::gData.controllerData.shockMode[0] },
    { 62, &GameSetup::gData.controllerData.shockImpact[0] },
    { 63, &GameSetup::gData.controllerData.controllerConfig[1] },
    { 64, &GameSetup::gData.controllerData.deadSpot[1] },
    { 65, &GameSetup::gData.controllerData.steeringRange[1] },
    { 66, &GameSetup::gData.controllerData.IImaxRange[1] },
    { 67, &GameSetup::gData.controllerData.ImaxRange[1] },
    { 68, &GameSetup::gData.controllerData.J1MAX[1] },
    { 69, &GameSetup::gData.controllerData.J1MIN[1] },
    { 70, &GameSetup::gData.controllerData.J2MAX[1] },
    { 71, &GameSetup::gData.controllerData.J2MIN[1] },
    { 72, &GameSetup::gData.controllerData.shockMode[1] },
    { 73, &GameSetup::gData.controllerData.shockImpact[1] },
    { 74, &GameSetup::gData.replayMode },
    { 76, &GameSetup::gData.userSetting.playlist[0] },
    { 259, 0 },
    { 260, &GameSetup::gData.carInfo[0].carType },
    { 261, &GameSetup::gData.carInfo[0].carClass },
    { 262, &GameSetup::gData.carInfo[0].Transmission },
    { 263, &GameSetup::gData.carInfo[0].RampSteering },
    { 264, &GameSetup::gData.carInfo[0].RampGas },
    { 265, &GameSetup::gData.carInfo[0].RampBrake },
    { 266, &GameSetup::gData.carInfo[0].Colour },
    { 267, &GameSetup::gData.carInfo[0].ColourSV },
    { 268, &GameSetup::gData.carInfo[0].ColourChange },
    { 269, &GameSetup::gData.carInfo[0].GroundEffects },
    { 270, &GameSetup::gData.carInfo[0].BrakeBias },
    { 271, &GameSetup::gData.carInfo[0].GearRatio },
    { 272, &GameSetup::gData.carInfo[0].EngineMods },
    { 273, &GameSetup::gData.carInfo[0].ABS },
    { 274, &GameSetup::gData.carInfo[0].WeightTransfer },
    { 275, &GameSetup::gData.carInfo[0].TireType },
    { 276, &GameSetup::gData.carInfo[0].carNameIndex },
    { 277, &GameSetup::gData.carInfo[0].Traction },
    { 278, &GameSetup::gData.carInfo[0].BrakeAssist },
    { 279, &GameSetup::gData.carInfo[0].Bestline },
    { 280, &GameSetup::gData.carInfo[0].Personality },
    { 281, &GameSetup::gData.carInfo[0].StartingPos },
    { 282, &GameSetup::gData.carInfo[0].HudSpeed },
    { 283, &GameSetup::gData.carInfo[0].HudTach },
    { 284, &GameSetup::gData.carInfo[0].HudMap },
    { 285, &GameSetup::gData.carInfo[0].HudOpponentID },
    { 286, &GameSetup::gData.carInfo[0].HudTime },
    { 287, &GameSetup::gData.carInfo[0].HudLapnum },
    { 288, &GameSetup::gData.carInfo[0].HudPosition },
    { 289, &GameSetup::gData.carInfo[0].HudMirror },
    { 290, &GameSetup::gData.carInfo[0].HudColour },
    { 291, &GameSetup::gData.carInfo[0].HudTachColour },
    { 292, &GameSetup::gData.carInfo[0].SpeechColour },
    { 293, &GameSetup::gData.carInfo[0].Country },
    { 294, &GameSetup::gData.carInfo[0].Camera[0] },
    { 295, &GameSetup::gData.carInfo[0].Camera[1] },
    { 296, &GameSetup::gData.carInfo[0].Camera[2] },
    { 297, &GameSetup::gData.carInfo[0].Camera[3] },
    { 0, 0 },
};

GameSetup::tData GameSetup::gData;

char GameSetup::gPersonalityNames[15][8] = {
    "Nemesis",
    "Bullit",
    "Frost",
    "Ranger",
    "Chump",
    "Snake",
    "Razor",
    "Thunder",
    "Roadhog",
    "Clutch",
    "Scooter",
    "WndrBoy",
    "Flash",
    "KikBut",
    "Laser",
};

char GameSetup::gCarNames[51][5] = {
    "MSLK",
    "BMWZ",
    "HSVT",
    "FORD",
    "CMRO",
    "TRAM",
    "ADB7",
    "JXKR",
    "BMWM",
    "CORV",
    "F550",
    "P993",
    "F375",
    "DIAB",
    "MCLK",
    "MCF1",
    "X993",
    "X375",
    "XCF1",
    "BNS1",
    "BNS2",
    "BNS2",
    "CCPR",
    "CHSV",
    "CBMW",
    "CCOV",
    "CCMR",
    "CDBL",
    "CHOP",
    "GUAT",
    "BMW5",
    "MERC",
    "WOLW",
    "SBUS",
    "SAND",
    "RIAT",
    "ARMY",
    "COMC",
    "TON1",
    "TON2",
    "VAN",
    "BEEP",
    "PKUP",
    "PKUP",
    "F500",
    "CUCA",
    "TAXI",
    "TBUS",
    "LOGT",
    "CBUS",
    "BAD!",
};

char GameSetup::gTrackNames[12][4] = {
    "ALP",
    "HWY",
    "CST",
    "FRN",
    "PAR",
    "HIL",
    "GER",
    "ENG",
    "GT1",
    "GT2",
    "GT3",
    "BAD",
};

void GameSetup::StartUp(sint32 *FrontEndDataStream)
{
    FUNCTION_MARKER(0x8009BD9C, "NFS4.EXE");
    Input::StartUp();
    sint32 *h = FrontEndDataStream;
    sint32 *s = h;
    while (*s != 0) {
        sint32 command = *s;
        if (command >= 0x103) {
            sint32 *p = 0;
            for (FEI::tList *l = FEI::gList; l->v != 0; ++l)
                if (l->v == command)
                    p = l->p;
            if (p != 0) {
                ptrdiff_t fieldOffset = reinterpret_cast<uint8 *>(p) - reinterpret_cast<uint8 *>(&gData.carInfo[0]);
                sint32 *field = reinterpret_cast<sint32 *>(reinterpret_cast<uint8 *>(&gData.carInfo[s[1]]) + fieldOffset);
                *field = s[2];
            }
            s += 3;
        } else if (command >= 0x4d) {
            Input::gHandler[command - 0x4d] = s[1];
            sint32 handler = s[1];
            sint32 (*startupfunc)(uint32) = Device::gDeviceList[handler & 0xff].startupfunc;
            if (startupfunc != 0)
                startupfunc(handler >> 8);
            s += 2;
        } else {
            sint32 *p = 0;
            for (FEI::tList *l = FEI::gList; l->v != 0; ++l)
                if (l->v == command)
                    p = l->p;
            if (p != 0) {
                if (*s < 0x4b) *p = s[1];
                else
                    for (sint32 i = 0; i < s[1]; ++i) *p++ = s[i + 2];
            }
            if (*s >= 0x4b)
                s += s[1];
            s += 2;
        }
    }
    purgememadr(h);
    timedwait(100);
    PAD_update();
    if (PAD_state(0) == 0x0909) gData.sgge = 8;
    if (PAD_state(0) == 0xa080) gData.sgge = 2;
    if (PAD_state(0) == 0xc001) gData.sgge = 4;
    if (PAD_state(0) == 0x0640) gData.sgge = 0x10;
    if (PAD_state(0) == 0x0610) gData.sgge = 0x20;
    if (PAD_state(0) == 0x0910) gData.sgge = 0x40;
    if (PAD_state(0) == 0x5010) gData.sgge = 0x80;
    if (PAD_state(0) == 0x0620) gData.sgge = 0x100;
    gMasterMusicLevel = gData.userSetting.musicLevel;
    gMasterSFXLevel = gData.userSetting.sfxLevel;
    gMasterFENarrationLevel = gData.userSetting.speechLevel;
    gMasterEngineLevel = gData.userSetting.engineLevel;
    gMasterAmbientLevel = gData.userSetting.ambientLevel;
    Device::SetHardCodedKeys();
}

void GameSetup::CleanUp(void)
{
    FUNCTION_MARKER(0x8009C120, "NFS4.EXE");
}
