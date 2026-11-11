#ifndef MATHNFS_H
#define MATHNFS_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

class Math
{
public:
    static sint32 DistXZ(coorddef *a, coorddef *b);
    static sint32 Dist3D(coorddef *a, coorddef *b);
    static sint32 BetterDist(sint32 a, sint32 b);
    static void ResolveRotatedVector(sint32 x, sint32 y, sint32 angle, sint32 *newx, sint32 *newy);
    static sint32 VectorLength(coorddef *v);
    static sint32 VectorLength2(coorddef *v);
    static void NormalizeVector(coorddef *v);
    static void NormalizeShortVector(coorddef *v);
    static void QDNormalizeVector(coorddef *v);
    static void fasttransmult(matrixtdef *m1, matrixtdef *m2, matrixtdef *mr);
};

#endif
