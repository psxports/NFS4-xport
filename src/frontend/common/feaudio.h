#ifndef FEAUDIO_H
#define FEAUDIO_H

#include "xport.h"

struct LUMPYHEAD;

struct SPEECHINFO;

class FeAudio
{
public:
    static sint32 StartLoadPatch(SPEECHINFO *info);
    static void StartPatch(SPEECHINFO *info);
    static void systemtask(sint32 x);
    static sint16 AsyncPlayCommentary(char *name);
    static sint16 AsyncPlaySpeech(sint32 type, sint32 index);
    static char * StartBigfileRead(char *fname, sint32 offset, sint32 length, sint32 *vivHandle);
    static void LocateBigfile(LUMPYHEAD *bigfileHeader, char *name, sint32 *offset, sint32 *length);
    static LUMPYHEAD * InitViv(char *fname);
    static void InitCommentary(sint32 language, sint32);
    static void DeInitCommentary(void);
    static void ResetOverlayState(void);
};

struct SPEECHINFO
{
    char name[4];
    sint32 multiplay;
    sint32 nHandle;
    sint32 nSoundHandle;
    char areLoading;
    char soundIsPlaying;
    char playNextOne;
    char *pBankHeader;
    char *sSpeechData;
    char *lastSpeechData;
    sint32 vivHandle;
};

extern SPEECHINFO ginfo;
extern sint32 commentaryActualLevel;
extern sint32 gStopCommentaryNow;
extern char gCurrentVIV;
extern LUMPYHEAD *speechfileHeader;
extern char currentSpeechViv[40];
extern char *allLanguages[6];

#endif
