#include <limits.h>
#include <new>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/anim.h"
#include "game/common/group.h"
#include "game/common/mathnfs.h"
#include "game/common/quatern.h"
#include "game/common/sim.h"
#include "game/common/track.h"
#include "game/common/trk.h"
#include "game/psx/draww.h"
#include "game/psx/platform.h"
#include "lib/psx/blkmov.h"

typedef struct tQuat Quatern_quat;

Trk::AnimateInst *animScripts[10];
AnimScript *animSlots[32];
Trk::AnimateInst *Anim::gInstanceFromIndex[32];

void Anim::Restart(void)
{
    FUNCTION_MARKER(0x80073A94, "NFS4.EXE");
    for (sint32 i = 0; i < 32; ++i) {
        AnimScript *script = animSlots[i];
        if (script != 0) {
            if (script->inst != 0)
                purgememadr(script->inst);
            purgememadr(script);
        }
        animSlots[i] = 0;
    }
    DrawW::ResetAnimationTimer();
}

sint32 Anim::InitSystem(char *trackName)
{
    FUNCTION_MARKER(0x80073B1C, "NFS4.EXE");
    Restart();
    char fname[80];
    sprintf(fname, "%sA.viv", trackName);
    char *bigFile = static_cast<char *>(loadfileadrz(fname, 0));
    if (bigFile == 0) {
        for (sint32 i = 9; i >= 0; --i)
            animScripts[i] = 0;
    } else {
        sint32 size = filesize(fname);
        static char allocationName[] = "animScripts";
        char *mem = Platform::GetDCTBuffer(size, allocationName);
        blockmove(bigFile, mem, size);
        purgememadr(bigFile);
        (void)strstr(trackName, "Tr");
        for (sint32 i = 0; i < 10; ++i) {
            sprintf(fname, "tr00a%02d.can", i);
            animScripts[i] = reinterpret_cast<Trk::AnimateInst *>(locatebig(mem, fname));
        }
    }
    if (gPersistObjInst != 0) {
        sint32 numParts = gPersistObjInst->m_num_elements;
        Trk::AnimateInst *objInstance = reinterpret_cast<Trk::AnimateInst *>(gPersistObjInst + 1);
        numParts = (sint32)((uint32)numParts - 1);
        while (numParts != -1) {
            if ((objInstance->type == 3 || objInstance->type == 7) && objInstance->objectIndex != 0)
                gInstanceFromIndex[objInstance->objectIndex] = objInstance;
            sint16 size = objInstance->size;
            numParts = (sint32)((uint32)numParts - 1);
            objInstance = reinterpret_cast<Trk::AnimateInst *>(reinterpret_cast<uint8 *>(objInstance) + size);
        }
    }
    return 0;
}

void Anim::DeInitSystem(void)
{
    FUNCTION_MARKER(0x80073CB0, "NFS4.EXE");
    for (sint32 i = 31; i >= 0; --i)
        gInstanceFromIndex[i] = 0;
}

sint32 Anim::Handle(sint32 num)
{
    FUNCTION_MARKER(0x80073CE8, "NFS4.EXE");
    sint32 handle = 0;
    while (handle < 32 && animSlots[handle] != 0)
        ++handle;
    void *memory = reservememadr(0, sizeof(AnimScript), 0);
    AnimScript *script = new (memory) AnimScript(num);

    if (handle == 32)
        gInstanceFromIndex[0] = reinterpret_cast<Trk::AnimateInst *>(script);
    else
        animSlots[handle] = script;
    return handle;
}

sint32 Anim::FreeHandle(sint32 handle)
{
    FUNCTION_MARKER(0x80073D6C, "NFS4.EXE");
    AnimScript *script = handle == 32 ? reinterpret_cast<AnimScript *>(gInstanceFromIndex[0]) : animSlots[handle];
    if (script != 0) {
        if (script->inst != 0)
            purgememadr(script->inst);
        purgememadr(script);
    }
    if (handle == 32)
        gInstanceFromIndex[0] = 0;
    else
        animSlots[handle] = 0;
    return 0;
}

AnimScript * Anim::GetAnim(sint32 handle)
{
    FUNCTION_MARKER(0x80073DEC, "NFS4.EXE");
    return handle == 32 ? reinterpret_cast<AnimScript *>(gInstanceFromIndex[0]) : animSlots[handle];
}

void Anim::GetLastRotPos(Trk::AnimateInst *animInst, coorddef *pt, matrixtdef *mat)
{
    FUNCTION_MARKER(0x80073E08, "NFS4.EXE");
    tFrame *animFrames = reinterpret_cast<tFrame *>(animInst + 1);
    sint32 animInd = animInst->count - 1;
    tQuat q;
    memcpy(&q, reinterpret_cast<uint8 *>(&animFrames[animInd]) + offsetof(tFrame, qx), sizeof(q));
    sint32 x = animFrames[animInd].x;
    sint32 y = animFrames[animInd].y;
    sint32 z = animFrames[animInd].z;
    pt->x = x;
    pt->y = y;
    pt->z = z;
    Quatern::QuatToMat(&q, mat);
}

sint32 Anim::GetLastAnimPosRot(sint32 animNum, sint32 flags, coorddef *pt, matrixtdef *mat)
{
    FUNCTION_MARKER(0x80073E80, "NFS4.EXE");
    GetLastRotPos(animScripts[animNum], pt, mat);
    if (((uint32)flags & 4) != 0) {
        sint32 r3 = mat->m[3];
        sint32 r4 = mat->m[4];
        sint32 r5 = mat->m[5];
        sint32 r6 = mat->m[6];
        sint32 r7 = mat->m[7];
        sint32 r8 = mat->m[8];
        mat->m[3] = r6;
        mat->m[6] = (sint32)(0u - (uint32)r3);
        mat->m[4] = r7;
        mat->m[7] = (sint32)(0u - (uint32)r4);
        mat->m[5] = r8;
        mat->m[8] = (sint32)(0u - (uint32)r5);
    }
    return 1;
}

sint32 Anim::GetRotPos(Trk::AnimateInst *animInst, sint32 flags, sint32 ticks, coorddef *pt, matrixtdef *mat)
{
    FUNCTION_MARKER(0x80073F18, "NFS4.EXE");
    uint8 *frameBytes = reinterpret_cast<uint8 *>(animInst);
    if (animInst->type == 3 || animInst->type == 7)
        frameBytes += sizeof(Trk::AnimateInst);
    else
        frameBytes += sizeof(Trk::AnimateBoomInst);
    tFrame *animFrames = reinterpret_cast<tFrame *>(frameBytes);
    uint32 rawInterval = (uint16)animInst->interval;
    sint32 interval = rawInterval - 1 < 400 ? animInst->interval : 6;
    sint32 animInd0 = ticks / interval;
    if (((uint32)flags & 2) != 0 && animInd0 >= animInst->count - 1)
        return 0;
    sint32 remainder = ticks % interval;
    sint32 frameCount = animInst->count - 1;
    if (frameCount == 0 || (frameCount == -1 && animInd0 == INT_MIN))
        abort();
    animInd0 %= frameCount;
    sint32 weight = (sint32)((uint32)remainder << 16) / interval;
    sint32 animInd1 = (sint32)((uint32)animInd0 + 1);
    tQuat q0;
    tQuat q1;
    memcpy(&q0, reinterpret_cast<uint8 *>(&animFrames[animInd0]) + offsetof(tFrame, qx), sizeof(q0));
    memcpy(&q1, reinterpret_cast<uint8 *>(&animFrames[animInd1]) + offsetof(tFrame, qx), sizeof(q1));
    coorddef objcp0;
    objcp0.x = animFrames[animInd0].x;
    objcp0.y = animFrames[animInd0].y;
    objcp0.z = animFrames[animInd0].z;
    coorddef objcp1;
    objcp1.x = animFrames[animInd1].x;
    objcp1.y = animFrames[animInd1].y;
    objcp1.z = animFrames[animInd1].z;
    tQuat q;
    Quatern::Interpolate(&q0, &q1, &objcp0, &objcp1, weight, &q, pt);
    Quatern::QuatToMat(&q, mat);
    return 1;
}

sint32 Anim::GetPos(Trk::AnimateInst *animInst, sint32 flags, sint32 ticks, coorddef *pt, sint32 *animTicks, sint32 *animLength)
{
    FUNCTION_MARKER(0x8007412C, "NFS4.EXE");
    uint32 rawInterval = (uint16)animInst->interval;
    sint32 interval = rawInterval - 1 < 400 ? animInst->interval : 6;
    sint32 animInd0 = ticks / interval;
    if (((uint32)flags & 2) != 0 && animInd0 >= animInst->count - 1)
        return 0;
    sint32 remainder = ticks % interval;
    sint32 frameCount = animInst->count - 1;
    if (frameCount == 0 || (frameCount == -1 && animInd0 == INT_MIN))
        abort();
    animInd0 %= frameCount;
    sint32 weight = (sint32)((uint32)remainder << 16) / interval;
    tFrame *animFrames = reinterpret_cast<tFrame *>(animInst + 1);
    coorddef objcp0;
    objcp0.x = animFrames[animInd0].x;
    objcp0.y = animFrames[animInd0].y;
    objcp0.z = animFrames[animInd0].z;
    sint32 animInd1 = (sint32)((uint32)animInd0 + 1);
    coorddef objcp1;
    objcp1.x = animFrames[animInd1].x;
    objcp1.y = animFrames[animInd1].y;
    objcp1.z = animFrames[animInd1].z;
    Quatern::VecInterpolate(&objcp0, &objcp1, weight, pt);
    if (animTicks != 0) {
        sint32 length = (sint32)((uint32)(animInst->count - 1) * (uint32)interval);
        if (length == 0 || (length == -1 && ticks == INT_MIN))
            abort();
        *animTicks = ticks % length;
    }
    if (animLength != 0)
        *animLength = (sint32)((uint32)(animInst->count - 1) * (uint32)interval);
    return 1;
}

AnimScript::AnimScript(sint32 num)
{
    FUNCTION_MARKER(0x80074360, "NFS4.EXE");
    inst = static_cast<Trk::AnimateInst **>(reservememadr(0, sizeof(Trk::AnimateInst *), 0));
    *inst = animScripts[num];
    sint32 ticks = simGlobal.gameTicks;
    flags = 6;
    baseTicks = ticks;
}

AnimScript::AnimScript(sint32 num, sint32 numParts)
{
    FUNCTION_MARKER(0x800743CC, "NFS4.EXE");
    sint32 bytes = (sint32)((uint32)numParts << 2);
    inst = static_cast<Trk::AnimateInst **>(reservememadr(0, bytes, 0));
    for (sint32 i = 0; i < numParts; ++i) {
        sint32 scriptIndex = (sint32)((uint32)num + (uint32)i);
        inst[i] = animScripts[scriptIndex];
    }
    sint32 ticks = simGlobal.gameTicks;
    flags = 6;
    baseTicks = ticks;
}

AnimScript::AnimScript(Group *instanceGroup, sint32 type, sint32 boomIndex, sint32 numParts)
{
    FUNCTION_MARKER(0x80074468, "NFS4.EXE");
    Trk::AnimateBoomInst *objInstance = reinterpret_cast<Trk::AnimateBoomInst *>(instanceGroup + 1);
    sint32 numElems = instanceGroup->m_num_elements;
    sint32 bytes = (sint32)((uint32)numParts << 2);
    inst = static_cast<Trk::AnimateInst **>(reservememadr(0, bytes, 0));
    sint32 i = 0;
    numElems = (sint32)((uint32)numElems - 1);
    while (numElems != -1) {
        if (objInstance->type == type && objInstance->boomIndex == boomIndex) {
            inst[i] = reinterpret_cast<Trk::AnimateInst *>(objInstance);
            i = (sint32)((uint32)i + 1);
        }
        sint16 size = objInstance->size;
        numElems = (sint32)((uint32)numElems - 1);
        objInstance = reinterpret_cast<Trk::AnimateBoomInst *>(reinterpret_cast<uint8 *>(objInstance) + size);
    }
    sint32 ticks = simGlobal.gameTicks;
    flags = 6;
    baseTicks = ticks;
}

void AnimScript::SetAnimAttrib(sint32 flags)
{
    FUNCTION_MARKER(0x80074534, "NFS4.EXE");
    this->flags = flags;
}

void AnimScript::GetAnimFrameInfo(sint32 *frame, sint32 *numFrames)
{
    FUNCTION_MARKER(0x8007453C, "NFS4.EXE");
    Trk::AnimateInst *animInst = *inst;
    sint32 ticks = (sint32)((uint32)simGlobal.gameTicks - (uint32)baseTicks);
    uint32 rawInterval = (uint16)animInst->interval;
    sint32 interval = rawInterval - 1 < 400 ? animInst->interval : 6;
    sint32 quotient = ticks / interval;
    sint32 count = animInst->count - 1;
    if (count == 0 || (count == -1 && quotient == INT_MIN))
        abort();
    *frame = quotient % count;
    *numFrames = animInst->count - 1;
}

sint32 AnimScript::GetTimedAnimPosRot(coorddef *pt, matrixtdef *mat)
{
    FUNCTION_MARKER(0x800745F8, "NFS4.EXE");
    return GetTimedAnimPosRot(0, pt, mat);
}

sint32 AnimScript::GetTimedAnimPosRot(sint32 index, coorddef *pt, matrixtdef *mat)
{
    FUNCTION_MARKER(0x80074624, "NFS4.EXE");
    if (GetStatus() != 1)
        return -1;
    sint32 ticks = (sint32)((uint32)simGlobal.gameTicks - (uint32)baseTicks);
    if (Anim::GetRotPos(inst[index], flags, ticks, pt, mat) == 0) {
        Trk::AnimateInst **parts = inst;
        baseTicks = -1;
        parts[index] = 0;
        return 0;
    }
    if (((uint32)flags & 4) != 0) {
        sint32 r3 = mat->m[3];
        sint32 r4 = mat->m[4];
        sint32 r5 = mat->m[5];
        sint32 r6 = mat->m[6];
        sint32 r7 = mat->m[7];
        sint32 r8 = mat->m[8];
        mat->m[3] = r6;
        mat->m[6] = (sint32)(0u - (uint32)r3);
        mat->m[4] = r7;
        mat->m[7] = (sint32)(0u - (uint32)r4);
        mat->m[5] = r8;
        mat->m[8] = (sint32)(0u - (uint32)r5);
    }
    return 1;
}

sint32 AnimScript::GetStatus(void)
{
    FUNCTION_MARKER(0x8007471C, "NFS4.EXE");
    return *inst != 0;
}
