#ifndef ANIM_H
#define ANIM_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/group.h"
#include "game/common/matrixtdef.h"
#include "game/common/trk.h"

class AnimScript

{
public:
    AnimScript(sint32 num);

    AnimScript(sint32 num, sint32 numParts);

    AnimScript(Group *instanceGroup, sint32 type, sint32 boomIndex, sint32 numParts);

    void SetAnimAttrib(sint32 flags);

    void GetAnimFrameInfo(sint32 *frame, sint32 *numFrames);

    sint32 GetTimedAnimPosRot(coorddef *pt, matrixtdef *mat);

    sint32 GetTimedAnimPosRot(sint32 index, coorddef *pt, matrixtdef *mat);

    sint32 GetStatus(void);

    sint32 baseTicks, baseFrame, flags, numParts;

    Trk::AnimateInst **inst;
};

class Anim
{
public:
    static Trk::AnimateInst *gInstanceFromIndex[32];

    struct tFrame
    {
        sint32 x;
        sint32 y;
        sint32 z;
        sint16 qx;
        sint16 qy;
        sint16 qz;
        sint16 qw;
    };

    static void Restart(void);
    static sint32 InitSystem(char *trackName);
    static void DeInitSystem(void);
    static sint32 Handle(sint32 num);
    static sint32 FreeHandle(sint32 handle);
    static AnimScript * GetAnim(sint32 handle);
    static void GetLastRotPos(Trk::AnimateInst *animInst, coorddef *pt, matrixtdef *mat);
    static sint32 GetLastAnimPosRot(sint32 animNum, sint32 flags, coorddef *pt, matrixtdef *mat);
    static sint32 GetRotPos(Trk::AnimateInst *animInst, sint32 flags, sint32 ticks, coorddef *pt, matrixtdef *mat);
    static sint32 GetPos(Trk::AnimateInst *animInst, sint32 flags, sint32 ticks, coorddef *pt, sint32 *animTicks, sint32 *animLength);
};

#endif
