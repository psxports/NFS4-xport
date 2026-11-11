#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/pad.h"
#include "game/common/anim.h"
#include "game/common/audedit.h"
#include "game/common/audioclc.h"
#include "game/common/audiocmn.h"
#include "game/common/audiotrk.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/copspeak.h"
#include "game/common/renderview.h"
#include "game/common/sim.h"
#include "game/common/trk.h"
#include "game/psx/draww.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/random.h"

struct AudioTrk::tAmbientChannel
{
    AudioElem *se;
    sint32 handle;
    sint32 slice;
    sint16 patch;
    sint16 repeat;
};

struct AudioTrk::tGlobals
{
    AudioTrk::tAmbientChannel chan[16];
};

AudioTrk::tGlobals *AudioTrk::g;

void AudioTrk::Reset(void)
{
    FUNCTION_MARKER(0x8007C614, "NFS4.EXE");
    if (g != 0) {
        for (sint32 i = 0; i < 16; ++i) {
            if (g->chan[i].handle != -1) {
                freeVoiceChannel(i + 55);
                g->chan[i].handle = -1;
            }
            g->chan[i].se = 0;
            g->chan[i].patch = -1;
        }
    }
    if (gGameAudioList != 0 && gGameAudioList->numElements_ > 0) {
        AudioElem *se = reinterpret_cast<AudioElem *>(gGameAudioList + 1);
        for (sint32 i = 0; i < gGameAudioList->numElements_; ++i) {
            se[i].chan = (char)255;
            se[i].nextDelay = 0;
        }
    }
}

void AudioTrk::StartUp(void)
{
    FUNCTION_MARKER(0x8007C6F4, "NFS4.EXE");
    if (g == 0) {
        g = static_cast<tGlobals *>(reservememadr("trck globals", sizeof(tGlobals), 0));
        for (sint32 i = 15; i >= 0; --i)
            g->chan[i].handle = -1;
    }
    Reset();
}

void AudioTrk::AddCustomObject(AudioElem *se, sint32 tck, coorddef *vel, sint32 fade, Car::tObj *car, sint32 trkazi)
{
    FUNCTION_MARKER(0x8007C750, "NFS4.EXE");
    if (g == 0)
        return;

    sint32 dst = Math::Dist3D(&se->cp, &AudioClc::gRenderView.translation);
    sint32 n = (sint8)se->chan;
    sint32 patch = (uint8)se->patchID;
    sint32 range = (sint32)((uint32)se->range << 16);
    if (dst >= (sint32)((uint32)(se->range + 100) << 16) || patch >= CopSpeak::gNumTrackSfx)
    {
        if (n >= 0)
        {
            tAmbientChannel &c = g->chan[n];
            if (c.handle != -1)
            {
                freeVoiceChannel(n + 55);
                c.handle = -1;
            }
            c.se->chan = (char)0xFF;
            c.se = 0;
            c.patch = -1;
        }
        return;
    }

    if (n < 0)
    {
        if (se->nextDelay != 0 && se->nextDelay != tck)
            return;

        for (sint32 i = 0; i < 16; ++i)
        {
            if (g->chan[i].se == 0)
            {
                n = i;
                break;
            }
        }

        if (n < 0)
        {
            sint32 maxind = -1;
            sint32 maxdst = 0;
            for (sint32 i = 0; i < 16; ++i)
            {
                const tAmbientChannel &candidate = g->chan[i];
                sint32 chkdst = Math::Dist3D(&candidate.se->cp, &AudioClc::gRenderView.translation);
                if (candidate.handle != -1 && SNDover(candidate.handle) != 0 && chkdst > maxdst)
                {
                    maxind = i;
                    maxdst = chkdst;
                }
            }
            if (maxind < 0 || dst >= maxdst)
                return;

            n = maxind;
            tAmbientChannel &previous = g->chan[n];
            previous.se->chan = (char)0xFF;
            freeVoiceChannel(n + 55);
            previous.handle = -1;
        }

        tAmbientChannel &c = g->chan[n];
        c.se = se;
        c.slice = -1;
        uint32 repeat = (uint8)se->minRepeat;
        if (se->randomRepeat != 0)
            repeat += random() % ((uint8)se->randomRepeat + 1U);
        c.repeat = (sint16)repeat;
        se->chan = (char)n;
    }

    tAmbientChannel &c = g->chan[n];
    if (c.patch != patch)
    {
        if (c.handle != -1)
        {
            freeVoiceChannel(n + 55);
            c.handle = -1;
        }
        c.patch = (sint16)patch;
    }

    uint32 type = (uint8)se->type;
    bool repeatnow = false;
    if (c.handle != -1 && SNDover(c.handle) != 0)
    {
        if (c.repeat != 0)
        {
            repeatnow = true;
            c.handle = -1;
            c.repeat = (sint16)((uint16)c.repeat - 1U);
        }
        else if (type != 1 || range < dst)
        {
            freeVoiceChannel(n + 55);
            c.handle = -1;
            c.patch = -1;
            c.se->chan = (char)0xFF;
            c.se = 0;
            return;
        }
    }

    sint32 fadeIn = (uint8)se->fadeIn << 16;
    if (type == 1 && !repeatnow)
    {
        if (c.handle != -1 && SNDover(c.handle) != 0)
            return;
        if (dst < fadeIn && c.slice == -1)
        {
            BWorldSm::Pos slicePos;
            BWorldSm::SetSlice(car->N.simRoadInfo.slice, &slicePos);
            BWorldSm::FindClosestSlice(&se->cp, &slicePos);
            c.slice = slicePos.slice;
        }
        if (car->N.simRoadInfo.slice != c.slice || fadeIn < dst)
        {
            AudioCmn::GetAsyncSfx(0, c.patch, false);
            return;
        }
    }

    sint32 dop = 0x10000;
    sint32 azimuth = 0;
    uint32 vol = 0;
    if (dst < range)
    {
        if (type == 3)
            azimuth = trkazi;
        else if (type != 2)
        {
            azimuth = AudioClc::CalcAzimuth(&AudioClc::gRenderView, &se->cp);
            dop = AudioClc::CalcDopplerShiftRatio(&se->cp, vel);
        }

        if (type - 4U < 32U || type == 1)
        {
            sint32 rangesq = se->range * se->range;
            sint32 dist = dst >> 2;
            sint32 numerator = (sint32)(((uint32)(rangesq >> 4) << 16) - (uint32)fixedmult(dist, dist));
            if (rangesq == 0)
                abort();
            uint32 level = (uint32)(numerator / rangesq) * (uint32)fade * 127U;
            if ((sint32)level < 0)
                level += 0xFFFFU;
            vol = level >> 19;
        }
        else
        {
            vol = 127;
            if (dst >= fadeIn)
            {
                sint32 ambdist = se->range - (uint8)se->fadeIn;
                sint32 rangesq = ambdist * ambdist;
                sint32 dist = (sint32)((uint32)dst - (uint32)fadeIn) >> 2;
                sint32 numerator = (sint32)(((uint32)(rangesq >> 4) << 16) - (uint32)fixedmult(dist, dist));
                if (rangesq == 0)
                    abort();
                uint32 level = (uint32)(numerator / rangesq) * 0x7F0U;
                if ((sint32)level < 0)
                    level += 0xFFFFU;
                vol = level >> 16;
            }
        }
    }

    if (dop > 0xA0000)
        dop = 0xA0000;
    if (dop < 1)
        dop = 1;
    if ((PAD_state(4) & 0x400) == 0)
        c.handle = AudioCmn::PlaySFX(n + 55, c.patch, 0x40, dop, vol & 0xFFU, (uint16)azimuth);
}

void AudioTrk::SoundTrack(Car::tObj *car, sint32 trkazi)
{
    FUNCTION_MARKER(0x8007CDC4, "NFS4.EXE");
    if (GameSetup::gData.commMode == 1 || gMasterAmbientLevel == 0 || g == 0 || gGameAudioList == 0)
        return;
    sint32 ticks = simGlobal.gameTicks;
    sint32 numelems = gGameAudioList->numElements_;
    sint32 quater = (numelems >> 2) + 1;
    sint32 gtck = ticks >> 3;
    sint32 vx = AudioClc::gRenderView.translation.x;
    sint32 vz = AudioClc::gRenderView.translation.z;
    sint32 start = (sint32)((uint32)((ticks >> 1) % 4) * (uint32)quater);
    sint32 end = (sint32)((uint32)start + (uint32)quater);
    if (numelems < end)
        end = numelems;
    AudioElem *se = reinterpret_cast<AudioElem *>(gGameAudioList + 1) + start;
    for (sint32 i = start; i < end; ++i, ++se) {
        sint32 channel = (sint8)se->chan;
        if (channel != -1 && g->chan[channel].se != se)
            se->chan = (char)255;
        Trk::AnimateInst *anim = 0;
        sint32 tck = gtck;
        sint32 cur;
        sint32 max;
        coorddef v;
        if ((uint32)(uint8)se->type - 4 < 32) {
            v.z = 0;
            v.y = 0;
            v.x = 0;
            anim = Anim::gInstanceFromIndex[(uint8)se->type - 4];
            if (anim != 0) {
                tck = DrawW::GetAnimationTime(anim);
                Anim::GetPos(anim, 1, tck, &se->cp, &cur, &max);
            }
        } else if (((uint8)se->minDelay != 0 || (uint8)se->randomDelay != 0) &&
                   se->nextDelay < tck && (sint8)se->chan == -1) {
            uint32 randtick = 0;
            if ((uint8)se->randomDelay >= 2) {
                uint32 draw = random();
                uint32 divisor = (uint8)se->randomDelay;
                if (divisor == 0)
                    abort();
                randtick = draw % divisor;
            }
            se->nextDelay = (uint16)((uint32)(uint8)se->minDelay + (uint32)tck + randtick);
        }
        sint32 x = (sint32)((uint32)se->cp.x - (uint32)vx);
        if (x <= 0)
            x = (sint32)((uint32)vx - (uint32)se->cp.x);
        sint32 z = (sint32)((uint32)se->cp.z - (uint32)vz);
        if (z <= 0)
            z = (sint32)((uint32)vz - (uint32)se->cp.z);
        sint32 d = z < x ? (sint32)((uint32)x + (uint32)(z >> 2)) : (sint32)((uint32)z + (uint32)(x >> 2));
        coorddef *vel = 0;
        sint32 range = (sint32)((uint32)(se->range + 100) << 16);
        if (d < range || (sint8)se->chan >= 0) {
            sint32 fade = 128;
            if ((uint32)(uint8)se->type - 4 < 32) {
                if (anim != 0) {
                    se->nextDelay = (uint8)se->minDelay;
                    sint32 fadeIn = (uint8)se->fadeIn * 8;
                    if (cur < fadeIn)
                        fade = 0;
                    else if (cur < fadeIn + 128)
                        fade = (sint32)((uint32)cur - (uint32)fadeIn);
                    sint32 fadeOut = (uint8)se->randomDelay * 8;
                    if (fadeOut != 0) {
                        if (fadeOut + 128 < cur)
                            fade = 0;
                        else if (fadeOut < cur)
                            fade = (sint32)((uint32)fadeOut - ((uint32)cur - 128));
                    }
                    if (fade > 0) {
                        sint32 next = (sint32)((uint32)max - 1);
                        sint32 ahead = (sint32)((uint32)tck + 32);
                        if (ahead < next)
                            next = ahead;
                        coorddef nextcp;
                        Anim::GetPos(anim, 1, next, &nextcp, 0, 0);
                        sint32 time = (sint32)((uint32)next - (uint32)tck);
                        if (time == 32) {
                            v.x = (sint32)(((uint32)nextcp.x - (uint32)se->cp.x) << 1);
                            v.y = (sint32)(((uint32)nextcp.y - (uint32)se->cp.y) << 1);
                            v.z = (sint32)(((uint32)nextcp.z - (uint32)se->cp.z) << 1);
                            vel = &v;
                        } else if (time != 0) {
                            sint32 dividend = (sint32)(((uint32)nextcp.x - (uint32)se->cp.x) << 6);
                            if (time == -1 && dividend == INT_MIN)
                                abort();
                            v.x = dividend / time;
                            dividend = (sint32)(((uint32)nextcp.y - (uint32)se->cp.y) << 6);
                            if (time == -1 && dividend == INT_MIN)
                                abort();
                            v.y = dividend / time;
                            dividend = (sint32)(((uint32)nextcp.z - (uint32)se->cp.z) << 6);
                            if (time == -1 && dividend == INT_MIN)
                                abort();
                            v.z = dividend / time;
                            vel = &v;
                        }
                    }
                    if (max > 0)
                        tck %= max;
                    tck >>= 3;
                }
                if ((uint32)(uint8)se->type - 4 < 32 && anim == 0)
                    continue;
            }
            AddCustomObject(se, tck, vel, fade, car, trkazi);
        }
    }
}

sint32 AudioTrk::PreLoad(void)
{
    FUNCTION_MARKER(0x8007D35C, "NFS4.EXE");
    if (gGameAudioList == 0 || gMasterAmbientLevel == 0)
        return 1;
    sint32 vx = BWorldSm::slices->center[0];
    sint32 vz = BWorldSm::slices->center[2];
    sint32 loaded = 0;
    sint32 tick = (sint32)((uint32)gettick() + 640);
    sint32 numelems = gGameAudioList->numElements_;
    for (;;) {
        sint32 again = 0;
        if (!loaded && gettick() < tick) {
            sint32 check;
            again = SNDmemlargestunused(&check) > 32768;
        }
        loaded = 1;
        if (!again)
            break;
        AudioElem *se = reinterpret_cast<AudioElem *>(gGameAudioList + 1);
        for (sint32 i = 0; i < numelems; ++i) {
            sint32 x = (sint32)((uint32)se[i].cp.x - (uint32)vx);
            if (x <= 0)
                x = (sint32)((uint32)vx - (uint32)se[i].cp.x);
            sint32 z = (sint32)((uint32)se[i].cp.z - (uint32)vz);
            if (z <= 0)
                z = (sint32)((uint32)vz - (uint32)se[i].cp.z);
            sint32 d;
            if (z < x)
                d = (sint32)((uint32)x + (uint32)(z >> 2));
            else
                d = (sint32)((uint32)z + (uint32)(x >> 2));
            sint32 range = (sint32)((uint32)(se[i].range + 100) << 16);
            if (d < range && (uint8)se[i].patchID < CopSpeak::gNumTrackSfx &&
                AudioCmn::GetAsyncSfx(0, (uint8)se[i].patchID, 0) == -1)
                loaded = 0;
        }
        CopSpeak::Server();
        systemtask(0);
    }
    return gettick() < tick;
}

void AudioTrk::CleanUp(void)
{
    FUNCTION_MARKER(0x8007D52C, "NFS4.EXE");
    Reset();
    if (g != 0) {
        for (sint32 i = 0; i < 16; ++i) {
            if (g->chan[i].handle != -1) {
                freeVoiceChannel(i + 55);
                g->chan[i].handle = -1;
            }
        }
        purgememadr(g);
        g = 0;
    }
}
