#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/clock.h"
#include "game/common/mathnfs.h"
#include "game/common/r3dcar.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"
#include "game/psx/drawc.h"
#include "game/psx/fe3dmenu.h"
#include "game/psx/texture.h"
#include "game/psx/trsproj.h"
#include "lib/psx/sinfunc.h"

sint32 gShowroomLights;
sint32 showRoomFlag;
sint32 gMenuRotate[2];
COORD16 Fe3D::spotVertex[33];
COORD16 Fe3D::lightsVertex[64];

void CameraLookAt(matrixtdef *mat, coorddef *camPoint)
{
    FUNCTION_MARKER(0x80012AE4, "FRONT.BIN");
    coorddef fwdVec;
    coorddef rightVec;
    coorddef upVec = { 0, 65536, 0 };
    fwdVec.x = (sint32)(0U - (uint32)camPoint->x);
    fwdVec.y = (sint32)(0U - (uint32)camPoint->y);
    fwdVec.z = (sint32)(0U - (uint32)camPoint->z);
    Math::NormalizeVector(&fwdVec);
    crossproduct(&upVec, &fwdVec, &rightVec);
    crossproduct(&fwdVec, &rightVec, &upVec);
    mat->m[0] = rightVec.x;
    mat->m[1] = rightVec.y;
    mat->m[2] = rightVec.z;
    mat->m[3] = upVec.x;
    mat->m[4] = upVec.y;
    mat->m[5] = upVec.z;
    mat->m[6] = fwdVec.x;
    mat->m[7] = fwdVec.y;
    mat->m[8] = fwdVec.z;
}

void Fe3D::InitShowroom(void)
{
    FUNCTION_MARKER(0x80012938, "FRONT.BIN");
    sint32 i;
    sint32 angle = 0;
    for (i = 0; i < 32; ++i, angle += 0x80)
    {
        spotVertex[i].x = (sint16)((sint32)((uint32)csin((sint32)angle) * 48U) >> 8);
        spotVertex[i].y = 0;
        spotVertex[i].z = (sint16)((sint32)((uint32)ccos((sint32)angle) * 48U) >> 8);
    }
    spotVertex[i].x = 0;
    spotVertex[i].y = 0;
    spotVertex[i].z = 0;

    angle = 0;
    for (i = 0; i < 32; ++i, angle += 0x20)
    {
        sint16 iPlus = (sint16)(i * 2);
        sint32 angle_sin = fastintsin((sint32)angle) >> 3;
        sint32 angle_cos = fastintcos((sint32)angle) >> 3;
        lightsVertex[iPlus].x = (sint16)(((uint32)angle_sin * 3U) >> 5);
        lightsVertex[iPlus].y = 0;
        lightsVertex[iPlus].z = (sint16)(((uint32)angle_cos * 3U) >> 5);
        lightsVertex[iPlus + 1].x = (sint16)((sint32)((uint32)angle_sin * 21U) >> 8);
        lightsVertex[iPlus + 1].y = 0;
        lightsVertex[iPlus + 1].z = (sint16)((sint32)((uint32)angle_cos * 21U) >> 8);
    }
}

void Draw::MenuRenderingView(Car::tObj *carObj, DRender::tView *Vi, sint32 posX, sint32 posY, sint32 player, uint32 rotation, sint32 camRot, float camY, float camZ, sint32 light, sint32 reflection)
{
    FUNCTION_MARKER(0x80012BF8, "FRONT.BIN");
    TrsProj::ResetTransPrecision();
    if (showRoomFlag != 0)
        posY = 140;
    TrsProj::SetMenuProjection((sint32)((uint32)posX - 256), (sint32)((uint32)posY - 128), 512, 256);
    sint32 showroom = showRoomFlag;
    gMenuRotate[player] = (sint32)((uint32)gMenuRotate[player] + rotation);
    matrixtdef temp;
    if (showroom != 0)
    {
        camY = psx_float_value(__mulsf3PSX(psx_float_bits(camY), 0x3f000000));
        coorddef targetPos;
        targetPos.x = 0;
        targetPos.y = __fixsfsi(psx_float_value(__mulsf3PSX(psx_float_bits(camY), 0x47800000)));
        targetPos.z = __fixsfsi(psx_float_value(__mulsf3PSX(psx_float_bits(camZ), 0x47800000)));
        matrixtdef temp1;
        CameraLookAt(&temp1, &targetPos);
        matrixtdef temp2;
        fixedxformy(&temp2, (sint32)((uint32)camRot << 6));
        Math::fasttransmult(&temp1, &temp2, &temp);
        uint64 z = psx_double_bits(__extendsfdf2(camZ));
        uint64 y = psx_double_bits(__extendsfdf2(camY));
        y = __divdf3PSX(y, psx_double_bits(2.5));
        z = __adddf3PSX(z, y);
        camZ = __truncdfsf2(psx_double_value(z));
        camY = 0;
    }
    else
        fixedxformy(&temp, (sint32)((uint32)camRot << 6));
    matrixtdef correct = { { 65536, 0, 0, 0, 0x8980, 0, 0, 0, 65536 } };
    Math::fasttransmult(&correct, &temp, &Vi->cview.mrotation);
    transpose(&Vi->cview.mrotation, &Vi->cview.mrotationInv);
    coorddef pos;
    pos.x = 0;
    pos.y = __fixsfsi(psx_float_value(__mulsf3PSX(psx_float_bits(camY), 0x47800000)));
    pos.z = __fixsfsi(psx_float_value(__mulsf3PSX(psx_float_bits(camZ), 0x47800000)));
    transform(&pos, &Vi->cview.mrotation, &Vi->cview.translation);
    pos.x = (sint32)(0U - (uint32)Vi->cview.translation.x);
    pos.y = (sint32)(0U - (uint32)Vi->cview.translation.y);
    pos.z = (sint32)(0U - (uint32)Vi->cview.translation.z);
    transform(&pos, &Vi->cview.mrotationInv, &Vi->cview.translationInv);
    carObj->N.orientMat.m[0] = 65536;
    carObj->N.orientMat.m[4] = 65536;
    carObj->N.orientMat.m[8] = 65536;
    carObj->N.orientMat.m[1] = 0;
    carObj->N.orientMat.m[2] = 0;
    carObj->N.orientMat.m[3] = 0;
    carObj->N.orientMat.m[5] = 0;
    carObj->N.orientMat.m[6] = 0;
    carObj->N.orientMat.m[7] = 0;
    carObj->N.position.x = 0;
    carObj->N.position.y = 0;
    carObj->N.position.z = 0;
    if (player == 0)
    {
        if (R3DCar::ClockLast < generic128HzClock)
        {
            sint32 ticks = generic128HzClock;
            R3DCar::Clock = 1;
            R3DCar::ClockLast = (sint32)((uint32)ticks + 5);
        }
        else
            R3DCar::Clock = 0;
    }
    Vi->player = player;
    Texture::LoadMenuTexture();
    R3DCar::InsertCarFacetMenu(carObj, Vi);
    R3DCar::InsertCarFacetMenuII(carObj, light);
    R3DCar::Showroom(Vi);
    if (reflection != 0)
    {
        R3DCar::InMenu |= 0x80;
        Draw::CarCache *sd = &DrawC::carCache;
        sd->head.mirror ^= 1;

        stackSpeedUpEnbabledFlag = 1;
        R3DCar::InsertCarFacetMenu(carObj, Vi);
        stackSpeedUpEnbabledFlag = 0;
        R3DCar::InsertCarFacetMenuII(carObj, light);
        R3DCar::InMenu &= 0x7f;
        sd->head.mirror ^= 1;
    }
}
