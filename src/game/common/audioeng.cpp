#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/splysdef.h"
#include "game/common/audiocmn.h"
#include "game/common/audioeng.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/gmesetup.h"
#include "game/common/paths.h"
#include "lib/psx/fixdmult.h"

struct AudioEng::AudioEng_t
{
    AudioEng::tAdjustments adjust;
    sint32 tick;
    sint32 vol[16];
    sint32 azi;
    sint32 sep;
    sint32 dop;
    char bhandle;
    char pad;
    AudioEng::tState queue[16];
    char setpos;
    char plypos;
    AudioEng::tChanAttr chan[16];
    AudioEng::tVoiceAttr left[16];
    AudioEng::tVoiceAttr right[16];
    char delay[16];
    char *tables;
};

struct AudioEng::tDef
{
    sint32 id;
    char ver;
    char resolved;
    char startevent;
    char controllerpercent;
    sint16 lowrandomtargetlevel;
    sint16 lowrandomtargetrange;
    sint32 lowrandomattacktime;
    sint32 lowrandomattackrange;
    sint16 highrandomtargetlevel;
    sint16 highrandomtargetrange;
    sint32 highrandomattacktime;
    sint32 highrandomattackrange;
    char patchnum[8];
    AudioEng::tEvent event[16];
    AudioEng::tTable * pvoltable[8];
    AudioEng::tTable * pbendtable[8];
};

AudioEng::AudioEng_t *AudioEng::g[2];

static AudioEng::AudioEng_t *WritableEngine(sint32 player)
{
    if ((uint32)player >= 2)
        return 0;
    AudioEng::AudioEng_t *engine = AudioEng::g[player];
    if (engine != 0 && (uint8)engine->plypos == 15 && (((uint8)engine->setpos + 1) & 1) != 0)
        return 0;
    return engine;
}

void AudioEng::Set(sint32 player, sint32 vol, sint32 esp, sint32 gas, sint32 cam, sint32 dop, sint32 azi, sint32 dir)
{
    FUNCTION_MARKER(0x8007B5A8, "NFS4.EXE");
    AudioEng_t *g = WritableEngine(player);
    if (g == 0)
        return;
    tAdjustments *a = &g->adjust;
    tState *s = &g->queue[(uint8)g->setpos];
    sint32 timbre = (a->timbreScale * esp) >> 14;
    if (timbre >= 512)
        timbre = 511;
    s->esp = (uint16)timbre;
    sint32 speed;
    if (Cars::gList[player]->carInfo->carType == 28)
        speed = ((esp >> 2) + 0xc000);
    else
        speed = (esp + 0x3333);
    sint32 pitch = fixedmult(speed, dop);
    s->dop = (uint16)((a->pitchScale * pitch) >> 10);
    sint32 throttle = ((gas + (gas >> 5)) + (gas >> 6));
    s->gas = throttle < 129 ? (uint8)throttle : 128;
    uint8 boost;
    if (cam == 0) {
        s->exh = a->inCarExhaust;
        if (GameSetup::gData.commMode == 1) {
            s->sep = 0;
            s->azi = (uint16)azi;
        } else {
            s->azi = 0;
            s->sep = 0x3fff;
        }
        boost = a->inCarBoost;
    } else {
        s->exh = a->outCarExhaust;
        uint8 exhaust = s->exh;
        sint32 product;
        if (dir < 0)
            product = ((dir * a->fwdEngBoost) * exhaust);
        else
            product = ((dir * a->rwdExhBoost) * (128 - exhaust));
        sint32 modulation = (product >> 7) / 0x10000;
        s->exh = (uint8)(exhaust + modulation);
        s->azi = (uint16)azi;
        s->sep = 0;
        boost = a->outCarBoost;
    }
    sint32 volume = (vol * boost) >> 6;
    if (volume >= 2048)
        volume = 2047;
    s->vol = (uint16)volume;
    g->setpos = (char)(((uint8)g->setpos + 1) & 15);
}

void AudioEng::Update(void)
{
    FUNCTION_MARKER(0x8007B824, "NFS4.EXE");
    for (sint32 player = 0; player < 2; ++player) {
        AudioEng_t *g = AudioEng::g[player];
        if (g == 0)
            return;
        tState *s = &g->queue[(uint8)g->plypos];
        sint32 tick = gettick();
        if ((uint8)g->plypos != (uint8)g->setpos) {
            sint32 now = gettick();
            if (g->tick < now) {
                g->tick = (tick + 2);
                for (sint32 n = 0; n < 16; ++n) {
                    if ((sint8)g->chan[n].patchnum < 0)
                        continue;
                    sint32 gain = 0;
                    if (s->esp >= g->chan[n].min && s->esp < g->chan[n].max)
                        gain = (sint8)g->chan[n].xlate[s->esp - g->chan[n].min];
                    uint32 vol = 0;
                    if (gain != 0) {
                        sint32 gasIndex = n < 8 ? 128 - s->gas : s->gas;
                        vol = (uint32)gain * (uint8)Xfade[gasIndex];
                        uint32 exhaust;
                        if ((sint8)g->chan[n].patchnum < 64) {
                            exhaust = (uint8)Xfade[s->exh];
                        } else {
                            exhaust = (uint8)Xfade[128 - s->exh];
                            if (s->sep != 0)
                                exhaust = (exhaust * 47) >> 6;
                        }
                        vol *= exhaust;
                    }
                    vol = (vol * s->vol) >> 21;
                    if (vol > 127)
                        vol = 127;
                    g->vol[n] = vol;
                    g->azi = s->azi;
                    g->sep = s->sep;
                    g->dop = s->dop;
                }
                g->plypos = (char)(((uint8)g->plypos + 1) & 15);
            }
        }
        for (sint32 n = 0; n < 16; ++n) {
            if ((sint8)g->chan[n].patchnum < 0)
                continue;
            if (g->vol[n] == 0) {
                if (g->left[n].handle != -1) {
                    SNDstop(g->left[n].handle);
                    if (g->right[n].handle != -1)
                        SNDstop(g->right[n].handle);
                    g->left[n].handle = -1;
                    g->right[n].handle = -1;
                }
                continue;
            }
            uint16 leftazim;
            uint16 rightazim;
            if ((sint8)g->chan[n].patchnum < 64) {
                leftazim = (uint16)g->azi;
                rightazim = leftazim;
            } else {
                leftazim = (uint16)(((uint16)g->azi) - (uint16)g->sep);
                rightazim = (uint16)(((uint16)g->azi) + (uint16)g->sep);
            }
            SNDPLAYOPTS playopts;
            SNDplaysetdef(&playopts);
            playopts.bhandle = g->bhandle;
            playopts.vol = 0;
            playopts.patnum = (sint8)g->chan[n].patchnum;
            playopts.pitchmult = (uint16)g->dop;
            playopts.use3dpos = 1;
            playopts.elevation = 0;
            if (g->left[n].handle == -1) {
                playopts.azimuth = leftazim;
                g->left[n].handle = SNDplay(&playopts);
                g->delay[n] = 2;
                g->left[n].vol = 0;
                g->left[n].azim = leftazim;
                g->left[n].pitch = (uint16)g->dop;
                continue;
            }
            if (g->sep == 0 || (sint8)g->chan[n].patchnum < 64 || g->right[n].handle != -1) {
                sint32 current = (sint8)g->left[n].vol;
                sint32 target = g->vol[n];
                if (current != target) {
                    if (target < current) {
                        current -= 2;
                        if (current < target)
                            current = target;
                    } else {
                        current += 2;
                        if (target < current)
                            current = target;
                    }
                    g->left[n].vol = (char)current;
                    SNDvol(g->left[n].handle, (sint8)g->left[n].vol);
                }
            }
            if (g->left[n].azim != leftazim) {
                SND3dpos(g->left[n].handle, leftazim, 0);
                g->left[n].azim = leftazim;
            }
            if (g->left[n].pitch != g->dop) {
                SNDpitchmult(g->left[n].handle, g->dop);
                g->left[n].pitch = (uint16)g->dop;
            }
            if ((sint8)g->chan[n].patchnum < 64 || g->sep == 0) {
                if (g->right[n].handle != -1) {
                    SNDstop(g->right[n].handle);
                    g->right[n].handle = -1;
                    g->delay[n] = 2;
                }
            } else if (g->right[n].handle == -1) {
                g->delay[n] = (char)((uint8)g->delay[n] - 1);
                if ((sint8)g->delay[n] == -1) {
                    playopts.azimuth = rightazim;
                    g->right[n].handle = SNDplay(&playopts);
                    g->right[n].vol = 0;
                    g->right[n].azim = rightazim;
                    g->right[n].pitch = (uint16)g->dop;
                }
            } else {
                sint32 current = (sint8)g->right[n].vol;
                sint32 target = g->vol[n];
                if (current != target) {
                    if (target < current) {
                        current -= 2;
                        if (current < target)
                            current = target;
                    } else {
                        current += 2;
                        if (target < current)
                            current = target;
                    }
                    g->right[n].vol = (char)current;
                    SNDvol(g->right[n].handle, (sint8)g->right[n].vol);
                }
                if (g->right[n].azim != rightazim) {
                    SND3dpos(g->right[n].handle, rightazim, 0);
                    g->right[n].azim = rightazim;
                }
                if (g->right[n].pitch != g->dop) {
                    SNDpitchmult(g->right[n].handle, g->dop);
                    g->right[n].pitch = (uint16)g->dop;
                }
            }
        }
    }
}

void AudioEng::LoadDef(char *filename, char *name, NativeFileSlot *handle, sint32 offset, sint32 size, AudioEng::tDef **ed)
{
    FUNCTION_MARKER(0x8007BDDC, "NFS4.EXE");
    *ed = 0;
    if (size < 360)
        return;
    tDef *definition = static_cast<tDef *>(reservememadr(name, (sint32)(sizeof(tDef) + size), 16));
    if (definition != 0)
    {
        char *serialized = reinterpret_cast<char *>(definition + 1);
        XPORT_UNPLACED::FILE::readsync(handle, offset, serialized, size, 100);
        memcpy(definition, serialized, 296);
        for (sint32 i = 0; i < 8; ++i)
        {
            definition->pvoltable[i] = 0;
            definition->pbendtable[i] = 0;
        }
        *ed = definition;
    }
}

sint32 AudioEng::StartUp(sint32 player, char *carname)
{
    FUNCTION_MARKER(0x8007BE54, "NFS4.EXE");
    if ((uint32)player >= 2 || AudioEng::g[player] != 0)
        return 0;
    sint32 tablesize = 0;
    sint32 spu = 0;
    AudioEng_t *g = static_cast<AudioEng_t *>(reservememadr("Engine Audio", sizeof(AudioEng_t), 0));
    AudioEng::g[player] = g;
    g->tables = 0;
    g->tick = 0;
    g->azi = 0;
    g->sep = 0;
    g->dop = 0x1000;
    for (sint32 i = 0; i < 16; ++i) {
        g->vol[i] = 0;
        g->left[i].handle = -1;
        g->right[i].handle = -1;
        g->chan[i].patchnum = (char)-1;
        g->chan[i].min = 512;
        g->chan[i].max = 512;
        g->chan[i].xlate = 0;
    }
    tDef *cruisedef = 0;
    tDef *loaddef = 0;
    g->adjust.inCarBoost = 50;
    g->adjust.inCarExhaust = 31;
    g->adjust.outCarBoost = 166;
    g->adjust.outCarExhaust = 43;
    g->adjust.fwdEngBoost = 64;
    g->adjust.rwdExhBoost = 121;
    g->adjust.pitchScale = 80;
    g->adjust.timbreScale = 104;
    g->setpos = 0;
    g->plypos = 0;
    bool bankloaded = false;
    char filename[64];
    sprintf(filename, GameSetup::gData.commMode == 1 ? "%s%sens.viv" : "%s%seng.viv", Paths_Paths[28], carname);
    char *header = loadbigfileheader(filename, 16);
    if (header == 0) {
        sprintf(filename, GameSetup::gData.commMode == 1 ? "%sp993ens.viv" : "%sp993eng.viv", Paths_Paths[28]);
        header = loadbigfileheader(filename, 16);
    }
    if (header != 0) {
        NativeFileSlot *handle;
        XPORT_UNPLACED::FILE::opensync(filename, 1, 100, &handle);
        static char bankPattern[] = "*.bnk";
        static char loadPattern[] = "*.ltb";
        static char cruisePattern[] = "*.ctb";
        for (sint32 i = 0; i < bigcount(header); ++i) {
            sint32 size;
            sint32 offset;
            char *name = locatebigentry(header, 0, i, &offset, &size);
            if (wildcard(name, bankPattern) != 0 && !bankloaded) {
                char *pdata = static_cast<char *>(reservememadr(name, size, 16));
                if (pdata != 0) {
                    bankloaded = true;
                    XPORT_UNPLACED::FILE::readsync(handle, offset, pdata, size, 100);
                    spu = AudioCmn::AddBank(name, size, pdata, player);
                    g->bhandle = (char)gSndBnk[player].bnkID;
                }
            } else if (wildcard(name, loadPattern) != 0 && loaddef == 0) {
                LoadDef(filename, name, handle, offset, size, &loaddef);
            } else if (wildcard(name, cruisePattern) != 0 && cruisedef == 0) {
                LoadDef(filename, name, handle, offset, size, &cruisedef);
            }
        }
        XPORT_UNPLACED::FILE::closesync(handle, 100);
        purgememadr(header);
    }
    if (!bankloaded || cruisedef == 0 || loaddef == 0)
        return spu;
    for (sint32 c = 0; c < 2; ++c) {
        tDef *ed = c == 0 ? cruisedef : loaddef;
        if (ed->resolved == 0) {
            for (sint32 i = 0; i < 8; ++i) {
                if ((sint8)ed->patchnum[i] >= 0) {
                    sint32 volumeOffset;
                    sint32 bendOffset;
                    char *definition = reinterpret_cast<char *>(ed + 1);
                    memcpy(&volumeOffset, definition + 296 + i * 4, sizeof(volumeOffset));
                    memcpy(&bendOffset, definition + 328 + i * 4, sizeof(bendOffset));
                    ed->pvoltable[i] = reinterpret_cast<tTable *>(definition + 296 + i * 4 + volumeOffset);
                    ed->pbendtable[i] = reinterpret_cast<tTable *>(definition + 328 + i * 4 + bendOffset);
                    tChanAttr *chan = &g->chan[c * 8 + i];
                    chan->patchnum = ed->patchnum[i];
                    sint32 j = 0;
                    while (j < 512 && ed->pvoltable[i]->xlate[j] == 0)
                        ++j;
                    chan->min = (sint16)j;
                    while (j < 512 && ed->pvoltable[i]->xlate[j] != 0)
                        ++j;
                    chan->max = (sint16)j;
                    tablesize = (tablesize + (chan->max - chan->min));
                }
                ed->resolved = 1;
            }
        }
    }
    char *current = static_cast<char *>(reservememadr("Engine Tables", tablesize, 0));
    g->tables = current;
    for (sint32 c = 0; c < 2; ++c) {
        tDef *ed = c == 0 ? cruisedef : loaddef;
        for (sint32 i = 0; i < 8; ++i) {
            tChanAttr *chan = &g->chan[c * 8 + i];
            if (chan->max != chan->min) {
                sint32 size = chan->max - chan->min;
                chan->xlate = current;
                char *destination = current;
                current += size;
                memcpy(destination, ed->pvoltable[i]->xlate + chan->min, size);
            }
        }
    }
    purgememadr(loaddef);
    purgememadr(cruisedef);
    return spu;
}

void AudioEng::StartServer(void)
{
    FUNCTION_MARKER(0x8007C434, "NFS4.EXE");
    iSNDserveradd100hzclient(Update);
}

void AudioEng::StopServer(void)
{
    FUNCTION_MARKER(0x8007C458, "NFS4.EXE");
    iSNDserverremove100hzclient(Update);
}

bool AudioEng::TryQueueState(sint32 player, const tState &state)
{
    AudioEng_t *g = WritableEngine(player);
    if (g == 0)
        return false;
    g->queue[(uint8)g->setpos] = state;
    g->setpos = (char)(((uint8)g->setpos + 1) & 15);
    return true;
}

void AudioEng::CleanUp(void)
{
    FUNCTION_MARKER(0x8007C534, "NFS4.EXE");
    for (sint32 player = 0; player < 2; ++player) {
        AudioEng_t *g = AudioEng::g[player];
        if (g == 0)
            return;
        for (sint32 i = 0; i < 16; ++i) {
            if (g->left[i].handle != -1) {
                SNDstop(g->left[i].handle);
                if (g->right[i].handle != -1)
                    SNDstop(g->right[i].handle);
                g->left[i].handle = -1;
                g->right[i].handle = -1;
            }
        }
        purgememadr(g->tables);
        purgememadr(g);
        AudioEng::g[player] = 0;
    }
}
