#include <stddef.h>
#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/gmesetup.h"
#include "game/common/mathnfs.h"
#include "game/common/sim.h"
#include "game/common/souffle.h"
#include "game/common/trgsfx.h"
#include "game/psx/skidmark.h"
#include "lib/psx/random.h"

struct tSkid
{
    coorddef pt;
    CVECTOR clr;
    sint32 type;
    Skidmark::Segment *nseg;
    sint32 chunk;
};

sint32 gTAddCarWheelDelay;
sint32 gTAddCSmoke;
sint32 gTEnviroEffect[8];
sint32 gTAddCarSfx[8];
sint32 gTAddCarWheelSfx[8][4];
sint32 gTAddCarExtraSfx[8][4];
sint32 gStatusSm[8][4];
sint32 gSaveChunk[8][4];
tSkid gPrevSkidSm[8][4];
Skidmark::Segment *gSaveSeg[8][4];

void TrgSfx::AddEnviroEffect(sint32 obj, sint32 type, coorddef *emitterpt, coorddef *vec)
{
    FUNCTION_MARKER(0x800BB068, "NFS4.EXE");
    sint32 slot = (uint32)obj & 7;
    sint32 ticks = simGlobal.gameTicks;
    if ((sint32)((uint32)ticks - (uint32)gTEnviroEffect[slot]) >= 11) {
        gTEnviroEffect[slot] = ticks;
        Souffle::Add(emitterpt, type, vec, 0, 0, 0);
    }
}

void TrgSfx::AddCarSfx(sint32 car, coorddef *skidpt, sint32 type, coorddef *vec)
{
    FUNCTION_MARKER(0x800BB0D0, "NFS4.EXE");
    sint32 slot = (uint32)car & 7;
    sint32 ticks = simGlobal.gameTicks;
    if ((sint32)((uint32)ticks - (uint32)gTAddCarSfx[slot]) >= 8) {
        gTAddCarSfx[slot] = ticks;
        Souffle::Add(skidpt, type, vec, 0, 0, 0);
    }
}

void TrgSfx::AddCarWheelSfx(sint32 car, sint32 wheel, coorddef *skidpt, sint32 type, coorddef *vec, sint32 delay)
{
    FUNCTION_MARKER(0x800BB13C, "NFS4.EXE");
    sint32 slot = (uint32)car & 7;
    sint32 ticks = simGlobal.gameTicks;
    if (delay < (sint32)((uint32)ticks - (uint32)gTAddCarWheelSfx[slot][wheel])) {
        coorddef dir = *vec;
        dir.x >>= 1;
        dir.z >>= 1;
        gTAddCarWheelSfx[slot][wheel] = simGlobal.gameTicks;
        Souffle::Add(skidpt, type, &dir, 0, 0, 0);
    }
}

bool TrgSfx::AddCarExtraCheck(sint32 car, sint32 wheel)
{
    FUNCTION_MARKER(0x800BB1EC, "NFS4.EXE");
    sint32 slot = (uint32)car & 7;
    sint32 ticks = simGlobal.gameTicks;
    return (sint32)((uint32)ticks - (uint32)gTAddCarExtraSfx[slot][wheel]) >= 8;
}

void TrgSfx::AddCarExtraSfx(sint32 car, sint32 wheel, coorddef *skidpt, sint32 type, coorddef *vec, sint32 velY, sint32 ground, sint32 colour)
{
    FUNCTION_MARKER(0x800BB228, "NFS4.EXE");
    sint32 slot = (uint32)car & 7;
    coorddef dir = *vec;
    dir.y = (sint32)((uint32)dir.y + (uint32)(velY >> 3));
    gTAddCarExtraSfx[slot][wheel] = simGlobal.gameTicks;
    Souffle::Add(skidpt, type, &dir, 0, ground, colour);
}

void TrgSfx::AddCarSplash(sint32 car, sint32 wheel, coorddef *skidpt, sint32 type, coorddef *vec, sint32 delay, sint32 velXZ)
{
    FUNCTION_MARKER(0x800BB2BC, "NFS4.EXE");
    sint32 slot = (uint32)car & 7;
    sint32 ticks = simGlobal.gameTicks;
    if (delay < (sint32)((uint32)ticks - (uint32)gTAddCarWheelSfx[slot][wheel])) {
        coorddef dir = *vec;
        dir.x >>= 1;
        dir.z >>= 1;
        gTAddCarWheelSfx[slot][wheel] = simGlobal.gameTicks;
        Souffle::Add(skidpt, type, &dir, velXZ, 0, 0);
    }
}

void TrgSfx::CrashCar(coorddef *location)
{
    FUNCTION_MARKER(0x800BB370, "NFS4.EXE");
    sint32 ticks = simGlobal.gameTicks;
    if ((sint32)((uint32)ticks - (uint32)gTAddCSmoke) >= 5) {
        gTAddCSmoke = ticks;
        if ((random() & 15) != 0) {
            Souffle::tISouffle *is = Souffle::Add(location, 1, 0, 0, 0, 0);
            is->motion.y = (sint32)((uint32)is->motion.y + 0xf5c);
        }
        Souffle::tISouffle *is = Souffle::Add(location, 3, 0, 0, 0, 0);
        is->motion.y = (sint32)((uint32)is->motion.y + 0xf5c);
    }
}

void TrgSfx::AddSkidmark(sint32 car, sint32 wheel, coorddef *skidpt, sint32 end, sint32 intensity, Car::tObj *carObj, sint32 type)
{
    FUNCTION_MARKER(0x800BB41C, "NFS4.EXE");
    sint32 velXZ = carObj->N.speedXZ;
    sint32 slice = carObj->N.simRoadInfo.slice;
    sint32 tireWidth = wheel < 2 ? carObj->N.wheelWidthF : carObj->N.wheelWidthB;
    sint32 slot = (uint32)car & 7;
    sint32 value = (sint32)((uint32)intensity * 255) / 0x70000;
    if (value >= 256)
        value = 255;
    CVECTOR color;

    color.cd = gPrevSkidSm[slot][wheel].clr.cd;
    color.r = (uint8)value;
    color.g = (uint8)value;
    color.b = (uint8)value;
    skidpt->x = (sint32)((uint32)skidpt->x + (uint32)(carObj->N.linearVel.x >> 6));
    skidpt->y = (sint32)((uint32)skidpt->y + (uint32)(carObj->N.linearVel.y >> 6));
    skidpt->z = (sint32)((uint32)skidpt->z + (uint32)(carObj->N.linearVel.z >> 6));
    sint32 status = gStatusSm[slot][wheel];
    if (status == 0)
    {
        if (end == 0)
        {
            gStatusSm[slot][wheel] = 1;
            gPrevSkidSm[slot][wheel].clr = color;
            gPrevSkidSm[slot][wheel].type = type;
            coorddef point = *skidpt;
            gPrevSkidSm[slot][wheel].pt = point;
            gPrevSkidSm[slot][wheel].nseg = 0;
        }
        return;
    }
    if (end != 0)
    {
        if (status == 2)
            Skidmark::Stretch(gSaveSeg[slot][wheel], gSaveChunk[slot][wheel],
                &gPrevSkidSm[slot][wheel], skidpt, &color, tireWidth, type);
        else
            Skidmark::Add(&gPrevSkidSm[slot][wheel], skidpt, &color, tireWidth, type, slice);
        gStatusSm[slot][wheel] = 0;
        return;
    }
    bool stretch = false;
    if (gPrevSkidSm[slot][wheel].nseg != 0)
    {
        sint32 oldX = gPrevSkidSm[slot][wheel].pt.x;
        sint32 dx = (sint32)((uint32)oldX - (uint32)skidpt->x);
        if (dx <= 0)
            dx = (sint32)((uint32)skidpt->x - (uint32)oldX);
        sint32 oldZ = gPrevSkidSm[slot][wheel].pt.z;
        sint32 dz = (sint32)((uint32)oldZ - (uint32)skidpt->z);
        if (dz <= 0)
            dz = (sint32)((uint32)skidpt->z - (uint32)oldZ);
        sint32 maximum = velXZ >= 0xa0000 ? 0xc000 : 0x3000;
        sint32 distance = dz < dx ? (sint32)((uint32)dx + (uint32)(dz >> 2)) :
            (sint32)((uint32)dz + (uint32)(dx >> 2));
        stretch = distance < maximum;
    }
    if (stretch)
    {
        if (gStatusSm[slot][wheel] == 2)
            Skidmark::Stretch(gSaveSeg[slot][wheel], gSaveChunk[slot][wheel],
                &gPrevSkidSm[slot][wheel], skidpt, &color, tireWidth, type);
        else
        {
            gStatusSm[slot][wheel] = 2;
            Skidmark::AddStretch(&gSaveSeg[slot][wheel], &gSaveChunk[slot][wheel],
                &gPrevSkidSm[slot][wheel], skidpt, &color, tireWidth, type, slice);
        }
    }
    else
    {
        if (gStatusSm[slot][wheel] == 2)
            Skidmark::EndStretch(gSaveSeg[slot][wheel], gSaveChunk[slot][wheel],
                &gPrevSkidSm[slot][wheel], skidpt, &color, tireWidth, type);
        else
            Skidmark::Add(&gPrevSkidSm[slot][wheel], skidpt, &color, tireWidth, type, slice);
        gStatusSm[slot][wheel] = 1;
    }
}

void TrgSfx::InitTrgSfx(void)
{
    FUNCTION_MARKER(0x800BB900, "NFS4.EXE");
    sint32 t = simGlobal.gameTicks;
    gTAddCSmoke = t;
    for (sint32 i = 0; i < 8; ++i) {
        gTEnviroEffect[i] = t;
        gTAddCarSfx[i] = t;
        for (sint32 j = 0; j < 4; ++j) {
            gTAddCarWheelSfx[i][j] = t;
            gTAddCarExtraSfx[i][j] = t;
            gStatusSm[i][j] = 0;
            gPrevSkidSm[i][j].pt.x = 0;
            gPrevSkidSm[i][j].pt.y = 0;
            gPrevSkidSm[i][j].pt.z = 0;
            gPrevSkidSm[i][j].nseg = 0;
        }
    }
    gTAddCarWheelDelay = 8;
    if (GameSetup::gData.commMode == 1)
        gTAddCarWheelDelay = 12;
}

void TrgSfx::RestartTrgSfx(void)
{
    FUNCTION_MARKER(0x800BB9D8, "NFS4.EXE");
    KillTrgSfx();
    InitTrgSfx();
}

void TrgSfx::KillTrgSfx(void)
{
    FUNCTION_MARKER(0x800BBA00, "NFS4.EXE");
}
