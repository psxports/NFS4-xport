#include <stddef.h>
#include "xport.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/gmesetup.h"
#include "game/common/replay.h"
#include "game/common/sim.h"
#include "game/psx/force.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

sint32 Force::gVblHandle;
uint16 Force::gTick;
uint8 Force::rand_256[256] = { 0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240, 8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248, 4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244, 12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252, 2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242, 10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250, 6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246, 14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254, 1, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241, 9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249, 5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245, 13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253, 3, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243, 11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251, 7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247, 15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 254 };
uint8 Force::gActAlign[6] = { 0, 1, 255, 255, 255, 255 };
uint8 Force::gOffAlign[6] = { 255, 255, 255, 255, 255, 255 };
Force::tGlobal Force::g[2];

static void UpdateForceForSchedule(void *car)
{
    Force::Update(static_cast<Car::tObj *>(car));
}

void Force::Vbl(void)
{
    FUNCTION_MARKER(0x800CA998, "NFS4.EXE");
    for (sint32 i = 0; i < 2; ++i)
    {
        sint32 padnum = i << 4;
        tGlobal *f = &g[i];
        sint32 padstate = PadGetState(padnum);
        if (padstate == 6)
        {
            if (f->active == 0)
            {
                PadSetAct(padnum, f->actuator, 2);
                PadSetActAlign(padnum, gActAlign);
                f->active = 1;
            }
        }
        else if (padstate < 4)
            f->active = 0;
        sint32 high;
        sint32 low;
        if (f->fade < f->time)
        {
            high = f->high + f->jolt;
            low = f->low + f->jolt;
        }
        else if (f->time != 0)
        {
            sint32 jolt = f->jolt * f->time / f->fade;
            high = jolt + f->high;
            low = jolt + f->low;
        }
        else
        {
            f->jolt = 0;
            uint8 sample = rand_256[(gTick >> 1) & 255];
            f->actuator[1] = f->low;
            f->actuator[0] = sample < f->high;
            continue;
        }
        f->actuator[0] = rand_256[(gTick >> 1) & 255] < high;
        f->actuator[1] = (uint8)(low < 256 ? low : 255);
        --f->time;
    }
    ++gTick;
}

void Force::Update(Car::tObj *car)
{
    FUNCTION_MARKER(0x800CABC0, "NFS4.EXE");
    if ((uint32)car->carIndex >= 2)
        return;
    tGlobal *f = &g[car->carIndex];
    if (Replay::ReplayMode >= 2)
    {
        f->high = 0;
        f->low = 0;
        f->time = 0;
        return;
    }
    sint32 skids = GameSetup::gData.controllerData.shockMode[car->carIndex];
    sint32 impacts = GameSetup::gData.controllerData.shockImpact[car->carIndex];
    sint32 frontmultiplier = skids == 0 ? 0 : (sint32)(((uint32)skids + 16) * 0x2DA6);
    sint32 rearmultiplier = skids == 0 ? 0 : (sint32)(((uint32)skids + 16) * 0x1E6E);
    sint32 impactmultiplier = impacts == 0 ? 0 : (sint32)(((uint32)impacts + 16) * 0xB699);
    sint32 v0 = 0;
    sint32 v1 = 0;
    if (car->N.flightTime == 0)
    {
        switch (car->N.driveSurfaceType)
        {
        case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
        {
            sint32 speed = car->linearVel_ch.z;
            if (speed < 0)
                speed = (sint32)(0 - (uint32)speed);
            speed >>= 2;
            v1 = speed > 0x78000 ? 0x78000 : speed;
            break;
        }
        case 10: case 11: case 12: case 13: case 15:
        {
            sint32 speed = car->linearVel_ch.z;
            if (speed < 0)
                speed = (sint32)(0 - (uint32)speed);
            speed >>= 1;
            v0 = speed > 0x58000 ? 0x58000 : speed;
            break;
        }
        default:
            break;
        }
    }
    for (sint32 c = (sint32)((uint32)car->audioCount - 1); c >= 0; --c)
    {
        Cars::tAudio *audio = &car->audio[c];
        if (audio->channel == 18)
        {
            sint32 force = (sint32)((uint32)audio->force << 1);
            if (force > v0)
                v0 = force;
        }
        else if (audio->channel == 20)
        {
            sint32 force = (sint32)((uint32)audio->force << 1);
            if (force > v1)
                v1 = force;
        }
        else if (audio->channel < 0 && impactmultiplier != 0 && audio->surface1 != 10 && audio->surface1 != 8)
        {
            sint32 force = audio->force;
            sint32 time;
            if (force > 0x28000)
            {
                force = 0x28000;
                sint32 scaledTime = (sint32)((uint32)fixeddiv(audio->force, 0x28000) << 5);
                if (scaledTime / 65536 < 97)
                {
                    scaledTime = (sint32)((uint32)fixeddiv(audio->force, 0x28000) << 5);
                    time = scaledTime / 65536;
                }
                else
                    time = 96;
            }
            else
                time = 32;
            sint32 shock = fixedmult(force, impactmultiplier) / 65536;
            if (f->jolt < shock || f->time < time)
            {
                f->fade = (uint8)(time >> 1);
                f->time = (uint8)time;
                f->jolt = (uint8)shock;
            }
        }
    }
    if (frontmultiplier != 0)
    {
        if (v0 > 0xA0000)
            v0 = 0xA0000;
        v0 = fixedmult(v0, frontmultiplier) / 65536;
    }
    else
        v0 = 0;
    if (rearmultiplier != 0)
    {
        if (v1 > 0xF0000)
            v1 = 0xF0000;
        v1 = fixedmult(v1, rearmultiplier) / 65536;
    }
    else
        v1 = 0;
    f->high = (uint8)v0;
    f->low = (uint8)v1;
}

void Force::StartUp(void)
{
    FUNCTION_MARKER(0x800CB018, "NFS4.EXE");
    for (tGlobal *f = g; f != g + 2; ++f)
    {
        f->active = 0;
        f->high = 0;
        f->low = 0;
        f->time = 0;
        f->actuator[0] = 0;
        f->actuator[1] = 0;
    }
    VSyncCallback(Vbl);
    Sched::AddFunction(simGlobal.schedule32Hz, UpdateForceForSchedule, Cars::gHumanRaceCarList[0], 50);
    if (GameSetup::gData.commMode == 1)
        Sched::AddFunction(simGlobal.schedule32Hz, UpdateForceForSchedule, Cars::gHumanRaceCarList[1], 50);
}

void Force::Disable(void)
{
    FUNCTION_MARKER(0x800CB0E4, "NFS4.EXE");
    for (tGlobal *f = g; f != g + 2; ++f)
    {
        f->actuator[0] = 0;
        f->actuator[1] = 0;
    }
    PadSetActAlign(0, gOffAlign);
    PadSetActAlign(4, gOffAlign);
    VSyncCallback(0);
}

sint32 Force::IsForceOn(Car::tObj *car)
{
    FUNCTION_MARKER(0x800CB158, "NFS4.EXE");

    if (Replay::ReplayMode >= 2)
        return 0;
    if ((uint32)car->carIndex >= 2)
        return 0;
    tGlobal *f = &g[car->carIndex];
    return f->active == 1;
}

void Force::Pause(void)
{
    FUNCTION_MARKER(0x800CB1B4, "NFS4.EXE");

    for (tGlobal *f = g; f != g + 2; ++f)
    {
        f->high = 0;
        f->low = 0;
        f->time = 0;
    }
}

void Force::UnPause(void)
{
    FUNCTION_MARKER(0x800CB1F4, "NFS4.EXE");
}

void Force::HitSign(Car::tObj *car)
{
    FUNCTION_MARKER(0x800CB1FC, "NFS4.EXE");

    (void)car;
}

void Force::HitWall(sint32 impulse)
{
    FUNCTION_MARKER(0x800CB204, "NFS4.EXE");

    (void)impulse;
}
