#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/mathnfs.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdinv.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/fixdsqrt.h"

sint32 Math::DistXZ(coorddef *a, coorddef *b)
{
    FUNCTION_MARKER(0x8009D3E4, "NFS4.EXE");

    sint32 ax = a->x;
    sint32 bx = b->x;
    sint32 az = a->z;
    sint32 bz = b->z;
    sint32 x = (sint32)((uint32)ax - bx);
    if (x <= 0)
        x = (sint32)((uint32)bx - ax);
    sint32 z = (sint32)((uint32)az - bz);
    if (z <= 0)
        z = (sint32)((uint32)bz - az);
    sint32 dist;
    if (z < x)
        dist = (sint32)((uint32)x + (z >> 2));
    else
        dist = (sint32)((uint32)z + (x >> 2));
    return dist;
}

sint32 Math::Dist3D(coorddef *a, coorddef *b)
{
    FUNCTION_MARKER(0x8009D438, "NFS4.EXE");

    sint32 x = (sint32)((uint32)a->x - b->x);
    if (x <= 0)
        x = (sint32)(0 - (uint32)x);
    sint32 y = (sint32)((uint32)a->y - b->y);
    if (y <= 0)
        y = (sint32)(0 - (uint32)y);
    sint32 z = (sint32)((uint32)a->z - b->z);
    if (z <= 0)
        z = (sint32)(0 - (uint32)z);
    sint32 dist = y < x ? (sint32)((uint32)x + (y >> 2)) : (sint32)((uint32)y + (x >> 2));
    sint32 dist2 = z < dist ? (sint32)((uint32)dist + (z >> 2)) : (sint32)((uint32)z + (dist >> 2));
    return dist2;
}

sint32 Math::BetterDist(sint32 a, sint32 b)
{
    FUNCTION_MARKER(0x8009D4C8, "NFS4.EXE");

    sint32 x = a < b ? b : a;
    sint32 y = a < b ? a : b;
    if (y < (x >> 2))
        return (sint32)((uint32)x + (y >> 4) + (y >> 6) + (y >> 7) + (y >> 9) + (y >> 14) + (y >> 15) + (y >> 16));
    if (y < (x >> 1))
        return (sint32)((uint32)x + (y >> 3) + (y >> 5) + (y >> 6) + (y >> 7) + (y >> 8) + (y >> 9) + (y >> 12) + (y >> 13) + (y >> 14) + (y >> 16));
    if (y < (x >> 1) + (x >> 2))
        return (sint32)((uint32)x + (y >> 2) + (y >> 5) + (y >> 8) + (y >> 10) + (y >> 11) + (y >> 12) + (y >> 13));
    return (sint32)((uint32)x + (y >> 2) + (y >> 4) + (y >> 5) + (y >> 6) + (y >> 7) + (y >> 8) + (y >> 9) + (y >> 11) + (y >> 12) + (y >> 13) + (y >> 14) + (y >> 15));
}

void Math::ResolveRotatedVector(sint32 x, sint32 y, sint32 angle, sint32 *newx, sint32 *newy)
{
    FUNCTION_MARKER(0x8009D628, "NFS4.EXE");

    sint32 a = angle / 256;
    sint32 ds = fixedsin(a);
    sint32 dc = fixedcos(a);
    sint32 first = fixedmult(dc, x);
    sint32 second = fixedmult(ds, y);
    *newx = (sint32)((uint32)first - second);
    first = fixedmult(ds, x);
    second = fixedmult(dc, y);
    *newy = (sint32)((uint32)first + second);
}

sint32 Math::VectorLength(coorddef *v)
{
    FUNCTION_MARKER(0x8009D6EC, "NFS4.EXE");

    sint32 shifts = 0;
    sint32 vx = v->x;
    sint32 vy = v->y;
    sint32 vz = v->z;
    while ((vx < 0 ? (sint32)(0 - (uint32)vx) : vx) > 0x1000000 ||
           (vy < 0 ? (sint32)(0 - (uint32)vy) : vy) > 0x1000000 ||
           (vz < 0 ? (sint32)(0 - (uint32)vz) : vz) > 0x1000000)
    {
        vx >>= 1;
        vy >>= 1;
        vz >>= 1;
        ++shifts;
    }
    uint32 vx2 = fixedmult(vx, vx);
    uint32 vy2 = fixedmult(vy, vy);
    uint32 vz2 = fixedmult(vz, vz);
    uint32 vx2s = vx2 >> 2;
    uint32 vy2s = vy2 >> 2;
    uint32 vz2s = vz2 >> 2;
    while (vx2s + vy2s + vz2s > 0x40000000)
    {
        vx >>= 1;
        vy >>= 1;
        vz >>= 1;
        ++shifts;
        vx2 = fixedmult(vx, vx);
        vy2 = fixedmult(vy, vy);
        vz2 = fixedmult(vz, vz);
        vx2s = vx2 >> 2;
        vy2s = vy2 >> 2;
        vz2s = vz2 >> 2;
    }
    uint32 length = fixedsqrt((sint32)(vx2 + vy2 + vz2));
    while (shifts != 0)
    {
        length <<= 1;
        --shifts;
    }
    return (sint32)length;
}

sint32 Math::VectorLength2(coorddef *v)
{
    FUNCTION_MARKER(0x8009D888, "NFS4.EXE");

    sint32 shifts = 0;
    sint32 vx = v->x;
    sint32 vy = v->y;
    sint32 vz = v->z;
    while ((vx < 0 ? (sint32)(0 - (uint32)vx) : vx) > 0x1000000 ||
           (vy < 0 ? (sint32)(0 - (uint32)vy) : vy) > 0x1000000 ||
           (vz < 0 ? (sint32)(0 - (uint32)vz) : vz) > 0x1000000)
    {
        vx >>= 1;
        vy >>= 1;
        vz >>= 1;
        ++shifts;
    }
    uint32 vx2 = fixedmult(vx, vx);
    uint32 vy2 = fixedmult(vy, vy);
    uint32 vz2 = fixedmult(vz, vz);
    uint32 vx2s = vx2 >> 2;
    uint32 vy2s = vy2 >> 2;
    uint32 vz2s = vz2 >> 2;
    while (vx2s + vy2s + vz2s > 0x40000000)
    {
        vx >>= 1;
        vy >>= 1;
        vz >>= 1;
        ++shifts;
        vx2 = fixedmult(vx, vx);
        vy2 = fixedmult(vy, vy);
        vz2 = fixedmult(vz, vz);
        vx2s = vx2 >> 2;
        vy2s = vy2 >> 2;
        vz2s = vz2 >> 2;
    }
    uint32 length2 = vx2 + vy2 + vz2;
    while (shifts != 0)
    {
        length2 <<= 2;
        --shifts;
    }
    return (sint32)length2;
}

void Math::NormalizeVector(coorddef *v)
{
    FUNCTION_MARKER(0x8009DA1C, "NFS4.EXE");

    sint32 vx = v->x;
    sint32 vy = v->y;
    sint32 vz = v->z;
    while ((vx < 0 ? (sint32)(0 - (uint32)vx) : vx) > 0x1000000 ||
           (vy < 0 ? (sint32)(0 - (uint32)vy) : vy) > 0x1000000 ||
           (vz < 0 ? (sint32)(0 - (uint32)vz) : vz) > 0x1000000)
    {
        vx >>= 1;
        vy >>= 1;
        vz >>= 1;
    }
    uint32 vx2 = fixedmult(vx, vx);
    uint32 vy2 = fixedmult(vy, vy);
    uint32 vz2 = fixedmult(vz, vz);
    uint32 vx2s = vx2 >> 2;
    uint32 vy2s = vy2 >> 2;
    uint32 vz2s = vz2 >> 2;
    while (vx2s + vy2s + vz2s > 0x40000000)
    {
        vx >>= 1;
        vy >>= 1;
        vz >>= 1;
        vx2 = fixedmult(vx, vx);
        vy2 = fixedmult(vy, vy);
        vz2 = fixedmult(vz, vz);
        vx2s = vx2 >> 2;
        vy2s = vy2 >> 2;
        vz2s = vz2 >> 2;
    }
    sint32 length = fixedsqrt((sint32)(vx2 + vy2 + vz2));
    if (length != 0)
    {
        v->x = fixeddiv(vx, length);
        v->y = fixeddiv(vy, length);
        v->z = fixeddiv(vz, length);
    }
}

void Math::NormalizeShortVector(coorddef *v)
{
    FUNCTION_MARKER(0x8009DBD4, "NFS4.EXE");

    uint32 vx2 = fixedmult(v->x, v->x);
    uint32 vy2 = fixedmult(v->y, v->y);
    uint32 vz2 = fixedmult(v->z, v->z);
    sint32 length = fixedsqrt((sint32)(vx2 + vy2 + vz2));
    if (length != 0)
    {
        sint32 inverse = fixeddiv(65536, length);
        v->x = fixedmult(v->x, inverse);
        v->y = fixedmult(v->y, inverse);
        v->z = fixedmult(v->z, inverse);
    }
}

void Math::QDNormalizeVector(coorddef *v)
{
    FUNCTION_MARKER(0x8009DC80, "NFS4.EXE");

    coorddef temp = *v;
    if (temp.x < 0)
        temp.x = (sint32)(0 - (uint32)temp.x);
    if (temp.y < 0)
        temp.y = (sint32)(0 - (uint32)temp.y);
    if (temp.z < 0)
        temp.z = (sint32)(0 - (uint32)temp.z);
    sint32 length = BetterDist(temp.x, temp.y);
    length = BetterDist(length, temp.z);
    if (length != 0)
    {
        sint32 inverse = fixedinverse(length) / 256;
        v->x = (sint32)((uint32)(v->x / 256) * inverse);
        v->y = (sint32)((uint32)(v->y / 256) * inverse);
        v->z = (sint32)((uint32)(v->z / 256) * inverse);
    }
}

void Math::fasttransmult(matrixtdef *m1, matrixtdef *m2, matrixtdef *mr)
{
    FUNCTION_MARKER(0x8009DD8C, "NFS4.EXE");

    matrixtdef mtmp;
    for (sint32 i = 0; i < 9; i += 3)
        for (sint32 j = 0; j < 3; ++j)
        {
            sint32 ta = m1->m[i] >> 3;
            sint32 tb = m2->m[j] >> 3;
            sint32 tc = m1->m[i + 1] >> 3;
            sint32 td = m2->m[j + 3] >> 3;
            sint32 te = m1->m[i + 2] >> 3;
            sint32 tf = m2->m[j + 6] >> 3;
            sint32 val = (sint32)((uint32)((sint32)((uint32)ta * tb) >> 10) + ((sint32)((uint32)tc * td) >> 10) + ((sint32)((uint32)te * tf) >> 10));
            mtmp.m[i + j] = val;
        }
    for (sint32 i = 0; i < 9; ++i)
        mr->m[i] = mtmp.m[i];
}
