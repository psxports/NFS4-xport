#ifndef QUATERN_H
#define QUATERN_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

struct tQuat
{
    sint16 x;
    sint16 y;
    sint16 z;
    sint16 w;
};

class Quatern
{
public:
    static void VecInterpolate(coorddef *cp0, coorddef *cp1, sint32 weight, coorddef *cp);
    static void Interpolate(tQuat *q0, tQuat *q1, coorddef *cp0, coorddef *cp1, sint32 weight, tQuat *q, coorddef *cp);
    static void QuatToMat(tQuat *q, matrixtdef *matrix);
    static void MatToQuat(matrixtdef *matrix, tQuat *q);
};

#endif
