#ifndef CAMERA_H
#define CAMERA_H

#include "xport.h"
#include "game/common/bworldsm.h"
#include "game/common/matrixtdef.h"
#include "game/common/renderview.h"

struct camera_info;

struct camera_flags;

class Camera
{
public:

    typedef camera_info tInfo;

    typedef camera_flags tFlags;
    static tInfo gInfo[2];
    static tFlags gFlags[19];

    static sint32 gGeomScreen;
    static sint32 gCopDist[6];

    struct tCamSlot;

    static void SetCamera(DRender::tView *cview);
    static void LookBack(matrixtdef *src, matrixtdef *tgt);
    static void TunnelLimit(sint32 player, sint32 *armheight);
    static void UpdateCollisionCam(sint32 player);
    static void UpdateSimpleCam(sint32 player);
    static void UpdateBumperCam(sint32 player);
    static void UpdateTailCam(sint32 player, sint32 behavior);
    static void UpdateHeliCam(sint32 player, sint32 behavior);
    static void UpdateCircleCam(sint32 player);
    static void UpdateTVCam(sint32 player);
    static void UpdateAnimCam(sint32 player);
    static void UpdateFinishCam(sint32 player);
    static void UpdateBlimpCam(sint32 player);
    static void SetSplineCam(sint32 player);
    static void UpdateSplineCam(sint32 player);
    static sint32 IslandProfile(uint16 before);
    static void UpdatePulloverCam(sint32 player);
    static void UpdateCopCam1(sint32 player);
    static void UpdateCopCam2(sint32 player);
    static void UpdateBTCopCam(sint32 player);
    static void Update(void);
    static void Init(void);
    static void Kill(void);
    static void PitchAndRoll(sint32 player);
    static sint32 TooSteep(sint32 player, BWorldSm::Pos *slicePos);
    static void CheckWallCollisions(sint32 player, coorddef *pos);
    static void SetAboveGround(sint32 player, coorddef *pos);
    static void AcquireTarget(sint32 player, coorddef *point, coorddef *pos, matrixtdef *rot, sint32 hirez);
    static void OpponentLookBehind(sint32 player, coorddef *pos, sint32 reset);
    static void GetViewInfo(sint32 cviewP, DRender::tCalcView *cview, sint32 viewID);
    static void GetAudioViewInfo(sint32 cviewP, DRender::tCalcView *cview, coorddef **cvel);
    static sint32 GetMode(sint32 cviewP);
    static void SetMode(sint32 cviewP, sint32 mode);
    static void NextMode(sint32 cviewP);
    static void ReplayUpdate(sint32 cviewP, tCamSlot *ptr);
    static void ResetRelPos(sint32 bitMask);
};

#endif
