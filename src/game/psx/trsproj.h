#ifndef TRSPROJ_H
#define TRSPROJ_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/matrixtdef.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"

struct RelCoord16;

class TrsProj
{
public:
    static sint32 precision;

    static void SetTransPrecision(sint32 val);
    static void ResetTransPrecision(void);
    static void SetProjection(sint32 cx, sint32 cy, sint32 w, sint32 h);
    static void SetMenuProjection(sint32 cx, sint32 cy, sint32 w, sint32 h);
    static void SetViewTrsProjEnviro(DRender::tView *Vi);
    static void SetPsxMatrix(matrixtdef *m, coorddef *t);
    static void SetPsxTrans(coorddef *t);
    static void SetPsxTransZero(void);
    static void TransPt(coorddef *s, coorddef *d);
    static void TransPtN16(RelCoord16 *s, coorddef *d, sint32 n);
    static void TransformProjectVertex(matrixtdef *m, coorddef *t, sint32 n, coorddef *s, Draw::tVertex *v);
};

#endif
