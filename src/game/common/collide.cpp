#include <stddef.h>
#include "xport.h"
#include "game/common/bo.h"
#include "game/common/bworldsm.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/collide.h"
#include "game/common/mathnfs.h"
#include "game/common/newton.h"
#include "game/common/object.h"
#include "game/common/physics.h"
#include "game/psx/force.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/fixdsqrt.h"

static sint32 xRange;
static sint32 yRange;
static sint32 zRange;
sint32 findClosestSideDave;
sint32 frictionLess;
BO::tNewtonObj *obj0;
BO::tNewtonObj *obj1;
coorddef *pNormal;
coorddef *pP;
static sint32 basisDots[9];
static sint32 scaledBasisDots[9];
static sint32 scaledBasisDotsOther[9];
static sint32 basisDotRelative[6];
sint32 Collide::gRaiseUp = 1;
sint32 Collide::gNumRegistered;
BO::tNewtonObj *Collide::gRegistry[16];

bool CollisionHumanFlag(const BO::tNewtonObj *obj)
{
    for (sint32 i = 0; i < 5; ++i) {
        if (obj == &InfiniteMassNewton[i]) {
            if (i < 4)
                return (InfiniteMassNewton[i + 1].simRoadInfo.quadPts[1].y & 4) != 0;

            return false;
        }
    }
    return (reinterpret_cast<const Car::tObj *>(obj)->carFlags & 4) != 0;
}

bool CollisionCopFlag(const BO::tNewtonObj *obj)
{
    for (sint32 i = 0; i < 5; ++i) {
        if (obj == &InfiniteMassNewton[i]) {
            if (i < 4)
                return (InfiniteMassNewton[i + 1].simRoadInfo.quadPts[1].y & 0x20) != 0;

            return false;
        }
    }
    return (reinterpret_cast<const Car::tObj *>(obj)->carFlags & 0x20) != 0;
}

sint32 CHECK_CENTER_VERTEX_WITH_DIRS(sint32 X_DIR, sint32 Y_DIR, sint32 Z_DIR)
{
    FUNCTION_MARKER(0x8008BA70, "NFS4.EXE");
    xRange = ((((X_DIR * scaledBasisDots[0]) + (Y_DIR * scaledBasisDots[1])) + (Z_DIR * scaledBasisDots[2])) + basisDotRelative[0]);
    yRange = ((((X_DIR * scaledBasisDots[3]) + (Y_DIR * scaledBasisDots[4])) + (Z_DIR * scaledBasisDots[5])) + basisDotRelative[1]);
    zRange = ((((X_DIR * scaledBasisDots[6]) + (Y_DIR * scaledBasisDots[7])) + (Z_DIR * scaledBasisDots[8])) + basisDotRelative[2]);
    BO::tNewtonObj *bounds = obj0;
    sint32 dimension = bounds->dimension.z;
    if (zRange > dimension || zRange < -dimension)
        return 0;
    dimension = bounds->dimension.x;
    if (xRange > dimension || xRange < -dimension)
        return 0;
    dimension = bounds->dimension.y;
    if (yRange > dimension || yRange < -dimension)
        return 0;
    BO::tNewtonObj *vertex = obj1;
    pNormal->x = (((X_DIR * vertex->orientMat.m[0]) + (Y_DIR * vertex->orientMat.m[3])) + (Z_DIR * vertex->orientMat.m[6]));
    pNormal->y = (((X_DIR * vertex->orientMat.m[1]) + (Y_DIR * vertex->orientMat.m[4])) + (Z_DIR * vertex->orientMat.m[7]));
    pNormal->z = (((X_DIR * vertex->orientMat.m[2]) + (Y_DIR * vertex->orientMat.m[5])) + (Z_DIR * vertex->orientMat.m[8]));
    pP->x = (((vertex->position.x + ((X_DIR * (vertex->orientMat.m[0] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[3] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[6] / 256)) * (vertex->dimension.z / 256)));
    pP->y = (((vertex->position.y + ((X_DIR * (vertex->orientMat.m[1] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[4] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[7] / 256)) * (vertex->dimension.z / 256)));
    pP->z = (((vertex->position.z + ((X_DIR * (vertex->orientMat.m[2] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[5] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[8] / 256)) * (vertex->dimension.z / 256)));
    return 1;
}

sint32 CHECK_CENTER_VERTEX_WITH_DIRS_OTHER(sint32 X_DIR, sint32 Y_DIR, sint32 Z_DIR)
{
    FUNCTION_MARKER(0x8008BEF0, "NFS4.EXE");
    xRange = ((((X_DIR * scaledBasisDotsOther[0]) + (Y_DIR * scaledBasisDotsOther[3])) + (Z_DIR * scaledBasisDotsOther[6])) - basisDotRelative[3]);
    yRange = ((((X_DIR * scaledBasisDotsOther[1]) + (Y_DIR * scaledBasisDotsOther[4])) + (Z_DIR * scaledBasisDotsOther[7])) - basisDotRelative[4]);
    zRange = ((((X_DIR * scaledBasisDotsOther[2]) + (Y_DIR * scaledBasisDotsOther[5])) + (Z_DIR * scaledBasisDotsOther[8])) - basisDotRelative[5]);
    BO::tNewtonObj *bounds = obj1;
    sint32 dimension = bounds->dimension.z;
    if (zRange > dimension || zRange < -dimension)
        return 0;
    dimension = bounds->dimension.x;
    if (xRange > dimension || xRange < -dimension)
        return 0;
    dimension = bounds->dimension.y;
    if (yRange > dimension || yRange < -dimension)
        return 0;
    BO::tNewtonObj *vertex = obj0;
    pNormal->x = (-(((X_DIR * vertex->orientMat.m[0]) + (Y_DIR * vertex->orientMat.m[3])) + (Z_DIR * vertex->orientMat.m[6])));
    pNormal->y = (-(((X_DIR * vertex->orientMat.m[1]) + (Y_DIR * vertex->orientMat.m[4])) + (Z_DIR * vertex->orientMat.m[7])));
    pNormal->z = (-(((X_DIR * vertex->orientMat.m[2]) + (Y_DIR * vertex->orientMat.m[5])) + (Z_DIR * vertex->orientMat.m[8])));
    pP->x = (((vertex->position.x + ((X_DIR * (vertex->orientMat.m[0] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[3] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[6] / 256)) * (vertex->dimension.z / 256)));
    pP->y = (((vertex->position.y + ((X_DIR * (vertex->orientMat.m[1] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[4] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[7] / 256)) * (vertex->dimension.z / 256)));
    pP->z = (((vertex->position.z + ((X_DIR * (vertex->orientMat.m[2] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[5] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[8] / 256)) * (vertex->dimension.z / 256)));
    return 1;
}

sint32 CHECK_VERTEX_WITH_DIRS(sint32 X_DIR, sint32 Y_DIR, sint32 Z_DIR)
{
    FUNCTION_MARKER(0x8008C37C, "NFS4.EXE");
    xRange = ((((X_DIR * scaledBasisDots[0]) + (Y_DIR * scaledBasisDots[1])) + (Z_DIR * scaledBasisDots[2])) + basisDotRelative[0]);
    yRange = ((((X_DIR * scaledBasisDots[3]) + (Y_DIR * scaledBasisDots[4])) + (Z_DIR * scaledBasisDots[5])) + basisDotRelative[1]);
    zRange = ((((X_DIR * scaledBasisDots[6]) + (Y_DIR * scaledBasisDots[7])) + (Z_DIR * scaledBasisDots[8])) + basisDotRelative[2]);
    BO::tNewtonObj *bounds = obj0;
    sint32 dimension = bounds->dimension.z;
    if (zRange > dimension || zRange < -dimension)
        return 0;
    dimension = bounds->dimension.x;
    if (xRange > dimension || xRange < -dimension)
        return 0;
    dimension = bounds->dimension.y;
    if (yRange > dimension || yRange < -dimension)
        return 0;
    BO::tNewtonObj *vertex = obj1;
    pP->x = (((vertex->position.x + ((X_DIR * (vertex->orientMat.m[0] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[3] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[6] / 256)) * (vertex->dimension.z / 256)));
    pP->y = (((vertex->position.y + ((X_DIR * (vertex->orientMat.m[1] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[4] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[7] / 256)) * (vertex->dimension.z / 256)));
    pP->z = (((vertex->position.z + ((X_DIR * (vertex->orientMat.m[2] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[5] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[8] / 256)) * (vertex->dimension.z / 256)));
    return 1;
}

sint32 CHECK_VERTEX_WITH_DIRS_OTHER(sint32 X_DIR, sint32 Y_DIR, sint32 Z_DIR)
{
    FUNCTION_MARKER(0x8008C744, "NFS4.EXE");
    xRange = ((((X_DIR * scaledBasisDotsOther[0]) + (Y_DIR * scaledBasisDotsOther[3])) + (Z_DIR * scaledBasisDotsOther[6])) - basisDotRelative[3]);
    yRange = ((((X_DIR * scaledBasisDotsOther[1]) + (Y_DIR * scaledBasisDotsOther[4])) + (Z_DIR * scaledBasisDotsOther[7])) - basisDotRelative[4]);
    zRange = ((((X_DIR * scaledBasisDotsOther[2]) + (Y_DIR * scaledBasisDotsOther[5])) + (Z_DIR * scaledBasisDotsOther[8])) - basisDotRelative[5]);
    BO::tNewtonObj *bounds = obj1;
    sint32 dimension = bounds->dimension.z;
    if (zRange > dimension || zRange < -dimension)
        return 0;
    dimension = bounds->dimension.x;
    if (xRange > dimension || xRange < -dimension)
        return 0;
    dimension = bounds->dimension.y;
    if (yRange > dimension || yRange < -dimension)
        return 0;
    BO::tNewtonObj *vertex = obj0;
    pP->x = (((vertex->position.x + ((X_DIR * (vertex->orientMat.m[0] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[3] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[6] / 256)) * (vertex->dimension.z / 256)));
    pP->y = (((vertex->position.y + ((X_DIR * (vertex->orientMat.m[1] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[4] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[7] / 256)) * (vertex->dimension.z / 256)));
    pP->z = (((vertex->position.z + ((X_DIR * (vertex->orientMat.m[2] / 256)) * (vertex->dimension.x / 256))) + ((Y_DIR * (vertex->orientMat.m[5] / 256)) * (vertex->dimension.y / 256))) + ((Z_DIR * (vertex->orientMat.m[8] / 256)) * (vertex->dimension.z / 256)));
    return 1;
}

void Collide::DoObjectFixedObjectCollision(BO::tNewtonObj *o, coorddef *p, coorddef *v, coorddef *n)
{
    FUNCTION_MARKER(0x8008CB0C, "NFS4.EXE");
    coorddef velocity = *v;
    coorddef normal = *n;
    sint32 velocityLength = 0;
    uint32 x = (normal.x / 256) * (uint32)(o->linearVel.x / 256);
    uint32 y = (normal.y / 256) * (uint32)(o->linearVel.y / 256);
    uint32 z = (normal.z / 256) * (uint32)(o->linearVel.z / 256);
    sint32 temp0 = (x + y + z);
    coorddef r;
    r.x = (p->x - o->position.x);
    r.y = (p->y - o->position.y);
    r.z = (p->z - o->position.z);
    coorddef RCrossN;
    RCrossN.x = ((((r.y / 256)) * (normal.z / 256)) - (((r.z / 256)) * (normal.y / 256)));
    RCrossN.y = ((((r.z / 256)) * (normal.x / 256)) - (((r.x / 256)) * (normal.z / 256)));
    RCrossN.z = ((((r.x / 256)) * (normal.y / 256)) - (((r.y / 256)) * (normal.x / 256)));
    x = (o->angularVel.x / 256) * (uint32)(RCrossN.x / 256);
    y = (o->angularVel.y / 256) * (uint32)(RCrossN.y / 256);
    z = (o->angularVel.z / 256) * (uint32)(RCrossN.z / 256);
    sint32 temp1 = (x + y + z);
    sint32 temp2 = o->massInv / 4;
    sint32 inertiaTerm = Math::VectorLength2(&RCrossN);
    inertiaTerm = fixedmult(inertiaTerm, ((uint32)o->moInertiaInv << 1));
    sint32 numerator = (-(temp0 + temp1));
    sint32 denominator = (temp2 + (inertiaTerm / 2));
    sint32 impulse = fixedmult(fixeddiv(numerator, denominator), 0x6666);
    sint32 normalSpeed = temp0;
    if (normalSpeed < 0)
        normalSpeed = (-normalSpeed);
    o->collision.impulse = ((uint32)normalSpeed << 2);
    sint32 surface = o->driveSurfaceType;
    o->collision.otherObj = 0;
    o->collision.sfxType = surface | 0x30000;
    coorddef contactPoint = *p;
    o->collision.collisionPoint = contactPoint;
    if (velocity.x != 0 || velocity.y != 0 || velocity.z != 0) {
        x = (velocity.x / 256) * (uint32)(normal.x / 256);
        y = (velocity.y / 256) * (uint32)(normal.y / 256);
        z = (velocity.z / 256) * (uint32)(normal.z / 256);
        sint32 projection = (sint32)(x + y + z) / 256;
        velocity.x = (velocity.x - (projection * (normal.x / 256)));
        velocity.y = (velocity.y - (projection * (normal.y / 256)));
        velocity.z = (velocity.z - (projection * (normal.z / 256)));
        x = (velocity.x / 256) * (uint32)(velocity.x / 256);
        y = (velocity.y / 256) * (uint32)(velocity.y / 256);
        z = (velocity.z / 256) * (uint32)(velocity.z / 256);
        velocityLength = fixedsqrt((x + y + z));
        sint32 scale = -0x10000;
        if (velocityLength / 2 != 0)
            scale = (-fixeddiv(0x8000, velocityLength / 2));
        velocity.x = fixedmult(scale, velocity.x);
        velocity.y = fixedmult(scale, velocity.y);
        velocity.z = fixedmult(scale, velocity.z);
    }
    sint32 speed = o->speedXZ;
    if (speed < 0)
        speed = (-speed);
    if (speed < 0xA0000) {
        static const coorddef upVec = { 0, 0x10000, 0 };
        x = (upVec.x / 256) * (uint32)(o->orientMat.m[0] / 256);
        y = (upVec.y / 256) * (uint32)(o->orientMat.m[1] / 256);
        z = (upVec.z / 256) * (uint32)(o->orientMat.m[2] / 256);
        sint32 dotx = (x + y + z);
        x = (upVec.x / 256) * (uint32)(o->orientMat.m[3] / 256);
        y = (upVec.y / 256) * (uint32)(o->orientMat.m[4] / 256);
        z = (upVec.z / 256) * (uint32)(o->orientMat.m[5] / 256);
        sint32 doty = (x + y + z);
        x = (upVec.x / 256) * (uint32)(o->orientMat.m[6] / 256);
        y = (upVec.y / 256) * (uint32)(o->orientMat.m[7] / 256);
        z = (upVec.z / 256) * (uint32)(o->orientMat.m[8] / 256);
        sint32 dotz = (x + y + z);
        if (dotx < 0)
            dotx = (-dotx);
        if (dotz < 0)
            dotz = (-dotz);
        if (dotx > 0xF5C2 || dotz > 0xF5C2 || doty < -0xF5C2)
            frictionLess = 1;
    }
    sint32 frictionalImpulse = fixedmult(frictionLess == 1 ? 0x28F : 0x8000, impulse);
    coorddef deltaVFromFriction;
    deltaVFromFriction.x = fixedmult(fixedmult(frictionalImpulse, o->massInv / 2), velocity.x);
    deltaVFromFriction.y = fixedmult(fixedmult(frictionalImpulse, o->massInv / 2), velocity.y);
    deltaVFromFriction.z = fixedmult(fixedmult(frictionalImpulse, o->massInv / 2), velocity.z);
    sint32 inverseMoment = ((uint32)o->moInertiaInv << 1);
    frictionLess = 0;
    sint32 frictionalImpulseOverMoment = fixedmult(frictionalImpulse, inverseMoment);
    coorddef temp;
    temp.x = ((((velocity.y / 256)) * (r.z / 256)) - (((velocity.z / 256)) * (r.y / 256)));
    temp.y = ((((velocity.z / 256)) * (r.x / 256)) - (((velocity.x / 256)) * (r.z / 256)));
    temp.z = ((((velocity.x / 256)) * (r.y / 256)) - (((velocity.y / 256)) * (r.x / 256)));

    temp.x = ((((temp.y / 256)) * (r.z / 256)) - (((temp.z / 256)) * (r.y / 256)));
    temp.y = ((((temp.z / 256)) * (r.x / 256)) - (((temp.x / 256)) * (r.z / 256)));
    temp.z = ((((temp.x / 256)) * (r.y / 256)) - (((temp.y / 256)) * (r.x / 256)));
    temp.x = fixedmult(frictionalImpulseOverMoment, temp.x);
    temp.y = fixedmult(frictionalImpulseOverMoment, temp.y);
    temp.z = fixedmult(frictionalImpulseOverMoment, temp.z);
    deltaVFromFriction.x = (deltaVFromFriction.x + temp.x);
    deltaVFromFriction.y = (deltaVFromFriction.y + temp.y);
    deltaVFromFriction.z = (deltaVFromFriction.z + temp.z);
    x = (deltaVFromFriction.x / 256) * (uint32)(velocity.x / 256);
    y = (deltaVFromFriction.y / 256) * (uint32)(velocity.y / 256);
    z = (deltaVFromFriction.z / 256) * (uint32)(velocity.z / 256);
    sint32 deltaSpeedInFrictionDirection = (x + y + z);
    if (velocityLength < deltaSpeedInFrictionDirection)
        frictionalImpulse = fixedmult(frictionalImpulse, fixeddiv(velocityLength, deltaSpeedInFrictionDirection));
    coorddef frictionImpulse;
    frictionImpulse.x = fixedmult(frictionalImpulse, velocity.x);
    frictionImpulse.y = fixedmult(frictionalImpulse, velocity.y);
    frictionImpulse.z = fixedmult(frictionalImpulse, velocity.z);
    if (impulse > 0) {
        coorddef impulseV;
        impulseV.x = fixedmult(impulse, normal.x);
        impulseV.y = fixedmult(impulse, normal.y);
        impulseV.z = fixedmult(impulse, normal.z);
        impulseV.x = (impulseV.x + frictionImpulse.x);
        impulseV.y = (impulseV.y + frictionImpulse.y);
        impulseV.z = (impulseV.z + frictionImpulse.z);
        coorddef deltaV;
        deltaV.x = fixedmult(o->massInv / 2, impulseV.x);
        deltaV.y = fixedmult(o->massInv / 2, impulseV.y);
        deltaV.z = fixedmult(o->massInv / 2, impulseV.z);
        o->linearVel.x = (o->linearVel.x + deltaV.x);
        o->linearVel.y = (o->linearVel.y + deltaV.y);
        o->linearVel.z = (o->linearVel.z + deltaV.z);
        RCrossN.x = ((((r.y / 256)) * (impulseV.z / 256)) - (((r.z / 256)) * (impulseV.y / 256)));
        RCrossN.y = ((((r.z / 256)) * (impulseV.x / 256)) - (((r.x / 256)) * (impulseV.z / 256)));
        RCrossN.z = ((((r.x / 256)) * (impulseV.y / 256)) - (((r.y / 256)) * (impulseV.x / 256)));
        deltaV.x = fixedmult(((uint32)o->moInertiaInv << 1), RCrossN.x);
        deltaV.y = fixedmult(((uint32)o->moInertiaInv << 1), RCrossN.y);
        deltaV.z = fixedmult(((uint32)o->moInertiaInv << 1), RCrossN.z);
        o->angularVel.x = (o->angularVel.x + deltaV.x);
        o->angularVel.y = (o->angularVel.y + deltaV.y);
        o->angularVel.z = (o->angularVel.z + deltaV.z);
    }
}

void Collide::LimitAngularVel(BO::tNewtonObj *o)
{
    FUNCTION_MARKER(0x8008D8B4, "NFS4.EXE");
    sint32 x = o->angularVel.x;
    if (x < -0x18000)
        x = -0x18000;
    else if (x > 0x18000)
        x = 0x18000;
    o->angularVel.x = x;
    sint32 y = o->angularVel.y;
    if (y < -0x18000)
        y = -0x18000;
    else if (y > 0x18000)
        y = 0x18000;
    o->angularVel.y = y;
    sint32 z = o->angularVel.z;
    if (z < -0x18000)
        z = -0x18000;
    else if (z > 0x18000)
        z = 0x18000;
    o->angularVel.z = z;
}

void Collide::TestWithPlane(BO::tNewtonObj *o, coorddef *normal, coorddef *samplePoint)
{
    FUNCTION_MARKER(0x8008D9A8, "NFS4.EXE");
    o->angularVel.x = fixedmult(0x6487E, o->angularVel.x);
    o->angularVel.y = fixedmult(0x6487E, o->angularVel.y);
    o->angularVel.z = fixedmult(0x6487E, o->angularVel.z);
    if (normal->x == 0 && normal->y == 0 && normal->z == 0)
        normal->y = 0x10000;
    sint32 basisDots[3];
    for (sint32 row = 0; row < 3; ++row) {
        uint32 x = (normal->x / 256) * (uint32)(o->orientMat.m[row * 3] / 256);
        uint32 y = (normal->y / 256) * (uint32)(o->orientMat.m[row * 3 + 1] / 256);
        uint32 z = (normal->z / 256) * (uint32)(o->orientMat.m[row * 3 + 2] / 256);
        sint32 dot = (x + y + z);
        sint32 dimension = row == 0 ? o->dimension.x : (row == 1 ? o->dimension.y : o->dimension.z);
        basisDots[row] = (((dot / 256)) * (dimension / 256));
    }
    sint32 xDir = basisDots[0] < 0 ? 1 : -1;
    sint32 Y_DIR = basisDots[1] < 0 ? 1 : -1;
    sint32 zDir = basisDots[2] < 0 ? 1 : -1;
    coorddef relativePos;
    relativePos.x = (o->position.x - samplePoint->x);
    relativePos.y = (o->position.y - samplePoint->y);
    relativePos.z = (o->position.z - samplePoint->z);
    uint32 dotX = (normal->x / 256) * (uint32)(relativePos.x / 256);
    uint32 dotY = (normal->y / 256) * (uint32)(relativePos.y / 256);
    uint32 dotZ = (normal->z / 256) * (uint32)(relativePos.z / 256);
    sint32 relativeDot = (dotX + dotY + dotZ);
    sint32 height = ((((xDir * basisDots[0]) + (Y_DIR * basisDots[1])) + (zDir * basisDots[2])) + relativeDot);
    if (height < 0) {
        coorddef spot;
        uint32 x = xDir * (uint32)(o->orientMat.m[0] / 256) * (o->dimension.x / 256);
        uint32 y = Y_DIR * (uint32)(o->orientMat.m[3] / 256) * (o->dimension.y / 256);
        uint32 z = zDir * (uint32)(o->orientMat.m[6] / 256) * (o->dimension.z / 256);
        spot.x = (o->position.x + x + y + z);
        x = xDir * (uint32)(o->orientMat.m[1] / 256) * (o->dimension.x / 256);
        y = Y_DIR * (uint32)(o->orientMat.m[4] / 256) * (o->dimension.y / 256);
        z = zDir * (uint32)(o->orientMat.m[7] / 256) * (o->dimension.z / 256);
        spot.y = (o->position.y + x + y + z);
        x = xDir * (uint32)(o->orientMat.m[2] / 256) * (o->dimension.x / 256);
        y = Y_DIR * (uint32)(o->orientMat.m[5] / 256) * (o->dimension.y / 256);
        z = zDir * (uint32)(o->orientMat.m[8] / 256) * (o->dimension.z / 256);
        spot.z = (o->position.z + x + y + z);
        sint32 raiseUp = (-height);
        if (raiseUp < 0)
            raiseUp = 0;
        coorddef Raise = { 0, 0, 0 };
        if (gRaiseUp != 0 && raiseUp != 0) {
            sint32 halfRaise = raiseUp / 2;
            Raise.x = fixedmult(halfRaise, normal->x);
            Raise.y = fixedmult(halfRaise, normal->y);
            Raise.z = fixedmult(halfRaise, normal->z);
            o->position.x = (o->position.x + Raise.x);
            o->position.y = (o->position.y + Raise.y);
            o->position.z = (o->position.z + Raise.z);
        }
        coorddef r;
        r.x = (spot.x - o->position.x);
        r.y = (spot.y - o->position.y);
        r.z = (spot.z - o->position.z);
        coorddef vertexVelocity;
        vertexVelocity.x = (((((o->angularVel.y / 256)) * (r.z / 256)) - (((o->angularVel.z / 256)) * (r.y / 256))) + o->linearVel.x);
        vertexVelocity.y = (((((o->angularVel.z / 256)) * (r.x / 256)) - (((o->angularVel.x / 256)) * (r.z / 256))) + o->linearVel.y);
        vertexVelocity.z = (((((o->angularVel.x / 256)) * (r.y / 256)) - (((o->angularVel.y / 256)) * (r.x / 256))) + o->linearVel.z);
        dotX = (normal->x / 256) * (uint32)(vertexVelocity.x / 256);
        dotY = (normal->y / 256) * (uint32)(vertexVelocity.y / 256);
        dotZ = (normal->z / 256) * (uint32)(vertexVelocity.z / 256);
        if ((sint32)(dotX + dotY + dotZ) < 0)
            DoObjectFixedObjectCollision(o, &spot, &vertexVelocity, normal);
        if (gRaiseUp != 0 && raiseUp != 0 && Raise.y > 0) {
            sint32 correction = ((uint32)fixedmult(0x9CCCC, Raise.y) << 1);
            sint32 v2 = fixedmult(o->linearVel.y, o->linearVel.y);
            if (v2 < correction) {
                o->linearVel.y = 0;
            } else {
                sint32 speed = fixedsqrt(((v2 - correction)));
                if (o->linearVel.y > 0)
                    o->linearVel.y = speed;
                else
                    o->linearVel.y = (-speed);
            }
        }
    }
    o->angularVel.x = fixedmult(0x28BE, o->angularVel.x);
    o->angularVel.y = fixedmult(0x28BE, o->angularVel.y);
    o->angularVel.z = fixedmult(0x28BE, o->angularVel.z);
    LimitAngularVel(o);
    sint32 first = fixedmult(normal->x, o->linearVel.x);
    sint32 second = fixedmult(normal->y, o->linearVel.y);
    sint32 third = fixedmult(normal->z, o->linearVel.z);
    sint32 dot = ((first + second) + third);
    if (dot <= 0)
        dot = (-dot);
    sint32 impulse = ((uint32)dot << 2);
    if (impulse <= 0xA0000)
        return;
    first = fixedmult(normal->x, o->orientMat.m[0]);
    second = fixedmult(normal->y, o->orientMat.m[1]);
    third = fixedmult(normal->z, o->orientMat.m[2]);
    sint32 right = ((first + second) + third);
    first = fixedmult(normal->x, o->orientMat.m[3]);
    second = fixedmult(normal->y, o->orientMat.m[4]);
    third = fixedmult(normal->z, o->orientMat.m[5]);
    sint32 top = ((first + second) + third);
    first = fixedmult(normal->x, o->orientMat.m[6]);
    second = fixedmult(normal->y, o->orientMat.m[7]);
    third = fixedmult(normal->z, o->orientMat.m[8]);
    sint32 front = ((first + second) + third);
    sint32 zone = 9;
    if (top < -0x8000) {
        zone = 8;
        Newton::AddDamageZone(o, impulse, zone, 0);
    }
    if (front < -0x1999) {
        if (right >= 0x199A)
            zone = 0;
        else if (right < -0x1999)
            zone = 2;
    } else if (front >= 0x199A) {
        if (right >= 0x199A)
            zone = 4;
        else if (right < -0x1999)
            zone = 6;
    }
    if (zone < 8)
        Newton::AddDamageZone(o, impulse, zone, 0);
}

sint32 Collide::DoObjectObjectCollision(BO::tNewtonObj *o0, BO::tNewtonObj *o1, coorddef *p, coorddef *normal)
{
    FUNCTION_MARKER(0x8008E5D4, "NFS4.EXE");
    coorddef Rt0;
    coorddef Rt1;
    Rt0.x = (p->x - o0->position.x);
    Rt0.y = (p->y - o0->position.y);
    Rt0.z = (p->z - o0->position.z);
    Rt1.x = (p->x - o1->position.x);
    Rt1.y = (p->y - o1->position.y);
    Rt1.z = (p->z - o1->position.z);
    coorddef R0CrossN;
    coorddef R1CrossN;
    sint32 first;
    sint32 second;
    sint32 third;
    first = fixedmult(Rt0.y, normal->z);
    second = fixedmult(Rt0.z, normal->y);
    R0CrossN.x = (first - second);
    first = fixedmult(Rt0.z, normal->x);
    second = fixedmult(Rt0.x, normal->z);
    R0CrossN.y = (first - second);
    first = fixedmult(Rt0.x, normal->y);
    second = fixedmult(Rt0.y, normal->x);
    R0CrossN.z = (first - second);
    first = fixedmult(Rt1.y, normal->z);
    second = fixedmult(Rt1.z, normal->y);
    R1CrossN.x = (first - second);
    first = fixedmult(Rt1.z, normal->x);
    second = fixedmult(Rt1.x, normal->z);
    R1CrossN.y = (first - second);
    first = fixedmult(Rt1.x, normal->y);
    second = fixedmult(Rt1.y, normal->x);
    R1CrossN.z = (first - second);
    first = fixedmult(o0->linearVel.x, normal->x);
    second = fixedmult(o0->linearVel.y, normal->y);
    third = fixedmult(o0->linearVel.z, normal->z);
    sint32 linear0 = ((first + second) + third);
    first = fixedmult(o1->linearVel.x, normal->x);
    second = fixedmult(o1->linearVel.y, normal->y);
    third = fixedmult(o1->linearVel.z, normal->z);
    sint32 linear1 = ((first + second) + third);
    first = fixedmult(o0->angularVel.x, R0CrossN.x);
    second = fixedmult(o0->angularVel.y, R0CrossN.y);
    third = fixedmult(o0->angularVel.z, R0CrossN.z);
    sint32 angular0 = ((first + second) + third);
    first = fixedmult(o1->angularVel.x, R1CrossN.x);
    second = fixedmult(o1->angularVel.y, R1CrossN.y);
    third = fixedmult(o1->angularVel.z, R1CrossN.z);
    sint32 angular1 = ((first + second) + third);
    first = fixedmult(R0CrossN.x, R0CrossN.x);
    second = fixedmult(R0CrossN.y, R0CrossN.y);
    third = fixedmult(R0CrossN.z, R0CrossN.z);
    sint32 moment0 = fixedmult((((first + second) + third)), o0->moInertiaInv);
    first = fixedmult(R1CrossN.x, R1CrossN.x);
    second = fixedmult(R1CrossN.y, R1CrossN.y);
    third = fixedmult(R1CrossN.z, R1CrossN.z);
    sint32 moment1 = fixedmult((((first + second) + third)), o1->moInertiaInv);
    sint32 numerator = (((linear1 - linear0) - angular0) + angular1);
    sint32 denominator = (((((o0->massInv / 2)) + (o1->massInv / 2)) + (moment0 >> 1)) + (moment1 >> 1));
    sint32 impulse = fixeddiv(numerator, denominator);
    if (impulse < 0)
        return 0;
    if (CollisionHumanFlag(o0) && o0->collision.collided == 0)
        o0->collision.collided = 2;
    if (CollisionHumanFlag(o1) && o1->collision.collided == 0)
        o1->collision.collided = 2;
    sint32 coefficient = CollisionHumanFlag(o0) && CollisionHumanFlag(o1) ? 0x4000 : 0x3333;
    impulse = fixedmult(coefficient, impulse);
    coorddef impulseV;
    impulseV.x = fixedmult(impulse, normal->x);
    impulseV.y = fixedmult(impulse, normal->y);
    impulseV.z = fixedmult(impulse, normal->z);
    first = fixedmult(o0->linearVel.x, normal->x);
    second = fixedmult(o0->linearVel.y, normal->y);
    third = fixedmult(o0->linearVel.z, normal->z);
    linear0 = ((first + second) + third);
    first = fixedmult(o1->linearVel.x, normal->x);
    second = fixedmult(o1->linearVel.y, normal->y);
    third = fixedmult(o1->linearVel.z, normal->z);
    linear1 = ((first + second) + third);
    sint32 impulseWST = fixedmult(((linear0 - linear1)), (o0->mass + o1->mass));
    sint32 objectImpulse0 = fixedmult(impulseWST, o0->massInv);
    if (objectImpulse0 <= 0)
        objectImpulse0 = (-objectImpulse0);
    o0->collision.impulse = objectImpulse0;
    sint32 objectImpulse1 = fixedmult(impulseWST, o1->massInv);
    if (objectImpulse1 <= 0)
        objectImpulse1 = (-objectImpulse1);
    o1->collision.impulse = objectImpulse1;
    if (o0->objID >= 0x200 || o1->objID >= 0x200) {
        o0->collision.impulse /= 3;
        o1->collision.impulse /= 3;
    }
    coorddef damageVector = { 0, 0, 0 };
    for (sint32 which = 0; which < 2; ++which) {
        BO::tNewtonObj *obj = which == 0 ? o0 : o1;
        BO::tNewtonObj *peer = which == 0 ? o1 : o0;
        coorddef deltaV;
        deltaV.x = fixedmult(obj->massInv, impulseV.x);
        deltaV.y = fixedmult(obj->massInv, impulseV.y);
        deltaV.z = fixedmult(obj->massInv, impulseV.z);
        if (which == 0) {
            obj->linearVel.x = (obj->linearVel.x + deltaV.x);
            obj->linearVel.y = (obj->linearVel.y + deltaV.y);
        } else {
            obj->linearVel.x = (obj->linearVel.x - deltaV.x);
            obj->linearVel.y = (obj->linearVel.y - deltaV.y);
        }
        sint32 oldZ = obj->linearVel.z;
        sint32 objID = obj->objID;
        obj->collision.sfxType = 0x50001;
        obj->collision.otherObj = peer;
        obj->linearVel.z = which == 0 ? (oldZ + deltaV.z) : (oldZ - deltaV.z);
        if (objID < 0x200) {
            sint32 damageImpulse = obj->collision.impulse;
            if (which == 0) {
                damageVector.x = (o0->position.x - o1->position.x);
                damageVector.y = (o0->position.y - o1->position.y);
                damageVector.z = (o0->position.z - o1->position.z);
                Math::NormalizeShortVector(&damageVector);
            } else {
                damageVector.x = (-damageVector.x);
                damageVector.z = (-damageVector.z);
                damageVector.y = (-damageVector.y);
            }
            if (damageImpulse > 0xA0000) {
                first = fixedmult(damageVector.x, obj->orientMat.m[0]);
                second = fixedmult(damageVector.y, obj->orientMat.m[1]);
                third = fixedmult(damageVector.z, obj->orientMat.m[2]);
                sint32 right = ((first + second) + third);
                first = fixedmult(damageVector.x, obj->orientMat.m[3]);
                second = fixedmult(damageVector.y, obj->orientMat.m[4]);
                third = fixedmult(damageVector.z, obj->orientMat.m[5]);
                sint32 top = ((first + second) + third);
                first = fixedmult(damageVector.x, obj->orientMat.m[6]);
                second = fixedmult(damageVector.y, obj->orientMat.m[7]);
                third = fixedmult(damageVector.z, obj->orientMat.m[8]);
                sint32 front = ((first + second) + third);
                sint32 zone = 9;
                if (top < -0x8000) {
                    zone = 8;
                    Newton::AddDamageZone(obj, damageImpulse, zone, 2);
                }
                sint32 absRight = right < 0 ? -right : right;
                sint32 absFront = front < 0 ? -front : front;
                sint32 selected = 0;
                if (absRight < 0x1999) {
                    if (front < -0x1999) { zone = 1; selected = 1; }
                    else if (front > 0x1999) { zone = 5; selected = 1; }
                }
                if (!selected && absFront < 0x1999) {
                    if (right > 0x1999) { zone = 7; selected = 1; }
                    else if (right < -0x1999) { zone = 3; selected = 1; }
                }
                if (!selected) {
                    if (front < -0x1999) {
                        if (right > 0x1999) zone = 0;
                        else if (right < -0x1999) zone = 2;
                    } else if (front > 0x1999) {
                        if (right > 0x1999) zone = 6;
                        else if (right < -0x1999) zone = 4;
                    }
                }
                if (zone < 8)
                    Newton::AddDamageZone(obj, damageImpulse, zone, 2);
            }
        }
    }
    first = fixedmult(Rt0.y, impulseV.z);
    second = fixedmult(Rt0.z, impulseV.y);
    R0CrossN.x = (first - second);
    first = fixedmult(Rt0.z, impulseV.x);
    second = fixedmult(Rt0.x, impulseV.z);
    R0CrossN.y = (first - second);
    first = fixedmult(Rt0.x, impulseV.y);
    second = fixedmult(Rt0.y, impulseV.x);
    R0CrossN.z = (first - second);
    sint32 inverseMoment0 = o0->moInertiaInv;
    if (CollisionHumanFlag(o0) && impulse <= 0x3FFFFF)
        inverseMoment0 = (inverseMoment0 * 3) / 4;
    first = fixedmult(inverseMoment0, R0CrossN.x);
    second = fixedmult(inverseMoment0, R0CrossN.y);
    third = fixedmult(inverseMoment0, R0CrossN.z);
    o0->angularVel.x = (o0->angularVel.x + first);
    o0->angularVel.y = (o0->angularVel.y + second);
    o0->angularVel.z = (o0->angularVel.z + third);
    first = fixedmult(Rt1.y, impulseV.z);
    second = fixedmult(Rt1.z, impulseV.y);
    R1CrossN.x = (first - second);
    first = fixedmult(Rt1.z, impulseV.x);
    second = fixedmult(Rt1.x, impulseV.z);
    R1CrossN.y = (first - second);
    first = fixedmult(Rt1.x, impulseV.y);
    second = fixedmult(Rt1.y, impulseV.x);
    R1CrossN.z = (first - second);
    sint32 inverseMoment1 = o1->moInertiaInv;
    if (CollisionHumanFlag(o1) && impulse <= 0x3FFFFF)
        inverseMoment1 = (inverseMoment1 * 3) / 4;
    first = fixedmult(inverseMoment1, R1CrossN.x);
    second = fixedmult(inverseMoment1, R1CrossN.y);
    third = fixedmult(inverseMoment1, R1CrossN.z);
    o1->angularVel.x = (o1->angularVel.x - first);
    o1->angularVel.y = (o1->angularVel.y - second);
    o1->angularVel.z = (o1->angularVel.z - third);
    o0->flightTime = (uint16)(o0->flightTime + 1);
    o1->flightTime = (uint16)(o1->flightTime + 1);
    sint32 midpoint = (o0->position.x + o1->position.x) / 2;
    o1->collision.collisionPoint.x = midpoint;
    o0->collision.collisionPoint.x = midpoint;
    midpoint = (o0->position.y + o1->position.y) / 2;
    o1->collision.collisionPoint.y = midpoint;
    o0->collision.collisionPoint.y = midpoint;
    midpoint = (o0->position.z + o1->position.z) / 2;
    o1->collision.collisionPoint.z = midpoint;
    o0->collision.collisionPoint.z = midpoint;
    return 1;
}

sint32 Collide::DoActualObjectCollisionCheck(BO::tNewtonObj *o0, BO::tNewtonObj *o1, coorddef *p, coorddef *normal)
{
    FUNCTION_MARKER(0x8008F550, "NFS4.EXE");
    pP = p;
    obj0 = o0;
    obj1 = o1;
    pNormal = normal;
    if (CHECK_CENTER_VERTEX_WITH_DIRS(1, 0, 0)) return 1;
    if (CHECK_CENTER_VERTEX_WITH_DIRS(0, 1, 0)) return 1;
    if (CHECK_CENTER_VERTEX_WITH_DIRS(0, 0, 1)) return 1;
    if (CHECK_CENTER_VERTEX_WITH_DIRS(-1, 0, 0)) return 1;
    if (CHECK_CENTER_VERTEX_WITH_DIRS(0, -1, 0)) return 1;
    if (CHECK_CENTER_VERTEX_WITH_DIRS(0, 0, -1)) return 1;
    if (CHECK_CENTER_VERTEX_WITH_DIRS_OTHER(1, 0, 0)) return 1;
    if (CHECK_CENTER_VERTEX_WITH_DIRS_OTHER(0, 1, 0)) return 1;
    if (CHECK_CENTER_VERTEX_WITH_DIRS_OTHER(0, 0, 1)) return 1;
    if (CHECK_CENTER_VERTEX_WITH_DIRS_OTHER(-1, 0, 0)) return 1;
    if (CHECK_CENTER_VERTEX_WITH_DIRS_OTHER(0, -1, 0)) return 1;
    if (CHECK_CENTER_VERTEX_WITH_DIRS_OTHER(0, 0, -1)) return 1;
    static const sint32 directions[8][3] = {
        { 1, 1, 1 }, { 1, 1, -1 }, { 1, -1, 1 }, { 1, -1, -1 },
        { -1, 1, 1 }, { -1, 1, -1 }, { -1, -1, 1 }, { -1, -1, -1 }
    };
    sint32 index = 0;
    while (index < 8 && !CHECK_VERTEX_WITH_DIRS( directions[index][0], directions[index][1], directions[index][2]))
        ++index;
    sint32 other = index == 8;
    if (other) {
        index = 0;
        while (index < 8 && !CHECK_VERTEX_WITH_DIRS_OTHER( directions[index][0], directions[index][1], directions[index][2]))
            ++index;
        if (index == 8)
            return 0;
    }
    findClosestSideDave = 0;
    sint32 vx = (o1->linearVel.x - o0->linearVel.x);
    if (vx <= 0)
        vx = (o0->linearVel.x - o1->linearVel.x);
    sint32 vy = (o1->linearVel.y - o0->linearVel.y);
    if (vy <= 0)
        vy = (o0->linearVel.y - o1->linearVel.y);
    sint32 vz = (o1->linearVel.z - o0->linearVel.z);
    if (vz <= 0)
        vz = (o0->linearVel.z - o1->linearVel.z);
    sint32 maxv = vx;
    if (maxv < vy) maxv = vy;
    if (maxv < vz) maxv = vz;
    if (maxv > 0xF0000)
        findClosestSideDave = 1;
    BO::tNewtonObj *faceObj = other ? o1 : o0;
    if (findClosestSideDave != 0) {
        coorddef vel = other ? o0->linearVel : o1->linearVel;
        coorddef normalx = { faceObj->orientMat.m[0], faceObj->orientMat.m[1], faceObj->orientMat.m[2] };
        coorddef normaly = { faceObj->orientMat.m[3], faceObj->orientMat.m[4], faceObj->orientMat.m[5] };
        coorddef normalz = { faceObj->orientMat.m[6], faceObj->orientMat.m[7], faceObj->orientMat.m[8] };
        coorddef *normals[3] = { &normalx, &normaly, &normalz };
        sint32 ranges[3] = { xRange, yRange, zRange };
        sint32 dots[3];
        for (sint32 row = 0; row < 3; ++row) {
            coorddef *axis = normals[row];
            if (other ? ranges[row] < 0 : ranges[row] > 0) {
                axis->x = (-axis->x);
                axis->z = (-axis->z);
                axis->y = (-axis->y);
            }
            uint32 x = (axis->x / 256) * (uint32)(vel.x / 256);
            uint32 y = (axis->y / 256) * (uint32)(vel.y / 256);
            uint32 z = (axis->z / 256) * (uint32)(vel.z / 256);
            sint32 dot = (x + y + z);
            if (dot < 0)
                dot = (-dot);
            dots[row] = dot;
        }
        sint32 dotx = dots[0];
        sint32 doty = dots[1];
        sint32 dotz = dots[2];
        if (doty < dotx && dotz < dotx) *normal = normalx;
        else if (doty > dotz) *normal = normaly;
        else *normal = normalz;
        return 1;
    }
    sint32 xDiff = xRange < 0 ? (faceObj->dimension.x + xRange) : (faceObj->dimension.x - xRange);
    sint32 yDiff = yRange < 0 ? (faceObj->dimension.y + yRange) : (faceObj->dimension.y - yRange);
    sint32 zDiff = zRange < 0 ? (faceObj->dimension.z + zRange) : (faceObj->dimension.z - zRange);
    sint32 selectedRange;
    sint32 row;
    if (xDiff < yDiff && xDiff < zDiff) {
        selectedRange = xRange;
        row = 0;
    } else if (yDiff < zDiff) {
        selectedRange = yRange;
        row = 3;
    } else {
        selectedRange = zRange;
        row = 6;
    }
    coorddef selectedNormal = {
        faceObj->orientMat.m[row], faceObj->orientMat.m[row + 1], faceObj->orientMat.m[row + 2]
    };
    *normal = selectedNormal;
    if (other ? selectedRange < 0 : selectedRange > 0) {
        normal->x = (-normal->x);
        normal->z = (-normal->z);
        normal->y = (-normal->y);
    }
    return 1;
}

sint32 Collide::TestObjectVertices(BO::tNewtonObj *o0, BO::tNewtonObj *o1, coorddef *p, coorddef *normal)
{
    FUNCTION_MARKER(0x80090144, "NFS4.EXE");
    sint32 collisionHappened = 0;
    coorddef backoff0 = { 0, 0, 0 };
    coorddef backoff1 = { 0, 0, 0 };
    for (sint32 row = 0; row < 3; ++row) {
        for (sint32 column = 0; column < 3; ++column) {
            uint32 x = (o0->orientMat.m[row * 3] / 256) * (uint32)(o1->orientMat.m[column * 3] / 256);
            uint32 y = (o0->orientMat.m[row * 3 + 1] / 256) * (uint32)(o1->orientMat.m[column * 3 + 1] / 256);
            uint32 z = (o0->orientMat.m[row * 3 + 2] / 256) * (uint32)(o1->orientMat.m[column * 3 + 2] / 256);
            basisDots[row * 3 + column] = (x + y + z);
        }
    }
    for (sint32 i = 0; i < 9; ++i) {
        sint32 column = i % 3;
        sint32 dimension = column == 0 ? o1->dimension.x : (column == 1 ? o1->dimension.y : o1->dimension.z);
        scaledBasisDots[i] = (((basisDots[i] / 256)) * (dimension / 256));
    }
    for (sint32 i = 0; i < 9; ++i) {
        sint32 row = i / 3;
        sint32 dimension = row == 0 ? o0->dimension.x : (row == 1 ? o0->dimension.y : o0->dimension.z);
        scaledBasisDotsOther[i] = (((basisDots[i] / 256)) * (dimension / 256));
    }
    sint32 numbackoffsteps = 32;
    for (sint32 ctr = 0; ctr < numbackoffsteps; ++ctr) {
        coorddef relativePosition;
        relativePosition.x = (o1->position.x - o0->position.x);
        relativePosition.y = (o1->position.y - o0->position.y);
        relativePosition.z = (o1->position.z - o0->position.z);
        for (sint32 i = 0; i < 6; ++i) {
            BO::tNewtonObj *obj = i < 3 ? o0 : o1;
            sint32 row = (i % 3) * 3;
            uint32 x = (obj->orientMat.m[row] / 256) * (uint32)(relativePosition.x / 256);
            uint32 y = (obj->orientMat.m[row + 1] / 256) * (uint32)(relativePosition.y / 256);
            uint32 z = (obj->orientMat.m[row + 2] / 256) * (uint32)(relativePosition.z / 256);
            basisDotRelative[i] = (x + y + z);
        }
        if (!DoActualObjectCollisionCheck(o0, o1, p, normal))
            return collisionHappened;
        if (collisionHappened == 0) {
            coorddef relativeVelocity;
            relativeVelocity.x = (o0->linearVel.x - o1->linearVel.x);
            relativeVelocity.y = (o0->linearVel.y - o1->linearVel.y);
            relativeVelocity.z = (o0->linearVel.z - o1->linearVel.z);
            sint32 useVelocity = 0;
            sint32 inverseRelativeVelocityLength = 0;
            uint32 vx = (relativeVelocity.x / 256);
            uint32 vz = (relativeVelocity.z / 256);
            sint32 maxrv = (vx * vx + vz * vz);
            if (maxrv > 0xE100000) {
                uint32 px = (relativePosition.x / 256);
                uint32 pz = (relativePosition.z / 256);
                sint32 maxrp = (px * px + pz * pz);
                if (maxrp < 0xCCC) {
                    useVelocity = 1;
                } else {
                    coorddef velocityUnit = relativeVelocity;
                    Math::NormalizeVector(&velocityUnit);
                    coorddef positionUnit = relativePosition;
                    Math::NormalizeShortVector(&positionUnit);
                    uint32 x = (velocityUnit.x / 256) * (uint32)(positionUnit.x / 256);
                    uint32 y = (velocityUnit.y / 256) * (uint32)(positionUnit.y / 256);
                    uint32 z = (velocityUnit.z / 256) * (uint32)(positionUnit.z / 256);
                    sint32 dot = (x + y + z);
                    if (dot > 0xD999)
                        useVelocity = 1;
                }
                if (useVelocity) {
                    sint32 length = Math::VectorLength(&relativeVelocity);
                    inverseRelativeVelocityLength = 0x10000;
                    if (length / 2 != 0)
                        inverseRelativeVelocityLength = fixeddiv(0x8000, length / 2);
                }
            }
            if (useVelocity) {
                sint32 scale = (-inverseRelativeVelocityLength);
                if (o0->objID < 0x200) {
                    backoff0.x = fixedmult(scale, o0->linearVel.x) >> 5;
                    backoff0.y = fixedmult(scale, o0->linearVel.y) >> 5;
                    backoff0.z = fixedmult(scale, o0->linearVel.z) >> 5;
                } else {
                    backoff0.x = 0;
                    backoff0.y = 0;
                    backoff0.z = 0;
                }
                if (o1->objID < 0x200) {
                    backoff1.x = fixedmult(scale, o1->linearVel.x) >> 5;
                    backoff1.y = fixedmult(scale, o1->linearVel.y) >> 5;
                    backoff1.z = fixedmult(scale, o1->linearVel.z) >> 5;
                } else {
                    backoff1.x = 0;
                    backoff1.y = 0;
                    backoff1.z = 0;
                }
            } else {
                coorddef relativeUnit = relativePosition;
                Math::NormalizeShortVector(&relativeUnit);
                sint32 totalMass = (o0->mass + o1->mass);
                sint32 r0 = fixeddiv(o0->mass, totalMass);
                sint32 r1 = fixeddiv(o1->mass, totalMass);
                r1 /= 4;
                sint32 x = (relativeUnit.x * r1) / 0x4000;
                sint32 y = (relativeUnit.y * r1) / 0x4000;
                sint32 z = (relativeUnit.z * r1) / 0x4000;
                backoff0.x = -x >> 5;
                backoff0.y = -y >> 5;
                backoff0.z = -z >> 5;
                r0 /= 4;
                x = (relativeUnit.x * r0) / 0x4000;
                y = (relativeUnit.y * r0) / 0x4000;
                z = (relativeUnit.z * r0) / 0x4000;
                backoff1.x = x >> 5;
                backoff1.y = y >> 5;
                backoff1.z = z >> 5;
                if (o0->objID >= 0x200) {
                    backoff1.x /= 2;
                    backoff1.y /= 2;
                    backoff1.z /= 2;
                }
                if (o1->objID >= 0x200) {
                    backoff0.x /= 2;
                    backoff0.y /= 2;
                    backoff0.z /= 2;
                }
            }
            collisionHappened = 1;
        }
        o0->position.x = (o0->position.x + backoff0.x);
        o0->position.y = (o0->position.y + backoff0.y);
        o0->position.z = (o0->position.z + backoff0.z);
        o1->position.x = (o1->position.x + backoff1.x);
        o1->position.y = (o1->position.y + backoff1.y);
        o1->position.z = (o1->position.z + backoff1.z);
    }
    return collisionHappened;
}

sint32 Collide::CheckForCollisionBetween(BO::tNewtonObj *o0, BO::tNewtonObj *o1)
{
    FUNCTION_MARKER(0x80091374, "NFS4.EXE");
    o0->collision.impulse = 0;
    o1->collision.impulse = 0;

    coorddef p;
    coorddef normal;
    if (TestObjectVertices(o0, o1, &p, &normal) == 0)
        return 0;

    DoObjectObjectCollision(o0, o1, &p, &normal);

    Car::tObj *car0 = o0->objID < 0x200 ? reinterpret_cast<Car::tObj *>(o0) : 0;
    Car::tObj *car1 = o1->objID < 0x200 ? reinterpret_cast<Car::tObj *>(o1) : 0;
    for (sint32 count = 8; ; --count)
    {
        if (car0 != 0)
            Physics::TestForBarrierCollision(car0);
        if (car1 != 0)
            Physics::TestForBarrierCollision(car1);

        if (count == 0)
        {
            if (car0 != 0 && car0->crash != 0 && o0->speedXZ > 0xF0000)
                Newton::AddDamageZone(o0, 0x640000, 8, 2);
            if (car1 != 0 && car1->crash != 0 && o1->speedXZ > 0xF0000)
                Newton::AddDamageZone(o1, 0x640000, 8, 2);
        }

        if (TestObjectVertices(o0, o1, &p, &normal) == 0 || count == 0)
            return 1;
        if (DoObjectObjectCollision(o0, o1, &p, &normal) == 0)
            return 1;
    }
}

void Collide::ClearCollisionRegistry(void)
{
    FUNCTION_MARKER(0x800914D4, "NFS4.EXE");
    gNumRegistered = 0;
    for (sint32 i = 0; i < Object::GetNumIMassObjects(); ++i) {
        BO::tNewtonObj *obj = &InfiniteMassNewton[i];
        Object::GetIMassObjectMotion(i, &obj->position, &obj->orientMat, &obj->linearVel);
        obj->angularVel.x = 0;
        obj->angularVel.y = 0;
        obj->angularVel.z = 0;
        gRegistry[i] = obj;
        gNumRegistered = (gNumRegistered + 1);
        BWorldSm::FindClosestSlice(&obj->position, &obj->simRoadInfo);
        sint32 slice = obj->simRoadInfo.slice;
        Trk::NewSlice *roadSlice = &BWorldSm::slices[slice];
        coorddef relVec;

        relVec.x = (obj->position.x - roadSlice->center[0]);
        relVec.y = (obj->position.y - roadSlice->center[0]);
        relVec.z = (obj->position.z - roadSlice->center[0]);
        coorddef rightVec;
        rightVec.x = (sint8)roadSlice->right[0] * 512;
        rightVec.y = (sint8)roadSlice->right[1] * 512;
        rightVec.z = (sint8)roadSlice->right[2] * 512;
        uint32 x = (relVec.x / 256) * (uint32)(rightVec.x / 256);
        uint32 y = (relVec.y / 256) * (uint32)(rightVec.y / 256);
        uint32 z = (relVec.z / 256) * (uint32)(rightVec.z / 256);
        obj->xRelRoadCenter = (x + y + z);
    }
    sint32 carLoop = 0;
    while (carLoop++ < Cars::gNumCars) {
    }
}

sint32 Collide::CheckAccuratePointRadiusCollision(BO::tNewtonObj *newObj, coorddef *point, sint32 radius)
{
    FUNCTION_MARKER(0x800916D4, "NFS4.EXE");
    coorddef d;
    d.x = (point->x - newObj->position.x);
    d.y = 0;
    d.z = (point->z - newObj->position.z);
    sint32 first = fixedmult(d.x, newObj->orientMat.m[0]);
    sint32 second = fixedmult(d.y, newObj->orientMat.m[1]);
    sint32 third = fixedmult(d.z, newObj->orientMat.m[2]);
    sint32 temp = ((first + second) + third);
    if (temp <= 0)
        temp = (-temp);
    if (temp > (newObj->dimension.x + radius))
        return 0;
    first = fixedmult(d.x, newObj->orientMat.m[6]);
    second = fixedmult(d.y, newObj->orientMat.m[7]);
    third = fixedmult(d.z, newObj->orientMat.m[8]);
    temp = ((first + second) + third);
    if (temp <= 0)
        temp = (-temp);
    return temp <= (newObj->dimension.z + radius);
}

void Collide::CheckMeForCollisions(BO::tNewtonObj *newObj)
{
    FUNCTION_MARKER(0x800918CC, "NFS4.EXE");
    if (newObj->active == 0)
        return;
    Car::tObj *carObj = reinterpret_cast<Car::tObj *>(newObj);
    if (newObj->simOptz != 0) {
        Physics::TestForBarrierCollision(carObj);
        return;
    }
    if (newObj->collision.disableCollisionTimer > 0)
        --newObj->collision.disableCollisionTimer;
    if (newObj->collision.disableCollisionTimer == 0 && (newObj->groundSurfaceType & 0x40) != 0) {
        Object::tSimObjList objList;
        sint32 numObjs;
        sint32 fixedRadius;
        coorddef pos;
        Object::InitCollisionCheckLoop(&newObj->simRoadInfo, &objList, &numObjs);
        for (sint32 i = 0; i < numObjs; ++i) {
            coorddef pointList[3];
            sint32 closestDist = 0xA0000;
            sint32 closestPoint = -1;
            sint32 numPoints = 1;
            Object::GetRadiusCollisionData(&objList, i, &pos, &fixedRadius);
            Object::GetPointsCollisionData(&objList, i, &numPoints, pointList);
            for (sint32 j = 0; j < numPoints; ++j) {
                sint32 dist = Math::DistXZ(&pointList[j], &newObj->position);
                sint32 radius = (fixedRadius + newObj->dimensionRadius);
                if (dist < radius && dist < closestDist) {
                    closestDist = dist;
                    closestPoint = j;
                }
            }
            if (closestPoint < 0 || CheckAccuratePointRadiusCollision(newObj, &pointList[closestPoint], fixedRadius) == 0)
                continue;
            sint32 signCase = Object::CheckCollisionResults(&objList, i, newObj);
            if (signCase == 0)
                continue;
            Object::GetRadiusCollisionData(&objList, i, &pos, &fixedRadius);
            if (signCase == 1) {
                coorddef normal;
                normal.y = 0;
                normal.x = (newObj->position.x - pos.x);
                normal.z = (newObj->position.z - pos.z);
                Math::NormalizeShortVector(&normal);
                sint32 impulse = 0;
                sint32 radius = fixedRadius / 256;
                coorddef samplePoint;
                sint32 x = (radius * normal.x);
                sint32 y = (radius * normal.y);
                sint32 z = (radius * normal.z);
                samplePoint.x = (((x / 256)) + pos.x);
                samplePoint.y = (((y / 256)) + pos.y);
                samplePoint.z = (((z / 256)) + pos.z);
                sint32 speed = newObj->speedXZ;
                if (speed > 0x280000) {
                    impulse = ((uint32)speed << 1);
                    if (impulse > 0x960000)
                        impulse = 0x960000;
                } else if (speed > 0x190000) {
                    impulse = speed;
                    if (impulse < 0x1E0000)
                        impulse = 0x1E0000;
                } else {
                    frictionLess = 1;
                }
                TestWithPlane(newObj, &normal, &samplePoint);
                coorddef postNormal = normal;
                frictionLess = 0;
                Newton::DoPostBarrierCollisionHandling(newObj, postNormal);
                newObj->collision.impulse = impulse;
                newObj->collision.collided = 1;
                newObj->collision.otherObj = 0;
                if (impulse != 0) {
                    newObj->collision.sfxType = 0x50001;
                    coorddef collisionPoint = newObj->position;
                    newObj->collision.collisionPoint = collisionPoint;
                }
            } else if ((signCase == 2 && newObj->speedXZ > 0x20000) || signCase == -1) {
                newObj->collision.impulse = 0xF0000;
                newObj->collision.sfxType = signCase == 2 ? 0x60006 : 0x60005;
                newObj->collision.otherObj = 0;
                coorddef collisionPoint = newObj->position;
                newObj->collision.collisionPoint = collisionPoint;
                if (Force::IsForceOn(carObj))
                    Force::HitSign(carObj);
            }
        }
    }
    if ((newObj->groundSurfaceType & 0x80) != 0)
        Newton::TestForUndrivableSurfaces(newObj);
    if (newObj->flightTime != 0)
        Physics::TestForBarrierCollision(carObj);
    if (newObj->collision.disableCollisionTimer != 0)
        return;
    gRegistry[gNumRegistered] = newObj;
    for (sint32 i = 0; i < gNumRegistered; ++i) {
        BO::tNewtonObj *otherObj = gRegistry[i];
        sint32 radius = (newObj->dimensionRadius + otherObj->dimensionRadius);
        sint32 dist = (newObj->position.z - otherObj->position.z);
        if (dist <= 0)
            dist = (otherObj->position.z - newObj->position.z);
        if (dist >= radius)
            continue;
        radius = (newObj->dimensionRadius + otherObj->dimensionRadius);
        dist = (newObj->position.x - otherObj->position.x);
        if (dist <= 0)
            dist = (otherObj->position.x - newObj->position.x);
        if (dist >= radius)
            continue;
        radius = (newObj->dimensionRadius + otherObj->dimensionRadius);
        dist = (newObj->position.y - otherObj->position.y);
        if (dist <= 0)
            dist = (otherObj->position.y - newObj->position.y);
        if (dist >= radius)
            continue;
        newObj->angularVel.x = fixedmult(0x6487E, newObj->angularVel.x);
        newObj->angularVel.y = fixedmult(0x6487E, newObj->angularVel.y);
        newObj->angularVel.z = fixedmult(0x6487E, newObj->angularVel.z);
        otherObj->angularVel.x = fixedmult(0x6487E, otherObj->angularVel.x);
        otherObj->angularVel.y = fixedmult(0x6487E, otherObj->angularVel.y);
        otherObj->angularVel.z = fixedmult(0x6487E, otherObj->angularVel.z);
        CheckForCollisionBetween(newObj, otherObj);
        newObj->angularVel.x = fixedmult(0x28BE, newObj->angularVel.x);
        newObj->angularVel.y = fixedmult(0x28BE, newObj->angularVel.y);
        newObj->angularVel.z = fixedmult(0x28BE, newObj->angularVel.z);
        otherObj->angularVel.x = fixedmult(0x28BE, otherObj->angularVel.x);
        otherObj->angularVel.y = fixedmult(0x28BE, otherObj->angularVel.y);
        otherObj->angularVel.z = fixedmult(0x28BE, otherObj->angularVel.z);
        LimitAngularVel(newObj);
        LimitAngularVel(otherObj);
    }
    gNumRegistered = (gNumRegistered + 1);
}
