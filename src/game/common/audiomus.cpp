#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/slimits.h"
#include "eaclib/psx/splysdef.h"
#include "eaclib/psx/sstrstat.h"
#include "eaclib/psx/sststat.h"
#include "game/common/audiomus.h"
#include "game/common/paths.h"
#include "game/psx/audio.h"
#include "game/psx/hud.h"

struct AudioMus::tMusicGlobals
{
    sint32 volume;
    sint32 totalsongs;
    sint32 availablesongs;
    sint32 requestsong;
    sint32 firstswitch;
    sint32 switchsong;
    sint32 newswitch;
    sint32 failby;
    sint32 errorcode;
    sint32 fadetime;
    sint32 randomize;
    char *songname;
    char title64[64];
    char *streambuffer;
    sint32 streamhandle;
    sint32 requesthandle;
    sint32 serveractive;
    sint32 driveractive;
    sint32 greedy;
    sint32 threshold;
    char *bigfileheader;
    SNDSTREAMSTATUS streamstatus;
    SNDREQUESTSTATUS requeststatus;
    char bigfilename[64];
    char playlist[32];
    AudioMus::tCurrentSong current;
};

AudioMus::tMusicGlobals *AudioMus::g;

void AudioMus::RefreshStatus(void)
{
    FUNCTION_MARKER(0x80079EF4, "NFS4.EXE");
    if (g->streamhandle < 0) {
        g->streamstatus.outstandingrequests = 0;
    } else {
        SNDSTRM::status(g->streamhandle, &g->streamstatus);
        if (g->streamstatus.outstandingrequests > 0)
            SNDSTRM::requeststatus(g->streamstatus.currentrequest, &g->requeststatus);
    }
}

sint32 AudioMus::Threshold(void)
{
    FUNCTION_MARKER(0x80079F58, "NFS4.EXE");
    tMusicGlobals *state = g;
    if (state == 0 || state->bigfileheader == 0 || state->errorcode != 0)
        return 0;
    if (state->switchsong != 2) {
        if (state->streamstatus.outstandingrequests == 0)
            return 0;
        if (state->requeststatus.timetoend <= state->requeststatus.timebuffered)
            return 0;
    }
    return state->threshold;
}

sint32 AudioMus::Buffered(void)
{
    FUNCTION_MARKER(0x80079FDC, "NFS4.EXE");
    tMusicGlobals *state = g;
    if (state == 0 || state->bigfileheader == 0 || state->streamstatus.outstandingrequests == 0)
        return 0;
    return state->requeststatus.timebuffered;
}

AudioMus::tCurrentSong * AudioMus::GetCurrentSong(void)
{
    FUNCTION_MARKER(0x8007A028, "NFS4.EXE");
    if (g == 0)
        return 0;
    tCurrentSong *curr = &g->current;
    curr->remaining = g->requeststatus.timetoend;
    sint32 index = g->errorcode;
    if (index == 0)
        index = (g->requestsong + 1);
    curr->index = index;
    tSongEntry *info = &curr->info;
    static char bufferError[] = "BUFFER NOT ALLOCATED";
    static char streamError[] = "STREAM NOT CREATED";
    if (g->errorcode == -4)
        info->title = bufferError;
    else if (g->errorcode == -3)
        info->title = streamError;
    if (g->newswitch == 0) {
        g->current.newsong = 0;
    } else {
        g->newswitch = 0;
        g->current.newsong = 1;
    }
    return &g->current;
}

void AudioMus::SwitchSong(void)
{
    FUNCTION_MARKER(0x8007A0E4, "NFS4.EXE");
    tMusicGlobals *state = g;
    if (state == 0 || state->availablesongs == 0 || state->volume <= 0)
        return;
    state->newswitch = 1;
    state->songname = 0;
    state->errorcode = 0;
    state->current.remaining = 0;
    tSongEntry *info = &state->current.info;
    info->length = 0;
    info->filename = 0;
    info->title = 0;
    info->artist = 0;
    info->label = 0;
    info->notes = 0;
    if (state->switchsong == 0 && state->requestsong >= 0) {
        SNDSTRM::autovol(state->streamhandle, 2000, 0);
        g->fadetime = 1000;
    }
    state = g;
    sint32 next = (state->requestsong + 1);
    sint32 turnOff = Hud::kTurnSongOffNext;
    state->requestsong = next;
    if (turnOff == 0 && next < state->availablesongs) {
        state->switchsong = 1;
    } else {
        Hud::kTurnSongOffNext = 0;
        state->firstswitch = 0;
        state->switchsong = -1;
        state->requestsong = -1;
    }
}

void AudioMus::Fail(sint32 errorcode)
{
    FUNCTION_MARKER(0x8007A1DC, "NFS4.EXE");
    sint32 outstanding = g->streamstatus.outstandingrequests;
    g->errorcode = errorcode;
    if (outstanding != 0) {
        sint32 ticks = Buffered();
        SNDSTRM::autovol(g->streamhandle, ticks, 0);
        ticks = Buffered();
        g->fadetime = ticks;
    }
    tMusicGlobals *state = g;
    state->newswitch = 1;
    state->firstswitch = 0;
    state->songname = 0;
    state->switchsong = -1;
    state->requestsong = -1;
}

void AudioMus::QueueRequestedSong(void)
{
    FUNCTION_MARKER(0x8007A258, "NFS4.EXE");
    sint32 offset;
    uint8 *bytes = (uint8 *)g;
    sint32 entry = bytes[offsetof(tMusicGlobals, playlist) + g->requestsong];
    char *name = locatebigentry(g->bigfileheader, 0, entry, &offset, 0);
    g->songname = name;
    if (g->streamhandle >= 0) {
        sint32 handle = SNDSTRM::queuefile(g->streamhandle, 1000, g->bigfilename, offset);
        g->requesthandle = handle;
    }
    g->switchsong = 2;
    sint32 tick = gettick();
    tMusicGlobals *state = g;
    state->failby = (tick + 0x280);
    state->current.remaining = 0;
    tSongEntry *info = &state->current.info;
    info->length = 0;
    info->filename = 0;
    info->title = 0;
    info->artist = 0;
    info->label = 0;
    info->notes = 0;
}

void AudioMus::SetEntry(AudioMus::tSongEntry *info)
{
    FUNCTION_MARKER(0x8007A308, "NFS4.EXE");
    char *p = info->filename;
    sint32 titlechar = 0;
    sint32 havefile = 0;
    info->artist = 0;
    info->label = 0;
    info->date = 0;
    info->notes = 0;
    while (*p != 0) {
        if (*p == '-') {
            if (havefile != 0) {
                info->artist = p + 1;
                break;
            }
            havefile = 1;
            titlechar = 0;
        } else if (titlechar < 31) {
            info->strbuf[titlechar++] = *p;
        }
        ++p;
    }
    info->strbuf[titlechar] = 0;
    info->title = info->strbuf;
}

void AudioMus::SetCurrentSongInfo(void)
{
    FUNCTION_MARKER(0x8007A390, "NFS4.EXE");
    tMusicGlobals *state = g;
    sint32 remaining = state->requeststatus.timetoend;
    sint32 current = state->requeststatus.currenttime;
    tSongEntry *info = &state->current.info;
    state->current.remaining = remaining;
    info->length = (remaining + current);
    info->filename = state->songname;
    SetEntry(info);
}

sint32 AudioMus::Server(sint32 mode, sint32 ticks)
{
    FUNCTION_MARKER(0x8007A3D0, "NFS4.EXE");
    if (SNDSTRM::IsPaused() || g->bigfileheader == 0)
        return 0;
    if (g->bigfilename[0] != '.' && CdDiskReady(1) == 16) {
        if (g->errorcode == 0) {
            sint32 request = g->requestsong;
            g->errorcode = -2;
            g->newswitch = 1;
            if (request >= 0) {
                sint32 buffered = Buffered();
                SNDSTRM::autovol(g->streamhandle, buffered, 0);
            }
        }
        return 0;
    }
    if (g->errorcode == -2) {
        if (CdDiskReady(1) != 2)
            return 0;
        tMusicGlobals *state = g;
        sint32 request = state->requestsong;
        state->errorcode = -5;
        if (request < 0)
            return 0;
        state->newswitch = 1;
        state->switchsong = 2;
        sint32 tick = gettick();
        g->failby = (tick + 0x280);
        return 0;
    }
    RefreshStatus();
    if (Threshold() != 0 && g->switchsong != 2) {
        if (Buffered() < 550) {
            Fail(-5);
        } else if (Buffered() < 1500) {
            if (g->greedy == 0) {
                SNDSTRM::setgreedystate(g->streamhandle, 1);
                g->greedy = 1;
            }
        } else {
            sint32 buffered = Buffered();
            if (buffered >= g->threshold && g->greedy != 0) {
                SNDSTRM::setgreedystate(g->streamhandle, 0);
                g->greedy = 0;
            }
        }
    }
    tMusicGlobals *state = g;
    if (state->switchsong == 0) {
        if (state->streamstatus.outstandingrequests != 0 || state->requestsong < 0)
            return 0;
        sint32 request = state->requestsong;
        sint32 available = state->availablesongs;
        if (available > 1) {
            if (state->randomize == 0) {
                state->requestsong = (request + 1) % available;
            } else {
                sint32 first = GetRCnt(0);
                sint32 draw = GetRCnt(0);
                if (first <= 0)
                    draw = (-draw);
                sint32 candidate = ((request + 1) + ((draw % (available - 1))));
                state->requestsong = candidate % available;
            }
        }
        SNDSTRM::vol(g->streamhandle, 0);
        QueueRequestedSong();
        state = g;
        state->newswitch = 1;
        state->firstswitch = 1;
        return 0;
    }
    if (state->switchsong == 2) {
        if (state->streambuffer == 0) {
            Fail(-4);
            return 0;
        }
        if (state->streamhandle < 0) {
            Fail(-3);
            return 0;
        }
        if (state->streamstatus.outstandingrequests == 0 || state->requeststatus.timebuffered <= state->threshold)
            return 0;
        sint32 handle;
        sint32 volume;
        if (state->errorcode == -5) {
            handle = state->streamhandle;
            volume = state->volume;
            state->errorcode = 0;
        } else {
            SetCurrentSongInfo();
            handle = g->streamhandle;
            volume = g->volume;
        }
        SNDSTRM::autovol(handle, 2000, volume);
        g->switchsong = 0;
        return 0;
    }
    if (state->streamstatus.outstandingrequests != 0 && SNDSTRM::getvol(state->streamhandle) != 0)
        return 0;
    if (g->streamhandle >= 0)
        SNDSTRM::purge(g->streamhandle);
    state = g;
    sint32 switching = state->switchsong;
    state->songname = 0;
    if (switching != 1) {
        state->switchsong = 0;
        return 0;
    }
    state->fadetime = 0;
    QueueRequestedSong();
    g->switchsong = 2;
    sint32 tick = gettick();
    g->failby = (tick + 0x280);
    return 0;
}

AudioMus::tSongList * AudioMus::GetSongList(char *pattern, sint32 memtype)
{
    FUNCTION_MARKER(0x8007A880, "NFS4.EXE");
    if (g == 0) {
        tSongList *list = (tSongList *)reservememadr("Song List", sizeof(tSongList), memtype);
        list->numsongs = 0;
        list->currentsong = -1;
        return list;
    }
    sint32 numsongs = 0;
    for (sint32 i = 0; i < g->totalsongs; ++i) {
        char *name = locatebigentry(g->bigfileheader, 0, i, 0, 0);
        if (wildcard(name, pattern) != 0)
            numsongs = (numsongs + 1);
    }
    sint32 length = (sint32)(sizeof(tSongList) + (uint32)numsongs * sizeof(tSongEntry));
    tSongList *list = (tSongList *)reservememadr("Song List", length, memtype);
    tSongEntry *song = list->song;
    list->numsongs = 0;
    list->currentsong = -1;
    for (sint32 i = 0; i < g->totalsongs; ++i) {
        sint32 size;
        char *songname = locatebigentry(g->bigfileheader, 0, i, 0, &size);
        if (wildcard(songname, pattern) != 0) {
            song->filename = songname;
            SetEntry(song);
            sint32 product = (size * 10);
            song->index = i;
            song->length = product / 252;
            if (songname == g->songname)
                list->currentsong = i;
            ++song;
            list->numsongs = (list->numsongs + 1);
        }
    }
    return list;
}

void AudioMus::InitGlobals(void)
{
    FUNCTION_MARKER(0x8007AA54, "NFS4.EXE");
    tMusicGlobals *state = g;
    state->bigfileheader = 0;
    state->streambuffer = 0;
    state->streamhandle = -1;
    state->serveractive = 0;
    state->driveractive = 0;
    state->totalsongs = 0;
}

void AudioMus::InitDriverGlobals(void)
{
    FUNCTION_MARKER(0x8007AA78, "NFS4.EXE");
    tMusicGlobals *state = g;
    state->requestsong = -1;
    tSongEntry *info = &state->current.info;
    state->volume = 0;
    state->fadetime = 0;
    state->availablesongs = 0;
    state->firstswitch = 0;
    state->newswitch = 0;
    state->songname = 0;
    state->switchsong = 0;
    state->errorcode = 0;
    state->greedy = 0;
    state->current.remaining = 0;
    info->length = 0;
    info->filename = 0;
    info->title = 0;
    info->artist = 0;
    info->label = 0;
    info->notes = 0;
    state->driveractive = 1;
}

void AudioMus::DriverStartUp(sint32 buffersize, sint32 spusize)
{
    FUNCTION_MARKER(0x8007AAD4, "NFS4.EXE");
    if (g == 0)
        return;
    if (g->driveractive == 0)
        InitDriverGlobals();
    g->threshold = (buffersize + spusize) >> 5;
    if (g->streamhandle < 0 && g->streambuffer != 0) {
        sint32 chunks = buffersize / 1024;
        sint32 size = (buffersize + SNDSTRM::overhead(1, chunks));
        SNDLIMITS sndlimits;
        SNDgetlimits(&sndlimits);
        sndlimits.packetbufsize = spusize;
        SNDsetlimits(&sndlimits);
        SNDPLAYOPTS opts;
        SNDplaysetdef(&opts);
        opts.vol = 0;
        sint32 handle = SNDSTRM::create(&opts, 1, chunks, g->streambuffer, size);
        g->streamhandle = handle;
        if (handle >= 0) {
            SNDSTRM::setgreedylevel(handle, 0);
            SNDSTRM::setpriority(g->streamhandle, 255, 5);
        }
    }
    sint32 active = g->serveractive;
    gMusicHandle = g->streamhandle;
    if (active == 0) {
        addsystemtask(Server, 25, 0);
        g->serveractive = 1;
    }
}

void AudioMus::SysStartUp(sint32 buffersize, sint32 spusize, char *songs)
{
    FUNCTION_MARKER(0x8007AC18, "NFS4.EXE");
    if (g != 0)
        return;
    g = (tMusicGlobals *)reservememadr("Music Globals", sizeof(tMusicGlobals), 0);
    if (g == 0)
        return;
    InitGlobals();
    sint32 overhead = SNDSTRM::overhead(1, buffersize / 1024);
    sint32 size = (buffersize + overhead);
    char *buffer = (char *)reservememadr("Music Buffer", size, 0);
    g->streambuffer = buffer;
    DriverStartUp(buffersize, spusize);
    sprintf(g->bigfilename, "%szzz%s.viv", Paths_Paths[27], songs);
    char *header = loadbigfileheader(g->bigfilename, 0);
    g->bigfileheader = header;
    if (header != 0) {
        sint32 count = bigcount(header);
        g->totalsongs = count;
    }
}

void AudioMus::DriverCleanUp(void)
{
    FUNCTION_MARKER(0x8007AD10, "NFS4.EXE");
    if (g == 0)
        return;
    if (g->serveractive != 0) {
        delsystemtask(Server);
        g->serveractive = 0;
    }
    if (g->streamhandle >= 0) {
        SNDSTRM::destroy(g->streamhandle);
        g->streamhandle = -1;
    }
    g->driveractive = 0;
}

void AudioMus::SysCleanUp(void)
{
    FUNCTION_MARKER(0x8007AD8C, "NFS4.EXE");
    if (g == 0)
        return;
    DriverCleanUp();
    if (g->streambuffer != 0)
        purgememadr(g->streambuffer);
    if (g->bigfileheader != 0)
        purgememadr(g->bigfileheader);
    purgememadr(g);
    g = 0;
}

sint32 AudioMus::SetPaused(bool paused)
{
    return g != 0 && g->streamhandle >= 0 ? SNDSTRM::SetPaused(g->streamhandle, paused) : -1;
}

void AudioMus::StopSong(sint32 fadeticks)
{
    FUNCTION_MARKER(0x8007AE04, "NFS4.EXE");
    if (g == 0 || g->requestsong < 0)
        return;
    if (fadeticks == 0) {
        if (g->streamhandle >= 0)
            SNDSTRM::purge(g->streamhandle);
        tMusicGlobals *state = g;
        state->fadetime = 0;
        state->songname = 0;
        state->switchsong = 0;
    } else {
        if (g->switchsong == 0) {
            if (g->streamhandle < 0) {
                g->fadetime = 0;
            } else {
                SNDSTRM::autovol(g->streamhandle, fadeticks, 0);
                g->fadetime = fadeticks;
            }
            g->songname = 0;
        }
        g->switchsong = -1;
    }
    g->requestsong = -1;
}

void AudioMus::BuildPlayList(sint32 numplaylistsongs, sint32 *playlist)
{
    FUNCTION_MARKER(0x8007AED8, "NFS4.EXE");
    if (g == 0)
        return;
    g->availablesongs = 0;
    for (sint32 i = 0; i < numplaylistsongs; ++i) {
        sint32 entry = playlist[i];
        if (entry >= 0 && entry < g->totalsongs) {
            uint8 *bytes = (uint8 *)g;
            bytes[offsetof(tMusicGlobals, playlist) + g->availablesongs] = (uint8)entry;
            g->availablesongs = (g->availablesongs + 1);
        }
    }
}

void AudioMus::BuildPattern(char *pattern)
{
    FUNCTION_MARKER(0x8007AF60, "NFS4.EXE");
    if (g == 0)
        return;
    sint32 total = g->totalsongs;
    g->availablesongs = 0;
    if (total <= 0)
        return;
    for (sint32 i = 0; ; ++i) {
        if (g->availablesongs >= 32)
            return;
        char *name = locatebigentry(g->bigfileheader, 0, i, 0, 0);
        if (wildcard(name, pattern) != 0) {
            g->playlist[g->availablesongs] = (char)i;
            g->availablesongs = (g->availablesongs + 1);
        }
        if (i + 1 >= g->totalsongs)
            break;
    }
}

sint32 AudioMus::PlaySong(char *pattern)
{
    FUNCTION_MARKER(0x8007B030, "NFS4.EXE");
    if (g == 0)
        return 0;
    if (pattern != 0) {
        BuildPattern(pattern);
        if (g->availablesongs == 0 && strlen(pattern) < 61) {
            char title[128];
            sprintf(title, "*-%s", pattern);
            BuildPattern(title);
        }
    }
    if (g->volume == 0)
        return 0;
    sint32 available = g->availablesongs;
    if (available != 0) {
        g->randomize = 1;
        sint32 newsong;
        if (available == 1) {
            newsong = 0;
        } else {
            sint32 previous = g->requestsong;
            sint32 alternatives = (available - 1);
            sint32 first = GetRCnt(0);
            sint32 draw = GetRCnt(0);
            if (first <= 0)
                draw = (-draw);
            if (pattern != 0) {
                if (available == -1 && draw == INT_MIN)
                    abort();
                newsong = draw % available;
            } else {
                if (alternatives == 0 || (alternatives == -1 && draw == INT_MIN))
                    abort();
                sint32 candidate = ((previous + 1) + ((draw % alternatives)));
                if (available == -1 && candidate == INT_MIN)
                    abort();
                newsong = candidate % available;
            }
        }
        tMusicGlobals *state = g;
        if (state->switchsong != 0) {
            state->switchsong = 1;
            state->requestsong = newsong;
        } else if (state->requestsong < 0) {
            state->fadetime = 0;
            SNDSTRM::vol(state->streamhandle, 0);
            g->requestsong = newsong;
            QueueRequestedSong();
        } else {
            SNDSTRM::autovol(state->streamhandle, 2000, 0);
            state = g;
            state->fadetime = 2000;
            state->switchsong = 1;
            state->requestsong = newsong;
            state->songname = 0;
        }
        state = g;
        state->firstswitch = 1;
        state->errorcode = 0;
        if (pattern != 0)
            state->newswitch = 1;
    }
    return g->availablesongs;
}

void AudioMus::Volume(sint32 volume)
{
    FUNCTION_MARKER(0x8007B2B0, "NFS4.EXE");
    if (g == 0 || g->volume == volume)
        return;
    sint32 ticksleft = 0;
    if (volume == 0) {
        g->volume = 0;
        StopSong(0);
        return;
    }
    if (g->fadetime != 0 && g->streamhandle >= 0) {
        sint32 curvol = SNDSTRM::getvol(g->streamhandle);
        if (curvol > 0 && g->volume > 0) {
            sint32 product = (g->fadetime * curvol);
            ticksleft = product / g->volume;
        }
    }
    if (ticksleft == 0) {
        if (g->streamhandle >= 0 && g->switchsong != 2)
            SNDSTRM::vol(g->streamhandle, volume);
    } else {
        sint32 product = (volume * ticksleft);
        sint32 fade = g->fadetime;
        if (fade == 0 || (fade == -1 && product == INT_MIN))
            abort();
        SNDSTRM::vol(g->streamhandle, product / fade);
        SNDSTRM::autovol(g->streamhandle, ticksleft, 0);
    }
    if (g->volume == 0) {
        g->volume = volume;
        PlaySong(0);
    } else {
        g->volume = volume;
    }
}

void AudioMus::AutoVolume(sint32 fadeticks, sint32 volume)
{
    FUNCTION_MARKER(0x8007B46C, "NFS4.EXE");
    if (g == 0 || g->volume == volume)
        return;
    if (volume == 0) {
        g->volume = 0;
        StopSong(0);
        return;
    }
    if (g->volume == 0) {
        g->volume = volume;
        PlaySong(0);
    } else {
        g->volume = volume;
    }
    if (g->switchsong != 2 && g->fadetime == 0 && g->streamhandle >= 0)
        SNDSTRM::autovol(g->streamhandle, fadeticks, volume);
}
