#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/splysdef.h"
#include "game/common/audiocmn.h"
#include "game/common/audioeng.h"
#include "game/common/audiomus.h"
#include "game/common/audiotrk.h"
#include "game/common/bworldsm.h"
#include "game/common/cameraobj.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/copspeak.h"
#include "game/common/gmesetup.h"
#include "game/common/paths.h"
#include "game/common/replay.h"
#include "game/common/sim.h"
#include "game/common/stats.h"
#include "game/psx/audio.h"
#include "game/psx/weather.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/random.h"

struct AudioCmn::tReTrig
{
    sint32 count;
    sint32 pan;
    sint16 delay;
    sint16 nextd;
    sint32 patch;
    char bend;
    char amp;
    coorddef pos;
};

struct Channels_t
{
    sint32 Partial;
    sint32 SFXnum;
};

struct AudioCmn::tAsyncSfxSlot
{
    sint32 bank;
    sint32 patch;
    sint32 handle;
    sint32 ticks;
    sint32 addr;
    char *header;
};

static char SkidInitMaxFreq[71] = { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static sint32 PlayersRampedGasLevel[2];
sint32 falseLapTrigCur;
sint32 flaseLapTrigTrack;
sint32 currentWindPan;
sint32 nextWindPan;
sint32 gQuickSirenCount;
sint32 gMasterFENarrationLevel = 127;
sint32 gMasterMusicLevel = 127;
sint32 gMasterSFXLevel = 127;
sint32 gMasterEngineLevel = 127;
sint32 gMasterAmbientLevel = 127;
sint32 gStereoMode = 1;
char trackMusicState = 4;
sint32 audioBackwardsDirection;
sint32 intensityFalseLapCounter;
sint32 falseLapCounter;
sint32 gMusicStyle;
char FadingMusic;
char StartedNewMusic;
sint32 recordLapTime;
sint32 SgameSFXvol = 40;
sint32 previousSFXLevel = 127;
sint32 gFEmusicON = 1;
sint32 NumSFXOn;
char fReverbOn;
char fReverbLevel;
char carbankname[12];
sint32 gBankNumLookupTable[71] = { 0, 2, 0, 2, 0, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 5, 5, 3, 3, 3, 3, 5, 2, 2, 2, 2, 0, 1, 2, 2, 2, 2, 0, 0, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0 };

sint32 falseLapTrigNumsForward[10][2] =
{
    { 4, 7 },
    { 4, 7 },
    { 4, 7 },
    { -1, -1 },
    { 4, 7 },
    { 4, 9 },
    { 4, 9 },
    { -1, -1 },
    { -1, -1 },
    { 4, 9 }
};

sint32 falseLapTrigNumsBackward[10][2] =
{
    { 4, 5 },
    { 4, 5 },
    { -1, -1 },
    { -1, -1 },
    { 4, 5 },
    { 4, 5 },
    { 4, 5 },
    { -1, -1 },
    { -1, -1 },
    { 4, 5 }
};

char Xfade[129] = { 0, 3, 7, 10, 13, 16, 19, 22, 24, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 46, 48, 50, 51, 53, 54, 55, 57, 58, 60, 61, 62, 63, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 84, 85, 86, 87, 88, 88, 89, 90, 91, 91, 92, 93, 94, 94, 95, 96, 96, 97, 98, 98, 99, 100, 100, 101, 101, 102, 103, 103, 104, 104, 105, 106, 106, 107, 107, 108, 108, 109, 109, 110, 110, 111, 111, 112, 112, 113, 113, 114, 114, 115, 115, 116, 116, 117, 117, 118, 118, 119, 119, 119, 120, 120, 121, 121, 122, 122, 122, 123, 123, 124, 124, 125, 125, 125, 126, 127, 127 };
sint32 bSirenOn[6];
sint32 bSirenPitchingUp[6] = { 1, 1, 1, 1, 1, 1 };
sint32 quickSirenActive[6];
sint32 sirenPitchWidth[6] = { 32, 40, 28, 42, 28, 42 };
sint32 sirenCurrentPitch[6] = { 32, 32, 32, 32, 32, 32 };
sint32 slowSirenReps[6] = { 3, 3, 3, 3, 3, 3 };
sint32 sirenCount[6];
sint32 reachedSirenMin[6];
sint32 quickSirenTimeCount[6];
char currentLap[2];
sint32 bestLapTime[2];
sint32 gtotallaptimes[2];
char fMysticWindON[2];
char fAmbientRangeON[2];
sint32 currentWindVal[2];
sint32 nextWindVal[2];
static char xport_AudioCmn_LanguageName_string_0[] = "eng";
static char xport_AudioCmn_LanguageName_string_1[] = "ger";
static char xport_AudioCmn_LanguageName_string_2[] = "frn";
static char xport_AudioCmn_LanguageName_string_3[] = "spn";
static char xport_AudioCmn_LanguageName_string_4[] = "itl";
char countdown;
Channels_t gaChannel[71];
SndBnk_t gSndBnk[7];
sint32 AudioCmn::gCursorSndHandle;
sint32 AudioCmn::gLastFade;
sint32 AudioCmn::gResume;
sint32 AudioCmn::gStreamRestartTimer;
sint32 AudioCmn::ThunderAmp;
sint32 AudioCmn::ThunderAzi;
sint32 AudioCmn::ThunderDel;
sint32 AudioCmn::kAudioOn = 1;
sint32 AudioCmn::kAudioStreamingOn = 1;
sint32 AudioCmn::musicInteractive = 1;
sint32 AudioCmn::gPlayerArrested[2];
char *AudioCmn::LanguageName[7] = { xport_AudioCmn_LanguageName_string_0, xport_AudioCmn_LanguageName_string_1, xport_AudioCmn_LanguageName_string_2, xport_AudioCmn_LanguageName_string_3, xport_AudioCmn_LanguageName_string_4, xport_AudioCmn_LanguageName_string_0, xport_AudioCmn_LanguageName_string_0 };
void *AudioCmn::gLoadTables;
void *AudioCmn::gCruiseTables;
AudioCmn::tReTrig AudioCmn::gReTrig[2];
AudioCmn::tAsyncSfxSlot AudioCmn::gSfxSlot[32];

sint32 AudioCmn::MusicLevel(sint32 level)
{
    FUNCTION_MARKER(0x80076420, "NFS4.EXE");
    sint32 curve = level;
    if (level >= 86) {
        sint32 scaled = (sint32)(((uint32)level - 85) * 7);
        curve = scaled / 2 + 85;
    }
    return (sint32)((uint32)curve * 70) >> 7;
}

sint32 AudioCmn::GetTrackRecordLapTime(void)
{
    FUNCTION_MARKER(0x80076480, "NFS4.EXE");
    return GameSetup::gData.userSetting.bestlap;
}

void AudioCmn::InitThunder(void)
{
    FUNCTION_MARKER(0x80076490, "NFS4.EXE");
    ThunderAmp = 0;
    ThunderAzi = 0;
    ThunderDel = 0;
}

void AudioCmn::PlayThunder(sint32 intensity, sint32 azimuth)
{
    FUNCTION_MARKER(0x800764A4, "NFS4.EXE");
    if (ThunderAmp < intensity) {
        sint32 delay = (sint32)(135 - (uint32)intensity);
        ThunderAmp = intensity;
        ThunderAzi = azimuth;
        ThunderDel = delay;
    }
}

void AudioCmn::UpdateThunder(void)
{
    FUNCTION_MARKER(0x800764D0, "NFS4.EXE");
    sint32 amp = ThunderAmp;
    if (amp <= 0)
        return;
    ThunderDel = (sint32)((uint32)ThunderDel - 1);
    if (ThunderDel >= 0)
        return;
    PlaySound(gSndBnk[3].bnkID, 0x16, ThunderAzi, amp, 0x40);
    if (ThunderAmp < 46) {
        uint32 draw = random();
        ThunderAmp = (sint32)((uint32)ThunderAmp + 2 - (draw & 7));
    } else {
        uint32 draw = random();
        ThunderAmp = (sint32)((uint32)ThunderAmp + 3 - (draw & 15));
    }
    ThunderDel = (random() + 20) & 3;
}

void AudioCmn::InitAsyncSfx(void)
{
    FUNCTION_MARKER(0x8007657C, "NFS4.EXE");
    for (sint32 i = 0; i < 32; ++i) {
        gSfxSlot[i].patch = -1;
        gSfxSlot[i].handle = -1;
        gSfxSlot[i].header = 0;
    }
}

void AudioCmn::RemoveAsyncSfx(sint32 slot)
{
    FUNCTION_MARKER(0x800765B4, "NFS4.EXE");
    tAsyncSfxSlot *s = &gSfxSlot[slot];
    if (s->patch != -1) {
        if (s->handle != -1) {
            cSNDbankremove(s->handle, 0);
            char *header = s->header;
            s->handle = -1;
            if (header != 0) {
                purgememadr(header);
                s->header = 0;
            }
        }
        s->patch = -1;
    }
}

void AudioCmn::DeInitAsyncSfx(void)
{
    FUNCTION_MARKER(0x8007663C, "NFS4.EXE");
    for (sint32 i = 0; i < 32; ++i)
        RemoveAsyncSfx(i);
}

sint32 AudioCmn::RemoveOldestAsyncSfx(sint32 bank)
{
    FUNCTION_MARKER(0x80076674, "NFS4.EXE");
    sint32 oldest = -1;
    sint32 ticks = (sint32)((uint32)simGlobal.gameTicks - 16);
    for (sint32 i = 0; i < 32; ++i) {
        if (gSfxSlot[i].patch != -1 && gSfxSlot[i].ticks < ticks) {
            ticks = gSfxSlot[i].ticks;
            oldest = i;
        }
    }
    if (oldest == -1) {
        for (sint32 i = 0; i < 32; ++i) {
            if (gSfxSlot[i].patch != -1 && gSfxSlot[i].bank < bank)
                oldest = i;
        }
    }
    if (oldest >= 0)
        RemoveAsyncSfx(oldest);
    return oldest;
}

void AudioCmn::LoadAsyncSfx(sint32 bank, sint32 patch, void *pbank, sint32 size)
{
    FUNCTION_MARKER(0x8007675C, "NFS4.EXE");
    sint32 slot;
    for (slot = 0; slot < 32; ++slot) {
        if (gSfxSlot[slot].patch == patch && gSfxSlot[slot].bank == bank && gSfxSlot[slot].handle == -1)
            break;
    }
    if (slot == 32)
        return;
    tAsyncSfxSlot *s = &gSfxSlot[slot];
    if (size != 0) {
        sint32 check;
        for (;;) {
            sint32 available = SNDmemlargestunused(&check);
            if (size <= (sint32)((uint32)available - 0x1000))
                break;
            if (RemoveOldestAsyncSfx(bank) == -1) {
#ifdef _DEBUG
                if (bank == 2)
                    fputs("out of SPU ram on speech!\n", stdout);
#endif
                s->handle = -1;
                s->patch = -1;
                return;
            }
        }
        check = SNDbankadd(&s->handle, pbank);
        if (check == 7) {
            sint32 headerSize = SNDbankheadersize(s->handle);
            s->header = (char *)reservememadr("SFXHDR", headerSize, 0x10);
            if (s->header != 0) {
                SNDbankheadercopy(s->header, s->handle);
                s->patch = patch;
                s->ticks = simGlobal.gameTicks;
                return;
            }
        }
#ifdef _DEBUG
        if (bank == 2)
            fputs("SNDbankadd failed on speech!\n", stdout);
#endif
        SNDbankheadersize(s->handle);
        if (check >= 0)
            cSNDbankremove(s->handle, 0);
    }
    s->handle = -1;
    s->patch = -1;
}

sint32 scaleFrequency(sint32 sndPlayer, sint32 iSFXnum, sint32 tweakedForce)
{
    FUNCTION_MARKER(0x800778E8, "NFS4.EXE");
    bool impact = (uint32)sndPlayer - 18 < 2 || (uint32)sndPlayer - 20 < 2;
    if (!impact && gaChannel[sndPlayer].SFXnum == iSFXnum)
        return (uint8)SkidInitMaxFreq[sndPlayer];
    sint32 scaledFreq = (sint32)((uint32)tweakedForce * 127) / 0xa0000;
    if (scaledFreq >= 128)
        scaledFreq = 127;
    if (!impact)
        SkidInitMaxFreq[sndPlayer] = (char)scaledFreq;
    return scaledFreq;
}

sint32 ChooseImpactSample(sint32 force, s_type surface1, s_type surface2)
{
    FUNCTION_MARKER(0x800779B4, "NFS4.EXE");
    static sint32 lastImpactSample = 99;
    if (surface1 == kBackFire)
        return 19;
    if (surface1 == kTires && surface2 == kAsphalt)
        return 31;
    if (surface1 == kCarBody) {
        if (surface2 == kWoodWall)
            return 30;
        if (surface2 == kGrassWall)
            return 29;
        if (surface2 == kWall) {
            if (force > 110)
                return 33;
            return (random() & 1) != 0 ? 37 : 32;
        }
        if (surface2 == kWallSpark) {
            switch (random() & 3) {
            case 0: return 35;
            case 1: return 34;
            case 2: return 32;
            default: return 37;
            }
        }
        if (surface2 == kSign)
            return 29;
    }
    if (surface2 == kBlowOut)
        return 18;
    sint32 iSFXnum;
    if (surface1 == kCarBody && (surface2 == kAsphalt || surface2 == kGravel)) {
        if (force >= 81)
            iSFXnum = 33;
        else if (force >= 23)
            iSFXnum = 34;
        else if (force >= 17)
            iSFXnum = 32;
        else
            iSFXnum = 29;
        if (iSFXnum != lastImpactSample)
            return iSFXnum;
        switch (iSFXnum) {
        case 29: return (random() & 1) != 0 ? 34 : 32;
        case 32: return (random() & 1) != 0 ? 29 : 34;
        case 34: return (random() & 1) != 0 ? 29 : 32;
        default: return iSFXnum;
        }
    }
    if (force >= 91)
        iSFXnum = 33;
    else if (force >= 71)
        iSFXnum = 32;
    else if (force >= 51)
        iSFXnum = 37;
    else if (force >= 26)
        iSFXnum = 34;
    else
        iSFXnum = 30;
    if (iSFXnum != lastImpactSample)
        return iSFXnum;
    switch (iSFXnum) {
    case 30: return 34;
    case 32: return (random() & 1) != 0 ? 37 : 34;
    case 34: return (random() & 1) != 0 ? 30 : 37;
    case 37: return (random() & 1) != 0 ? 30 : 34;
    default: return iSFXnum;
    }
}

sint32 ChooseLoopedSample(s_type surface1, s_type surface2)
{
    FUNCTION_MARKER(0x80077C94, "NFS4.EXE");
    sint32 SFXnum = 44;
    if (surface1 == kTires) {
        switch (surface2) {
        case kAsphalt: SFXnum = 42; break;
        case kWetAsphalt: SFXnum = 45; break;
        case kRaceTireOnAsphalt: SFXnum = 46; break;
        case kRainTireOnAsphalt: SFXnum = 47; break;
        case kSnow: SFXnum = 43; break;
        case kGravel: SFXnum = 41; break;
        default: break;
        }
    }
    if (surface1 == kCarBody && (surface2 == kAsphalt || surface2 == kWetAsphalt ||
        surface2 == kGravel || surface2 == kSnow))
        SFXnum = 40;
    return SFXnum;
}

sint32 AudioCmn::GetAsyncSfx(sint32 bank, sint32 patch, bool checkonly)
{
    FUNCTION_MARKER(0x80076900, "NFS4.EXE");
    sint32 slot;
    for (slot = 0; slot < 32; ++slot) {
        tAsyncSfxSlot *s = &gSfxSlot[slot];
        if (s->patch == patch && s->bank == bank) {
            s->ticks = simGlobal.gameTicks;
            return s->handle;
        }
    }
    for (slot = 0; slot < 32; ++slot) {
        if (gSfxSlot[slot].patch == -1)
            break;
    }
    if (!checkonly && slot == 32)
        slot = RemoveOldestAsyncSfx(bank);
    if ((uint32)slot >= 32)
        return -1;
    if (!checkonly) {
        if (CopSpeak::SfxQueued() >= 6)
            return -1;
        CopSpeak::tRequest r;
        CopSpeak::InitRequest(&r);
        r.phrase = patch;
        r.sfx = 1;
        r.bank = (char)bank;
        if (CopSpeak::Request(&r) == -1)
            return -1;
    }
    gSfxSlot[slot].bank = bank;
    gSfxSlot[slot].patch = patch;
    gSfxSlot[slot].ticks = simGlobal.gameTicks;
    return -1;
}

void AudioCmn::Init(void)
{
    FUNCTION_MARKER(0x80076A74, "NFS4.EXE");
    if (kAudioOn != 0) {
        InitChannelArray();
        sint32 temptrack = GameSetup::gData.track;
        audioBackwardsDirection = GameSetup::gData.reverseTrack;
        if ((temptrack & 0x10) != 0)
            temptrack = (temptrack & 0xf) + 5;
        sint32 (*triggers)[2] = audioBackwardsDirection == 0 ?
            falseLapTrigNumsForward : falseLapTrigNumsBackward;
        falseLapTrigCur = triggers[temptrack][0];
        flaseLapTrigTrack = triggers[temptrack][1];
        intensityFalseLapCounter = 0;
        falseLapCounter = 0;
    }
    for (sint32 j = 0; j < 2; ++j) {
        gReTrig[j].count = 0;
        fAmbientRangeON[j] = 0;
        fMysticWindON[j] = 0;
        currentLap[j] = 0;
        bestLapTime[j] = 0;
        PlayersRampedGasLevel[j] = 0;
        gtotallaptimes[j] = 0x200;
        gPlayerArrested[j] = 0;
    }
    InitThunder();
    InitAsyncSfx();
    AudioTrk::StartUp();
    fReverbOn = 0;
    fReverbLevel = 0;
    FadingMusic = 0;
    StartedNewMusic = 0;
    recordLapTime = GetTrackRecordLapTime();
    trackMusicState = 4;
    gLastFade = 127;
    gResume = 0;
    gStreamRestartTimer = 0;
    gQuickSirenCount = 0;
    GameSetup::gData.userSetting.sfxLevel = gMasterSFXLevel;
}

void AudioCmn::Reset(void)
{
    FUNCTION_MARKER(0x80076BEC, "NFS4.EXE");
    CopSpeak::SilenceCop(0, 0);
    CopSpeak::Cancel();
    SPCH::ClearEventQueue();
    DeInitAsyncSfx();
    for (sint32 i = 0; i < 71; ++i) {
        if (gaChannel[i].Partial != -1) {
            SNDstop(gaChannel[i].Partial);
            gaChannel[i].Partial = -1;
            gaChannel[i].SFXnum = -1;
        }
    }
    if (fReverbOn != 0)
        ReverbOff();
    Init();
    if (gMasterAmbientLevel > 0) {
        sint32 ticks = gettick();
        sint32 deadline = (sint32)((uint32)ticks + 0x280);
        sint32 goodtogo = 0;
        while (goodtogo == 0 && gettick() < deadline) {
            goodtogo = 1;
            CopSpeak::Server();
            systemtask(0);
            sint32 check;
            if (SNDmemlargestunused(&check) > 0x8000) {
                if (GameSetup::gData.raceType == 1) {
                    for (sint32 i = 0; i < 4; ++i) {
                        if (GetAsyncSfx(2, i + 47, false) == -1)
                            goodtogo = 0;
                    }
                }
                if (GameSetup::gData.Weather == 1 && GetAsyncSfx(1, 0, false) == -1)
                    goodtogo = 0;
                for (sint32 i = 0; i < GameSetup::gData.numCars; ++i) {
                    if (GameSetup::gData.carInfo[i].carClass == 2) {
                        sint32 patch = CopSpeak::GetEnginePatch(GameSetup::gData.carInfo[i].carType, 0);
                        if (patch >= 0 && GetAsyncSfx(1, patch, false) == -1)
                            goodtogo = 0;
                        patch = CopSpeak::GetEnginePatch(GameSetup::gData.carInfo[i].carType, 1);
                        if (patch >= 0 && GetAsyncSfx(1, patch, false) == -1)
                            goodtogo = 0;
                    }
                }
            }
        }
    }
    AudioTrk::PreLoad();
    if (gMasterMusicLevel == 0)
        return;
    AudioMus::Volume(MusicLevel(gMasterMusicLevel));
    SNDSTRM::setpriority(gMusicHandle, 255, 255);
    sint32 ticks = gettick();
    sint32 deadline = (sint32)((uint32)ticks + 0x100);
    sint32 earliest = (sint32)((uint32)ticks + 0x40);
    gettick();
    AudioMus::Buffered();
    AudioMus::Threshold();
    for (;;) {
        bool wait = false;
        if (gettick() < earliest || AudioMus::Threshold() <= 0) {
            wait = gettick() < deadline;
        } else {
            sint32 buffered = AudioMus::Buffered();
            sint32 threshold = AudioMus::Threshold();
            if (buffered < threshold)
                wait = gettick() < deadline;
        }
        if (!wait)
            break;
        systemtask(0);
    }
    sint32 buffered = AudioMus::Buffered();
    sint32 threshold = AudioMus::Threshold();
    if (buffered < (sint32)((uint32)threshold - 100)) {
        GameSetup::gData.userSetting.musicLevel = 0;
        gMasterMusicLevel = 0;
        AudioMus::Volume(MusicLevel(0));
    } else {
        gettick();
    }
    gettick();
    AudioMus::Buffered();
    AudioMus::Threshold();
}

void AudioCmn::DeInit(void)
{
    FUNCTION_MARKER(0x80076F44, "NFS4.EXE");
    AudioEng::StopServer();
    AudioEng::CleanUp();
    CopSpeak::Cancel();
    gMasterSFXLevel = GameSetup::gData.userSetting.sfxLevel;
    gMasterMusicLevel = GameSetup::gData.userSetting.musicLevel;
    AudioTrk::CleanUp();
    DeInitAsyncSfx();
    for (sint32 i = 0; i < 71; ++i) {
        if (gaChannel[i].Partial != -1) {
            SNDstop(gaChannel[i].Partial);
            gaChannel[i].Partial = -1;
            gaChannel[i].SFXnum = -1;
        }
    }
    SNDstopall();
    cSNDbankremove(-1, 0);
    Audio::CleanUp();
    if (fReverbOn != 0)
        ReverbOff();
    kAudioStreamingOn = gFEmusicON;
}

void AudioCmn::SetLevels(void)
{
    FUNCTION_MARKER(0x8007701C, "NFS4.EXE");
    if (Replay::ReplayInterface.statsScreen != 0) {
        sint32 scaled = (sint32)((uint32)GameSetup::gData.userSetting.sfxLevel << 6);
        gLastFade = 0x20;
        gMasterSFXLevel = scaled / 127;
    }
}

sint32 AudioCmn::GetTimePhrase(sint32 time)
{
    FUNCTION_MARKER(0x8007706C, "NFS4.EXE");
    static sint8 compareTimes[25] = {
        30, 12, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4,
        -5, -6, -7, -8, -9, -10, -11, -12, -15, -20, -25, -30
    };
    sint32 seconds = (time < 0 ? (sint32)((uint32)time - 1) : time) >> 6;
    sint32 index = 0;
    while (index < 25 && seconds < compareTimes[index])
        ++index;
    return index + 53;
}

void AudioCmn::CheckState(Car::tObj *car)
{
    FUNCTION_MARKER(0x800770BC, "NFS4.EXE");
    uint8 carnum = (uint8)car->carIndex;
    if (GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) {
        if ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0)
            return;
        if (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0)
            return;
    }
    sint32 numLaps = GameSetup::gData.numLaps;
    if (numLaps == 1)
        return;
    sint32 carspeed = car->currentSpeed;
    if (GameSetup::gData.reverseTrack != 0)
        carspeed = (sint32)(0u - (uint32)carspeed);
    if (car->stats.lap < numLaps) {
        sint32 distance = fixedmult(carspeed, 0x50000);
        sint32 nextLapSlice = (sint32)(((uint32)car->stats.lap + 1) * (uint32)gNumSlices);
        sint32 projectedSlice = (sint32)((uint32)car->stats.sliceTotal + (uint32)(distance / 0x60000));
        if (nextLapSlice < projectedSlice) {
            if (recordLapTime == 0 || (sint32)((uint32)simGlobal.gameTicks - (uint32)gtotallaptimes[carnum]) < recordLapTime)
                GetAsyncSfx(2, 1, false);
            if (car->stats.lap != 0 &&
                (sint32)((uint32)simGlobal.gameTicks - (uint32)gtotallaptimes[carnum]) < bestLapTime[carnum])
                GetAsyncSfx(2, 0, false);
            sint32 opponents = Stats::GetNumOpponents();
            if (opponents > 1) {
                sint32 phrase;
                if (opponents < 3 && car->stats.checkpointDisplay != 0) {
                    phrase = GetTimePhrase((sint32)(0u - (uint32)car->stats.checkpointDifference));
                } else {
                    sint32 position = Stats::GetPosition(car);
                    if (position == opponents)
                        position = 8;
                    phrase = (sint32)((uint32)position + 79);
                }
                if (GameSetup::gData.commMode == 1) {
                    GetAsyncSfx(2, carnum + 51, false);
                    phrase = (sint32)((uint32)phrase + (carnum + 1) * 35);
                }
                GetAsyncSfx(2, phrase, false);
            }
            sint32 lap = car->stats.lap;
            numLaps = GameSetup::gData.numLaps;
            if (lap < (sint32)((uint32)numLaps - 1)) {
                sint32 phrase = lap < (sint32)((uint32)numLaps - 2) ? (sint32)((uint32)lap + 5) : 2;
                GetAsyncSfx(2, phrase, false);
            }
        }
    }
    if (car->lap == (uint8)currentLap[carnum] || gPlayerArrested[carnum] != 0)
        return;
    CopSpeak::tRequest r;
    CopSpeak::InitRequest(&r);
    bool saidplayer = false;
    if (GameSetup::gData.commMode == 1) {
        sint32 lap = car->stats.lap;
        sint32 completedTime;
        if (lap == 0) {
            completedTime = car->stats.lapTime;
        } else if ((uint32)lap - 1 < 4) {
            completedTime = car->stats.time[lap - 1];
        } else {
            uint32 timeOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, time)) + (((uint32)lap - 1) << 2);
            const uint8 *carBytes = reinterpret_cast<const uint8 *>(car);
            memcpy(&completedTime, carBytes + timeOffset, sizeof(completedTime));
        }
        if (completedTime < bestLapTime[car->carIndex]) {
            r.phrase = carnum + 51;
            CopSpeak::Request(&r);
            saidplayer = true;
        }
    }
    sint32 lap = car->stats.lap;
    sint32 completedTime;
    if (lap == 0) {
        completedTime = car->stats.lapTime;
    } else if ((uint32)lap - 1 < 4) {
        completedTime = car->stats.time[lap - 1];
    } else {
        uint32 timeOffset = (uint32)(offsetof(Car::tObj, stats) + offsetof(Car::tStats, time)) + (((uint32)lap - 1) << 2);
        const uint8 *carBytes = reinterpret_cast<const uint8 *>(car);
        memcpy(&completedTime, carBytes + timeOffset, sizeof(completedTime));
    }
    if (recordLapTime == 0 || completedTime < recordLapTime) {
        r.phrase = 1;
        bestLapTime[car->carIndex] = completedTime;
        recordLapTime = completedTime;
        CopSpeak::Request(&r);
    } else if (completedTime < bestLapTime[car->carIndex]) {
        r.phrase = 0;
        bestLapTime[car->carIndex] = completedTime;
        CopSpeak::Request(&r);
    }
    sint32 opponents = Stats::GetNumOpponents();
    if (opponents > 1) {
        sint32 position = Stats::GetPosition(car);
        if (car->lap < GameSetup::gData.numLaps) {
            if (opponents < 3 && car->stats.checkpointDisplay != 0) {
                sint32 phrase = GetTimePhrase((sint32)(0u - (uint32)car->stats.checkpointDifference));
                if (GameSetup::gData.commMode == 1 && !saidplayer &&
                    ((uint32)phrase - 59 < 3 || (uint32)phrase - 63 < 15)) {
                    r.phrase = carnum + 51;
                    CopSpeak::Request(&r);
                    saidplayer = true;
                }
                r.phrase = phrase;
            } else {
                r.phrase = position == opponents ? 87 : (sint32)((uint32)position + 78);
            }
            if (GameSetup::gData.commMode == 1 && !saidplayer)
                r.phrase = (sint32)((uint32)r.phrase + (carnum + 1) * 35);
        } else if (GameSetup::gData.commMode != 1 || saidplayer) {
            r.phrase = position == opponents && position >= 3 ? 18 : (sint32)((uint32)position + 10);
        } else if (position == opponents && position >= 3) {
            r.phrase = carnum == 0 ? 26 : 34;
        } else {
            r.phrase = (sint32)((uint32)position + (carnum == 0 ? 18 : 26));
        }
        CopSpeak::Request(&r);
    }
    lap = car->stats.lap;
    numLaps = GameSetup::gData.numLaps;
    if (lap < numLaps) {
        r.phrase = lap < (sint32)((uint32)numLaps - 1) ? (sint32)((uint32)lap + 4) : 2;
        CopSpeak::Request(&r);
    }
    if ((uint8)currentLap[carnum] == 0)
        bestLapTime[carnum] = (sint32)((uint32)simGlobal.gameTicks - (uint32)gtotallaptimes[carnum]);
    currentLap[carnum] = (char)car->lap;
    gtotallaptimes[carnum] = car->stats.lapTime;
    sint32 current = car->lap;
    falseLapCounter = current;
    intensityFalseLapCounter = current;
}

void AudioCmn::LoadFESamples(void)
{
    FUNCTION_MARKER(0x80077738, "NFS4.EXE");
    char filename[100];
    strcpy(filename, Paths_Paths[28]);
    strcat(filename, "fesfx");
    LoadBank(filename, 0);
}

void AudioCmn::LoadGameSamples(void)
{
    FUNCTION_MARKER(0x8007777C, "NFS4.EXE");
    AudioEng::StartUp(0, GameSetup::gCarNames[GameSetup::gData.carInfo[0].carType]);
    if (GameSetup::gData.commMode == 1)
        AudioEng::StartUp(1, GameSetup::gCarNames[GameSetup::gData.carInfo[1].carType]);
    AudioEng::StartServer();
    char filename[100];
    strcpy(filename, Paths_Paths[28]);
    strcat(filename, "Gen");
    const char *TrackGenBank[11] = {
        "eng", "brt", "eng", "fre", "eng", "brt",
        "ger", "brt", "eng", "eng", "eng"
    };
    strcat(filename, TrackGenBank[GameSetup::gData.track]);
    LoadBank(filename, 3);
    gSndBnk[5].bnkID = -2;
    gSndBnk[2].bnkID = -3;
}

void AudioCmn::InitChannelArray(void)
{
    FUNCTION_MARKER(0x800778B0, "NFS4.EXE");
    for (sint32 i = 0; i < 71; ++i) {
        gaChannel[i].Partial = -1;
        gaChannel[i].SFXnum = -1;
    }
    gCursorSndHandle = -1;
}

void AudioCmn::SFX(sint32 sndPlayer, s_type surface1, s_type surface2, sint32 tweakedForce, sint32 Distsq, sint32 azimuth)
{
    FUNCTION_MARKER(0x80077D50, "NFS4.EXE");
    if (kAudioOn == 0)
        return;
    sint32 amplitude = 0;
    if (Distsq < 0x1324)
        amplitude = (sint32)((0x1324u - (uint32)Distsq) * 127) / 0x1324;
    sint32 iSFXnumber;
    sint32 frequency;
    if (sndPlayer < 0) {
        sint32 force = (sint32)((uint32)tweakedForce * 127) / 0xa0000;
        if (force >= 128)
            force = 127;
        iSFXnumber = ChooseImpactSample(force, surface1, surface2);
        if (iSFXnumber == 31) {
            sint32 tempAmp = (sint32)((uint32)amplitude * (uint32)force) / 127;
            tempAmp = (sint32)((uint32)tempAmp << 1);
            if (tempAmp > 127)
                tempAmp = 127;
            PlaySFX(49, 31, 64, 0x10000, amplitude, azimuth);
            amplitude = tempAmp;
            iSFXnumber = 35;
        } else if (iSFXnumber == 18) {
            PlaySound(gSndBnk[3].bnkID, 18, azimuth, 127, 64);
            PlaySound(gSndBnk[3].bnkID, 22, azimuth, 127, 64);
            PlaySound(gSndBnk[3].bnkID, 35, azimuth, amplitude, 64);
            return;
        }
        sndPlayer = 49;
        frequency = 64;
    } else {
        if (tweakedForce <= 0) {
            freeVoiceChannel(sndPlayer);
            uint32 carIndex = (uint32)sndPlayer - 18;
            if (carIndex < 2) {
                freeVoiceChannel((sint32)((uint32)sndPlayer + 4));
                Cars::gList[carIndex]->audioDamageScrape = 0;
            }
            return;
        }
        iSFXnumber = ChooseLoopedSample(surface1, surface2);
        frequency = scaleFrequency(sndPlayer, iSFXnumber, tweakedForce);
        uint32 carIndex = (uint32)sndPlayer - 18;
        if (carIndex < 2) {
            sint32 force = (sint32)((uint32)tweakedForce * 127) / 0xa0000;
            if (force >= 128)
                force = 127;
            amplitude = (sint32)((uint32)amplitude * (uint32)force) >> 7;
            if (amplitude < 31) {
                freeVoiceChannel(sndPlayer + 4);
                Cars::gList[carIndex]->audioDamageScrape = 0;
            } else {
                Car::tObj *c = Cars::gList[carIndex];
                if (c->carInfo->carType < 28 &&
                    ((c->render.currentRoll > 0 && c->N.damage[7] > 0x1e0000) ||
                     (c->render.currentRoll < 0 && c->N.damage[3] > 0x1e0000))) {
                    PlaySFX(sndPlayer + 4, 40, (sint32)((uint32)frequency + 10),
                        0x10000, (sint32)((uint32)amplitude * 3), azimuth);
                    c->audioDamageScrape = amplitude;
                }
            }
        }
        if (iSFXnumber == 44) {
            frequency = (sint32)((uint32)frequency - 60);
            if (frequency < 0)
                frequency = 0;
        }
    }
    PlaySFX(sndPlayer, iSFXnumber, frequency, 0x10000, amplitude, azimuth);
}

void freeVoiceChannel(sint32 sndPlayer)
{
    FUNCTION_MARKER(0x800780D0, "NFS4.EXE");
    if (AudioCmn::kAudioOn == 0 || sndPlayer == -1)
        return;
    if (gaChannel[sndPlayer].Partial != -1) {
        SNDautovol(gaChannel[sndPlayer].Partial, 5, -1);
        gaChannel[sndPlayer].Partial = -1;
        gaChannel[sndPlayer].SFXnum = -1;
        NumSFXOn = (sint32)((uint32)NumSFXOn - 1);
    }
}

sint32 AudioCmn::PlayDoppleredSound(sint32 bhandle, sint32 patchNum, sint32 azimuth, sint32 vol, sint32 bend, sint32 doppler)
{
    FUNCTION_MARKER(0x80078140, "NFS4.EXE");
    SNDPLAYOPTS playopts;
    SNDplaysetdef(&playopts);
    playopts.bhandle = (char)bhandle;
    sint32 volume;
    if (patchNum == 125) {
        volume = (sint32)((uint32)gMasterSFXLevel * (uint32)vol) >> 7;
    } else if (bhandle == -4) {
        volume = (sint32)((uint32)gMasterFENarrationLevel * 129) >> 7;
    } else if (bhandle == gSndBnk[2].bnkID) {
        volume = (sint32)((uint32)gMasterAmbientLevel * ((uint32)vol * 39)) >> 14;
    } else if (bhandle == gSndBnk[5].bnkID) {
        volume = (sint32)((uint32)gMasterAmbientLevel * ((uint32)vol * 130)) >> 14;
    } else if (patchNum == 22 || patchNum == 18) {
        volume = (sint32)((uint32)gMasterSFXLevel * (uint32)vol) >> 7;
    } else if (patchNum == 3 && (bhandle == gSndBnk[0].bnkID || bhandle == gSndBnk[2].bnkID)) {
        volume = (sint32)((uint32)gMasterSFXLevel * ((uint32)vol * 127)) >> 14;
    } else if (patchNum == 0) {
        volume = (sint32)((uint32)gMasterEngineLevel * ((uint32)vol * 14)) >> 14;
    } else {
        volume = (sint32)((uint32)gMasterSFXLevel * ((uint32)vol * 39)) >> 14;
    }
    playopts.vol = (char)volume;
    playopts.patnum = patchNum == 99 ? 1 : patchNum;
    playopts.bend = (char)bend;
    playopts.pitchmult = (uint16)(doppler >> 4);
    playopts.use3dpos = Audio::direct3davail != 0;
    if (Audio::direct3davail != 0) {
        playopts.azimuth = (uint16)azimuth;
    } else if (gStereoMode == 0) {
        playopts.pan = 64;
    } else if ((uint32)azimuth - 0x4000 <= 0x7fff) {
        playopts.pan = (char)((sint32)(0xbfff - (uint32)azimuth) >> 8);
    } else {
        playopts.pan = (char)(((uint32)azimuth + 0x4000) >> 8);
    }
    if ((sint8)playopts.bhandle < -1) {
        sint32 bank = (sint8)playopts.bhandle == -4 ? 2 : ((sint8)playopts.bhandle == -3 ? 1 : 0);
        playopts.bhandle = (char)GetAsyncSfx(bank, patchNum, false);
        playopts.patnum = 0;
    }
    sint32 shandle = -1;
    if ((sint8)playopts.bhandle >= 0)
        shandle = SNDplay(&playopts);
    NumSFXOn = (sint32)((uint32)NumSFXOn + 1);
    return shandle;
}

sint32 AudioCmn::PlaySound(sint32 bhandle, sint32 patchNum, sint32 azimuth, sint32 vol, sint32 bend)
{
    FUNCTION_MARKER(0x800783A0, "NFS4.EXE");
    return PlayDoppleredSound(bhandle, patchNum, azimuth, vol, bend, 0x10000);
}

void quickSirenOn(sint32 sirennum)
{
    FUNCTION_MARKER(0x8007971C, "NFS4.EXE");
    sint32 patch = (sirennum & 1) != 0 ? 6 : 8;
    sint32 handle = AudioCmn::PlaySound(gSndBnk[3].bnkID, patch, 64, 0, sirenCurrentPitch[sirennum]);
    gaChannel[sirennum + 43].Partial = handle;
}

void SuperCopSirenOn(sint32 sirennum)
{
    FUNCTION_MARKER(0x80079790, "NFS4.EXE");
    sint32 handle = AudioCmn::PlaySound(gSndBnk[3].bnkID, 9, 64, 0, sirenCurrentPitch[sirennum]);
    gaChannel[sirennum + 43].Partial = handle;
}

void SirenOn(sint32 sirennum, sint32 supercop)
{
    FUNCTION_MARKER(0x800797F8, "NFS4.EXE");
    if (bSirenOn[sirennum] != 0)
        return;
    bSirenOn[sirennum] = 1;
    quickSirenActive[sirennum] = 1;
    if (supercop == 0) {
        sirenCount[sirennum] = gQuickSirenCount;
        quickSirenOn(sirennum);
        gQuickSirenCount = (sint32)((uint32)gQuickSirenCount + 1);
    } else {
        sirenCount[sirennum] = -1;
        SuperCopSirenOn(sirennum);
    }
}

void SirenOff(sint32 sirennum)
{
    FUNCTION_MARKER(0x800798A0, "NFS4.EXE");
    bSirenOn[sirennum] = 0;
    bSirenPitchingUp[sirennum] = 1;
    quickSirenActive[sirennum] = 0;
    sirenCurrentPitch[sirennum] = 32;
    slowSirenReps[sirennum] = 3;
    reachedSirenMin[sirennum] = 0;
    quickSirenTimeCount[sirennum] = 0;
    SNDautovol(gaChannel[sirennum + 43].Partial, 5, -1);
}

void UpdateSiren(sint32 sirennum, sint32 amp, sint32 dop, sint32 azimuth, sint32 supercop)
{
    FUNCTION_MARKER(0x8007995C, "NFS4.EXE");
    if (bSirenOn[sirennum] == 0)
        return;
    if (SNDover(gaChannel[sirennum + 43].Partial) != 0) {
        if (supercop == 0)
            quickSirenOn(sirennum);
        else
            SuperCopSirenOn(sirennum);
    }
    quickSirenTimeCount[sirennum] = (sint32)((uint32)quickSirenTimeCount[sirennum] + 1);
    sint32 iFreq = dop / 1024;
    if (iFreq < 0)
        iFreq = 0;
    if (iFreq > 127)
        iFreq = 127;
    SNDpitchbend(gaChannel[sirennum + 43].Partial, iFreq);
    uint32 scale = sirenCount[sirennum] == -1 ? 37 : 47;
    sint32 volume = (sint32)((uint32)gMasterAmbientLevel * ((uint32)amp * scale)) >> 14;
    SNDvol(gaChannel[sirennum + 43].Partial, volume);
    if (Audio::direct3davail != 0) {
        SND3dpos(gaChannel[sirennum + 43].Partial, azimuth, 0);
    } else if (gStereoMode != 0) {
        sint32 pan;
        if ((uint32)azimuth - 0x4000 <= 0x7fff)
            pan = (sint32)(0xbfff - (uint32)azimuth);
        else
            pan = ((uint32)azimuth + 0x4000) & 0xffff;
        SNDpan(gaChannel[sirennum + 43].Partial, pan >> 8);
    }
}

sint32 AudioCmn::PlaySFX(sint32 sndPlayer, sint32 iSFXnum, sint32 iFreqIn, sint32 iDopplerIn, sint32 iAmpIn, sint32 azimuth)
{
    FUNCTION_MARKER(0x800783CC, "NFS4.EXE");
    sint32 iFreq = iFreqIn;
    if (iFreq < 0)
        iFreq = 0;
    if (iFreq > 127)
        iFreq = 127;
    sint32 iAmp = iAmpIn;
    if (iAmp < 0)
        iAmp = 0;
    if (iAmp > 127)
        iAmp = 127;
    sint32 PatchBank;
    if ((uint32)sndPlayer - 28 < 4 || (uint32)sndPlayer - 50 < 4 ||
        sndPlayer == 36 || sndPlayer == 54 || (uint32)sndPlayer - 55 < 16) {
        PatchBank = gSndBnk[5].bnkID;
    } else if ((uint32)sndPlayer - 10 < 8 || (uint32)sndPlayer - 26 < 2) {
        PatchBank = gSndBnk[2].bnkID;
    } else if (iSFXnum == 125) {
        PatchBank = gSndBnk[5].bnkID;
    } else if (iSFXnum == 8 || iSFXnum == 10 || iSFXnum == 11) {
        PatchBank = gSndBnk[3].bnkID;
    } else {
        PatchBank = gSndBnk[(uint8)gBankNumLookupTable[sndPlayer]].bnkID;
    }
    if (sndPlayer == 49) {
        gaChannel[49].Partial = PlayDoppleredSound(PatchBank, iSFXnum, azimuth, iAmp, iFreq, iDopplerIn);
        gaChannel[49].SFXnum = iSFXnum;
        NumSFXOn = (sint32)((uint32)NumSFXOn - 1);
        return gaChannel[sndPlayer].Partial;
    }
    if (gaChannel[sndPlayer].SFXnum == iSFXnum) {
        if (SNDover(gaChannel[sndPlayer].Partial) != 0) {
            gaChannel[sndPlayer].Partial = -1;
            gaChannel[sndPlayer].SFXnum = -1;
        }
        if (PatchBank < -1 && GetAsyncSfx(PatchBank == -3 ? 1 : 0, iSFXnum, false) == -1) {
            gaChannel[sndPlayer].Partial = -1;
            gaChannel[sndPlayer].SFXnum = -1;
        }
        if (gaChannel[sndPlayer].SFXnum == iSFXnum) {
            sint32 iPartial = gaChannel[sndPlayer].Partial;
            if (iPartial == -1) {
                gaChannel[sndPlayer].SFXnum = -1;
            } else {
                sint32 volume;
                if (iSFXnum == 125) {
                    volume = (sint32)((uint32)gMasterSFXLevel * (uint32)iAmp) >> 7;
                } else if (PatchBank == gSndBnk[2].bnkID) {
                    volume = (sint32)((uint32)gMasterAmbientLevel * ((uint32)iAmp * 39)) >> 14;
                } else if (PatchBank == gSndBnk[5].bnkID) {
                    volume = (sint32)((uint32)gMasterAmbientLevel * ((uint32)iAmp * 130)) >> 14;
                } else if (iSFXnum == 99) {
                    volume = (sint32)((uint32)gMasterAmbientLevel * ((uint32)iAmp * 39)) >> 14;
                } else if (iSFXnum == 3 && (PatchBank == gSndBnk[0].bnkID || PatchBank == gSndBnk[2].bnkID)) {
                    volume = (sint32)((uint32)gMasterSFXLevel * ((uint32)iAmp * 127)) >> 14;
                } else if (iSFXnum == 0) {
                    volume = (sint32)((uint32)gMasterEngineLevel * ((uint32)iAmp * 14)) >> 14;
                } else {
                    volume = (sint32)((uint32)gMasterSFXLevel * ((uint32)iAmp * 39)) >> 14;
                }
                SNDvol(iPartial, volume);
                if (Audio::direct3davail != 0) {
                    SND3dpos(gaChannel[sndPlayer].Partial, azimuth, 0);
                } else if (gStereoMode != 0) {
                    uint32 pan;
                    if ((uint32)azimuth - 0x4000 <= 0x7fff)
                        pan = 0xbfff - (uint32)azimuth;
                    else
                        pan = ((uint32)azimuth + 0x4000) & 0xffff;
                    SNDpan(gaChannel[sndPlayer].Partial, (sint32)pan >> 8);
                }
                SNDpitchbend(gaChannel[sndPlayer].Partial, iFreq);
                SNDpitchmult(gaChannel[sndPlayer].Partial, iDopplerIn >> 4);
            }
            return gaChannel[sndPlayer].Partial;
        }
    }
    if (gaChannel[sndPlayer].Partial != -1) {
        SNDstop(gaChannel[sndPlayer].Partial);
        NumSFXOn = (sint32)((uint32)NumSFXOn - 1);
    }
    sint32 iPartial = PlayDoppleredSound(PatchBank, iSFXnum, azimuth, iAmp, iFreq, iDopplerIn);
    gaChannel[sndPlayer].Partial = iPartial;
    gaChannel[sndPlayer].SFXnum = iPartial == -1 ? -1 : iSFXnum;
    return gaChannel[sndPlayer].Partial;
}

void AudioCmn::SoundCar(Car::tObj *car, sint32 dst, sint32 iFreqIn, sint32 doppler, sint32 azimuth, sint32 trackazim, sint32 relvel, sint32 cardir)
{
    FUNCTION_MARKER(0x800788BC, "NFS4.EXE");
    (void)trackazim;
    CheckState(car);
    if (kAudioOn == 0)
        return;
    sint32 numerator = 0x08000000;
    sint32 distanceOffset = 0x20000;
    if (Camera::gInfo[car->carIndex].mode == 12) {
        numerator = 0x10000000;
        distanceOffset = 0x10000;
    }
    sint32 amplitude = fixeddiv(numerator, (sint32)((uint32)dst + (uint32)distanceOffset)) / 0x10000;
    sint32 speed = car->linearVel_ch.z;
    if (speed < 0)
        speed = (sint32)(0u - (uint32)speed);
    sint32 roadNoiseAmp = 0;
    sint32 excessSpeed = (sint32)((uint32)speed - 0xd8000);
    if (excessSpeed >= 0)
        roadNoiseAmp = excessSpeed >> 15;
    sint32 cobblestoneAmp = speed >> 15;
    if (cobblestoneAmp > 127)
        cobblestoneAmp = 127;
    sint32 cobbleMod = 8 - cobblestoneAmp / 16;
    if (cobbleMod < 3)
        cobbleMod = 3;
    sint32 roadNoiseFreq = 64;
    sint32 freq = iFreqIn;
    if (freq > 150)
        freq = 150;
    if (freq < 0)
        freq = 0;
    sint32 roadSurface = car->N.driveSurfaceType;
    bool tuntrig = BWorldSm::TunnelFlagSm(&car->N.simRoadInfo);
    if (tuntrig) {
        if (GameSetup::gData.commMode != 1 && (uint8)fReverbLevel < 100) {
            sint32 level = (uint8)fReverbLevel + 10;
            if (level > 100)
                level = 100;
            fReverbLevel = (char)level;
            SNDfxmasterlevel(0, (uint8)level);
            fReverbOn = 1;
        }
        roadNoiseAmp = (sint32)((uint32)roadNoiseAmp + 20);
        roadNoiseFreq = 94;
    } else if ((uint8)fReverbLevel < 32) {
        fReverbLevel = 32;
        SNDfxmasterlevel(0, 32);
        fReverbOn = 1;
    } else if ((uint8)fReverbLevel > 32) {
        sint32 level = (uint8)fReverbLevel - 5;
        if (level < 32)
            level = 32;
        fReverbLevel = (char)level;
        SNDfxmasterlevel(0, (uint8)level);
    }
    bool SPSC = GameSetup::gData.commMode == 1;
    sint32 PlayerPan = 0;
    if (SPSC)
        PlayerPan = car->carIndex == 0 ? 0xc002 : 0x3ffe;
    if (car->blowout != 0) {
        roadSurface = 4;
        cobblestoneAmp = (sint32)((uint32)cobblestoneAmp << 1);
    }
    if (dst < 0x460000) {
        sint32 squareDistance = fixedmult(dst, dst);
        sint32 attenuation = (sint32)(0x13240000u - (uint32)squareDistance) / 0x1324;
        cobblestoneAmp = (sint32)((uint32)attenuation * (uint32)cobblestoneAmp) / 0x10000;
    } else {
        cobblestoneAmp = 0;
    }
    static char cobbleCount = 0;
    switch (roadSurface) {
    case 2:
    case 10:
    case 11:
    case 13:
        roadNoiseAmp = (sint32)((uint32)roadNoiseAmp + (uint32)(roadNoiseAmp / 4));
        roadNoiseFreq = 24;
        break;
    case 4:
    case 7:
    case 12:
        cobbleCount = (char)((uint8)cobbleCount + 1);
        cobbleCount = (char)((uint8)cobbleCount % cobbleMod);
        if ((uint8)cobbleCount == 0 && car->N.objAltitude < 0x3333 && gMasterSFXLevel > 0)
            PlaySound(gSndBnk[3].bnkID, 29, SPSC ? PlayerPan : 0, cobblestoneAmp / 2, roadSurface == 7 ? 40 : 64);
        break;
    default:
        break;
    }
    if (gMasterEngineLevel == 0)
        return;
    sint32 mode = Camera::gInfo[car->carIndex].mode;
    sint32 cam;
    if (mode >= 0 && mode < 2) {
        cam = 0;
    } else {
        cam = mode >= 2 && mode < 5 ? 1 : 2;
        roadNoiseAmp >>= 1;
    }
    sint32 previousGas = PlayersRampedGasLevel[car->carIndex];
    sint32 gasTarget = (uint8)car->control.gasLevel;
    if (previousGas < gasTarget) {
        sint32 step = (sint32)((uint32)gasTarget - (uint32)previousGas) / 2;
        if (step <= 0)
            step = 1;
        PlayersRampedGasLevel[car->carIndex] = (sint32)((uint32)previousGas + (uint32)step);
    } else if (gasTarget < previousGas) {
        sint32 step = (sint32)((uint32)gasTarget - (uint32)previousGas) / 8;
        if (step >= 0)
            step = -1;
        PlayersRampedGasLevel[car->carIndex] = (sint32)((uint32)previousGas + (uint32)step);
    }
    sint32 CurCarGasLevel = PlayersRampedGasLevel[car->carIndex];
    if (CurCarGasLevel > 255)
        CurCarGasLevel = 255;
    sint32 load = (sint32)((uint32)amplitude * (uint32)(freq + 40)) / 128;
    if (load < amplitude)
        load = amplitude;
    sint32 loadAmp = (sint32)((uint32)load * 127) >> 7;
    roadNoiseAmp = (sint32)((uint32)roadNoiseAmp * (uint32)load) >> 7;

    sint32 wetNoiseAmp = tuntrig ? 0 : Weather::GetNumParticles(previousGas);
    if (wetNoiseAmp > 127)
        wetNoiseAmp = 127;
    sint32 wetNoiseFreq = (sint32)(72u - (uint32)(wetNoiseAmp >> 3));
    sint32 iAmpIn = roadNoiseAmp < 128 ? roadNoiseAmp : 127;
    if (relvel != 0 || Camera::gInfo[car->carIndex].mode == 11) {
        sint32 dividend = (sint32)((uint32)loadAmp << 16);
        if (doppler == 0 || (doppler == -1 && dividend == INT_MIN))
            abort();
        loadAmp = dividend / doppler;
    }
    sint32 roadNoisePatch = cam == 0 && GameSetup::gData.commMode != 1 ? 0 : 14;
    sint32 sndPlayer = car->carIndex == 0 ? 24 : 25;
    if (iAmpIn == 0)
        freeVoiceChannel(sndPlayer);
    else
        PlaySFX(sndPlayer, roadNoisePatch, roadNoiseFreq, doppler, iAmpIn, azimuth);
    if (car->carIndex == 0) {
        if (GameSetup::gData.Weather == 1 && wetNoiseAmp != 0)
            PlaySFX(26, 0, wetNoiseFreq, doppler, wetNoiseAmp, 0);
        else
            freeVoiceChannel(26);
    }
    if (car->control.gearShiftTimer != 0 &&
        (uint8)car->control.lastGear < (uint8)car->control.gear &&
        (uint8)car->control.lastGear != 1 && CurCarGasLevel != 0) {
        sint32 timer = (uint8)car->control.gearShiftTimer;
        sint32 dividend = (sint32)((uint32)loadAmp * (uint32)timer);
        sint32 divisor = car->specs->gearShiftDelay;
        if (divisor == 0 || (divisor == -1 && dividend == INT_MIN))
            abort();
        loadAmp = (sint32)((uint32)loadAmp + (uint32)(dividend / divisor)) >> 1;
        if (timer == 5)
            CurCarGasLevel = (sint32)((uint32)CurCarGasLevel - (uint32)(CurCarGasLevel >> 2));
        else if (timer == 4)
            CurCarGasLevel >>= 1;
        else if (timer == 3)
            CurCarGasLevel >>= 2;
        else if (timer < 3)
            CurCarGasLevel = 0;
        PlayersRampedGasLevel[car->carIndex] = CurCarGasLevel;
    }
    sint32 dividend = (sint32)((uint32)car->flywheelRpm << 16);
    sint32 divisor = car->specs->redline;
    if (divisor == 0 || (divisor == -1 && dividend == INT_MIN))
        abort();
    sint32 esp = dividend / divisor;
    sint32 gas = car->revLimit != 0 ? 127 : CurCarGasLevel >> 1;
    sint32 engineVolume = (sint32)((uint32)gMasterEngineLevel * ((uint32)loadAmp * 14)) >> 14;
    AudioEng::Set(car->carIndex, engineVolume, esp, gas, cam, doppler, azimuth, cardir);
}

void AudioCmn::TrafficSFX(sint32 iChan, sint32 iSFXnum, sint32 freq, sint32 doppler, sint32 dst, sint32 azimuth, sint32 relvel, sint32 dir)
{
    FUNCTION_MARKER(0x80079104, "NFS4.EXE");
    sint32 player = 0;
    if (GameSetup::gData.commMode == 1)
        player = iChan < 8 ? 0 : 1;
    sint32 numerator = 0x08000000;
    sint32 offset = 0x20000;
    if (Camera::gInfo[player].mode == 12) {
        numerator = 0x10000000;
        offset = 0x10000;
    }
    sint32 iAmpIn = fixeddiv(numerator, (sint32)((uint32)dst + (uint32)offset)) / 0x10000;
    sint32 engineChannel = (sint32)((uint32)iChan + 4);
    sint32 exhaustChannel = (sint32)((uint32)iChan + 8);
    if (iAmpIn == 0 && iChan != -1) {
        if (gaChannel[iChan].Partial != -1)
            freeVoiceChannel(iChan);
        if (gaChannel[engineChannel].Partial != -1)
            freeVoiceChannel(engineChannel);
        if (gaChannel[exhaustChannel].Partial != -1)
            freeVoiceChannel(exhaustChannel);
        return;
    }
    sint32 pitchmult = fixedmult((sint32)((uint32)freq + 0x3333), doppler);
    pitchmult = (sint32)((uint32)pitchmult * 80) >> 10;
    sint32 patch = CopSpeak::GetEnginePatch(iSFXnum, 0);
    sint32 forwardIndex = (sint32)((uint32)(dir >> 12) + 64 - (uint32)(dir >> 10));
    sint32 amplitude = (sint32)((uint32)iAmpIn * (uint8)Xfade[forwardIndex]) >> 7;
    pitchmult = (sint32)((uint32)pitchmult << 4);
    PlaySFX(engineChannel, patch, 64, pitchmult, amplitude, azimuth);
    patch = CopSpeak::GetEnginePatch(iSFXnum, 1);
    sint32 reverseIndex = (sint32)((uint32)(dir >> 10) - ((uint32)(dir >> 12) - 64));
    amplitude = (sint32)((uint32)iAmpIn * (uint8)Xfade[reverseIndex]) >> 7;
    PlaySFX(exhaustChannel, patch, 64, pitchmult, amplitude, azimuth);
    if (relvel > 0x280000)
        relvel = 0x280000;
    amplitude = (sint32)((uint32)iAmpIn * (uint32)relvel) / 0x280000;
    PlaySFX(iChan, 99, freq, doppler, amplitude, azimuth);
}

void AudioCmn::TrafficSkidSFX(sint32 sndPlayer, s_type surface1, s_type surface2, sint32 force, sint32 Distsq, sint32 azimuth)
{
    FUNCTION_MARKER(0x80079390, "NFS4.EXE");
    if (kAudioOn == 0)
        return;
    sint32 iAmpIn = 0;
    if (Distsq < 0x1324)
        iAmpIn = (sint32)((0x1324u - (uint32)Distsq) * 127) / 0x1324;
    if (iAmpIn == 0 && sndPlayer != -1) {
        if (gaChannel[sndPlayer].Partial != -1)
            freeVoiceChannel(sndPlayer);
    } else {
        SFX(sndPlayer, surface1, surface2, force, Distsq, azimuth);
    }
}

void AudioCmn::PlayerHornOn(sint32 carIndex, sint32 Distsq, sint32 iFreqIn, sint32 azimuth, sint32 doppler)
{
    FUNCTION_MARKER(0x80079444, "NFS4.EXE");
    if (kAudioOn == 0)
        return;
    sint32 iAmpIn = 0;
    if (Distsq < 0x1324)
        iAmpIn = (sint32)((0x1324u - (uint32)Distsq) * 127) / 0x1324;
    sint32 sfx = 3;
    sint32 player = 41;
    if (GameSetup::gData.commMode == 1) {
        sfx = 10;
        if (carIndex != 0)
            player = 42;
    }
    if (gaChannel[player].Partial == -1 || SNDover(gaChannel[player].Partial) == 0)
        PlaySFX(player, sfx, iFreqIn, doppler, iAmpIn, azimuth);
}

sint32 AudioCmn::PlayerHornOff(sint32 carIndex)
{
    FUNCTION_MARKER(0x80079548, "NFS4.EXE");
    if (kAudioOn == 0)
        return 0;
    sint32 player = 41;
    if (carIndex != 0 && GameSetup::gData.commMode == 1)
        player = 42;
    if (GameSetup::gData.commMode != 1 && Cars::gList[carIndex]->carInfo->carType == 20 &&
        gaChannel[player].Partial != -1 && SNDover(gaChannel[player].Partial) == 0)
        return 1;
    freeVoiceChannel(player);
    return 0;
}

void AudioCmn::PlayFESFX(sint32 SFXnum)
{
    FUNCTION_MARKER(0x80079604, "NFS4.EXE");
    PlayFESFXVol(SFXnum, 127);
}

void AudioCmn::PlayFESFXVol(sint32 SFXnum, sint32 vol)
{
    FUNCTION_MARKER(0x80079624, "NFS4.EXE");
    SNDPLAYOPTS playopts;
    SNDplaysetdef(&playopts);
    sint32 volume = (sint32)((uint32)gMasterSFXLevel * ((uint32)vol * 120)) >> 14;
    playopts.patnum = SFXnum;
    playopts.use3dpos = 0;
    playopts.bend = 64;
    playopts.pan = 64;
    playopts.bhandle = (char)gSndBnk[0].bnkID;
    playopts.vol = (char)volume;
    SNDplay(&playopts);
    NumSFXOn = (sint32)((uint32)NumSFXOn + 1);
}

void AudioCmn::PlayWrongWaySFX(void)
{
    FUNCTION_MARKER(0x800796AC, "NFS4.EXE");
    PlaySound(gSndBnk[3].bnkID, 16, 0, 127, 64);
}

void AudioCmn::PlayPauseSound(sint32 patch)
{
    FUNCTION_MARKER(0x800796E4, "NFS4.EXE");
    gCursorSndHandle = PlaySound(gSndBnk[3].bnkID, patch, 0, 127, 64);
}

void AudioCmn::Pause(void)
{
    FUNCTION_MARKER(0x80079B60, "NFS4.EXE");
    CopSpeak::SilenceCop(0, 0);
    for (sint32 i = 0; i < 71; ++i) {
        if (gaChannel[i].Partial != -1)
            SNDvol(gaChannel[i].Partial, 0);
    }
    sint32 musicVoice = AudioMus::SetPaused(true);
    if (fReverbOn != 0)
        ReverbOff();
    const AudioEng::tState silent = { 0, 0x1000, 0, 0, 0, 0, 0 };
    for (sint32 player = 0; player < 2; ++player)
        if (!AudioEng::TryQueueState(player, silent))
            break;
    StopSoundVoicesExcept(musicVoice);
    gMasterSFXLevel = GameSetup::gData.userSetting.sfxLevel;
    gMasterMusicLevel = GameSetup::gData.userSetting.musicLevel;
}

void AudioCmn::UnPause(void)
{
    FUNCTION_MARKER(0x80079C18, "NFS4.EXE");
    SNDmastervol(127);
    sint32 level = gMasterMusicLevel;
    sint32 curve = level;
    if (level >= 86) {
        sint32 scaled = (sint32)(((uint32)level - 85) * 7);
        curve = scaled / 2 + 85;
    }
    sint32 volume = (sint32)((uint32)curve * 70) >> 7;
    AudioMus::SetPaused(false);
    AudioMus::Volume(volume);
    GameSetup::gData.userSetting.sfxLevel = gMasterSFXLevel;
    GameSetup::gData.userSetting.musicLevel = gMasterMusicLevel;
}

void AudioCmn::UnPauseAndQuit(void)
{
    FUNCTION_MARKER(0x80079CA4, "NFS4.EXE");
    while (SNDover(gCursorSndHandle) == 0)
        systemtask(0);
    CopSpeak::Cancel();
    SNDmastervol(0);
    AudioMus::StopSong(0);
    for (sint32 i = 0; i < 71; ++i) {
        if (gaChannel[i].Partial != -1) {
            SNDstop(gaChannel[i].Partial);
            gaChannel[i].Partial = -1;
            gaChannel[i].SFXnum = -1;
        }
    }
    GameSetup::gData.userSetting.sfxLevel = gMasterSFXLevel;
    GameSetup::gData.userSetting.musicLevel = gMasterMusicLevel;
    if (fReverbOn != 0)
        ReverbOff();
    for (sint32 i = 0; i < 128; ++i)
        SNDmastervol(i);
}

void AudioCmn::UnPauseAndRestart(void)
{
    FUNCTION_MARKER(0x80079D8C, "NFS4.EXE");
    SPCH::ClearEventQueue();
    while (SNDover(gCursorSndHandle) == 0)
        systemtask(0);
    CopSpeak::Cancel();
    SNDmastervol(0);
    DeInitAsyncSfx();
    AudioMus::StopSong(0);
    for (sint32 i = 0; i < 71; ++i) {
        if (gaChannel[i].Partial != -1) {
            SNDstop(gaChannel[i].Partial);
            gaChannel[i].Partial = -1;
            gaChannel[i].SFXnum = -1;
        }
    }
    for (sint32 i = 0; i < 128; ++i)
        SNDmastervol(i);
    if (fReverbOn != 0)
        ReverbOff();
    GameSetup::gData.userSetting.sfxLevel = gMasterSFXLevel;
    GameSetup::gData.userSetting.musicLevel = gMasterMusicLevel;
    Init();
}

void AudioCmn::InitReverb(void)
{
    FUNCTION_MARKER(0x80079E88, "NFS4.EXE");
    SNDfxinitbus(0, 127, 10, -1, -1);
    fReverbLevel = 0;
    SNDfxmasterlevel(0, 0);
    fReverbOn = 0;
}

void AudioCmn::ReverbOff(void)
{
    FUNCTION_MARKER(0x80079ECC, "NFS4.EXE");
    SNDfxmasterlevel(0, 0);
    fReverbLevel = 0;
    fReverbOn = 0;
}
