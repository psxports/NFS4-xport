#ifndef AUDIOCMN_H
#define AUDIOCMN_H

#include "xport.h"
#include "game/common/car.h"

struct SndBnk_t
{
    sint32 bnkID;
    char *phdr;
    char *pdata;
};

enum s_type
{
    kAsphalt = 0,
    kCarBody = 1,
    kTires = 2,
    kGravel = 3,
    kWall = 4,
    kSign = 5,
    kBush = 6,
    kBlowOut = 7,
    kBackFire = 8,
    kHorn = 9,
    kGearShift = 10,
    kSnow = 11,
    kGrass = 12,
    kWoodWall = 13,
    kGrassWall = 14,
    kWallSpark = 15,
    kWetAsphalt = 16,
    kRaceTireOnAsphalt = 17,
    kRainTireOnAsphalt = 18
};

class AudioCmn
{
public:
    struct tReTrig;

    struct tAsyncSfxSlot;

    static sint32 gCursorSndHandle;
    static sint32 gLastFade;
    static sint32 gResume;
    static sint32 gStreamRestartTimer;
    static sint32 ThunderAmp;
    static sint32 ThunderAzi;
    static sint32 ThunderDel;
    static sint32 kAudioOn;
    static sint32 kAudioStreamingOn;
    static sint32 musicInteractive;
    static sint32 gPlayerArrested[2];
    static char * LanguageName[7];
    static tReTrig gReTrig[2];
    static tAsyncSfxSlot gSfxSlot[32];
    static void * gLoadTables;
    static void * gCruiseTables;

    static sint32 MusicLevel(sint32 level);
    static sint32 GetTrackRecordLapTime(void);
    static void InitThunder(void);
    static void PlayThunder(sint32 intensity, sint32 azimuth);
    static void UpdateThunder(void);
    static void InitAsyncSfx(void);
    static void RemoveAsyncSfx(sint32 slot);
    static void DeInitAsyncSfx(void);
    static sint32 RemoveOldestAsyncSfx(sint32 bank);
    static void LoadAsyncSfx(sint32 bank, sint32 patch, void *pbank, sint32 size);
    static sint32 GetAsyncSfx(sint32 bank, sint32 patch, bool checkonly);
    static void Init(void);
    static void Reset(void);
    static void DeInit(void);
    static void SetLevels(void);
    static sint32 GetTimePhrase(sint32 time);
    static void CheckState(Car::tObj *car);
    static void LoadFESamples(void);
    static void LoadGameSamples(void);
    static void InitChannelArray(void);
    static void SFX(sint32 sndPlayer, s_type surface1, s_type surface2, sint32 tweakedForce, sint32 Distsq, sint32 azimuth);
    static sint32 PlayDoppleredSound(sint32 bhandle, sint32 patchNum, sint32 azimuth, sint32 vol, sint32 bend, sint32 doppler);
    static sint32 PlaySound(sint32 bhandle, sint32 patchNum, sint32 azimuth, sint32 vol, sint32 bend);
    static sint32 PlaySFX(sint32 sndPlayer, sint32 iSFXnum, sint32 iFreqIn, sint32 iDopplerIn, sint32 iAmpIn, sint32 azimuth);
    static void SoundCar(Car::tObj *car, sint32 dst, sint32 iFreqIn, sint32 doppler, sint32 azimuth, sint32 trackazim, sint32 relvel, sint32 cardir);
    static void TrafficSFX(sint32 iChan, sint32 iSFXnum, sint32 freq, sint32 doppler, sint32 dst, sint32 azimuth, sint32 relvel, sint32 dir);
    static void TrafficSkidSFX(sint32 sndPlayer, s_type surface1, s_type surface2, sint32 force, sint32 Distsq, sint32 azimuth);
    static void PlayerHornOn(sint32 carIndex, sint32 Distsq, sint32 iFreqIn, sint32 azimuth, sint32 doppler);
    static sint32 PlayerHornOff(sint32 carIndex);
    static void PlayFESFX(sint32 SFXnum);
    static void PlayFESFXVol(sint32 SFXnum, sint32 vol);
    static void PlayWrongWaySFX(void);
    static void PlayPauseSound(sint32 patch);
    static void Pause(void);
    static void UnPause(void);
    static void UnPauseAndQuit(void);
    static void UnPauseAndRestart(void);
    static void InitReverb(void);
    static void ReverbOff(void);
    static sint32 AddBank(char *filename, sint32 size, char *pdata, sint32 BankNum);
    static sint32 LoadBank(char *filename, sint32 BankNum);
};

extern SndBnk_t gSndBnk[7];

void freeVoiceChannel(sint32 sndPlayer);
void quickSirenOn(sint32 sirennum);
void SuperCopSirenOn(sint32 sirennum);
void SirenOn(sint32 sirennum, sint32 supercop);
void SirenOff(sint32 sirennum);
void UpdateSiren(sint32 sirennum, sint32 amp, sint32 dop, sint32 azimuth, sint32 supercop);

extern sint32 gStereoMode;
extern sint32 gMasterFENarrationLevel;
extern sint32 gMasterMusicLevel;
extern sint32 gMasterSFXLevel;
extern sint32 gMasterEngineLevel;
extern sint32 gMasterAmbientLevel;
extern sint32 bSirenOn[6];
extern char Xfade[129];
extern char countdown;

#endif
