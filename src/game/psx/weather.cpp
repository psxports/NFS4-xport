#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "psx_gte.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/bworldsm.h"
#include "game/common/cameraobj.h"
#include "game/common/genericpmx.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/renderview.h"
#include "game/common/sim.h"
#include "game/psx/draw.h"
#include "game/psx/drawc.h"
#include "game/psx/trackspec.h"
#include "game/psx/weather.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/random.h"

struct Weather::tSys
{
    sint32 num[2];
    SVECTOR velocity;
    sint32 hdist;
    sint16 znear;
    sint16 zfar;
    sint16 length;
    sint16 bottom;
    sint16 top;
    sint16 height;
    sint16 width;
};

struct Weather::tSplatInfo
{
    DVECTOR pos;
    sint32 startTick;
};

static sint32 timechange;
sint32 gCurrentNumSplats;
coorddef prevCamPos[2];
matrixtdef prevCamMat[2];
sint32 Weather::gLastTimeProcessed;
sint32 Weather::gDensityGoalState;
sint32 Weather::gIntensityGoalState;
sint32 Weather::gDensityChangeFactor;
sint32 Weather::gIntensityChangeFactor;
sint32 Weather::gDensityTimerGoal;
sint32 Weather::gIntensityTimerGoal;
sint32 Weather::gSnowTrack;
sint32 Weather::gTrackIntensityLimit;
char *Weather::gWasDrawn;
Weather::tSys Weather::gSys;
CWeatherSpec *Weather::gTrackSpec;
Weather::tSplatInfo *Weather::gSplatInfo;
SVECTOR *Weather::gPos;
DVECTOR *Weather::gPrevPos;
sint32 Weather::gDensityTbl[4] = { 0, 50, 100, 150 };
sint32 Weather::gIntensityTbl[4] = { -104, -84, -70, -17 };
sint32 Weather::gTrackIntensityLimitTbl[16] = { 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

char Weather::gRandomVelocityVectors[12][3] = {
    { 252, 0, 0 },
    { 5, 0, 0 },
    { 250, 255, 0 },
    { 4, 255, 0 },
    { 0, 0, 253 },
    { 0, 0, 252 },
    { 0, 251, 1 },
    { 0, 253, 6 },
    { 4, 0, 4 },
    { 246, 0, 2 },
    { 8, 254, 255 },
    { 254, 248, 8 },
};

Weather::tSplatInfo *Weather::gSplatInfoServer[2];
char *Weather::gDrawnServer[2];
Weather::tState Weather::gType = Weather::kSnow;
SVECTOR *Weather::gPServer[2];
DVECTOR *Weather::gPrevPServer[2];
sint32 Weather::gLastProcessTime[2];

sint32 Weather::GetNumParticles(sint32 player)
{
    FUNCTION_MARKER(0x800E1DC0, "NFS4.EXE");
    return gSys.num[0];
}

void Weather::SetMatrix(matrixtdef *m)
{
    FUNCTION_MARKER(0x800E1DD0, "NFS4.EXE");
    MATRIX mpsx;
    for (sint32 row = 0; row < 3; ++row)
    {
        sint32 r0 = m->m[row];
        sint32 r1 = m->m[row + 3];
        sint32 r2 = m->m[row + 6];
        mpsx.m[row][0] = (sint16)(r0 >> 4);
        mpsx.m[row][1] = (sint16)(r1 >> 4);
        mpsx.m[row][2] = (sint16)(r2 >> 4);
    }
    mpsx.t[0] = 0;
    mpsx.t[1] = 0;
    mpsx.t[2] = 0;
    gte_SetRotMatrix(&mpsx);
    gte_SetTransMatrix(&mpsx);
}

void Weather::SetIdentMatrix(void)
{
    FUNCTION_MARKER(0x800E1E94, "NFS4.EXE");
    MATRIX Ident;
    Ident.m[0][0] = 4096;
    Ident.m[0][1] = 0;
    Ident.m[0][2] = 0;
    Ident.m[1][0] = 0;
    Ident.m[1][1] = -4096;
    Ident.m[1][2] = 0;
    Ident.m[2][0] = 0;
    Ident.m[2][1] = 0;
    Ident.m[2][2] = 4096;
    Ident.t[0] = 0;
    Ident.t[1] = 0;
    Ident.t[2] = 0;
    gte_SetRotMatrix(&Ident);
    gte_SetTransMatrix(&Ident);
}

void Weather::InitSnow(void)
{
    FUNCTION_MARKER(0x800E1F18, "NFS4.EXE");
    gSys.num[0] = 150;
    gSys.width = 640;
    gSys.znear = 320;
    gSys.zfar = 1600;
    gSys.bottom = -320;
    gSys.top = 960;
    gSys.velocity.vy = -18;
    gSys.length = 1280;
    gSys.height = 1280;
    gSys.velocity.vx = 0;
    gSys.velocity.vz = 0;
    gSys.hdist = 160;
}

void Weather::InitRain(void)
{
    FUNCTION_MARKER(0x800E1F78, "NFS4.EXE");
    gSys.num[0] = 150;
    gSys.znear = 384;
    gSys.zfar = 2560;
    gSys.length = 2176;
    gSys.bottom = -320;
    gSys.height = 1408;
    gSys.velocity.vy = -104;
    gSys.width = 1088;
    gSys.top = 1088;
    gSys.velocity.vx = 0;
    gSys.velocity.vz = 0;
    gSys.hdist = 160;
}

void Weather::InitSplats(void)
{
    FUNCTION_MARKER(0x800E1FD8, "NFS4.EXE");
    for (sint32 i = 0; i < 19; ++i)
    {
        uint32 ySize = GameSetup::gData.commMode == 1 ? 120U : 240U;
        uint32 value = random();
        gSplatInfo[i].pos.vx = (sint16)(value % 320);
        value = random();
        gSplatInfo[i].pos.vy = (sint16)(value % ySize);
        value = random();
        gSplatInfo[i].startTick = (value % 300U);
    }
}

sint32 Weather::GetNewState(void)
{
    FUNCTION_MARKER(0x800E20EC, "NFS4.EXE");
    sint32 randseed = random() & 3U;
    return randseed < 2 ? 0 : randseed == 2 ? 1 : -1;
}

void Weather::ChangeDensityState(void)
{
    FUNCTION_MARKER(0x800E212C, "NFS4.EXE");
    sint32 statechange = GetNewState();
    sint32 next = (gDensityGoalState + statechange);
    if (next >= 4)
        statechange = -1;
    else if (next < 0)
        statechange = 1;
    gDensityChangeFactor = statechange;
    gDensityGoalState = (gDensityGoalState + statechange);
    if (statechange == 0)
        gDensityTimerGoal = (simGlobal.gameTicks + 0x400);
}

void Weather::ChangeIntensityState(void)
{
    FUNCTION_MARKER(0x800E21A0, "NFS4.EXE");
    sint32 statechange = GetNewState();
    sint32 next = (gIntensityGoalState + statechange);
    if (next > gTrackIntensityLimit)
        statechange = -1;
    else if (next < 0)
        statechange = 1;
    gIntensityChangeFactor = statechange;
    gIntensityGoalState = (gIntensityGoalState + statechange);
    if (statechange == 0)
        gIntensityTimerGoal = (simGlobal.gameTicks + 0x400);
}

void Weather::ChangeDensityBasedOnTime(void)
{
    FUNCTION_MARKER(0x800E2218, "NFS4.EXE");
    bool change;
    if (gDensityChangeFactor > 0)
        change = gSys.num[0] >= gDensityTbl[gDensityGoalState];
    else if (gDensityChangeFactor < 0)
        change = gSys.num[0] <= gDensityTbl[gDensityGoalState];
    else
        change = simGlobal.gameTicks > gDensityTimerGoal;
    if (change)
        ChangeDensityState();
    gSys.num[0] = (gSys.num[0] + gDensityChangeFactor);
    if (gSys.num[0] <= 0)
        gSys.num[0] = 0;
}

void Weather::ChangeIntensityBasedOnTime(void)
{
    FUNCTION_MARKER(0x800E22F0, "NFS4.EXE");
    bool change;
    if (gIntensityChangeFactor > 0)
        change = gSys.velocity.vy >= gIntensityTbl[gIntensityGoalState];
    else if (gIntensityChangeFactor < 0)
        change = gSys.velocity.vy <= gIntensityTbl[gIntensityGoalState];
    else
        change = simGlobal.gameTicks > gIntensityTimerGoal;
    if (change)
        ChangeIntensityState();
    gSys.velocity.vy = (sint16)(((uint16)gSys.velocity.vy) + (uint32)(uint16)gIntensityChangeFactor);
    gType = gSys.velocity.vy < -32 ? kRain : kSnow;
}

void Weather::InitStateControls(void)
{
    FUNCTION_MARKER(0x800E23E8, "NFS4.EXE");
    sint32 track = GameSetup::gData.track;
    gTrackIntensityLimit = gTrackIntensityLimitTbl[track];
    if (track == 0 || track == 4)
    {
        gSnowTrack = 1;
        gIntensityGoalState = 3;
    }
    else
    {
        gSnowTrack = 0;
        gIntensityGoalState = 0;
    }
    gSys.velocity.vy = (sint16)gIntensityTbl[gIntensityGoalState];
    gType = gIntensityGoalState == 3 ? kSnow : kRain;
    sint32 ticks = simGlobal.gameTicks;
    gDensityGoalState = 3;
    sint32 count = gDensityTbl[3];
    gIntensityChangeFactor = 0;
    gDensityChangeFactor = 0;
    sint32 timer = (ticks + 0x400);
    gIntensityTimerGoal = timer;
    gSys.num[0] = count;
    gDensityTimerGoal = timer;
    timechange = ticks;
}

void Weather::Restart(void)
{
    FUNCTION_MARKER(0x800E24B4, "NFS4.EXE");
    if (GameSetup::gData.Weather == 0)
        return;
    if (GameSetup::gData.commMode != 1)
        InitStateControls();
    sint32 ticks = simGlobal.gameTicks;
    gLastProcessTime[1] = ticks;
    gLastProcessTime[0] = ticks;
    for (sint32 i = 0; i < 152; ++i)
        gWasDrawn[i] = 0;
}

void Weather::Init(void)
{
    FUNCTION_MARKER(0x800E2530, "NFS4.EXE");
    gTrackSpec = &TrackSpec::gSpec.weatherspec;
    if (GameSetup::gData.Weather == 0)
        return;
    sint32 type = TrackSpec::gSpec.weatherspec.type;
    gType = static_cast<tState>(type);
    if (type == 1)
        InitRain();
    else if (type == 0)
        InitSnow();
    if (gSplatInfo == 0)
        gSplatInfo = static_cast<tSplatInfo *>(reservememadr("raindrop", 0xa8, 0));
    if (gPos == 0)
        gPos = static_cast<SVECTOR *>(reservememadr("weather1", 0x4c0, 0));
    if (gPrevPos == 0)
        gPrevPos = static_cast<DVECTOR *>(reservememadr("weather2", 0x260, 0));
    if (gWasDrawn == 0)
        gWasDrawn = static_cast<char *>(reservememadr("weather3", 0x98, 0));
    sint32 ticks = simGlobal.gameTicks;
    gLastProcessTime[1] = ticks;
    gLastProcessTime[0] = ticks;
    gPServer[0] = gPos;
    gPrevPServer[0] = gPrevPos;
    gDrawnServer[0] = gWasDrawn;
    gSplatInfoServer[0] = gSplatInfo;
    if (GameSetup::gData.commMode == 1)
    {
        gPServer[1] = gPos + 76;
        gPrevPServer[1] = gPrevPos + 76;
        gDrawnServer[1] = gWasDrawn + 76;
        gSys.num[0] = 76;
        gSys.num[1] = 76;
        gSplatInfoServer[1] = gSplatInfo + 9;
    }
    prevCamPos[1] = Camera::gInfo[0].position;
    prevCamPos[0] = prevCamPos[1];
    memcpy(&prevCamMat[0], &Camera::gInfo[0].rotation, sizeof(matrixtdef));
    memcpy(&prevCamMat[1], &prevCamMat[0], sizeof(matrixtdef));
    XASSERT(gSys.width != 0 && gSys.height != 0 && gSys.length != 0);
    SVECTOR *sv = gPos;
    for (sint32 i = 0; i < 152; ++i)
    {
        uint32 value = random();
        sv->vx = (sint16)((value % (gSys.width)) * 2U - (uint32)(uint16)gSys.width);
        value = random();
        sv->vy = (sint16)(((uint16)gSys.bottom) + value % ((sint32)gSys.height));
        value = random();
        sv->vz = (sint16)(((uint16)gSys.znear) + value % ((sint32)gSys.length));
        ++sv;
    }
    for (sint32 i = 0; i < 152; ++i)
        gWasDrawn[i] = 0;
    gCurrentNumSplats = 0;
    InitSplats();
}

void Weather::DeInit(void)
{
    FUNCTION_MARKER(0x800E287C, "NFS4.EXE");
    if (GameSetup::gData.Weather == 0)
        return;
    if (gSplatInfo != 0)
        purgememadr(gSplatInfo);
    if (gPos != 0)
        purgememadr(gPos);
    if (gPrevPos != 0)
        purgememadr(gPrevPos);
    if (gWasDrawn != 0)
        purgememadr(gWasDrawn);
    gSplatInfo = 0;
    gPos = 0;
    gPrevPos = 0;
    gWasDrawn = 0;
}

void Weather::TransformVertex(matrixtdef *m, sint32 n, SVECTOR *s)
{
    FUNCTION_MARKER(0x800E2910, "NFS4.EXE");
    SetMatrix(m);
    gte_ldv0(s);
    gte_rt();
    uint32 remaining = (uint32)n - 1;
    VECTOR tv;
    gte_stlvnl(&tv);
    for (;;)
    {
        --remaining;
        SVECTOR *next = s + 1;
        if (remaining == 0xffffffff)
            break;
        gte_ldv0(next);
        gte_rt();
        s->vx = (sint16)tv.vx;
        s->vy = (sint16)tv.vy;
        s->vz = (sint16)tv.vz;
        gte_stlvnl(&tv);
        s = next;
    }
    s->vx = (sint16)tv.vx;
    s->vy = (sint16)tv.vy;
    s->vz = (sint16)tv.vz;
}

sint16 Weather::CheckAndResetParticles(SVECTOR *pt)
{
    FUNCTION_MARKER(0x800E29D4, "NFS4.EXE");
    XASSERT(gSys.width != 0 && gSys.height != 0 && gSys.length != 0 && gSys.zfar / 2 != 0);
    sint16 flag = 0;
    if (pt->vx > gSys.width)
    {
        pt->vx = (sint16)(64 - (uint16)gSys.width);
        uint32 value = random();
        pt->vy = (sint16)(((uint16)gSys.bottom) + value % ((sint32)gSys.height));
        value = random();
        pt->vz = (sint16)(((uint16)gSys.znear) + value % ((sint32)gSys.length));
        flag = 1;
    }
    else if (pt->vx < -gSys.width)
    {
        pt->vx = (sint16)((uint16)gSys.width - 64);
        uint32 value = random();
        pt->vy = (sint16)(((uint16)gSys.bottom) + value % ((sint32)gSys.height));
        value = random();
        pt->vz = (sint16)(((uint16)gSys.znear) + value % ((sint32)gSys.length));
        flag = 2;
    }
    if (pt->vz > gSys.zfar)
    {
        pt->vz = (sint16)((uint16)gSys.znear + 64);
        uint32 value = random();
        pt->vx = (sint16)((value % (gSys.width)) * 2 - (uint16)gSys.width);
        value = random();
        pt->vy = (sint16)(((uint16)gSys.bottom) + value % ((sint32)gSys.height));
        flag = 3;
    }
    else if (pt->vz < gSys.znear)
    {
        flag = 4;
        uint32 value = random();
        pt->vz = (sint16)(((uint16)gSys.znear) + value % (gSys.zfar / 2));
        value = random();
        pt->vx = (sint16)((value % (gSys.width)) * 2 - (uint16)gSys.width);
        value = random();
        pt->vy = (sint16)(((uint16)gSys.bottom) + value % ((sint32)gSys.height));
    }
    if (pt->vy > gSys.top)
    {
        pt->vy = (sint16)((uint16)gSys.bottom + 64);
        uint32 value = random();
        pt->vx = (sint16)((value % (gSys.width)) * 2 - (uint16)gSys.width);
        value = random();
        pt->vz = (sint16)(((uint16)gSys.znear) + value % ((sint32)gSys.length));
        flag = 5;
    }
    else if (pt->vy < gSys.bottom)
    {
        pt->vy = (sint16)((uint16)gSys.top - 64);
        uint32 value = random();
        pt->vx = (sint16)((value % (gSys.width)) * 2 - (uint16)gSys.width);
        value = random();
        pt->vz = (sint16)(((uint16)gSys.znear) + value % ((sint32)gSys.length));
        flag = 6;
    }
    return flag;
}

void Weather::QuickReOrthogonalize(matrixtdef *rot, matrixtdef *rotNew)
{
    FUNCTION_MARKER(0x800E2D50, "NFS4.EXE");
    coorddef upvector = { 0, 65536, 0 };
    sint32 zx = rot->m[6];
    sint32 zy = rot->m[7];
    sint32 zz = rot->m[8];
    rotNew->m[6] = zx;
    rotNew->m[7] = zy;
    rotNew->m[8] = zz;
    coorddef nrotz;
    memcpy(&nrotz, rotNew->m + 6, sizeof(nrotz));
    Math::NormalizeVector(&nrotz);
    memcpy(rotNew->m + 6, &nrotz, sizeof(nrotz));
    sint32 px = fixedmult(upvector.x, rot->m[6]);
    sint32 py = fixedmult(upvector.y, rot->m[7]);
    sint32 pz = fixedmult(upvector.z, rot->m[8]);
    sint32 dot = ((px + py) + pz);
    if (dot <= 0)
        dot = (-dot);
    if (dot > 0xfd70)
    {
        upvector.x = rot->m[3];
        upvector.y = rot->m[4];
        upvector.z = rot->m[5];
    }
    sint32 first = fixedmult(upvector.y, rotNew->m[8]);
    sint32 second = fixedmult(upvector.z, rotNew->m[7]);
    rotNew->m[0] = (first - second);
    first = fixedmult(upvector.z, rotNew->m[6]);
    second = fixedmult(upvector.x, rotNew->m[8]);
    rotNew->m[1] = (first - second);
    first = fixedmult(upvector.x, rotNew->m[7]);
    second = fixedmult(upvector.y, rotNew->m[6]);
    rotNew->m[2] = (first - second);
    coorddef nrotx;
    memcpy(&nrotx, rotNew->m, sizeof(nrotx));
    Math::NormalizeVector(&nrotx);
    memcpy(rotNew->m, &nrotx, sizeof(nrotx));
    first = fixedmult(rotNew->m[7], rotNew->m[2]);
    second = fixedmult(rotNew->m[8], rotNew->m[1]);
    rotNew->m[3] = (first - second);
    first = fixedmult(rotNew->m[8], rotNew->m[0]);
    second = fixedmult(rotNew->m[6], rotNew->m[2]);
    rotNew->m[4] = (first - second);
    first = fixedmult(rotNew->m[6], rotNew->m[1]);
    second = fixedmult(rotNew->m[7], rotNew->m[0]);
    rotNew->m[5] = (first - second);
}

void Weather::ProcessParticles(DRender::tView *Vi, sint32 num, SVECTOR *wpt, char *wd)
{
    FUNCTION_MARKER(0x800E2FD0, "NFS4.EXE");
    matrixtdef matdiff;
    matrixtdef orthoMat;
    matrixtdef *rot = &Vi->cview.mrotationInvRaw;
    QuickReOrthogonalize(rot, &orthoMat);
    Math::fasttransmult(prevCamMat + Vi->player, &orthoMat, &matdiff);
    transpose(&orthoMat, prevCamMat + Vi->player);
    TransformVertex(&matdiff, num, wpt);
    SetMatrix(rot);
    SVECTOR temp_vector;
    coorddef *cp = prevCamPos + Vi->player;
    sint32 n = (Vi->cview.translation.x - cp->x);
    temp_vector.vx = (sint16)(n / 1024);
    n = (Vi->cview.translation.y - cp->y);
    temp_vector.vy = (sint16)(n / 1024);
    n = (Vi->cview.translation.z - cp->z);
    temp_vector.vz = (sint16)(n / 1024);
    gte_ldv0(&temp_vector);
    gte_rt();
    *cp = Vi->cview.translation;
    VECTOR result;
    gte_stlvnl(&result);
    SVECTOR total_vector_change;
    total_vector_change.vx = (sint16)-(uint16)result.vx;
    total_vector_change.vy = (sint16)-(uint16)result.vy;
    total_vector_change.vz = (sint16)-(uint16)result.vz;
    temp_vector.vx = gSys.velocity.vx;
    temp_vector.vy = gSys.velocity.vy;
    temp_vector.vz = gSys.velocity.vz;
    gte_ldv0(&temp_vector);
    gte_rt();
    gte_stlvnl(&result);
    SVECTOR velocity_vector_change;
    velocity_vector_change.vx = (sint16)result.vx;
    velocity_vector_change.vy = (sint16)result.vy;
    velocity_vector_change.vz = (sint16)result.vz;
    total_vector_change.vx = (sint16)((uint16)total_vector_change.vx + (uint16)velocity_vector_change.vx);
    total_vector_change.vz = (sint16)((uint16)total_vector_change.vz + (uint16)velocity_vector_change.vz);
    total_vector_change.vy = (sint16)((uint16)total_vector_change.vy + (uint16)velocity_vector_change.vy);
    SVECTOR Weather_gTransformedRandomVelocityVectors[12];
    for (n = 0; n < 12; ++n)
    {
        temp_vector.vx = (sint8)gRandomVelocityVectors[n][0];
        temp_vector.vy = (sint8)gRandomVelocityVectors[n][1];
        temp_vector.vz = (sint8)gRandomVelocityVectors[n][2];
        gte_ldv0(&temp_vector);
        gte_rt();
        gte_stlvnl(&result);
        Weather_gTransformedRandomVelocityVectors[n].vx = (sint16)result.vx;
        Weather_gTransformedRandomVelocityVectors[n].vy = (sint16)result.vy;
        Weather_gTransformedRandomVelocityVectors[n].vz = (sint16)result.vz;
    }
    for (n = 0; n < num; ++n, ++wpt)
    {
        SVECTOR *tv = Weather_gTransformedRandomVelocityVectors + n % 12;
        SVECTOR pt;
        pt.vx = (sint16)((uint16)wpt->vx + (uint16)total_vector_change.vx + (uint16)tv->vx);
        pt.vy = (sint16)((uint16)wpt->vy + (uint16)total_vector_change.vy + (uint16)tv->vy);
        pt.vz = (sint16)((uint16)wpt->vz + (uint16)total_vector_change.vz + (uint16)tv->vz);
        sint16 flag = CheckAndResetParticles(&pt);
        wpt->vx = pt.vx;
        wpt->vy = pt.vy;
        wpt->vz = pt.vz;
        if (flag != 0)
            wd[n] = 0;
    }
}

void Weather::CreateSnow(SVECTOR *pt)
{
    FUNCTION_MARKER(0x800E33BC, "NFS4.EXE");
    SVECTOR gv[4];
    gv[0].vx = pt->vx;
    gv[0].vy = pt->vy;
    gv[0].vz = pt->vz;
    gv[1].vx = (sint16)((uint16)gv[0].vx + 10);
    gv[1].vy = gv[0].vy;
    gv[1].vz = gv[0].vz;
    gv[2].vx = gv[0].vx;
    gv[2].vy = (sint16)((uint16)gv[0].vy - 10);
    gv[2].vz = gv[0].vz;
    gv[3].vx = gv[1].vx;
    gv[3].vy = gv[2].vy;
    gv[3].vz = gv[1].vz;
    gte_ldv3(&gv[0], &gv[1], &gv[2]);
    gte_rtpt();
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_FT4 *prim = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    setlen(prim, 9);
    gte_stsxy3(&prim->p[0].xy, &prim->p[1].xy, &prim->p[2].xy);
    gte_ldv0(&gv[3]);
    gte_rtps();
    prim->rgbc.rgbc = 0x2e202020;
    gte_stsxy(&prim->p[3].xy);
    Draw::tPixMap *pmx = gWeatherPixmap[(reinterpret_cast<uintptr_t>(pt) & 4) >> 2];
    uint32 l0, l1, l2, l3;
    memcpy(&l0, pmx, 4);
    memcpy(&l1, reinterpret_cast<const char *>(pmx) + 4, 4);
    memcpy(&l2, reinterpret_cast<const char *>(pmx) + 8, 4);
    memcpy(&l3, reinterpret_cast<const char *>(pmx) + 12, 4);
    prim->p[0].uv_attr = l0;
    prim->p[1].uv_attr = l1;
    prim->p[2].uv_attr = l2;
    prim->p[3].uv_attr = l3;
}

void Weather::CreateRain(SVECTOR *pt0, DVECTOR *pt1, char *wd)
{
    FUNCTION_MARKER(0x800E356C, "NFS4.EXE");
    SVECTOR gv;
    gv.vx = pt0->vx;
    gv.vy = pt0->vy;
    gv.vz = pt0->vz;
    const bool drawn = *wd != 0;
    DVECTOR previous;
    if (drawn)
        previous = *pt1;
    gte_ldv0(&gv);
    gte_rtps();
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    LINE_G2 *prim = reinterpret_cast<LINE_G2 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    setlen(prim, 4);
    prim->p[0].rgbc.rgbc = 0x52000000;
    prim->p[1].rgbc.rgbc = 0x00402020;
    gte_stsxy(&prim->p[1].xy);
    if (drawn)
    {
        sint32 nx = previous.vx + prim->p[1].xy.x * 3;
        sint32 ny = previous.vy + prim->p[1].xy.y;
        prim->p[0].xy.x = (sint16)(nx / 4);
        prim->p[0].xy.y = (sint16)(ny / 2);
    }
    else
        prim->p[0].xy.xy = prim->p[1].xy.xy;
    *wd = 1;
    memcpy(pt1, &prim->p[1].xy, sizeof(*pt1));
}

void Weather::CreateSplat(Weather::tSplatInfo *splat)
{
    FUNCTION_MARKER(0x800E3730, "NFS4.EXE");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_FT4 *prim = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    sint16 vx = splat->pos.vx;
    sint16 vy = splat->pos.vy;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    setlen(prim, 9);
    prim->rgbc.code = 0x2e;
    sint32 size = (splat->pos.vx & 1) != 0 ? 12 : 18;
    sint32 splatTick = (simGlobal.gameTicks - splat->startTick);
    uint8 color = (uint8)(128 - ((uint32)splatTick << 2));
    sint32 expand = splatTick >> 3;
    prim->rgbc.b = color;
    prim->rgbc.g = color;
    prim->rgbc.r = color;
    sint16 bottom = (sint16)(vy + size + expand * 3);
    sint16 left = (sint16)(vx - expand);
    sint16 right = (sint16)(vx + size + expand);
    prim->p[2].xy.y = bottom;
    prim->p[3].xy.y = bottom;
    prim->p[0].xy.x = left;
    prim->p[0].xy.y = vy;
    prim->p[1].xy.x = right;
    prim->p[1].xy.y = vy;
    prim->p[2].xy.x = left;
    prim->p[3].xy.x = right;
    Draw::tPixMap *pmx = gWeatherPixmap[2];
    uint32 l0, l1, l2, l3;
    memcpy(&l0, pmx, 4);
    memcpy(&l1, reinterpret_cast<const char *>(pmx) + 4, 4);
    memcpy(&l2, reinterpret_cast<const char *>(pmx) + 8, 4);
    memcpy(&l3, reinterpret_cast<const char *>(pmx) + 12, 4);
    prim->p[0].uv_attr = l0;
    prim->p[1].uv_attr = l1;
    prim->p[2].uv_attr = l2;
    prim->p[3].uv_attr = l3;
}

void Weather::DoSplats(sint32 num, Weather::tSplatInfo *splats)
{
    FUNCTION_MARKER(0x800E3858, "NFS4.EXE");
    if (gCurrentNumSplats < num)
        gCurrentNumSplats = num;
    for (sint32 i = 0; i < gCurrentNumSplats; ++i, ++splats)
    {
        sint32 ticks = simGlobal.gameTicks;
        sint32 startTick = splats->startTick;
        if (startTick > ticks)
            continue;
        if ((startTick + 32) < ticks)
        {
            if (gCurrentNumSplats > num && i == gCurrentNumSplats - 1)
            {
                gCurrentNumSplats = i;
                continue;
            }
            uint32 value = random();
            splats->pos.vx = (sint16)(value % 320);
            bool split = GameSetup::gData.commMode == 1;
            value = random();
            splats->pos.vy = (sint16)(split ? (value % 240) >> 1 : value % 240);
            value = random();
            splats->startTick = (simGlobal.gameTicks + value % 100);
        }
        else
            CreateSplat(splats);
    }
}

void Weather::DoWeather(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800E3A1C, "NFS4.EXE");
    static sint32 prevLookBehind[2] = { -1, -1 };
    static sint32 prevCameraMode[2] = { -1, -1 };
    sint32 player = Vi->player;
    SVECTOR *wpt = gPServer[player];
    DVECTOR *wprevpt = gPrevPServer[player];
    char *wd = gDrawnServer[player];
    if (GameSetup::gData.commMode != 1)
    {
        sint32 ticks = simGlobal.gameTicks;
        if ((ticks - timechange) > 32)
        {
            timechange = ticks;
            if (gSnowTrack == 0)
                ChangeIntensityBasedOnTime();
            ChangeDensityBasedOnTime();
        }
    }
    if (gSys.num[0] == 0)
        return;
    sint32 ab = Input::gLookBehind[player];
    sint32 oldLookBehind = prevLookBehind[player];
    prevLookBehind[player] = ab;
    sint32 clean_up = ab != oldLookBehind && ab == 1;
    sint32 mode = Camera::GetMode(player);
    if (mode != prevCameraMode[player])
        clean_up = 1;
    prevCameraMode[player] = mode;
    if (clean_up != 0)
        for (sint32 i = 0; i < gSys.num[player]; ++i)
            wd[i] = 0;
    sint32 ticks = simGlobal.gameTicks;
    if ((ticks - gLastProcessTime[player]) > 1)
    {
        gLastProcessTime[player] = ticks;
        ProcessParticles(Vi, gSys.num[player], wpt, wd);
    }
    SetIdentMatrix();
    if (Camera::gInfo[player].inCar)
        DoSplats(gSys.num[player] >> 3, gSplatInfoServer[player]);
    for (sint32 n = 0; n < gSys.num[player]; ++n, ++wpt, ++wprevpt)
    {
        if (gType == kRain)
            CreateRain(wpt, wprevpt, wd + n);
        else
            CreateSnow(wpt);
    }
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    DR_MODE *prim = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    SetDrawMode(prim, 0, 0, 0x20, 0);
}

void Weather::BuildWeather(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800E3D30, "NFS4.EXE");
    if (GameSetup::gData.Weather != 0 && !BWorldSm::TunnelFlagSm(&Camera::gInfo[Vi->player].slicePos))
        DoWeather(Vi);
}
