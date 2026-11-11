#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/splysdef.h"
#include "eaclib/psx/sststat.h"
#include "game/common/audioclc.h"
#include "game/common/audiocmn.h"
#include "game/common/audiomus.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/copspeak.h"
#include "game/common/gmesetup.h"
#include "game/common/paths.h"
#include "game/psx/audio.h"
#include "game/psx/font.h"
#include "game/psx/hud.h"
#include "game/psx/platform.h"
#include "game/psx/textpix.h"
#include "lib/psx/random.h"

sint32 cdsectorreseek;
CopSpeak::tBank Copspeak_gBank[4];
sint32 CopSpeak::gQueueHead;
sint32 CopSpeak::gQueuePlay;
sint32 CopSpeak::gSpchHandle;
sint32 CopSpeak::gNumTrackSfx;
sint32 CopSpeak::gQueueLoad;
sint32 CopSpeak::gQueueReady;
sint32 CopSpeak::gWrongWay;
sint16 CopSpeak::gBufferLow;
sint16 CopSpeak::gBufferStart;
sint16 CopSpeak::gBufferEnd;
sint16 CopSpeak::gBufferHigh;
char CopSpeak::gHandleCount;
char *CopSpeak::gBuffer;
sint32 CopSpeak::gStaticHandle[2];
char CopSpeak::gStaticPatch[2];
CopSpeak::tRequest CopSpeak::gRequest;
CopSpeak::tRequest CopSpeak::gCurrent;
SNDSTREAMSTATUS CopSpeak::gMusicStat;
CopSpeak::tRequest CopSpeak::gQueue[64];

void CopSpeak::RadioStaticInit(void)
{
    FUNCTION_MARKER(0x800993DC, "NFS4.EXE");
    for (sint32 i = 0; i < 2; ++i) {
        gStaticHandle[i] = -1;
        gStaticPatch[i] = (char)-1;
    }
}

void CopSpeak::RadioStaticActive(sint32 noise)
{
    FUNCTION_MARKER(0x80099418, "NFS4.EXE");
    sint32 numpatches = noise > 96 ? 4 : 3;
    uint32 scaledNoise = (uint32)noise * 129;
    for (sint32 i = 0; i < 2; ++i) {
        if ((sint8)gStaticPatch[i] == -1)
            gStaticPatch[i] = (char)(random() % (uint32)numpatches);
        if (gStaticHandle[i] == -1) {
            sint32 bhandle = AudioCmn::GetAsyncSfx(2, (sint8)gStaticPatch[i] + 47, false);
            if (bhandle >= 0) {
                SNDPLAYOPTS playopts;
                SNDplaysetdef(&playopts);
                sint32 volume = (sint32)((uint32)gMasterFENarrationLevel * scaledNoise) >> 14;
                playopts.bhandle = (char)bhandle;
                playopts.patnum = 0;
                playopts.vol = (char)volume;
                gStaticHandle[i] = SNDplay(&playopts);
                gStaticPatch[i] = (char)-1;
                if (gStaticHandle[i] < 0)
                    AudioClc::SndError(gStaticHandle[i]);
            }
        } else if (SNDover(gStaticHandle[i]) != 0) {
            gStaticHandle[i] = -1;
        }
    }
}

void CopSpeak::RadioStaticSquelch(void)
{
    FUNCTION_MARKER(0x80099588, "NFS4.EXE");
    for (sint32 i = 0; i < 2; ++i) {
        if (gStaticHandle[i] != -1) {
            SNDstop(gStaticHandle[i]);
            gStaticHandle[i] = -1;
        }
    }
}

void CopSpeak::InitRequest(CopSpeak::tRequest *r)
{
    FUNCTION_MARKER(0x800995F0, "NFS4.EXE");
    r->buffer = -1;
    r->phrase = -1;
    r->car = 0;
    r->offset = 0;
    r->size = 0;
    r->player = 0;
    r->sfx = 0;
    r->bank = 2;
    r->noise = 0;
}

void CopSpeak::SilenceCop(Car::tObj *car, sint32 playerIndex)
{
    FUNCTION_MARKER(0x80099620, "NFS4.EXE");
    if (car == 0 || ((uint8)gCurrent.player == playerIndex && car == gCurrent.car)) {
        if (gSpchHandle != -1)
            SNDvol(gSpchHandle, 0);
        RadioStaticSquelch();
    }
}

void CopSpeak::Alloc(CopSpeak::tRequest *r)
{
    FUNCTION_MARKER(0x80099684, "NFS4.EXE");
    if (gQueueReady == gQueueLoad) {
        gBufferLow = 0;
        gBufferHigh = 0;
        gBufferStart = 0;
        gBufferEnd = 0x7ffc;
    }
    if (r->size <= (sint32)gBufferEnd - (sint32)gBufferStart) {
        r->buffer = gBufferStart;
        gBufferStart = (sint16)((uint16)gBufferStart + (uint16)r->size);
        return;
    }
    if (r->size <= (sint32)gBufferLow) {
        gBufferHigh = gBufferStart;
        gBufferEnd = gBufferLow;
        gBufferLow = 0;
        r->buffer = 0;
        gBufferStart = (sint16)r->size;
        return;
    }
    r->buffer = -1;
}

void CopSpeak::Free(CopSpeak::tRequest *r)
{
    FUNCTION_MARKER(0x8009971C, "NFS4.EXE");
    if (r->buffer >= 0 && r->size > 0) {
        sint32 end = (sint32)((uint32)r->buffer + (uint32)r->size);
        if (gBufferHigh == 0) {
            if (end == (sint32)gBufferStart) {
                gBufferStart = 0;
                r->buffer = -1;
                return;
            }
            gBufferLow = (sint16)((uint16)r->buffer + (uint16)r->size);
        } else if (end == (sint32)gBufferHigh) {
            gBufferHigh = 0;
            gBufferEnd = 0x7ffc;
        }
        r->buffer = -1;
    }
}

void CopSpeak::ReadyNextRequest(void)
{
    FUNCTION_MARKER(0x800997AC, "NFS4.EXE");
    tRequest *r = &gQueue[gQueueReady];
    sint32 ok = 1;
    if (r->buffer < 0) {
        if (r->sfx != 0 || (r->phrase >= 0 && (sint8)r->bank >= 0))
            AudioCmn::LoadAsyncSfx((sint8)r->bank, r->phrase, 0, 0);
    } else {
        sint32 status = XPORT_UNPLACED::FILE::opstatus(r->ophandle);
        if (status == 0)
            return;
        ok = status == 1;
        sint32 completed = XPORT_UNPLACED::FILE::completeop(r->ophandle);
        gHandleCount = (char)((uint8)gHandleCount - 1);
        if (completed != r->size)
            ok = 0;
        tBankHeader *bnk = (tBankHeader *)(gBuffer + r->buffer);
        if (bnk->id != 0x6c4b4e42)
            ok = 0;
        if (bnk->ver != 4 && bnk->ver != 2)
            ok = 0;
        if (!ok) {
            if (r->sfx != 0 && (sint8)r->bank >= 0)
                AudioCmn::LoadAsyncSfx((sint8)r->bank, r->phrase, 0, 0);
            bnk->id = 0x4c494146;
            r->ophandle = XPORT_UNPLACED::FILE::read(r->filehandle, r->offset, gBuffer + r->buffer, r->size, 0, 0);
            gHandleCount = (char)((uint8)gHandleCount + 1);
            XPORT_UNPLACED::FILE::operror(r->ophandle);
        } else {
            if ((sint8)r->bank >= 0) {
                if (r->sfx == 0 && r->phrase < 0) {
                    sint32 patch = (sint32)((uint32)r->offset + 0x4000);
                    if (AudioCmn::GetAsyncSfx((sint8)r->bank, patch, true) == -1) {
                        patch = (sint32)((uint32)r->offset + 0x4000);
                        AudioCmn::LoadAsyncSfx((sint8)r->bank, patch, gBuffer + r->buffer, r->size);
                    }
                } else if (AudioCmn::GetAsyncSfx((sint8)r->bank, r->phrase, true) == -1) {
                    AudioCmn::LoadAsyncSfx((sint8)r->bank, r->phrase, gBuffer + r->buffer, r->size);
                }
            }
            bnk->id = 0x4c494146;
            Free(r);
        }
    }
    if (ok)
        gQueueReady = gQueueReady < 63 ? gQueueReady + 1 : 0;
}

void CopSpeak::Cancel(void)
{
    FUNCTION_MARKER(0x80099A1C, "NFS4.EXE");
    while (gQueueReady != gQueueLoad) {
        ReadyNextRequest();
        systemtask(0);
    }
    if (gSpchHandle != -1)
        SNDstop(gSpchHandle);
    RadioStaticSquelch();
    gSpchHandle = -1;
    InitRequest(&gCurrent);
    gQueueHead = 0;
    gQueueLoad = 0;
    gQueueReady = 0;
    gQueuePlay = 0;
    gBufferLow = 0;
    gBufferHigh = 0;
    gBufferStart = 0;
    gBufferEnd = 0x7ffc;
}

void CopSpeak::CleanUp(void)
{
    FUNCTION_MARKER(0x80099ADC, "NFS4.EXE");
    Cancel();
    for (sint32 i = 0; i < 4; ++i) {
        tBank *bank = &Copspeak_gBank[i];
        if (bank->FileOpen != 0) {
            XPORT_UNPLACED::FILE::closesync(bank->FileHandle, 100);
            bank->FileOpen = 0;
        }
        if (bank->Index != 0) {
            purgememadr(bank->Index);
            bank->Index = 0;
        }
    }
    if (gBuffer != 0)
        gBuffer = 0;
}

void CopSpeak::InitVars(void)
{
    FUNCTION_MARKER(0x80099B70, "NFS4.EXE");
    for (sint32 i = 0; i < 4; ++i) {
        Copspeak_gBank[i].FileOpen = 0;
        Copspeak_gBank[i].Index = 0;
    }
    gSpchHandle = -1;
    gBuffer = 0;
    gQueueHead = 0;
    gQueueLoad = 0;
    gQueueReady = 0;
    gQueuePlay = 0;
    gWrongWay = 0;
    gBufferLow = 0;
    gBufferHigh = 0;
    gBufferStart = 0;
    gBufferEnd = 0x7ffc;
    InitRequest(&gCurrent);
}

void CopSpeak::DirectRequest(sint32 filehandle, sint32 offset, sint32 size, Car::tObj *car, char *name)
{
    FUNCTION_MARKER(0x80099BEC, "NFS4.EXE");
    (void)name;
    tRequest *r = &gQueue[gQueueHead];
    sint32 next = gQueueHead < 63 ? gQueueHead + 1 : 0;
    if (next != gQueuePlay) {
        InitRequest(r);
        r->filehandle = filehandle;
        r->offset = offset;
        r->size = size;
        r->noise = 127;
        r->car = car;
        gQueueHead = next;
    }
}

void CopSpeak::GenericBankRequest(sint32 patch, Car::tObj *car)
{
    FUNCTION_MARKER(0x80099C98, "NFS4.EXE");
    tRequest *r = &gQueue[gQueueHead];
    sint32 next = gQueueHead < 63 ? gQueueHead + 1 : 0;
    if (next != gQueuePlay && next != gQueueReady) {
        InitRequest(r);
        r->bank = 3;
        r->phrase = patch;
        r->noise = 127;
        r->car = car;
        gQueueHead = next;
    }
}

void CopSpeak::StartUp(void)
{
    FUNCTION_MARKER(0x80099D3C, "NFS4.EXE");
    InitVars();
    RadioStaticInit();
    char filename[100];
    sprintf(filename, "%szzzwzzz.viv", Paths_Paths[28]);
    char *header = loadbigfileheader(filename, 0x10);
    if (header != 0) {
        tBank *bank = &Copspeak_gBank[1];
        bank->Count = 0;
        while (strcmp(GameSetup::gCarNames[bank->Count], "BAD!") != 0)
            bank->Count = (sint32)((uint32)bank->Count + 1);
        bank->Count = (sint32)((uint32)bank->Count * 2 + 1);
        bank->Index = (tFileIndex *)reservememadr("ocar index", (sint32)((uint32)bank->Count * 8), 0);
        const char *Generic[1] = { "rain.bnk" };
        for (sint32 i = 0; i < 1; ++i) {
            if (locatebigentryz(header, Generic[i], 0, &bank->Index[i].offset, &bank->Index[i].size) == 0) {
                bank->Index[i].offset = 0;
                bank->Index[i].size = 0;
            }
        }
        for (sint32 i = 0; i < (sint32)((uint32)bank->Count - 1); ++i) {
            const char *timbre[2] = { "lden", "ldex" };
            char bankname[16];
            sprintf(bankname, "%.4s%.4s.bnk", GameSetup::gCarNames[i / 2], timbre[i % 2]);
            if (locatebigentryz(header, bankname, 0, &bank->Index[i + 1].offset, &bank->Index[i + 1].size) == 0) {
                bank->Index[i + 1].offset = 0;
                bank->Index[i + 1].size = 0;
            }
        }
        purgememadr(header);
        bank->FileOpen = XPORT_UNPLACED::FILE::opensync(filename, 1, 100, &bank->FileHandle);
    }
    gNumTrackSfx = 0;
    sprintf(filename, "%szzzw%s.viv", Paths_Paths[28], GameSetup::gTrackNames[GameSetup::gData.track]);
    header = loadbigfileheader(filename, 0x10);
    if (header == 0) {
        sprintf(filename, "%szzzwcst.viv", Paths_Paths[28]);
        header = loadbigfileheader(filename, 0x10);
    }
    if (header != 0) {
        tBank *bank = &Copspeak_gBank[0];
        bank->Count = bigcount(header);
        bank->Index = (tFileIndex *)reservememadr("trck index", (sint32)((uint32)bank->Count * 8), 0);
        gNumTrackSfx = bank->Count;
        for (sint32 i = 0; i < bank->Count; ++i)
            locatebigentry(header, 0, i, &bank->Index[i].offset, &bank->Index[i].size);
        purgememadr(header);
        bank->FileOpen = XPORT_UNPLACED::FILE::opensync(filename, 1, 100, &bank->FileHandle);
    }
    const char *SpeechLanguage;
    if (GameSetup::gData.userSetting.language == 1)
        SpeechLanguage = "ger";
    else if (GameSetup::gData.userSetting.language == 2)
        SpeechLanguage = "fre";
    else
        SpeechLanguage = "eng";
    sprintf(filename, "%szzzx%s.viv", Paths_Paths[29], SpeechLanguage);
    header = loadbigfileheader(filename, 0x10);
    if (header != 0) {
        sint32 offset;
        sint32 size;
        tBank *bank = &Copspeak_gBank[2];
        if (locatebigentryz(header, "oneshots.txt", 0, &offset, &size) != 0 && size != 0) {
            char *list = (char *)reservememadr("copspeak temp", size, 0x10);
            bank->FileOpen = XPORT_UNPLACED::FILE::opensync(filename, 1, 100, &bank->FileHandle);
            XPORT_UNPLACED::FILE::readsync(bank->FileHandle, offset, list, size, 100);
            sint32 quoted = 0;
            bank->Count = 0;
            for (char *p = list; p < list + size; ++p) {
                if (*p == '"') {
                    if (quoted != 0)
                        bank->Count = (sint32)((uint32)bank->Count + 1);
                    quoted ^= 1;
                }
            }
            bank->Index = (tFileIndex *)reservememadr("1sht index", (sint32)((uint32)bank->Count * 8), 0);
            quoted = 0;
            sint32 i = 0;
            char *name = list;
            for (char *p = list; p < list + size; ++p) {
                if (*p == '"') {
                    if (quoted != 0) {
                        *p = 0;
                        if (locatebigentryz(header, name, 0, &bank->Index[i].offset, &bank->Index[i].size) == 0)
                            bank->Index[i].size = 0;
                        ++i;
                    } else {
                        name = p + 1;
                    }
                    quoted ^= 1;
                }
            }
            purgememadr(list);
        }
        purgememadr(header);
    }
    static char bufferName[] = "CopSpeakBuf";
    gBuffer = Platform::GetDCTBuffer(0x8000, bufferName);
}

sint32 CopSpeak::GetEnginePatch(sint32 type, sint32 timbre)
{
    FUNCTION_MARKER(0x8009A2CC, "NFS4.EXE");
    sint32 patch = (sint32)((uint32)type * 2 + (uint32)timbre + 1);
    if (Copspeak_gBank[1].Index[patch].size == 0)
        patch = (sint32)((uint32)timbre + 69);
    return patch;
}

sint32 CopSpeak::Play(CopSpeak::tRequest *r, sint32 handle)
{
    FUNCTION_MARKER(0x8009A304, "NFS4.EXE");
    SNDPLAYOPTS playopts;
    SNDplaysetdef(&playopts);
    sint32 azimuth = 0;
    if (r->bank == 3) {
        playopts.bhandle = (char)gSndBnk[3].bnkID;
        playopts.patnum = r->phrase;
    } else {
        playopts.bhandle = (char)handle;
        playopts.patnum = 0;
    }
    sint32 noise = 48;
    if (r->car != 0) {
        sint32 distance = (r->car->N.distToPlayer >> 16) + 32;
        noise = distance < 128 ? distance : 127;
    }
    sint32 vol;
    if (r->filehandle == Copspeak_gBank[2].FileHandle) {
        sint32 scaled = (sint32)((uint32)gMasterFENarrationLevel * 129) >> 7;
        sint32 boosted = scaled * 2;
        vol = boosted < 128 ? boosted : 127;
    } else {
        sint32 attenuation = 128 - (noise >> 2);
        vol = (sint32)((uint32)gMasterFENarrationLevel * (uint32)attenuation * 129) >> 14;
    }
    playopts.bend = 64;
    playopts.vol = (char)vol;
    playopts.use3dpos = Audio::direct3davail != 0;
    if (Audio::direct3davail != 0)
        playopts.azimuth = (uint16)azimuth;
    else
        playopts.pan = 64;
    return SNDplay(&playopts);
}

void CopSpeak::Skip(void)
{
    FUNCTION_MARKER(0x8009A45C, "NFS4.EXE");
    tRequest *r = &gQueue[gQueueLoad];
    char sfx = r->sfx;
    r->buffer = -1;
    r->phrase = -1;
    if (sfx != 0)
        AudioCmn::LoadAsyncSfx((sint8)r->bank, -1, 0, 0);
    gQueueLoad = gQueueLoad < 63 ? gQueueLoad + 1 : 0;
}

sint32 CopSpeak::Request(CopSpeak::tRequest *r)
{
    FUNCTION_MARKER(0x8009A4C8, "NFS4.EXE");
    sint32 slot = gQueueHead;
    sint32 next = slot < 63 ? slot + 1 : 0;
    if (next == gQueuePlay)
        return -1;
    tBank *bank = &Copspeak_gBank[(sint8)r->bank];
    if (bank->FileOpen == 0 || bank->Index == 0 || r->phrase < 0 || r->phrase > bank->Count)
        return -1;
    r->filehandle = bank->FileHandle;
    r->offset = bank->Index[r->phrase].offset;
    r->size = bank->Index[r->phrase].size;
    if (r->size == 0)
        return -1;
    sint32 head = gQueueHead;
    memmove(&gQueue[slot], r, 16);
    memmove((char *)&gQueue[slot] + 16, (const char *)r + 16, 16);
    gQueueHead = head < 63 ? head + 1 : 0;
    return slot;
}

sint32 CopSpeak::BankVolume(CopSpeak::tRequest *r)
{
    FUNCTION_MARKER(0x8009A604, "NFS4.EXE");
    return r->bank == 2 ? gMasterFENarrationLevel : gMasterAmbientLevel;
}

void CopSpeak::LoadNextRequest(void)
{
    FUNCTION_MARKER(0x8009A634, "NFS4.EXE");
    sint32 buffered = AudioMus::Buffered();
    sint32 threshold = AudioMus::Threshold();
    if (buffered < threshold)
        return;
    for (;;) {
        sint32 slot = gQueueLoad;
        if (slot == gQueueHead || (((uint32)slot - (uint32)gQueueReady) & 63) >= 8)
            return;
        tRequest *r = &gQueue[slot];
        sint32 bank = (sint8)r->bank;
        if (bank < 0) {
            Skip();
            continue;
        }
        if (bank == 3) {
            r->buffer = -1;
            gQueueLoad = slot < 63 ? slot + 1 : 0;
            continue;
        }
        bool cached = false;
        if (r->sfx == 0) {
            if (r->phrase >= 0)
                cached = AudioCmn::GetAsyncSfx(bank, r->phrase, true) >= 0;
            if (!cached && r->sfx == 0) {
                sint32 patch = (sint32)((uint32)r->offset + 0x4000);
                cached = AudioCmn::GetAsyncSfx((sint8)r->bank, patch, true) >= 0;
            }
        }
        if (cached) {
            r->buffer = -1;
            gQueueLoad = gQueueLoad < 63 ? gQueueLoad + 1 : 0;
            continue;
        }
        if (gBuffer == 0 || BankVolume(r) == 0 || r->size == 0 || r->size > 0x7ffc) {
            Skip();
            continue;
        }
        Alloc(r);
        if (r->buffer < 0)
            return;
        char *buffer = gBuffer;
        tBankHeader *bnk = (tBankHeader *)(buffer + r->buffer);
        bnk->id = 0x4c494146;
        r->ophandle = XPORT_UNPLACED::FILE::read(r->filehandle, r->offset, buffer + r->buffer, r->size, 100, 0);
        gHandleCount = (char)((uint8)gHandleCount + 1);
        if (XPORT_UNPLACED::FILE::operror(r->ophandle) <= 0)
            gQueueLoad = gQueueLoad < 63 ? gQueueLoad + 1 : 0;
    }
}

void CopSpeak::PlayNextRequest(void)
{
    FUNCTION_MARKER(0x8009A828, "NFS4.EXE");
    tRequest *r = &gQueue[gQueuePlay];
    if ((sint8)r->bank >= 0) {
        if (r->sfx == 0) {
            if (gSpchHandle != -1)
                return;
            memmove(&gCurrent, r, 12);
            memmove((char *)&gCurrent + 12, (const char *)r + 12, 12);
            memmove((char *)&gCurrent + 24, (const char *)r + 24, 8);
            sint32 patch = r->phrase;
            if (patch == -1)
                patch = (sint32)((uint32)r->offset + 0x4000);
            sint32 handle = AudioCmn::GetAsyncSfx((sint8)r->bank, patch, true);
            if (handle >= 0 || r->bank == 3)
                gSpchHandle = Play(r, handle);
        }
        InitRequest(r);
    }
    gQueuePlay = gQueuePlay < 63 ? gQueuePlay + 1 : 0;
}

void CopSpeak::Flush(void)
{
    FUNCTION_MARKER(0x8009A944, "NFS4.EXE");
    sint32 i = gQueuePlay;
    while (i != gQueueHead) {
        gQueue[i].bank = (char)-1;
        i = i < 63 ? i + 1 : 0;
    }
}

void CopSpeak::Server(void)
{
    FUNCTION_MARKER(0x8009A994, "NFS4.EXE");
    if (gSpchHandle != -1) {
        sint32 patch = gCurrent.phrase;
        if (patch == -1)
            patch = (sint32)((uint32)gCurrent.offset + 0x4000);
        AudioCmn::GetAsyncSfx((sint8)gCurrent.bank, patch, true);
        if (SNDtimeremaining(gSpchHandle) < 5) {
            if (gQueuePlay != gQueueHead && gQueue[gQueuePlay].bank == 3 &&
                gQueue[gQueuePlay].phrase == 19 && gCurrent.car == 0)
                gQueue[gQueuePlay].phrase = 20;
            gSpchHandle = -1;
            InitRequest(&gCurrent);
        }
    }
    sint32 i = gQueuePlay;
    while (i != gQueueHead) {
        tRequest *r = &gQueue[i];
        sint32 bank = (sint8)r->bank;
        if (r->sfx == 0 && bank != 3 && bank >= 0) {
            sint32 patch = r->phrase;
            if (patch == -1)
                patch = (sint32)((uint32)r->offset + 0x4000);
            AudioCmn::GetAsyncSfx(bank, patch, true);
        }
        i = i < 63 ? i + 1 : 0;
    }
    if (gQueueReady != gQueueLoad)
        ReadyNextRequest();
    tRequest *next = 0;
    if (gSpchHandle != -1) {
        next = &gCurrent;
    } else if (gQueuePlay != gQueueReady && gQueuePlay != gQueueHead) {
        sint32 slot = gQueuePlay < 63 ? gQueuePlay + 1 : 0;
        next = &gQueue[slot];
    }
    if (next == 0 || next->noise == 0) {
        RadioStaticSquelch();
    } else {
        sint32 noise = 48;
        if (next->car != 0) {
            sint32 distance = (next->car->N.distToPlayer >> 16) + 32;
            noise = distance < 128 ? distance : 127;
        }
        RadioStaticActive(noise);
    }
    if ((gQueueReady == gQueueHead || gQueue[gQueueReady].sfx != 0 ||
            gQueue[gQueueReady].car != gQueue[gQueuePlay].car) &&
        gQueuePlay != gQueueReady && gQueuePlay != gQueueHead)
        PlayNextRequest();
    LoadNextRequest();
}

sint32 CopSpeak::SfxQueued(void)
{
    FUNCTION_MARKER(0x8009AC60, "NFS4.EXE");
    sint32 count = 0;
    sint32 chkQ = gQueuePlay;
    sint32 head = gQueueHead;
    while (chkQ != head) {
        if ((sint8)gQueue[chkQ].bank >= 0 && gQueue[chkQ].sfx != 0)
            ++count;
        chkQ = chkQ < 63 ? chkQ + 1 : 0;
    }
    return count;
}

void CopSpeak::ShowQueue(void)
{
    FUNCTION_MARKER(0x8009ACCC, "NFS4.EXE");
    sint32 chkQ = gQueuePlay;
    char bufferstat[66] = { 0 };
    while (chkQ != gQueueHead)
    {
        if ((sint8)gQueue[chkQ].bank == -1)
            strcat(bufferstat, "x");
        else if (gQueue[chkQ].sfx != 0)
            strcat(bufferstat, "s");
        else if (gQueue[chkQ].bank == 3)
            strcat(bufferstat, ".");
        else if (gQueue[chkQ].phrase >= 0)
            strcat(bufferstat, "n");
        else if (gQueue[chkQ].car == 0)
            strcat(bufferstat, "D");
        else
            sprintf(bufferstat + strlen(bufferstat), "%d", gQueue[chkQ].car->carIndex);
        if (chkQ == gQueueReady || chkQ == gQueueLoad)
            strcat(bufferstat, "|");
        chkQ = chkQ < 63 ? chkQ + 1 : 0;
    }
    if (strlen(bufferstat) != 0)
    {
        Font::TextXY(bufferstat, 10, 6);
        sint32 width = textpixels(bufferstat);
        Hud::FBuildF4(1, 8, 9, (sint32)((uint32)width + 5U), 9, 0x190505, 0, 0);
    }
}
