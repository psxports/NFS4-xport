#ifndef AUDIOMUS_H
#define AUDIOMUS_H

#include "xport.h"
#include "game/common/car.h"

class AudioMus
{
public:
    struct tSongEntry;

    struct tCurrentSong;

    struct tSongList;

    struct tMusicGlobals;
    static tMusicGlobals *g;

    static void RefreshStatus(void);
    static sint32 Threshold(void);
    static sint32 Buffered(void);
    static tCurrentSong * GetCurrentSong(void);
    static void SwitchSong(void);
    static void Fail(sint32 errorcode);
    static void QueueRequestedSong(void);
    static void SetEntry(tSongEntry *info);
    static void SetCurrentSongInfo(void);
    static sint32 Server(sint32 mode, sint32 ticks);
    static tSongList * GetSongList(char *pattern, sint32 memtype);
    static void InitGlobals(void);
    static void InitDriverGlobals(void);
    static void DriverStartUp(sint32 buffersize, sint32 spusize);
    static void SysStartUp(sint32 buffersize, sint32 spusize, char *songs);
    static void DriverCleanUp(void);
    static void SysCleanUp(void);
    static sint32 SetPaused(bool paused);
    static void StopSong(sint32 fadeticks);
    static void BuildPlayList(sint32 numplaylistsongs, sint32 *playlist);
    static void BuildPattern(char *pattern);
    static sint32 PlaySong(char *pattern);
    static void Volume(sint32 volume);
    static void AutoVolume(sint32 fadeticks, sint32 volume);
};

struct AudioMus::tSongEntry
{
    char *filename;
    char *title;
    char *artist;
    char *label;
    char *date;
    char *notes;
    sint32 length;
    sint32 index;
    char strbuf[32];
};

struct AudioMus::tCurrentSong
{
    sint32 newsong;
    sint32 index;
    sint32 remaining;
    AudioMus::tSongEntry info;
};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4200)
#endif
struct AudioMus::tSongList
{
    sint32 numsongs;
    sint32 currentsong;
    AudioMus::tSongEntry song[0];
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
