#include "xport.h"
#include "game/common/mathnfs.h"
#include "game/common/quatern.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/fixdsqrt.h"

void Quatern::VecInterpolate(coorddef *cp0, coorddef *cp1, sint32 weight, coorddef *cp)
{
    FUNCTION_MARKER(0x800AD5C8, "NFS4.EXE");

    sint32 delta = fixedmult(((cp1->x - cp0->x)), weight);
    cp->x = (cp0->x + delta);
    delta = fixedmult(((cp1->y - cp0->y)), weight);
    cp->y = (cp0->y + delta);
    delta = fixedmult(((cp1->z - cp0->z)), weight);
    cp->z = (cp0->z + delta);
}

void Quatern::Interpolate(tQuat *q0, tQuat *q1, coorddef *cp0, coorddef *cp1, sint32 weight, tQuat *q, coorddef *cp)
{
    FUNCTION_MARKER(0x800AD678, "NFS4.EXE");

    sint32 delta = fixedmult(q1->x - q0->x, weight);
    q->x = (sint16)((uint32)q0->x + delta);
    delta = fixedmult(q1->y - q0->y, weight);
    q->y = (sint16)((uint32)q0->y + delta);
    delta = fixedmult(q1->z - q0->z, weight);
    q->z = (sint16)((uint32)q0->z + delta);
    delta = fixedmult(q1->w - q0->w, weight);
    q->w = (sint16)((uint32)q0->w + delta);
    VecInterpolate(cp0, cp1, weight, cp);
}

void Quatern::QuatToMat(tQuat *q, matrixtdef *matrix)
{
    FUNCTION_MARKER(0x800AD780, "NFS4.EXE");

    sint32 qx = q->x;
    sint32 qw = q->w;
    sint32 qy = q->y;
    sint32 qz = q->z;
    sint32 x = qx * 2;
    sint32 y = qy * 2;
    sint32 z = qz * 2;
    uint32 one = 0x10000000;
    uint32 wx = (uint32)qw * x;
    uint32 wy = (uint32)qw * y;
    uint32 wz = (uint32)qw * z;
    uint32 xx = (uint32)qx * x;
    uint32 xy = (uint32)qx * y;
    uint32 yy = (uint32)qy * y;
    uint32 zz = (uint32)qz * z;
    uint32 xz = (uint32)qx * z;
    uint32 yz = (uint32)qy * z;
    matrix->m[0] = (sint32)(one - yy - zz) >> 12;
    matrix->m[1] = (sint32)(xy + wz) >> 12;
    matrix->m[3] = (sint32)(xy - wz) >> 12;
    matrix->m[4] = (sint32)(one - xx - zz) >> 12;
    matrix->m[8] = (sint32)(one - xx - yy) >> 12;
    matrix->m[2] = (sint32)(xz - wy) >> 12;
    matrix->m[6] = (sint32)(xz + wy) >> 12;
    matrix->m[5] = (sint32)(yz + wx) >> 12;
    matrix->m[7] = (sint32)(yz - wx) >> 12;
}

void Quatern::MatToQuat(matrixtdef *matrix, tQuat *q)
{
    FUNCTION_MARKER(0x800AD890, "NFS4.EXE");

    if (matrix->m[0] == 65536 && matrix->m[1] == 0 && matrix->m[2] == 0 && matrix->m[3] == 0 && matrix->m[4] == 65536 && matrix->m[5] == 0 && matrix->m[6] == 0 && matrix->m[7] == 0 && matrix->m[8] == 65536)
    {
        q->w = 16384;
        q->x = 0;
        q->y = 0;
        q->z = 0;
        return;
    }
    sint32 tr = ((matrix->m[0] + matrix->m[4]) + matrix->m[8]);
    sint32 s;
    sint32 component;
    if (tr > 0)
    {
        s = fixedsqrt(((tr + 65536)));
        q->w = (sint16)((s + ((uint32)s >> 31)) >> 3);
        s = fixeddiv(32768, s);
        component = fixedmult(((matrix->m[7] - matrix->m[5])), s);
        q->x = (sint16)(component >> 2);
        component = fixedmult(((matrix->m[2] - matrix->m[6])), s);
        q->y = (sint16)(component >> 2);
        component = fixedmult(((matrix->m[3] - matrix->m[1])), s);
        q->z = (sint16)(component >> 2);
        return;
    }
    sint32 i = 0;
    if (matrix->m[0] < matrix->m[4])
    {
        i = 1;
        if (matrix->m[4] < matrix->m[8])
            i = 2;
    }
    if (matrix->m[0] < matrix->m[8])
        i = 2;
    if (i == 0)
    {
        s = fixedsqrt(((((matrix->m[0] - matrix->m[4]) - matrix->m[8]) + 65536)));
        q->x = (sint16)((s + ((uint32)s >> 31)) >> 3);
        if (s != 0)
            s = fixeddiv(32768, s);
        component = fixedmult(((matrix->m[7] - matrix->m[5])), s);
        q->w = (sint16)(component >> 2);
        component = fixedmult(((matrix->m[3] + matrix->m[1])), s);
        q->y = (sint16)(component >> 2);
        component = fixedmult(((matrix->m[2] + matrix->m[6])), s);
        q->z = (sint16)(component >> 2);
    }
    else if (i == 1)
    {
        s = fixedsqrt(((((matrix->m[4] - matrix->m[8]) - matrix->m[0]) + 65536)));
        q->y = (sint16)((s + ((uint32)s >> 31)) >> 3);
        if (s != 0)
            s = fixeddiv(32768, s);
        component = fixedmult(((matrix->m[2] - matrix->m[6])), s);
        q->w = (sint16)(component >> 2);
        component = fixedmult(((matrix->m[7] + matrix->m[5])), s);
        q->z = (sint16)(component >> 2);
        component = fixedmult(((matrix->m[3] + matrix->m[1])), s);
        q->x = (sint16)(component >> 2);
    }
    else
    {
        s = fixedsqrt(((((matrix->m[8] - matrix->m[0]) - matrix->m[4]) + 65536)));
        q->z = (sint16)((s + ((uint32)s >> 31)) >> 3);
        if (s != 0)
            s = fixeddiv(32768, s);
        component = fixedmult(((matrix->m[3] - matrix->m[1])), s);
        q->w = (sint16)(component >> 2);
        component = fixedmult(((matrix->m[2] + matrix->m[6])), s);
        q->x = (sint16)(component >> 2);
        component = fixedmult(((matrix->m[5] + matrix->m[7])), s);
        q->y = (sint16)(component >> 2);
    }
}
