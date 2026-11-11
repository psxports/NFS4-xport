#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/renderview.h"
#include "game/common/replay.h"
#include "game/common/sim.h"
#include "game/common/souffle.h"
#include "game/psx/sfx.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/random.h"
#include "lib/psx/sinfunc.h"

sint32 gCISouffle;
sint32 gTMoveSouffle;
sint32 gWindDir;
Souffle::tISouffle *gISouffle;

static void DampSouffleComponent(sint32 *component, sint32 amount)
{
    sint32 value = *component;
    if (value < 0)
    {
        *component = value + amount;
        if (value + amount > 0)
            *component = 0;
    }
    else if (value > 0)
    {
        *component = value - amount;
        if (value - amount < 0)
            *component = 0;
    }
}

void GetGustWind(Souffle::tISouffle *is)
{
    FUNCTION_MARKER(0x800B764C, "NFS4.EXE");
    is->motion.x = random() & 1023;
    is->motion.z = random() & 1023;
    is->motion.y = (random() & 1023) + 2047;
}

void GetAngleWind(Souffle::tISouffle *is)
{
    FUNCTION_MARKER(0x800B7694, "NFS4.EXE");
    is->angle = (sint32)random();
    is->aspeed = (sint32)(random() & 127) - 63;
}

void MoveAngleWind(Souffle::tISouffle *is)
{
    FUNCTION_MARKER(0x800B76CC, "NFS4.EXE");
    sint32 aspeed = is->aspeed;
    is->angle = (sint32)((uint32)is->angle + (uint32)aspeed);
    if (aspeed < -8)
        is->aspeed = aspeed + 1;
    else if (aspeed > 8)
        is->aspeed = aspeed - 1;
}

Souffle::tISouffle *Souffle::Add(coorddef *soufflept, sint32 type, coorddef *vec, sint32 velVX, sint32 ground, sint32 colour)
{
    FUNCTION_MARKER(0x800B770C, "NFS4.EXE");
    sint32 inserti = 0;
    if (gCISouffle == 60)
    {
        sint32 maxc = (uint8)gISouffle[0].cycle;
        for (sint32 i = 1; i < 60; ++i)
        {
            sint32 cycle = (uint8)gISouffle[i].cycle;
            if (cycle < maxc)
            {
                inserti = i;
                maxc = cycle;
            }
        }
    }
    else
    {
        inserti = gCISouffle;
        gCISouffle = (sint32)((uint32)gCISouffle + 1);
    }
    tISouffle *is = gISouffle + inserti;
    coorddef source = *soufflept;
    is->source = source;
    is->cycle = 0;
    is->type = (char)type;
    is->id = (char)inserti;
    is->ground = ground;
    is->colour = colour;
    coorddef vempty = { 0, 0, 0 };
    if (vec == 0)
        vec = &vempty;
    switch (type)
    {
    case 1:
    case 2:
    case 3:
    case 6:
        GetGustWind(is);
        is->wind = 1;
        GetAngleWind(is);
        break;
    case 4:
    case 8:
    case 10:
    case 12:
        is->rndpixmap = velVX;

    case 7:
    case 9:
    case 11:
    case 13:
    case 14:
    case 257:
        is->motion.x = vec->x >> 5;
        is->motion.y = vec->y >> 6;
        {
            sint32 z = vec->z;
            is->wind = 0;
            is->motion.z = z >> 5;
        }
        GetAngleWind(is);
        break;
    }
    Sfx::Add(is);
    return is;
}

void Souffle::DoSouffle(void)
{
    FUNCTION_MARKER(0x800B78EC, "NFS4.EXE");
    sint32 gameTicks = simGlobal.gameTicks;
    if ((sint32)((uint32)gameTicks - (uint32)gTMoveSouffle) <= 0)
        return;
    gTMoveSouffle = gameTicks;
    if (gCISouffle == 0)
        return;
    gWindDir = (sint32)((uint32)gWindDir + 2);
    coorddef w;
    w.x = fixedmult(fastintcos(gWindDir), 0xCCC);
    w.z = fixedmult(fastintsin(gWindDir), 0xCCC);
    w.y = 0;
    for (sint32 i = 0; i < gCISouffle; ++i)
    {
        tISouffle *is = gISouffle + i;
        coorddef motion = is->motion;
        if (is->wind != 0)
        {
            motion.x = (sint32)((uint32)motion.x + (uint32)w.x);
            motion.y = (sint32)((uint32)motion.y + (uint32)w.y);
            motion.z = (sint32)((uint32)motion.z + (uint32)w.z);
        }
        if (Replay::ReplayMode == 2)
        {
            sint32 speed = (sint32)(2 - (uint32)Replay::ReplayInterface.speed);
            if (speed > 0)
            {
                motion.x >>= speed & 31;
                motion.z >>= speed & 31;
                motion.y >>= speed & 31;
            }
            if (speed < 0)
            {
                motion.x = (sint32)((uint32)motion.x << 1);
                motion.z = (sint32)((uint32)motion.z << 1);
                motion.y = (sint32)((uint32)motion.y << 1);
            }
        }
        is->source.x = (sint32)((uint32)is->source.x + (uint32)motion.x);
        is->source.y = (sint32)((uint32)is->source.y + (uint32)motion.y);
        is->source.z = (sint32)((uint32)is->source.z + (uint32)motion.z);
        DampSouffleComponent(&is->motion.x, 0xA3D);
        DampSouffleComponent(&is->motion.z, 0xA3D);
        if ((uint8)is->type == 13)
        {
            coorddef extramotion = is->extramotion;
            sint32 wave = fastintsin(is->angle);
            if (Replay::ReplayMode == 2)
            {
                sint32 speed = (sint32)(2 - (uint32)Replay::ReplayInterface.speed);
                if (speed > 0)
                {
                    extramotion.x >>= speed & 31;
                    extramotion.z >>= speed & 31;
                    extramotion.y >>= speed & 31;
                }
                if (speed < 0)
                {
                    extramotion.x = (sint32)((uint32)extramotion.x << 1);
                    extramotion.z = (sint32)((uint32)extramotion.z << 1);
                    extramotion.y = (sint32)((uint32)extramotion.y << 1);
                }
            }
            sint32 x = (sint32)((uint32)extramotion.x * (uint32)wave) >> 17;
            is->source.x = (sint32)((uint32)is->source.x + (uint32)x);
            sint32 z = (sint32)((uint32)extramotion.z * (uint32)wave) >> 17;
            is->source.z = (sint32)((uint32)is->source.z + (uint32)z);
            DampSouffleComponent(&is->extramotion.x, 0x147);
            DampSouffleComponent(&is->extramotion.z, 0x147);
            if (is->ground < is->source.y)
                is->motion.y = (sint32)((uint32)is->motion.y - 0x147);
            else
                is->type = 0;
        }
        MoveAngleWind(is);
        is->cycle = (char)((uint8)is->cycle - 1);
        if (is->cycle == 0)
            is->type = 0;
    }
    for (sint32 i = 0; i < gCISouffle; ++i)
    {
        if (gISouffle[i].type == 0)
        {
            memmove(&gISouffle[i], &gISouffle[gCISouffle - 1], sizeof(tISouffle));
            --gCISouffle;
            --i;
        }
    }
}

sint32 Souffle::CircleClip(coorddef *pt1, coorddef *pt2, sint32 r)
{
    FUNCTION_MARKER(0x800B7D88, "NFS4.EXE");
    sint32 dx = (sint32)((uint32)pt1->x - (uint32)pt2->x);
    if (dx <= 0)
        dx = (sint32)((uint32)pt2->x - (uint32)pt1->x);
    sint32 dz = (sint32)((uint32)pt1->z - (uint32)pt2->z);
    if (dz <= 0)
        dz = (sint32)((uint32)pt2->z - (uint32)pt1->z);
    sint32 dist;
    if (dz < dx)
        dist = (sint32)((uint32)dx + (uint32)(dz >> 2));
    else
        dist = (sint32)((uint32)dz + (uint32)(dx >> 2));
    return dist < r;
}

void Souffle::InsertFacet(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800B7DE8, "NFS4.EXE");
    for (sint32 i = 0; i < gCISouffle; ++i)
    {
        tISouffle *is = gISouffle + i;
        if (!CircleClip(&is->source, &Vi->cview.translation, 0x320000))
            continue;
        if ((uint8)is->type != 10)
        {
            Sfx::Transform(&is->source, &is->trans, &Vi->cview.translation);
            sint32 x = is->trans.vx;
            sint32 z = is->trans.vz;
            if (x < 0)
                x = -x;
            if (z < x || is->type == 0)
                continue;
        }
        Sfx::BuildSouffleFacet(Vi, is);
    }
}

void Souffle::InitTrackSouffle(void)
{
    FUNCTION_MARKER(0x800B7ED4, "NFS4.EXE");
    sint32 gameTicks = simGlobal.gameTicks;
    gWindDir = 0;
    gCISouffle = 0;
    gTMoveSouffle = gameTicks;
    gISouffle = static_cast<tISouffle *>(reservememadr("souffle", 4080, 0));
}

void Souffle::RestartTrackSouffle(void)
{
    FUNCTION_MARKER(0x800B7F14, "NFS4.EXE");
    sint32 gameTicks = simGlobal.gameTicks;
    gWindDir = 0;
    gCISouffle = 0;
    gTMoveSouffle = gameTicks;
}

void Souffle::KillTrackSouffle(void)
{
    FUNCTION_MARKER(0x800B7F30, "NFS4.EXE");
    tISouffle *memory = gISouffle;
    if (memory != 0)
        purgememadr(memory);
    gISouffle = 0;
}
