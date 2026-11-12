#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/anim.h"
#include "game/common/bo.h"
#include "game/common/bworldsm.h"
#include "game/common/cameraobj.h"
#include "game/common/cameraslot.h"
#include "game/common/car.h"
#include "game/common/input.h"
#include "game/common/mathnfs.h"
#include "game/common/newton.h"
#include "game/common/renderview.h"
#include "game/common/replay.h"
#include "game/common/sim.h"
#include "game/common/trk.h"
#include "game/psx/trsproj.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

struct camera_flags
{
    coorddef arm;
    sint32 pitch:1,jostling:1,tracking:1,checkwalls:1,noLookBack:1,checkcollisions:1;
};

sint32 camSpeedTable[7] = { 64225, 60948, 56360, 52428, 47841, 43253, 39321 };

coorddef feeler3[3] =
{
    {0, 0, 137625},
    {-117964, 0, -72089},
    {117964, 0, -72089},
};

static coorddef gCop1Target[2];
static sint32 gSplitCameras[3] = { 0, 2, 5 };
static sint32 gTunnelCamHeight[13] = { 373555, 321126, 340787, 242483, 321126, 373555, 255590, 176947, 288358, 268697, 268697, 0, 0 };
static char gAnimMode[13] = { 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0 };
static char gAnimCams[13][4] = { { 2, 6, 0, 0 }, { 1, 6, 0, 0 }, { 2, 6, 0, 0 }, { 2, 6, 0, 0 }, { 2, 6, 0, 0 }, { 2, 6, 0, 0 }, { 2, 6, 0, 0 }, { 2, 6, 0, 0 }, { 2, 6, 0, 0 }, { 2, 6, 0, 0 }, { 2, 6, 0, 0 }, { 1, 6, 0, 0 }, { 1, 6, 0, 0 } };

static coorddef gDriverCam[28] =
{
    { -23592, 26869, -17039 },
    { -23592, 26869, -17039 },
    { 25559, 30801, 15728 },
    { 23592, 26869, 11796 },
    { -23592, 26869, 0 },
    { -23592, 26869, 0 },
    { -23592, 26869, 0 },
    { -23592, 26869, 0 },
    { -23592, 26869, 15728 },
    { -19660, 26869, -4587 },
    { -23592, 22937, -8519 },
    { -19660, 26869, -8519 },
    { -23592, 22937, 11796 },
    { -23592, 26869, 28180 },
    { -19660, 22937, -3276 },
    { 0, 19005, 32112 },
    { -19660, 26869, -8519 },
    { 25559, 30801, 15728 },
    { -19660, 26869, -4587 },
    { -23592, 26869, 11796 },
    { -15728, 22937, 0 },
    { -15728, 22937, 0 },
    { -23592, 26869, 28180 },
    { 25559, 30801, 15728 },
    { -23592, 26869, 15728 },
    { -19660, 26869, -4587 },
    { -19660, 26869, -8519 },
    { -23592, 26869, 28180 },
};

sint32 Camera::gGeomScreen = 190;
sint32 Camera::gCopDist[6];
Camera::tInfo Camera::gInfo[2];

Camera::tFlags Camera::gFlags[19] =
{
    { { 0, 26214, 58982 }, 0, -1, 0, 0, 0, 0 },
    { { 0, 26214, 58982 }, 0, 0, 0, 0, -1, 0 },
    { { 0, 75366, -314572 }, -1, 0, -1, -1, 0, -1 },
    { { 0, 75366, -314572 }, -1, 0, -1, -1, 0, -1 },
    { { 0, 75366, -314572 }, -1, 0, -1, -1, 0, -1 },
    { { 0, 131072, -393216 }, -1, 0, -1, -1, 0, -1 },
    { { 0, 131072, -393216 }, -1, 0, -1, -1, 0, -1 },
    { { 0, 321126, -439091 }, -1, 0, -1, -1, 0, -1 },
    { { -114688, 16384, -245760 }, 0, 0, -1, -1, -1, -1 },
    { { 0, 100270, 268697 }, 0, 0, 0, -1, -1, -1 },
    { { 0, 0, 0 }, 0, 0, -1, -1, -1, -1 },
    { { 0, 0, 0 }, 0, 0, -1, 0, -1, 0 },
    { { 0, 0, 0 }, 0, 0, -1, 0, -1, 0 },
    { { 0, 983040, -131072 }, 0, 0, -1, 0, -1, 0 },
    { { 0, 0, 0 }, 0, 0, 0, 0, -1, 0 },
    { { 0, 58785, -32702 }, 0, -1, 0, 0, -1, 0 },
    { { 0, 0, 0 }, 0, 0, 0, -1, -1, 0 },
    { { 0, 0, 0 }, 0, 0, -1, 0, -1, 0 },
    { { 0, 0, 0 }, 0, 0, -1, 0, -1, 0 },
};

static sint32 CameraSplineSpeed(uint8 mode)
{
    if (mode == 7)
        return feeler3[0].x;
    return camSpeedTable[mode];
}

static bool CameraWallBlocked(sint32 player, BWorldSm::Pos *slicePos)
{
    coorddef quadUnderCamera;
    if (slicePos->simQuad != 0)
        quadUnderCamera = slicePos->quadPts[0];
    else
    {
        Trk::NewSlice *slice = &BWorldSm::slices[slicePos->slice];
        quadUnderCamera.x = slice->center[0];
        quadUnderCamera.y = slice->center[1];
        quadUnderCamera.z = slice->center[2];
    }
    if (slicePos->simQuad != 0 && (slicePos->simQuad->surface & 15) == 0)
        return true;
    if (slicePos->offEdge != 0 || Camera::TooSteep(player, slicePos) != 0)
        return true;
    if (slicePos->simQuad != 0 && (slicePos->simQuad->surface & 0x80) != 0)
    {
        sint32 height = (quadUnderCamera.y - Camera::gInfo[player].anchor->position.y);
        if (height > 0x38000)
            return true;
    }
    return false;
}

static void CopyCameraMatrix(matrixtdef *source, matrixtdef *destination)
{
    for (sint32 i = 0; i < 8; i += 4)
    {
        sint32 x = source->m[i];
        sint32 y = source->m[i + 1];
        sint32 z = source->m[i + 2];
        sint32 w = source->m[i + 3];
        destination->m[i] = x;
        destination->m[i + 1] = y;
        destination->m[i + 2] = z;
        destination->m[i + 3] = w;
    }
    destination->m[8] = source->m[8];
}

static void SetNativeCameraFlags(camera_info *info)
{
    camera_flags *flagMode = &Camera::gFlags[info->mode];
    info->pitch = flagMode->pitch != 0;
    info->jostling = flagMode->jostling != 0;
    info->tracking = flagMode->tracking != 0;
    info->checkwalls = flagMode->checkwalls != 0;
    info->noLookBack = flagMode->noLookBack != 0;
    info->checkcollisions = flagMode->checkcollisions != 0;
    info->inCar = info->mode < 2;
}

void EulerToMat(matrixtdef *m, sint32 ax, sint32 ay, sint32 az)
{
    FUNCTION_MARKER(0x8008060C, "NFS4.EXE");
    matrixtdef mx, my, mz, mt;
    fixedxformx(&mx, ax);
    fixedxformy(&my, ay);
    fixedxformz(&mz, az);
    Math::fasttransmult(&mz, &my, &mt);
    Math::fasttransmult(&mt, &mx, m);
    sint32 x = m->m[3];
    sint32 y = m->m[4];
    sint32 z = m->m[5];
    m->m[3] = (-m->m[6]);
    m->m[5] = (-m->m[8]);
    m->m[4] = (-m->m[7]);
    m->m[6] = x;
    m->m[7] = y;
    m->m[8] = z;
}

void SetCameraZoom(sint32 player, sint32 targetDist)
{
    FUNCTION_MARKER(0x80081D38, "NFS4.EXE");
    camera_info *info = &Camera::gInfo[player];
    if (info->splitscreen)
        Camera::gGeomScreen = 190;
    else if (targetDist > 0x3000)
    {
        sint32 zoom = info->zooming;
        if (zoom == 1 || zoom == 2)
        {
            sint32 product = (targetDist * 190);
            Camera::gGeomScreen = product / 0x3000;
            if (zoom == 2 && Camera::gGeomScreen > 1200)
            {
                Camera::gGeomScreen = (6000 - Camera::gGeomScreen) >> 2;
                if (Camera::gGeomScreen < 750)
                    Camera::gGeomScreen = 750;
            }
        }
    }
    SetGeomScreen(Camera::gGeomScreen);
}

void Camera::SetCamera(DRender::tView *cview)
{
    FUNCTION_MARKER(0x80080704, "NFS4.EXE");
    GetViewInfo(cview->player, &cview->cview, cview->id);
    TrsProj::SetViewTrsProjEnviro(cview);
}

void Camera::LookBack(matrixtdef *src, matrixtdef *tgt)
{
    FUNCTION_MARKER(0x8008073C, "NFS4.EXE");
    tgt->m[0] = (-src->m[0]);
    tgt->m[1] = (-src->m[1]);
    tgt->m[2] = (-src->m[2]);
    tgt->m[3] = src->m[3];
    tgt->m[4] = src->m[4];
    tgt->m[5] = src->m[5];
    tgt->m[6] = (-src->m[6]);
    tgt->m[7] = (-src->m[7]);
    tgt->m[8] = (-src->m[8]);
}

void Camera::TunnelLimit(sint32 player, sint32 *armheight)
{
    FUNCTION_MARKER(0x800807C4, "NFS4.EXE");
    BWorldSm::Pos *slicePos = &gInfo[player].slicePos;
    if (BWorldSm::TunnelFlagSm(slicePos) || BWorldSm::TunnelFlagSm(&gInfo[player].anchor->simRoadInfo))
    {
        coorddef quadnormal = *BWorldSm::UNormal(slicePos);
        coorddef underCam = gInfo[player].position;
        sint32 roadheight = Newton::FindGroundElevationGeneral(&underCam, &quadnormal, slicePos->quadPts);
        sint32 track = GameSetup::gData.track;
        if (track >= 16)
            track = (track - 7);
        sint32 maxheight = ((gTunnelCamHeight[track] - gInfo[player].anchor->position.y) + roadheight);
        if (maxheight < *armheight) *armheight = maxheight;
    }
}

void Camera::UpdateCollisionCam(sint32 player)
{
    FUNCTION_MARKER(0x8008090C, "NFS4.EXE");
    coorddef arm = { 0, 196608, -655360 };
    if (simVar.quickPauseSim != 0 && Replay::ReplayInterface.changeCamera == 0)
        return;
    if (InBetween != 0)
        return;
    camera_info *info = &gInfo[player];
    if (info->direction)
        arm.z = (-arm.z);
    coorddef newarm;
    transform(&arm, &info->anchor->roadMatrix, &newarm);
    TunnelLimit(player, &newarm.y);
    coorddef oldarm = info->relpos;
    sint32 step = fixedmult(((newarm.x - oldarm.x)), 0x51e);
    info->relpos.x = (oldarm.x + step);
    step = fixedmult(((newarm.y - oldarm.y)), 0x51e);
    info->relpos.y = (oldarm.y + step);
    step = fixedmult(((newarm.z - oldarm.z)), 0x51e);
    BO::tNewtonObj *anchor = info->anchor;
    info->relpos.z = (oldarm.z + step);
    info->position.x = (anchor->position.x + info->relpos.x);
    info->position.y = (anchor->position.y + info->relpos.y);
    sint32 z = (anchor->position.z + info->relpos.z);
    bool tumbling = info->tumbling != 0;
    info->position.z = z;
    if (!tumbling)
        info->intransition = 50;
    info->pitch = 1;
    info->jostling = 0;
    info->tracking = 1;
    info->checkwalls = 1;
    info->noLookBack = 1;
    info->checkcollisions = 1;
}

void Camera::UpdateSimpleCam(sint32 player)
{
    FUNCTION_MARKER(0x80080AC8, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    coorddef arm = gFlags[info->mode].arm;
    coorddef newarm;
    transform(&arm, &info->anchor->orientMat, &newarm);
    TunnelLimit(player, &newarm.y);
    sint32 x = (info->anchor->position.x + newarm.x);
    BO::tNewtonObj *anchor = info->anchor;
    info->position.x = x;
    info->position.y = (anchor->position.y + newarm.y);
    info->position.z = (anchor->position.z + newarm.z);
    LookBack(&anchor->orientMat, &info->rotation);
}

void Camera::UpdateBumperCam(sint32 player)
{
    FUNCTION_MARKER(0x80080BAC, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    coorddef arm = gFlags[info->mode].arm;
    bool lookingBehind = !info->noLookBack && Input::gLookBehind[player] != 0;
    coorddef newarm;
    if (lookingBehind)
    {
        transform(&arm, &info->anchor->orientMat, &newarm);
        sint32 x = (info->anchor->position.x + newarm.x);
        BO::tNewtonObj *anchor = info->anchor;
        info->audioPos.x = x;
        info->audioPos.y = (anchor->position.y + newarm.y);
        info->audioPos.z = (anchor->position.z + newarm.z);
        arm.z = (-arm.z);
    }
    transform(&arm, &info->anchor->orientMat, &newarm);
    sint32 x = (info->anchor->position.x + newarm.x);
    BO::tNewtonObj *anchor = info->anchor;
    info->position.x = x;
    info->position.y = (anchor->position.y + newarm.y);
    info->position.z = (anchor->position.z + newarm.z);
    if (lookingBehind)
        LookBack(&anchor->orientMat, &info->rotation);
    else
        CopyCameraMatrix(&info->anchor->orientMat, &info->rotation);
}

void Camera::UpdateTailCam(sint32 player, sint32 behavior)
{
    FUNCTION_MARKER(0x80080D84, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    coorddef arm = gFlags[info->mode].arm;
    Car::tObj *anchor = reinterpret_cast<Car::tObj *>(info->anchor);
    sint32 lookahead = 3;
    if (anchor->wrongway > 0)
    {
        if (GameSetup::gData.reverseTrack == 1)
            lookahead = -3;
    }
    else if (GameSetup::gData.reverseTrack == 0)
        lookahead = -3;
    if (simVar.quickPauseSim != 0 && Replay::ReplayInterface.changeCamera == 0)
        return;
    if (InBetween != 0)
        return;
    sint32 maxrate = behavior == 2 ? 0xccc : 0x1999;
    sint32 rate = 0x1999;
    sint32 rateY = 0xccc;
    if (behavior != 0)
    {
        sint32 vx = anchor->linearVel_ch.x;
        sint32 vz = anchor->linearVel_ch.z;
        if (vx < 0)
            vx = (-vx);
        if (vz < 0)
            vz = (-vz);
        sint32 speed;
        if (vz < vx)
            speed = (vx + (vz >> 2));
        else
            speed = (vz + (vx >> 2));
        rate = speed / 900;
        if (rate > maxrate)
            rate = maxrate;
        if (rate < 0x51e)
            rate = 0x51e;
    }
    if (Input::gLookBehind[player] != 0)
        transform(&arm, &info->anchor->orientMat, &info->relpos);
    sint32 slice = info->anchor->simRoadInfo.slice + lookahead;
    if (lookahead < 0)
    {
        if (slice < 0)
            slice = (slice + gNumSlices);
    }
    else if (slice >= gNumSlices)
        slice = (slice - gNumSlices);
    sint32 height = BWorldSm::slices[slice].center[1];
    sint32 backSlice = slice - lookahead;
    if (lookahead <= 0)
    {
        if (backSlice >= gNumSlices)
            backSlice = (backSlice - gNumSlices);
    }
    else if (backSlice < 0)
        backSlice = (backSlice + gNumSlices);
    sint32 vertigo = (height - BWorldSm::slices[backSlice].center[1]) / 3;
    if (behavior == 0)
        vertigo = 0;
    else if (behavior == 1)
    {
        if (vertigo > 0x10000)
            vertigo = 0x10000;
        if (vertigo < 0x4000)
            vertigo = 0x4000;
    }
    else if (behavior == 2)
    {
        if (vertigo > 0x20000)
            vertigo = 0x20000;
        if (vertigo < -0x4000)
            vertigo = -0x4000;
    }
    arm.y = (arm.y + vertigo);
    coorddef newarm;
    if (Input::gLookBehind[player] != 0)
    {
        transform(&arm, &info->anchor->orientMat, &newarm);
        sint32 x = (info->anchor->position.x + newarm.x);
        BO::tNewtonObj *body = info->anchor;
        info->audioPos.x = x;
        info->audioPos.y = (body->position.y + newarm.y);
        info->audioPos.z = (body->position.z + newarm.z);
        arm.z = (-arm.z);
        transform(&arm, &info->anchor->orientMat, &newarm);
        TunnelLimit(player, &newarm.y);
        x = (info->anchor->position.x + newarm.x);
        body = info->anchor;
        info->position.x = x;
        info->position.y = (body->position.y + newarm.y);
        info->position.z = (body->position.z + newarm.z);
        return;
    }
    transform(&arm, &info->anchor->orientMat, &newarm);
    TunnelLimit(player, &newarm.y);
    coorddef oldarm = info->relpos;
    if (info->intransition != 0)
    {
        info->intransition = (char)((uint8)info->intransition - 1);
        rateY = 0x1999;
        rate = 0x1999;
    }
    sint32 step = fixedmult(((newarm.x - oldarm.x)), rate);
    info->relpos.x = (oldarm.x + step);
    step = fixedmult(((newarm.y - oldarm.y)), rateY);
    info->relpos.y = (oldarm.y + step);
    step = fixedmult(((newarm.z - oldarm.z)), rate);
    bool transitioning = info->intransition != 0;
    info->relpos.z = (oldarm.z + step);
    if (!transitioning)
    {
        sint32 len = Math::VectorLength(&info->relpos);
        if (len < 0x4f0a3)
        {
            sint32 scale = fixeddiv(0x4f0a3, len);
            sint32 scaled = fixedmult(scale, info->relpos.x);
            sint32 y = info->relpos.y;
            info->relpos.x = scaled;
            scaled = fixedmult(scale, y);
            sint32 z = info->relpos.z;
            info->relpos.y = scaled;
            info->relpos.z = fixedmult(scale, z);
        }
    }
    sint32 x = (info->anchor->position.x + info->relpos.x);
    BO::tNewtonObj *body = info->anchor;
    info->position.x = x;
    info->position.y = (body->position.y + info->relpos.y);
    info->position.z = (body->position.z + info->relpos.z);
}

void Camera::UpdateHeliCam(sint32 player, sint32 behavior)
{
    FUNCTION_MARKER(0x800813CC, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    coorddef arm = gFlags[info->mode].arm;
    Car::tObj *anchor = reinterpret_cast<Car::tObj *>(info->anchor);
    sint32 lookahead = 3;
    if (anchor->wrongway > 0)
    {
        if (GameSetup::gData.reverseTrack == 1)
            lookahead = -3;
    }
    else if (GameSetup::gData.reverseTrack == 0)
        lookahead = -3;
    if (simVar.quickPauseSim != 0 && Replay::ReplayInterface.changeCamera == 0)
        return;
    if (InBetween != 0)
        return;
    sint32 maxrate = behavior == 2 ? 0xccc : 0x1999;
    sint32 rate = 0x1999;
    sint32 rateY = 0xccc;
    if (behavior != 0)
    {
        sint32 vx = anchor->linearVel_ch.x;
        sint32 vz = anchor->linearVel_ch.z;
        if (vx < 0)
            vx = (-vx);
        if (vz < 0)
            vz = (-vz);
        sint32 speed;
        if (vz < vx)
            speed = (vx + (vz >> 2));
        else
            speed = (vz + (vx >> 2));
        rate = speed / 900;
        if (rate > maxrate)
            rate = maxrate;
        if (rate < 0x51e)
            rate = 0x51e;
    }
    if (Replay::ReplayMode >= 2)
        rate = 0x28f;
    if (Input::gLookBehind[player] != 0)
        transform(&arm, &info->anchor->orientMat, &info->relpos);
    sint32 vx = anchor->linearVel_ch.x;
    sint32 vz = anchor->linearVel_ch.z;
    if (vx < 0)
        vx = (-vx);
    if (vz < 0)
        vz = (-vz);
    sint32 speed;
    if (vz < vx)
        speed = (vx + (vz >> 2));
    else
        speed = (vz + (vx >> 2));
    sint32 fallback = speed / 20;
    if (fallback > 0x20000)
        fallback = 0x20000;
    arm.z = (arm.z - fallback);
    sint32 slice = info->anchor->simRoadInfo.slice + lookahead;
    if (lookahead < 0)
    {
        if (slice < 0)
            slice = (slice + gNumSlices);
    }
    else if (slice >= gNumSlices)
        slice = (slice - gNumSlices);
    sint32 height = BWorldSm::slices[slice].center[1];
    sint32 backSlice = slice - lookahead;
    if (lookahead <= 0)
    {
        if (backSlice >= gNumSlices)
            backSlice = (backSlice - gNumSlices);
    }
    else if (backSlice < 0)
        backSlice = (backSlice + gNumSlices);
    sint32 vertigo = (height - BWorldSm::slices[backSlice].center[1]) / 2;
    if (behavior == 0)
        vertigo = 0;
    else if (behavior == 1)
    {
        if (vertigo > 0x14000)
            vertigo = 0x14000;
        if (vertigo < 0x4000)
            vertigo = 0x4000;
    }
    else if (behavior == 2)
    {
        if (vertigo > 0x30000)
            vertigo = 0x30000;
        if (vertigo < -0xc000)
            vertigo = -0xc000;
    }
    arm.y = (arm.y + vertigo);
    coorddef newarm;
    if (Input::gLookBehind[player] != 0)
    {
        transform(&arm, &info->anchor->orientMat, &newarm);
        sint32 x = (info->anchor->position.x + newarm.x);
        BO::tNewtonObj *body = info->anchor;
        info->audioPos.x = x;
        info->audioPos.y = (body->position.y + newarm.y);
        info->audioPos.z = (body->position.z + newarm.z);
        arm.z = (-arm.z);
        transform(&arm, &info->anchor->orientMat, &newarm);
        TunnelLimit(player, &newarm.y);
        x = (info->anchor->position.x + newarm.x);
        body = info->anchor;
        info->position.x = x;
        info->position.y = (body->position.y + newarm.y);
        info->position.z = (body->position.z + newarm.z);
        return;
    }
    transform(&arm, &info->anchor->orientMat, &newarm);
    TunnelLimit(player, &newarm.y);
    coorddef oldarm = info->relpos;
    if (info->intransition != 0)
    {
        info->intransition = (char)((uint8)info->intransition - 1);
        rateY = 0x1999;
        rate = 0x1999;
    }
    sint32 step = fixedmult(((newarm.x - oldarm.x)), rate);
    info->relpos.x = (oldarm.x + step);
    step = fixedmult(((newarm.y - oldarm.y)), rateY);
    info->relpos.y = (oldarm.y + step);
    step = fixedmult(((newarm.z - oldarm.z)), rate);
    bool transitioning = info->intransition != 0;
    info->relpos.z = (oldarm.z + step);
    if (!transitioning)
    {
        sint32 len = Math::VectorLength(&info->relpos);
        if (len < 0x651eb)
        {
            sint32 scale = fixeddiv(0x651eb, len);
            sint32 scaled = fixedmult(scale, info->relpos.x);
            sint32 y = info->relpos.y;
            info->relpos.x = scaled;
            scaled = fixedmult(scale, y);
            sint32 z = info->relpos.z;
            info->relpos.y = scaled;
            info->relpos.z = fixedmult(scale, z);
        }
    }
    sint32 x = (info->anchor->position.x + info->relpos.x);
    BO::tNewtonObj *body = info->anchor;
    info->position.x = x;
    info->position.y = (body->position.y + info->relpos.y);
    info->position.z = (body->position.z + info->relpos.z);
}

void Camera::UpdateCircleCam(sint32 player)
{
    FUNCTION_MARKER(0x80081AB8, "NFS4.EXE");
    if ((simVar.quickPauseSim != 0 && Replay::ReplayInterface.changeCamera == 0) || InBetween != 0 || simVar.pauseSim != 0)
        return;

    const sint32 circle_height[3] = { 0x10000, 0x20000, 0x30000 };
    camera_info *info = &gInfo[player];
    info->circleAngle = (sint16)(info->circleAngle + 1);

    sint32 sin;
    sint32 cos;
    intsincos(info->circleAngle, &sin, &cos);

    coorddef src;
    src.x = fixedmult(0x48000, cos);
    src.z = fixedmult(0x60000, sin);
    if (((info->circleAngle + 0x100) & 0x1FF) == 0)
        info->circleCounter = (sint16)(info->circleCounter + 1);

    sint32 index = info->circleCounter % 3;
    if (index < 0)
        index += 3;
    sint32 h0 = circle_height[index];
    sint32 ang = (info->circleAngle - 0x80) & 0x1FF;
    src.y = h0;
    if (ang < 0x80)
    {
        sint32 h1 = circle_height[(index + 1) % 3];
        src.y += ((h1 - h0) * ang) >> 7;
    }

    coorddef des;
    transform(&src, &info->anchor->orientMat, &des);
    TunnelLimit(player, &des.y);
    const coorddef &position = info->anchor->position;
    info->position.x = (position.x + des.x);
    info->position.y = (position.y + des.y);
    info->position.z = (position.z + des.z);
}

void Camera::UpdateTVCam(sint32 player)
{
    FUNCTION_MARKER(0x80081E48, "NFS4.EXE");
    static sint32 lastX[2];
    static sint32 lastY[2];
    camera_info *info = &gInfo[player];
    sint32 dist = Math::Dist3D(&info->target->position, &info->position);
    bool positive = dist > 0;
    dist = Math::Dist3D(&info->target->position, &info->position);
    if (!positive)
        dist = (-dist);
    sint32 targetDist = dist >> 4;
    if (lastX[player] != info->position.x)
    {
        lastX[player] = info->position.x;
        lastY[player] = info->position.y;
    }
    sint32 height = fixedmult(info->TVHeight >> 2, ((targetDist - 0x4000)));
    if (info->TVHeight < height)
        height = info->TVHeight;
    if (height < 0)
        height = 0;
    info->position.y = (lastY[player] + height);
    SetCameraZoom(player, targetDist);
}

void Camera::UpdateAnimCam(sint32 player)
{
    FUNCTION_MARKER(0x80081F94, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    if ((sint8)info->animHandle == -1) {
        sint32 animNum = (sint8)info->animNum;
        info->animNum = (char)(animNum - 1);
        info->animHandle = (char)Anim::Handle((uint8)gAnimCams[GameSetup::gData.track][animNum]);
    }

    coorddef animPos;
    matrixtdef animRot;
    AnimScript *script = Anim::GetAnim((sint8)info->animHandle);
    if (script->GetTimedAnimPosRot(&animPos, &animRot) <= 0) {
        if ((sint8)info->animNum <= 0) {
            if (Replay::ReplayMode != 2)
                SetMode(player, GameSetup::gData.carInfo[player].Camera[0]);
            else if (numValidCams != 0)
                Replay::ReplayFindClosestCamera(player, info->anchor->simRoadInfo.slice);
            else {
                SetMode(player, 11);
                info->splineMode = 3;
            }
            return;
        }
        Anim::FreeHandle((sint8)info->animHandle);
        sint32 animNum = (sint8)info->animNum;
        info->animNum = (char)(animNum - 1);
        info->animHandle = (char)Anim::Handle((uint8)gAnimCams[GameSetup::gData.track][animNum]);
        script = Anim::GetAnim((sint8)info->animHandle);
        script->GetTimedAnimPosRot(&animPos, &animRot);
    }

    uint32 animIndex = (uint8)info->animNum & 31U;
    if (((uint8)gAnimMode[GameSetup::gData.track] >> animIndex & 1U) != 0) {
        info->position = animPos;
        CopyCameraMatrix(&animRot, &info->rotation);
    } else {
        coorddef newarm;
        transform(&animPos, &info->anchor->orientMat, &newarm);
        BO::tNewtonObj *anchor = info->anchor;
        info->position.x = (anchor->position.x + newarm.x);
        info->position.y = (anchor->position.y + newarm.y);
        info->position.z = (anchor->position.z + newarm.z);
        Math::fasttransmult(&animRot, &anchor->orientMat, &info->rotation);
    }
}

void Camera::UpdateFinishCam(sint32 player)
{
    FUNCTION_MARKER(0x80082254, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    if ((sint8)info->animHandle == -1)
        info->animHandle = (char)Anim::Handle(1);
    AnimScript *script = Anim::GetAnim((sint8)info->animHandle);
    coorddef finishPos, newarm;
    matrixtdef finishRot;
    if (script->GetTimedAnimPosRot(&finishPos, &finishRot) <= 0)
        Anim::GetLastAnimPosRot(1, 6, &finishPos, &finishRot);
    transform(&finishPos, &info->anchor->orientMat, &newarm);
    sint32 x = (info->anchor->position.x + newarm.x);
    BO::tNewtonObj *anchor = info->anchor;
    info->position.x = x;
    info->position.y = (anchor->position.y + newarm.y);
    info->position.z = (anchor->position.z + newarm.z);
    Math::fasttransmult(&finishRot, &anchor->orientMat, &info->rotation);
    info->pitch = 0;
    info->jostling = 0;
    info->tracking = 0;
    info->checkwalls = 1;
    info->noLookBack = 1;
    info->checkcollisions = 1;
}

void Camera::UpdateBlimpCam(sint32 player)
{
    FUNCTION_MARKER(0x8008237C, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    coorddef arm = gFlags[info->mode].arm;
    TunnelLimit(player, &arm.y);
    coorddef oldarm;
    oldarm.x = (info->position.x - info->anchor->position.x);
    oldarm.y = (info->position.y - info->anchor->position.y);
    oldarm.z = (info->position.z - info->anchor->position.z);
    sint32 step = fixedmult(((arm.x - oldarm.x)), 0x1999);
    oldarm.x = (oldarm.x + step);
    step = fixedmult(((arm.y - oldarm.y)), 0x1999);
    oldarm.y = (oldarm.y + step);
    step = fixedmult(((arm.z - oldarm.z)), 0x1999);
    oldarm.z = (oldarm.z + step);
    sint32 x = (info->anchor->position.x + oldarm.x);
    BO::tNewtonObj *anchor = info->anchor;
    info->position.x = x;
    info->position.y = (anchor->position.y + oldarm.y);
    info->position.z = (anchor->position.z + oldarm.z);
}

void Camera::SetSplineCam(sint32 player)
{
    FUNCTION_MARKER(0x800824C0, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    Car::tObj *anchor = reinterpret_cast<Car::tObj *>(info->anchor);
    uint32 difference = 0x10000u - CameraSplineSpeed((uint8)info->splineMode);
    sint32 numSlice = ((sint32)(difference * 15) >> 16) + 1;
    if (numSlice >= 9)
        numSlice = 8;
    if (Replay::ReplayCamera[player].defaultCamera != 0)
        return;
    sint32 first = fixedmult(info->rotation.m[6], anchor->N.roadMatrix.m[6]);
    sint32 second = fixedmult(info->rotation.m[7], info->anchor->roadMatrix.m[7]);
    sint32 third = fixedmult(info->rotation.m[8], info->anchor->roadMatrix.m[8]);
    sint32 dot = ((first + second) + third);
    sint32 direction = numSlice;
    if (dot < 0)
        direction = (-direction);
    if (anchor->linearVel_ch.z < 0)
        direction = (-direction);
    sint32 current = anchor->N.simRoadInfo.slice;
    uint16 next = (uint16)((uint16)current + direction);
    if (direction < 0)
    {
        if (current + direction < 0)
            next = (uint16)((uint16)gNumSlices + next);
    }
    else if (current + direction >= gNumSlices)
        next = (uint16)(next - (uint16)gNumSlices);
    info->slicePos.slice = next;
    Trk::NewSlice *slice = &BWorldSm::slices[info->slicePos.slice];
    coorddef center = { slice->center[0], slice->center[1], slice->center[2] };
    info->position = center;
    BWorldSm::FindClosestQuadRez(&info->position, &info->slicePos, 1);
}

void Camera::UpdateSplineCam(sint32 player)
{
    FUNCTION_MARKER(0x800826C0, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    Car::tObj *anchor = reinterpret_cast<Car::tObj *>(info->anchor);
    if (simVar.quickPauseSim != 0 && Replay::ReplayInterface.changeCamera == 0)
        return;
    if (InBetween != 0)
        return;
    sint32 difference = anchor->N.simRoadInfo.slice - info->slicePos.slice;
    sint32 sliceDist = difference > 0 ? difference : -difference;
    if (gNumSlices / 2 < sliceDist)
    {
        difference = anchor->N.simRoadInfo.slice - info->slicePos.slice;
        if (difference > 0)
            sliceDist = (gNumSlices - difference);
        else
            sliceDist = (gNumSlices - (info->slicePos.slice - anchor->N.simRoadInfo.slice));
    }
    else
    {
        difference = anchor->N.simRoadInfo.slice - info->slicePos.slice;
        sliceDist = difference > 0 ? difference : -difference;
    }
    sint32 speed = CameraSplineSpeed((uint8)info->splineMode);
    sint32 change = ((0x10000 - speed) * 15) >> 16;
    if (change + 2 < sliceDist && speed < 0x10000 && Replay::ReplayCamera[player].defaultCamera == 0)
    {
        sint32 numSlice = change + 1;
        if (numSlice >= 9)
            numSlice = 8;
        sint32 first = fixedmult(info->rotation.m[6], info->anchor->roadMatrix.m[6]);
        sint32 second = fixedmult(info->rotation.m[7], info->anchor->roadMatrix.m[7]);
        sint32 third = fixedmult(info->rotation.m[8], info->anchor->roadMatrix.m[8]);
        sint32 dot = ((first + second) + third);
        sint32 direction = numSlice;
        if (dot < 0)
            direction = (-direction);
        if (anchor->linearVel_ch.z < 0)
            direction = (-direction);
        sint32 current = anchor->N.simRoadInfo.slice;
        uint16 next = (uint16)((uint16)current + direction);
        if (direction < 0)
        {
            if (current + direction < 0)
                next = (uint16)((uint16)gNumSlices + next);
        }
        else if (current + direction >= gNumSlices)
            next = (uint16)(next - (uint16)gNumSlices);
        info->slicePos.slice = next;
        Trk::NewSlice *slice = &BWorldSm::slices[info->slicePos.slice];
        coorddef center = { slice->center[0], slice->center[1], slice->center[2] };
        info->position = center;
    }
    BWorldSm::FindClosestQuadRez(&info->position, &info->slicePos, 1);
    Trk::NewSlice *slice = &BWorldSm::slices[info->slicePos.slice];
    coorddef splineVel = { slice->center[0], slice->center[1], slice->center[2] };
    sint32 next = info->slicePos.slice + 1;
    Trk::NewSlice *nextSlice = next < gNumSlices ? &BWorldSm::slices[next] : BWorldSm::slices;
    coorddef nextVel = { nextSlice->center[0], nextSlice->center[1], nextSlice->center[2] };
    splineVel.x = (nextVel.x - splineVel.x);
    splineVel.y = (nextVel.y - splineVel.y);
    splineVel.z = (nextVel.z - splineVel.z);
    Math::NormalizeVector(&splineVel);
    sint32 first = fixedmult(anchor->N.linearVel.x, splineVel.x);
    sint32 second = fixedmult(anchor->N.linearVel.y, splineVel.y);
    sint32 third = fixedmult(anchor->N.linearVel.z, splineVel.z);
    sint32 relativeVel = ((first + second) + third);
    if ((GameSetup::gData.sgge & 4) != 0)
        relativeVel = fixedmult(relativeVel, 0xcccc);
    relativeVel = fixedmult(relativeVel, CameraSplineSpeed((uint8)info->splineMode));
    coorddef cameraVel;
    cameraVel.x = fixedmult(relativeVel, splineVel.x);
    cameraVel.y = fixedmult(relativeVel, splineVel.y);
    cameraVel.z = fixedmult(relativeVel, splineVel.z);
    BO::tNewtonObj *target = info->target;
    info->position.x = (info->position.x + (cameraVel.x >> 6));
    info->position.y = (info->position.y + (cameraVel.y >> 6));
    info->position.z = (info->position.z + (cameraVel.z >> 6));
    sint32 dist = Math::Dist3D(&target->position, &info->position);
    bool positive = dist > 0;
    dist = Math::Dist3D(&info->target->position, &info->position);
    if (!positive)
        dist = (-dist);
    SetCameraZoom(player, dist >> 4);
}

sint32 Camera::IslandProfile(uint16 before)
{
    FUNCTION_MARKER(0x80082C3C, "NFS4.EXE");
    uint16 after = (uint16)(before & (before - 1));
    uint16 diff = (uint16)(before - after);
    while (before != 0)
    {
        if (before - after != diff)
            return 1;
        before = after;
        after = (uint16)(before & (before - 1));
        diff = (uint16)(diff << 1);
    }
    return 0;
}

void Camera::UpdatePulloverCam(sint32 player)
{
    FUNCTION_MARKER(0x80082C94, "NFS4.EXE");
    if (simVar.quickPauseSim != 0 && Replay::ReplayInterface.changeCamera == 0)
        return;
    if (InBetween != 0)
        return;
    SetGeomScreen(190);
    camera_info *info = &gInfo[player];
    if (simGlobal.gameTicks < info->POInhibitor)
    {
        UpdateCopCam2(player);
        return;
    }
    Trk::NewSlice *slice = &BWorldSm::slices[info->anchor->simRoadInfo.slice];
    coorddef sCenter = { slice->center[0], slice->center[1], slice->center[2] };
    coorddef sForward;
    sForward.x = (sint8)slice->forward[0] * 512;
    sForward.y = (sint8)slice->forward[1] * 512;
    sForward.z = (sint8)slice->forward[2] * 512;
    coorddef sccVec;
    sccVec.x = (info->anchor->position.x - sCenter.x);
    sccVec.y = (info->anchor->position.y - sCenter.y);
    sccVec.z = (info->anchor->position.z - sCenter.z);
    coorddef sRight;
    sRight.x = (sint8)slice->right[0] * 2048;
    sRight.y = (sint8)slice->right[1] * 2048;
    sRight.z = (sint8)slice->right[2] * 2048;
    sint32 first = fixedmult(sccVec.z, sForward.x);
    sint32 second = fixedmult(sccVec.x, sForward.z);
    sint32 ySign = (first - second);
    sint32 island = IslandProfile((uint16)BWorldSm::slices[info->anchor->simRoadInfo.slice].pavedProfile);
    if (ySign < 0)
        island ^= 1;
    if (island != 0)
    {
        sRight.x = (-sRight.x);
        sRight.z = (-sRight.z);
        sRight.y = (-sRight.y);
    }
    coorddef newarm;
    newarm.y = 0x60000;
    TunnelLimit(player, &newarm.y);
    newarm.x = (sCenter.x + sRight.x);
    newarm.y = ((newarm.y + sCenter.y) + sRight.y);
    newarm.z = (sCenter.z + sRight.z);
    sint32 step = fixedmult(((newarm.x - info->position.x)), 0x7ae);
    sint32 oldX = info->position.x;
    sint32 oldY = info->position.y;
    info->position.x = (oldX + step);
    step = fixedmult(((newarm.y - oldY)), 0x7ae);
    oldY = info->position.y;
    sint32 oldZ = info->position.z;
    info->position.y = (oldY + step);
    step = fixedmult(((newarm.z - oldZ)), 0x7ae);
    info->position.z = (info->position.z + step);
    info->pitch = 0;
    info->jostling = 0;
    info->tracking = 1;
    info->checkwalls = 1;
    info->noLookBack = 1;
    info->checkcollisions = 1;
}

void Camera::UpdateCopCam1(sint32 player)
{
    FUNCTION_MARKER(0x80083010, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    coorddef vec;
    vec.x = (info->anchor->position.x + info->target->position.x);
    vec.y = (info->anchor->position.y + info->target->position.y);
    vec.z = (info->anchor->position.z + info->target->position.z);
    gCop1Target[player].x = vec.x >> 1;
    gCop1Target[player].y = vec.y >> 1;
    gCop1Target[player].z = vec.z >> 1;
    vec.x = (info->anchor->position.x - info->target->position.x);
    vec.y = (info->anchor->position.y - info->target->position.y);
    vec.z = (info->anchor->position.z - info->target->position.z);
    Math::NormalizeVector(&vec);
    vec.x = fixedmult(0x40000, vec.x);
    vec.y = fixedmult(0x40000, vec.y);
    vec.z = fixedmult(0x40000, vec.z);
    sint32 x = ((info->anchor->position.x + vec.x) + vec.z);
    BO::tNewtonObj *anchor = info->anchor;
    info->position.x = x;
    info->position.y = (anchor->position.y + 0x18000);
    info->position.z = ((anchor->position.z - vec.x) + vec.z);
}

void Camera::UpdateCopCam2(sint32 player)
{
    FUNCTION_MARKER(0x800831A8, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    coorddef dirVec;
    dirVec.x = (info->target->position.x - info->anchor->position.x) / 4;
    dirVec.y = (info->target->position.y - info->anchor->position.y) / 4;
    dirVec.z = (info->target->position.z - info->anchor->position.z) / 4;
    Math::NormalizeVector(&dirVec);
    BO::tNewtonObj *anchor = info->anchor;
    sint32 x = (anchor->position.x - ((uint32)dirVec.x << 1));
    info->position.x = x;
    info->position.y = (anchor->position.y + 0x13333);
    info->position.z = (anchor->position.z - ((uint32)dirVec.z << 1));
}

void Camera::UpdateBTCopCam(sint32 player)
{
    FUNCTION_MARKER(0x800832B0, "NFS4.EXE");
    camera_info *info = &gInfo[player];
    forceFocus_t focusMode = info->forceFocus;
    if (focusMode == FOCUS_AI)
    {
        BO::tNewtonObj *anchor = &Cars::gList[(sint8)info->focusOnAICar]->N;
        sint32 focus = (sint8)info->focusOnAICar;
        info->anchor = anchor;
        info->target = &Cars::gList[focus]->N;
        UpdateCircleCam(player);
    }
    else if (focusMode == FOCUS_COPANDAI)
    {
        BO::tNewtonObj *anchor = &Cars::gList[player]->N;
        sint32 focus = (sint8)info->focusOnAICar;
        info->anchor = anchor;
        info->target = &Cars::gList[focus]->N;
        UpdateCopCam2(player);
    }
    gGeomScreen = 190;
    info->POInhibitor = (simGlobal.gameTicks + 0x140);
    info->pitch = 0;
    info->jostling = 0;
    info->tracking = 1;
    info->checkwalls = 1;
    info->noLookBack = 1;
    info->checkcollisions = 0;
}

void Camera::Update(void)
{
    FUNCTION_MARKER(0x800833D8, "NFS4.EXE");
    for (sint32 player = 0; player <= ((gInfo[0].splitscreen != 0) ? 1 : 0); ++player)
    {
        camera_info *info = &gInfo[player];
        Car::tObj *anchor = reinterpret_cast<Car::tObj *>(info->anchor);
        if (info->checkcollisions)
        {
            if ((uint8)info->tumbling != 0)
            {
                info->tumbling = (char)((uint8)info->tumbling - 1u);
                UpdateCollisionCam(player);
                info->inCar = 0;
                info->modechange = 1;
                continue;
            }
            if (anchor->N.orientationToGround.y < 0x8000)
            {
                sint32 x = fixedmult(info->rotation.m[6], anchor->N.roadMatrix.m[6]);
                sint32 y = fixedmult(info->rotation.m[7], info->anchor->roadMatrix.m[7]);
                sint32 z = fixedmult(info->rotation.m[8], info->anchor->roadMatrix.m[8]);
                sint32 direction = ((x + y) + z);
                info->direction = direction < 0;
                info->tumbling = 100;
            }
        }
        if ((anchor->carFlags & 1u) != 0 && anchor->stats.finishType == 2)
        {
            UpdateFinishCam(player);
            info->inCar = 0;
            info->modechange = 1;
            continue;
        }
        if ((sint32)info->forceFocus > 0)
        {
            UpdateBTCopCam(player);
            info->inCar = 0;
            info->modechange = 1;
            continue;
        }
        if (Cars::gHumanRaceCarList[player]->pullOver != 0 && Cars::gHumanRaceCarList[player]->stats.finishType != 3)
        {
            UpdatePulloverCam(player);
            info->inCar = 0;
            info->modechange = 1;
            continue;
        }
        if (info->modechange)
        {
            SetNativeCameraFlags(info);
            info->modechange = 0;
            info->anchor = &Cars::gHumanRaceCarList[player]->N;
            info->target = &Cars::gHumanRaceCarList[player]->N;
            if (Replay::ReplayMode >= 2 && Replay::ReplayCamera[player].cameraMode == 19)
                Replay::ReplayFindClosestCamera(player, info->anchor->simRoadInfo.slice);
        }
        switch (info->mode)
        {
        case 0:
        case 1: UpdateBumperCam(player); break;
        case 2: UpdateTailCam(player, 0); break;
        case 3: UpdateTailCam(player, 1); break;
        case 4: UpdateTailCam(player, 2); break;
        case 5: UpdateHeliCam(player, 0); break;
        case 6: UpdateHeliCam(player, 1); break;
        case 7: UpdateHeliCam(player, 2); break;
        case 8:
        case 9:
        case 15: UpdateSimpleCam(player); break;
        case 10: UpdateCircleCam(player); break;
        case 11: UpdateSplineCam(player); break;
        case 12: UpdateTVCam(player); break;
        case 13: UpdateBlimpCam(player); break;
        case 14: UpdateAnimCam(player); break;
        case 16: UpdateCopCam1(player); break;
        case 17:
        case 18: UpdateCopCam2(player); break;
        }
    }
}

void Camera::Init(void)
{
    FUNCTION_MARKER(0x80083858, "NFS4.EXE");
    sint32 splitScreen = GameSetup::gData.commMode == 1;
    BWorldSm::Pos slicePos;
    memset(&slicePos, 0, sizeof(slicePos));
    for (sint32 i = 0; i <= splitScreen; ++i)
    {
        camera_info *info = &gInfo[i];
        sint32 localCar = splitScreen ? i : GameSetup::gData.localCar;
        info->anchor = &Cars::gHumanRaceCarList[localCar]->N;
        info->target = &Cars::gHumanRaceCarList[localCar]->N;
        info->position = info->anchor->position;
        info->audioPos = info->anchor->position;
        info->TVHeight = 0;
        CopyCameraMatrix(&info->anchor->orientMat, &info->rotation);
        info->twist = 0;
        info->wallLeft = info->anchor->position;
        info->wallRight = info->anchor->position;
        info->camNum = 0;
        info->circleCounter = 0;
        info->circleAngle = 0;
        info->modechange = 0;
        info->pitch = 0;
        info->jostling = 0;
        info->tracking = 0;
        info->checkwalls = 0;
        info->noLookBack = 1;
        info->checkcollisions = 0;
        info->splitscreen = splitScreen != 0;
        info->intransition = 0;
        info->tumbling = 0;
        info->mode = 14;
        info->direction = 0;
        info->zooming = 0;
        info->inCar = 0;
        info->animNum = gAnimCams[GameSetup::gData.track][0];
        if ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) && ((Cars::gHumanRaceCarList[0]->carFlags & 0x200u) != 0 || (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200u) != 0)))
            info->animNum = 1;
        info->animHandle = (char)0xFF;
        info->splineMode = 0;
        info->forceFocus = FOCUS_NORMAL;
        info->focusOnAICar = (char)0xFF;
        info->POInhibitor = 64;
        info->slicePos = slicePos;
    }
    ResetRelPos(3);
    sint32 type = Cars::gHumanRaceCarList[0]->carInfo->carType;
    gGeomScreen = 190;
    if (type < 28 && (GameSetup::gData.sgge & 0x100u) != 0 && splitScreen == 0)
    {
        GameSetup::gData.carInfo[0].Camera[0] = 1;
        gFlags[1].arm = gDriverCam[type];
    }
}

void Camera::Kill(void)
{
    FUNCTION_MARKER(0x80083BEC, "NFS4.EXE");
    sint32 splitScreen = GameSetup::gData.commMode == 1;
    for (sint32 i = 0; i <= splitScreen; ++i)
    {
        sint32 handle = (sint8)gInfo[i].animHandle;
        if (handle != -1)
            Anim::FreeHandle(handle);
        gInfo[i].animHandle = (char)0xff;
    }
}

void Camera::PitchAndRoll(sint32 player)
{
    FUNCTION_MARKER(0x80083C74, "NFS4.EXE");
    Car::tObj *anchor = reinterpret_cast<Car::tObj *>(gInfo[player].anchor);
    matrixtdef m1, m2, m3;
    sint32 pitch = ((uint32)anchor->render.bodyPitch << 1);
    fixedxformz(&m1, anchor->render.bodyRoll);
    pitch = (sint32)(pitch | (uint32)gInfo[player].pitch);
    fixedxformx(&m2, pitch);
    Math::fasttransmult(&m1, &m2, &m3);
    Math::fasttransmult(&m3, &gInfo[player].rotation, &gInfo[player].rotation);
}

sint32 Camera::TooSteep(sint32 player, BWorldSm::Pos *slicePos)
{
    FUNCTION_MARKER(0x80083D28, "NFS4.EXE");
    BWorldSm::Pos *slicePos2 = &gInfo[player].anchor->simRoadInfo;
    coorddef normUnderCam = *BWorldSm::UNormal(slicePos);
    coorddef normUnderCar = *BWorldSm::UNormal(slicePos2);
    sint32 first = fixedmult(normUnderCam.x, normUnderCar.x);
    sint32 second = fixedmult(normUnderCam.y, normUnderCar.y);
    sint32 third = fixedmult(normUnderCam.z, normUnderCar.z);
    sint32 dot = ((first + second) + third);
    if (dot > 0xb4fc)
        return 0;
    coorddef camToCar;
    camToCar.x = (gInfo[player].anchor->position.x - gInfo[player].position.x);
    camToCar.y = (gInfo[player].anchor->position.y - gInfo[player].position.y);
    camToCar.z = (gInfo[player].anchor->position.z - gInfo[player].position.z);
    first = fixedmult(normUnderCam.x, camToCar.x);
    second = fixedmult(normUnderCam.y, camToCar.y);
    third = fixedmult(normUnderCam.z, camToCar.z);
    dot = ((first + second) + third);
    return dot > 0 && gInfo[player].anchor->flightTime == 0;
}

void Camera::CheckWallCollisions(sint32 player, coorddef *pos)
{
    FUNCTION_MARKER(0x80083E98, "NFS4.EXE");
    BWorldSm::Pos slicePos = gInfo[player].slicePos;
    coorddef triVec, triPnt;
    sint32 i;
    for (i = 0; i < 3; ++i)
    {
        transform(&feeler3[i], &gInfo[player].anchor->orientMat, &triVec);
        triPnt.x = (pos->x + triVec.x);
        triPnt.y = (pos->y + triVec.y);
        triPnt.z = (pos->z + triVec.z);
        BWorldSm::FindClosestQuadMaxIterations(&triPnt, &slicePos, 3);
        if (CameraWallBlocked(player, &slicePos))
            break;
    }
    if (i == 3)
        return;
    coorddef step;
    step.x = (gInfo[player].anchor->position.x - triPnt.x);
    step.y = (gInfo[player].anchor->position.y - triPnt.y);
    step.z = (gInfo[player].anchor->position.z - triPnt.z);
    coorddef edge = gInfo[player].anchor->position;
    step.x >>= 7;
    step.z >>= 7;
    step.y >>= 7;
    sint32 count = 0;
    do
    {
        edge.x = (edge.x - step.x);
        edge.y = (edge.y - step.y);
        edge.z = (edge.z - step.z);
        BWorldSm::FindClosestQuadMaxIterations(&edge, &slicePos, 3);
        if (CameraWallBlocked(player, &slicePos))
            break;
        ++count;
    } while (count < 128);
    step.x = (gInfo[player].anchor->position.x - edge.x) >> 1;
    step.y = (gInfo[player].anchor->position.y - edge.y) >> 1;
    step.z = (gInfo[player].anchor->position.z - edge.z) >> 1;
    coorddef temp;
    for (sint32 side = 0; side < 2; ++side)
    {
        sint32 camAngle = side == 0 ? 1 : -1;
        do
        {
            sint32 sin, cos;
            intsincos(camAngle, &sin, &cos);
            sint32 first = fixedmult(cos, step.x);
            sint32 second = fixedmult(sin, step.z);
            temp.x = (first - second);
            temp.y = step.y;
            first = fixedmult(sin, step.x);
            second = fixedmult(cos, step.z);
            temp.z = (first + second);
            temp.x = (edge.x + temp.x);
            temp.y = (edge.y + temp.y);
            temp.z = (edge.z + temp.z);
            BWorldSm::FindClosestQuadMaxIterations(&temp, &slicePos, 3);
            if (CameraWallBlocked(player, &slicePos))
            {
                if (side == 0)
                    gInfo[player].wallLeft = temp;
                else
                    gInfo[player].wallRight = temp;
                break;
            }
            camAngle += side == 0 ? 4 : -4;
        } while (side == 0 ? camAngle < 1024 : camAngle > -1024);
    }
    temp.x = (gInfo[player].wallRight.x - gInfo[player].wallLeft.x);
    temp.y = ((gInfo[player].wallRight.y - gInfo[player].wallLeft.y) + 0x8000);
    temp.z = (gInfo[player].wallRight.z - gInfo[player].wallLeft.z);
    Math::NormalizeVector(&temp);
    coorddef normal = { -temp.z, temp.y, temp.x };
    edge.x = (edge.x - triPnt.x);
    edge.z = (edge.z - triPnt.z);
    edge.y = (edge.y - triPnt.y);
    sint32 first = fixedmult(edge.x, normal.x);
    sint32 second = fixedmult(edge.y, normal.y);
    sint32 third = fixedmult(edge.z, normal.z);
    sint32 camDotNorm = ((first + second) + third);
    first = fixedmult(triVec.x, normal.x);
    second = fixedmult(triVec.y, normal.y);
    third = fixedmult(triVec.z, normal.z);
    sint32 edgeDotNorm = ((first + second) + third);
    camDotNorm = ((camDotNorm + edgeDotNorm) + 0x10000);
    if (camDotNorm > 0)
    {
        temp.x = fixedmult(camDotNorm, normal.x);
        temp.y = fixedmult(camDotNorm, normal.y);
        temp.z = fixedmult(camDotNorm, normal.z);
        pos->x = (pos->x + temp.x);
        pos->y = (pos->y + temp.y);
        pos->z = (pos->z + temp.z);
    }
}

void Camera::SetAboveGround(sint32 player, coorddef *pos)
{
    FUNCTION_MARKER(0x8008480C, "NFS4.EXE");
    BWorldSm::Pos *slicePos = &gInfo[player].slicePos;
    coorddef quadnormal = *BWorldSm::UNormal(slicePos);
    sint32 elevation = Newton::FindGroundElevationGeneral(pos, &quadnormal, slicePos->quadPts);
    elevation = (elevation + 0x10000);
    if (pos->y < elevation)
        pos->y = elevation;
}

void Camera::AcquireTarget(sint32 player, coorddef *point, coorddef *pos, matrixtdef *rot, sint32 hirez)
{
    FUNCTION_MARKER(0x80084898, "NFS4.EXE");
    coorddef upvector = { 0, 0x10000, 0 };
    coorddef tgtPos = point ? *point : gInfo[player].target->position;
    coorddef *rotx = reinterpret_cast<coorddef *>(&rot->m[0]);
    coorddef *roty = reinterpret_cast<coorddef *>(&rot->m[3]);
    coorddef *rotz = reinterpret_cast<coorddef *>(&rot->m[6]);
    sint32 delta = (tgtPos.x - pos->x);
    rotz->x = delta / 4;
    delta = (tgtPos.y - pos->y);
    rotz->y = delta / 4;
    delta = (tgtPos.z - pos->z);
    rotz->z = delta / 4;
    if (gInfo[player].pitch)
    {
        uint32 offset = ((uint16)gInfo[player].mode) - 2u < 3u ? 0x5333u : 0x6666u;
        rotz->y = (rotz->y + offset);
    }
    if (Replay::ReplayMode == 2)
        upvector.x = gInfo[player].twist;
    Math::NormalizeVector(rotz);
    sint32 first = fixedmult(upvector.x, rotz->x);
    sint32 second = fixedmult(upvector.y, rotz->y);
    sint32 third = fixedmult(upvector.z, rotz->z);
    sint32 dot = ((first + second) + third);
    bool positive = dot > 0;
    first = fixedmult(upvector.x, rotz->x);
    second = fixedmult(upvector.y, rotz->y);
    third = fixedmult(upvector.z, rotz->z);
    dot = ((first + second) + third);
    if (!positive)
        dot = (-dot);
    if (dot > 0xfd70)
        upvector = *roty;
    first = fixedmult(upvector.y, rotz->z);
    second = fixedmult(upvector.z, rotz->y);
    rotx->x = (first - second);
    first = fixedmult(upvector.z, rotz->x);
    second = fixedmult(upvector.x, rotz->z);
    rotx->y = (first - second);
    first = fixedmult(upvector.x, rotz->y);
    second = fixedmult(upvector.y, rotz->x);
    rotx->z = (first - second);
    Math::NormalizeVector(rotx);
    first = fixedmult(rotz->y, rotx->z);
    second = fixedmult(rotz->z, rotx->y);
    roty->x = (first - second);
    first = fixedmult(rotz->z, rotx->x);
    second = fixedmult(rotz->x, rotx->z);
    roty->y = (first - second);
    first = fixedmult(rotz->x, rotx->y);
    second = fixedmult(rotz->y, rotx->x);
    roty->z = (first - second);
    Math::NormalizeVector(roty);
}

void Camera::OpponentLookBehind(sint32 player, coorddef *pos, sint32 reset)
{
    FUNCTION_MARKER(0x80084C34, "NFS4.EXE");
    static coorddef lastOppVector[2];
    if (reset != 0)
    {
        Car::tObj *car = Cars::gHumanRaceCarList[player];
        coorddef forward = { car->N.orientMat.m[6], car->N.orientMat.m[7], car->N.orientMat.m[8] };
        lastOppVector[player] = forward;
        return;
    }
    Car::tObj *car = Cars::gHumanRaceCarList[player];
    coorddef oppVector = { car->N.orientMat.m[6], car->N.orientMat.m[7], car->N.orientMat.m[8] };
    sint32 oppSlice = 11;
    for (sint32 i = 0; i < Cars::gNumCars; ++i)
    {
        Car::tObj *opponent = Cars::gList[i];
        if (opponent == Cars::gHumanRaceCarList[player])
            continue;
        sint32 dist = opponent->N.simRoadInfo.slice - Cars::gHumanRaceCarList[player]->N.simRoadInfo.slice;
        if (dist < 0)
            dist = -dist;
        if (gNumSlices / 2 < dist)
            dist = (gNumSlices - dist);
        if (dist >= 11)
            continue;
        coorddef tempVector;
        tempVector.x = (pos->x - opponent->N.position.x);
        tempVector.y = (pos->y - Cars::gList[i]->N.position.y);
        tempVector.z = (pos->z - Cars::gList[i]->N.position.z);
        Math::NormalizeVector(&tempVector);
        sint32 first = fixedmult(Cars::gHumanRaceCarList[player]->N.orientMat.m[6], tempVector.x);
        sint32 second = fixedmult(Cars::gHumanRaceCarList[player]->N.orientMat.m[7], tempVector.y);
        sint32 third = fixedmult(Cars::gHumanRaceCarList[player]->N.orientMat.m[8], tempVector.z);
        sint32 dot = ((first + second) + third);
        sint32 oppAngle = intarccos(dot);
        bool positive = oppAngle > 0;
        oppAngle = intarccos(dot);
        if (!positive)
            oppAngle = (-oppAngle);
        if (oppAngle < 0x80 && dist < oppSlice)
        {
            oppVector = tempVector;
            oppSlice = dist;
        }
    }
    coorddef *last = &lastOppVector[player];
    sint32 delta = (oppVector.x - last->x);
    sint32 step = fixedmult(delta, 0x4ccc);
    last->x = (last->x + step);
    delta = (oppVector.y - last->y);
    step = fixedmult(delta, 0x4ccc);
    last->y = (last->y + step);
    delta = (oppVector.z - last->z);
    step = fixedmult(delta, 0x4ccc);
    last->z = (last->z + step);
    oppVector.x = (pos->x - last->x);
    oppVector.y = (pos->y - last->y);
    oppVector.z = (pos->z - last->z);
    sint32 mode = (sint16)((uint16)gInfo[player].mode - 2);
    switch (mode)
    {
    case 0:
    case 1:
    case 2:
        oppVector.y = (oppVector.y - 0x14ccc);
        break;
    case 5:
        oppVector.y = (oppVector.y - 0x6666);

    case 3:
    case 4:
        oppVector.y = (oppVector.y - 0x19999);
        break;
    }
    AcquireTarget(player, &oppVector, pos, &gInfo[player].rotation, 1);
}

void Camera::GetViewInfo(sint32 cviewP, DRender::tCalcView *cview, sint32 viewID)
{
    FUNCTION_MARKER(0x80085008, "NFS4.EXE");
    camera_info *info = &gInfo[cviewP];
    if (info->jostling)
        PitchAndRoll(cviewP);
    coorddef position = info->position;
    cview->translation = position;
    if (info->checkwalls)
        CheckWallCollisions(cviewP, &cview->translation);
    BWorldSm::FindClosestQuadRez(&cview->translation, &info->slicePos, 1);
    if (info->slicePos.offEdge == 0 && info->mode >= 2)
        SetAboveGround(cviewP, &cview->translation);
    if (info->tracking)
        AcquireTarget(cviewP, 0, &cview->translation, &info->rotation, 1);
    if (info->mode == 16)
        AcquireTarget(cviewP, &gCop1Target[cviewP], &cview->translation, &info->rotation, 1);
    if (!info->noLookBack)
        OpponentLookBehind(cviewP, &cview->translation, Input::gLookBehind[cviewP] == 0);
    CopyCameraMatrix(&info->rotation, &cview->mrotation);
    if (GameSetup::gData.mirrorTrack != 0)
    {
        sint32 t1 = cview->mrotation.m[0];
        sint32 t3 = cview->mrotation.m[2];
        cview->mrotation.m[0] = (-t1);
        sint32 t2 = cview->mrotation.m[1];
        cview->mrotation.m[2] = (-t3);
        cview->mrotation.m[1] = (-t2);
    }
    transpose(&cview->mrotation, &cview->mrotationInv);
    CopyCameraMatrix(&cview->mrotationInv, &cview->mrotationInvRaw);
    sint32 scale = 0xdc00;
    sint32 t1 = fixedmult(cview->mrotation.m[3], scale);
    sint32 t2 = fixedmult(cview->mrotation.m[4], scale);
    sint32 t3 = fixedmult(cview->mrotation.m[5], scale);
    cview->mrotation.m[3] = t1;
    cview->mrotation.m[4] = t2;
    cview->mrotation.m[5] = t3;
    transpose(&cview->mrotation, &cview->mrotationInv);
    coorddef tmp;
    tmp.x = (-cview->translation.x);
    tmp.y = (-cview->translation.y);
    tmp.z = (-cview->translation.z);
    transform(&tmp, &cview->mrotationInv, &cview->translationInv);
}

void Camera::GetAudioViewInfo(sint32 cviewP, DRender::tCalcView *cview, coorddef **cvel)
{
    FUNCTION_MARKER(0x8008533C, "NFS4.EXE");
    camera_info *info = &gInfo[cviewP];
    if (Input::gLookBehind[cviewP] == 0 || info->noLookBack)
    {
        coorddef position = info->position;
        cview->translation = position;
        CopyCameraMatrix(&info->rotation, &cview->mrotation);
    }
    else
    {
        bool tracking = info->tracking != 0;
        coorddef audioPos = info->audioPos;
        cview->translation = audioPos;
        if (tracking)
            AcquireTarget(cviewP, 0, &cview->translation, &cview->mrotation, 0);
        else
            LookBack(&info->rotation, &cview->mrotation);
    }
    sint32 mode = gInfo[cviewP].mode;
    if (mode == 12 || mode == 14) *cvel = 0;
    else *cvel = &gInfo[cviewP].anchor->linearVel;
}

sint32 Camera::GetMode(sint32 cviewP)
{
    FUNCTION_MARKER(0x80085568, "NFS4.EXE");
    Car::tObj *car = Cars::gHumanRaceCarList[cviewP];
    if ((car->carFlags & 1) != 0 && car->stats.finishType == 2)
        return 20;
    car = Cars::gHumanRaceCarList[cviewP];
    if (car->pullOver != 0 && car->stats.finishType != 3)
        return 20;
    return gInfo[cviewP].mode;
}

void Camera::SetMode(sint32 cviewP, sint32 mode)
{
    FUNCTION_MARKER(0x80085608, "NFS4.EXE");
    camera_info *info = &gInfo[cviewP];
    InBetween = 0;
    if (info->modechange)
        return;
    if (((uint16)info->mode) - 11u < 2u)
    {
        gGeomScreen = 190;
        TrsProj::SetProjection(0, 0, 320, 240);
    }
    if (((uint16)info->mode) - 2u >= 6u)
        ResetRelPos((cviewP + 1));
    sint32 handle = (sint8)info->animHandle;
    if (handle >= 0)
    {
        Anim::FreeHandle(handle);
        info->animHandle = (char)0xff;
    }
    info->mode = (sint16)mode;
    if (info->mode >= 20)
        info->mode = (sint16)GameSetup::gData.carInfo[cviewP].Camera[0];
    if (info->mode == 11)
        SetSplineCam(cviewP);
    info->intransition = 50;
    SetNativeCameraFlags(info);
}

void Camera::NextMode(sint32 cviewP)
{
    FUNCTION_MARKER(0x8008581C, "NFS4.EXE");
    camera_info *info = &gInfo[cviewP];
    if (info->mode == 14 || info->modechange)
        return;
    if (((uint16)info->mode) - 11u < 2u)
    {
        gGeomScreen = 190;
        TrsProj::SetProjection(0, 0, 320, 240);
    }
    if (((uint16)info->mode) - 2u >= 6u)
        ResetRelPos((cviewP + 1));
    sint32 handle = (sint8)info->animHandle;
    if (handle >= 0)
    {
        Anim::FreeHandle(handle);
        info->animHandle = (char)0xff;
    }
    if (info->splitscreen)
    {
        info->camNum = (sint16)((uint16)info->camNum + 1);
        info->mode = (sint16)gSplitCameras[info->camNum % 3];
    }
    else if ((GameSetup::gData.raceType == 1 || GameSetup::gData.raceType == 5) && ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0 || (Cars::gNumHumanRaceCars == 2 && (Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0)))
    {
        info->camNum = (sint16)((uint16)info->camNum + 1);
        info->mode = (sint16)GameSetup::gData.carInfo[cviewP].Camera[(uint16)info->camNum & 3];
    }
    else
    {
        info->camNum = (sint16)((uint16)info->camNum + 1);
        info->mode = (sint16)GameSetup::gData.carInfo[cviewP].Camera[info->camNum % 3];
    }
    if (info->mode >= 20)
        info->mode = (sint16)GameSetup::gData.carInfo[cviewP].Camera[0];
    if (info->mode == 11)
        SetSplineCam(cviewP);
    info->intransition = 50;
    SetNativeCameraFlags(info);
}

void Camera::ReplayUpdate(sint32 cviewP, Camera::tCamSlot *ptr)
{
    FUNCTION_MARKER(0x80085BD0, "NFS4.EXE");
    camera_info *info = &gInfo[cviewP];
    if (info->modechange)
        return;
    SetMode(cviewP, (uint8)ptr->mode);
    info->tracking = ptr->track != 0;
    info->zooming = ptr->zoom;
    info->splineMode = ptr->splineMode;
    sint32 fov = ptr->fov;
    coorddef pos = ptr->pos;
    info->position = pos;
    info->TVHeight = ptr->height;
    sint32 ax = ptr->euler.x;
    sint32 ay = ptr->euler.y;
    sint32 az = ptr->euler.z;
    gGeomScreen = fov;
    EulerToMat(&info->rotation, ax, ay, az);
}

void Camera::ResetRelPos(sint32 bitMask)
{
    FUNCTION_MARKER(0x80085CD4, "NFS4.EXE");
    if ((bitMask & 1) != 0)
    {
        BO::tNewtonObj *anchor = gInfo[0].anchor;
        gInfo[0].relpos.x = (-(sint32)((uint32)anchor->orientMat.m[6] << 2));
        gInfo[0].relpos.y = (-(sint32)((uint32)anchor->orientMat.m[7] << 2));
        gInfo[0].relpos.z = (-(sint32)((uint32)anchor->orientMat.m[8] << 2));
    }
    if ((bitMask & 2) != 0 && GameSetup::gData.commMode == 1)
    {
        BO::tNewtonObj *anchor = gInfo[1].anchor;
        gInfo[1].relpos.x = (-(sint32)((uint32)anchor->orientMat.m[6] << 2));
        gInfo[1].relpos.y = (-(sint32)((uint32)anchor->orientMat.m[7] << 2));
        gInfo[1].relpos.z = (-(sint32)((uint32)anchor->orientMat.m[8] << 2));
    }
}
