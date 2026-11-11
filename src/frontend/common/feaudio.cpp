#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/splysdef.h"
#include "frontend/common/feaudio.h"
#include "game/common/audiocmn.h"
#include "game/common/audiomus.h"
#include "game/common/paths.h"
#include "lib/psx/getm.h"
#include "lib/psx/nullfunc.h"

struct FILEINFO
{
    uint32 offset;
    uint32 length;
    char name[1];
};

struct FILEINFO;

struct LUMPYHEAD;

struct LUMPYHEAD
{
    uint32 type;
    uint32 len;
    uint32 num;
    uint32 hlen;
};

sint32 commentaryActualLevel;
sint32 gStopCommentaryNow;
char gCurrentVIV = -1;
SPEECHINFO ginfo;
LUMPYHEAD *speechfileHeader;
char currentSpeechViv[40];
static char xport_allLanguages_string_0[] = "zEngl";
static char xport_allLanguages_string_1[] = "zGerm";
static char xport_allLanguages_string_2[] = "zFren";
static char xport_allLanguages_string_3[] = "zSpan";
static char xport_allLanguages_string_4[] = "zItal";
static char xport_allLanguages_string_5[] = "zSwed";
char *allLanguages[6] = { xport_allLanguages_string_0, xport_allLanguages_string_1, xport_allLanguages_string_2, xport_allLanguages_string_3, xport_allLanguages_string_4, xport_allLanguages_string_5 };

static void StopCommentary(bool restoreMusic)
{
    gCurrentVIV = (char)0xFF;
    if (ginfo.areLoading != 0)
    {
        while (getasyncreadstatus(ginfo.vivHandle) == 0)
            ::systemtask(0);
    }
    if (ginfo.soundIsPlaying != 0)
    {
        SNDautovol(ginfo.nSoundHandle, 0, -1);
        cSNDbankremove(ginfo.nHandle, 0);
    }
    if (restoreMusic)
        AudioMus::Volume((sint32)(uint32(gMasterMusicLevel) * 70u) >> 7);
    if (ginfo.sSpeechData != 0)
    {
        purgememadr(ginfo.sSpeechData);
        ginfo.sSpeechData = 0;
    }
    if (ginfo.lastSpeechData != 0)
    {
        purgememadr(ginfo.lastSpeechData);
        ginfo.lastSpeechData = 0;
    }
    if (ginfo.pBankHeader != 0)
    {
        purgememadr(ginfo.pBankHeader);
        ginfo.pBankHeader = 0;
    }
    ginfo.soundIsPlaying = 0;
    ginfo.playNextOne = 0;
    gStopCommentaryNow = 0;
    ginfo.areLoading = 0;
}

sint32 FeAudio::StartLoadPatch(SPEECHINFO *info)
{
    FUNCTION_MARKER(0x80015780, "FRONT.BIN");
    nullfunction();
    sint32 offset;
    sint32 length;
    LocateBigfile(speechfileHeader, info->name, &offset, &length);
    if (offset == 0 || length == 0)
        return 0;
    if (info->sSpeechData != 0)
    {
        purgememadr(info->sSpeechData);
        info->sSpeechData = 0;
    }
    info->sSpeechData = StartBigfileRead(currentSpeechViv, offset, length, &info->vivHandle);
    info->areLoading = 1;
    info->playNextOne = 0;
    return 1;
}

void FeAudio::StartPatch(SPEECHINFO *info)
{
    FUNCTION_MARKER(0x80015824, "FRONT.BIN");
    if (info->sSpeechData == 0)
        return;
    sint32 result = SNDbankadd(&info->nHandle, info->sSpeechData);
    if (result == 7)
    {
        sint32 size = SNDbankheadersize(info->nHandle);
        char *header = static_cast<char *>(reservememadr("comHeader", size, 16));
        info->pBankHeader = header;
        SNDbankheadercopy(header, info->nHandle);
        if (info->sSpeechData != 0)
        {
            purgememadr(info->sSpeechData);
            info->sSpeechData = 0;
        }
    }
    else
    {
        char *data = info->sSpeechData;
        info->sSpeechData = 0;
        info->lastSpeechData = data;
    }
    SNDPLAYOPTS playopts;
    SNDplaysetdef(&playopts);
    playopts.patnum = 0;
    playopts.bhandle = (char)info->nHandle;
    playopts.vol = (char)commentaryActualLevel;
    info->nSoundHandle = SNDplay(&playopts);
    info->soundIsPlaying = 1;
    if (info->multiplay != 0)
    {
        if ((uint8)info->name[2] < 0x7A)
        {
            gCurrentVIV = (char)((uint8)gCurrentVIV + 1u);
            info->name[2] = (char)((uint8)info->name[2] + 1u);
            StartLoadPatch(info);
        }
        else
        {
            info->areLoading = 0;
            info->playNextOne = 0;
        }
    }
}

void FeAudio::systemtask(sint32 x)
{
    FUNCTION_MARKER(0x80015948, "FRONT.BIN");
    ::systemtask(x);
    if (ginfo.areLoading != 0 && getasyncreadstatus(ginfo.vivHandle) != 0)
    {
        ginfo.areLoading = 0;
        ginfo.playNextOne = 1;
    }
    if (ginfo.soundIsPlaying != 0 && SNDover(ginfo.nSoundHandle) != 0)
    {
        SNDautovol(ginfo.nSoundHandle, 0, -1);
        cSNDbankremove(ginfo.nHandle, 0);
        if (ginfo.lastSpeechData != 0)
        {
            purgememadr(ginfo.lastSpeechData);
            ginfo.lastSpeechData = 0;
        }
        if (ginfo.pBankHeader != 0)
        {
            purgememadr(ginfo.pBankHeader);
            ginfo.pBankHeader = 0;
        }
        if (ginfo.areLoading != 0)
        {
            while (getasyncreadstatus(ginfo.vivHandle) == 0)
                ::systemtask(0);
            ginfo.areLoading = 0;
            ginfo.playNextOne = 1;
        }
        ginfo.soundIsPlaying = 0;
        ginfo.nHandle = 0;
        if (ginfo.areLoading == 0 && ginfo.playNextOne == 0)
            gStopCommentaryNow = 1;
    }
    if (gStopCommentaryNow != 0)
        StopCommentary(true);
    if (ginfo.playNextOne != 0)
    {
        uint16 active;
        memcpy(&active, &ginfo.areLoading, sizeof(active));
        if (active == 0)
        {
            StartPatch(&ginfo);
            ginfo.playNextOne = 0;
        }
    }
}

sint16 FeAudio::AsyncPlayCommentary(char *name)
{
    FUNCTION_MARKER(0x80015BCC, "FRONT.BIN");
    commentaryActualLevel = (sint32)(uint32(gMasterFENarrationLevel) * 110u) >> 7;
    if (commentaryActualLevel > 90)
        commentaryActualLevel = 90;
    strncpy(ginfo.name, name, 4);
    ginfo.nHandle = 0;
    ginfo.nSoundHandle = 0;
    ginfo.areLoading = 0;
    ginfo.soundIsPlaying = 0;
    ginfo.playNextOne = 0;
    ginfo.pBankHeader = 0;
    ginfo.sSpeechData = 0;
    ginfo.lastSpeechData = 0;
    ginfo.vivHandle = 0;
    gCurrentVIV = (char)0xFF;
    ginfo.multiplay = (uint8)*name != 'c' ? 1 : 0;
    if (StartLoadPatch(&ginfo) != 0)
        AudioMus::Volume((sint32)(uint32(gMasterMusicLevel) * 35u) >> 7);
    return 1;
}

sint16 FeAudio::AsyncPlaySpeech(sint32 type, sint32 index)
{
    FUNCTION_MARKER(0x80015CB8, "FRONT.BIN");
    char vivname[5];
    if (type == 2)
        sprintf(vivname, "%c%02d", 'c', index);
    else if (type == 0)
        sprintf(vivname, "%c%ca", 'a', (sint32)(uint32(index) + 97u));
    else
        sprintf(vivname, "%c%ca", (sint32)(uint32(type) + 97u), (sint32)(uint32(index) + 48u));
    return AsyncPlayCommentary(vivname);
}

char * FeAudio::StartBigfileRead(char *fname, sint32 offset, sint32 length, sint32 *vivHandle)
{
    FUNCTION_MARKER(0x80015D34, "FRONT.BIN");
    if (length == 0)
        return 0;
    char *streamBuffer = static_cast<char *>(reservememadr("streamBuffer", length, 0));
    if (streamBuffer == 0)
        return 0;
    setasyncfile(fname);
    *vivHandle = (sint32)asyncloadsegment(offset, streamBuffer, length);
    return streamBuffer;
}

void FeAudio::LocateBigfile(LUMPYHEAD *bigfileHeader, char *name, sint32 *offset, sint32 *length)
{
    FUNCTION_MARKER(0x80015DCC, "FRONT.BIN");
    *offset = 0;
    *length = 0;
    if (bigfileHeader == 0 || bigfileHeader->num == 0)
        return;
    FILEINFO *info = reinterpret_cast<FILEINFO *>(bigfileHeader + 1);
    uint32 i = 1;
    for (;;)
    {
        if (strcmp(name, info->name) == 0)
        {
            uint32 value = info->offset;
            *offset = (sint32)((value << 24) | ((value & 0xFF00u) << 8) |
                ((value & 0xFF0000u) >> 8) | (value >> 24));
            value = info->length;
            *length = (sint32)((value << 24) | ((value & 0xFF00u) << 8) |
                ((value & 0xFF0000u) >> 8) | (value >> 24));
            return;
        }
        char *tempChar = info->name;
        while (*tempChar != '\0')
            ++tempChar;
        info = reinterpret_cast<FILEINFO *>(tempChar + 1);
        bool next = i < bigfileHeader->num;
        ++i;
        if (!next)
            break;
    }
}

LUMPYHEAD * FeAudio::InitViv(char *fname)
{
    FUNCTION_MARKER(0x80015F24, "FRONT.BIN");
    setasyncfile(fname);
    LUMPYHEAD lumpHead;
    uint32 vivHandle = asyncloadsegment(0, &lumpHead, 16);
    do
    {
        systemtask(0);
    } while (getasyncreadstatus(vivHandle) == 0);
    lumpHead.type = getm(&lumpHead.type, 4);
    lumpHead.hlen = getm(&lumpHead.hlen, 4);
    lumpHead.num = getm(&lumpHead.num, 4);
    sint32 length = (sint32)((uint32)lumpHead.hlen + 32);
    LUMPYHEAD *bigfileHeader = static_cast<LUMPYHEAD *>(reservememadr("lumpyhead", length, 0));
    if (bigfileHeader == 0)
        return 0;
    vivHandle = asyncloadsegment(0, bigfileHeader, (sint32)((uint32)lumpHead.hlen + 32));
    do
    {
        systemtask(0);
    } while (getasyncreadstatus(vivHandle) == 0);
    uint32 type = getm(&bigfileHeader->type, 4);
    uint32 hlen = getm(&bigfileHeader->hlen, 4);
    uint32 num = getm(&bigfileHeader->num, 4);
    bigfileHeader->type = type;
    bigfileHeader->hlen = hlen;
    bigfileHeader->num = num;
    return bigfileHeader;
}

void FeAudio::InitCommentary(sint32 language, sint32)
{
    FUNCTION_MARKER(0x800160D8, "FRONT.BIN");
    ginfo.nHandle = 0;
    ginfo.multiplay = 1;
    ginfo.nSoundHandle = 0;
    ginfo.areLoading = 0;
    ginfo.soundIsPlaying = 0;
    ginfo.playNextOne = 0;
    ginfo.pBankHeader = 0;
    ginfo.sSpeechData = 0;
    ginfo.lastSpeechData = 0;
    ginfo.vivHandle = 0;
    memcpy(ginfo.name, "000", 4);
    sprintf(currentSpeechViv, "%s%s.viv", Paths_Paths[38], allLanguages[language]);
    speechfileHeader = InitViv(currentSpeechViv);
}

void FeAudio::DeInitCommentary(void)
{
    FUNCTION_MARKER(0x80016178, "FRONT.BIN");
    if (ginfo.areLoading != 0 || ginfo.soundIsPlaying != 0 || ginfo.playNextOne != 0 ||
        ginfo.sSpeechData != 0 || ginfo.lastSpeechData != 0 || ginfo.pBankHeader != 0)
        StopCommentary(false);
    if (speechfileHeader != 0)
    {
        purgememadr(speechfileHeader);
        speechfileHeader = 0;
    }
}

void FeAudio::ResetOverlayState(void)
{
    ginfo = SPEECHINFO();
    speechfileHeader = 0;
    gStopCommentaryNow = 0;
    gCurrentVIV = (char)0xFF;
    commentaryActualLevel = 0;
    memset(currentSpeechViv, 0, sizeof(currentSpeechViv));
}
