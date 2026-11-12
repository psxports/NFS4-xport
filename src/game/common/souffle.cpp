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
        if (value + amount > 0) *component = 0;
    }
    else if (value > 0)
    {
        *component = value - amount;
        if (value - amount < 0) *component = 0;
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
    is->angle = (is->angle + aspeed);
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
        gCISouffle = (gCISouffle + 1);
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
    if ((gameTicks - gTMoveSouffle) <= 0)
        return;
    gTMoveSouffle = gameTicks;
    if (gCISouffle == 0)
        return;
    gWindDir = (gWindDir + 2);
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
            motion.x = (motion.x + w.x);
            motion.y = (motion.y + w.y);
            motion.z = (motion.z + w.z);
        }
        if (Replay::ReplayMode == 2)
        {
            sint32 speed = (2 - Replay::ReplayInterface.speed);
            if (speed > 0)
            {
                motion.x >>= speed & 31;
                motion.z >>= speed & 31;
                motion.y >>= speed & 31;
            }
            if (speed < 0)
            {
                motion.x = ((uint32)motion.x << 1);
                motion.z = ((uint32)motion.z << 1);
                motion.y = ((uint32)motion.y << 1);
            }
        }
        is->source.x = (is->source.x + motion.x);
        is->source.y = (is->source.y + motion.y);
        is->source.z = (is->source.z + motion.z);
        DampSouffleComponent(&is->motion.x, 0xA3D);
        DampSouffleComponent(&is->motion.z, 0xA3D);
        if ((uint8)is->type == 13)
        {
            coorddef extramotion = is->extramotion;
            sint32 wave = fastintsin(is->angle);
            if (Replay::ReplayMode == 2)
            {
                sint32 speed = (2 - Replay::ReplayInterface.speed);
                if (speed > 0)
                {
                    extramotion.x >>= speed & 31;
                    extramotion.z >>= speed & 31;
                    extramotion.y >>= speed & 31;
                }
                if (speed < 0)
                {
                    extramotion.x = ((uint32)extramotion.x << 1);
                    extramotion.z = ((uint32)extramotion.z << 1);
                    extramotion.y = ((uint32)extramotion.y << 1);
                }
            }
            sint32 x = (extramotion.x * wave) >> 17;
            is->source.x = (is->source.x + x);
            sint32 z = (extramotion.z * wave) >> 17;
            is->source.z = (is->source.z + z);
            DampSouffleComponent(&is->extramotion.x, 0x147);
            DampSouffleComponent(&is->extramotion.z, 0x147);
            if (is->ground < is->source.y)
                is->motion.y = (is->motion.y - 0x147);
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
    sint32 dx = (pt1->x - pt2->x);
    if (dx <= 0)
        dx = (pt2->x - pt1->x);
    sint32 dz = (pt1->z - pt2->z);
    if (dz <= 0)
        dz = (pt2->z - pt1->z);
    sint32 dist;
    if (dz < dx)
        dist = (dx + (dz >> 2));
    else
        dist = (dz + (dx >> 2));
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
