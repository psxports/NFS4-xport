#include <stddef.h>
#include "xport.h"
#include "psx.h"
#include "psx_gte.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/camera.h"
#include "game/common/gmesetup.h"
#include "game/common/mathnfs.h"
#include "game/common/renderview.h"
#include "game/psx/trsproj.h"

struct PCOORD16
{
    sint16 x;
    sint16 y;
    sint16 z;
    sint16 p;
};

struct Draw::tVertex
{
    PCOORD16 sv;
};

struct RelCoord16
{
    sint16 x;
    sint16 z;
};

sint32 TrsProj::precision = 10;

void TrsProj::SetTransPrecision(sint32 val)
{
    FUNCTION_MARKER(0x800E192C, "NFS4.EXE");
    precision = val;
}

void TrsProj::ResetTransPrecision(void)
{
    FUNCTION_MARKER(0x800E1938, "NFS4.EXE");
    precision = 10;
}

void TrsProj::SetProjection(sint32 cx, sint32 cy, sint32 w, sint32 h)
{
    FUNCTION_MARKER(0x800E1948, "NFS4.EXE");
    SetGeomScreen(Camera::gGeomScreen);
    sint32 x = (cx + (w / 2));
    sint32 y = (cy + (h / 2));
    gte_SetGeomOffset(x, y);
}

void TrsProj::SetMenuProjection(sint32 cx, sint32 cy, sint32 w, sint32 h)
{
    FUNCTION_MARKER(0x800E19C8, "NFS4.EXE");
    gte_ctc2(0x200, 26);
    sint32 x = (cx + (w / 2));
    sint32 y = (cy + (h / 2));
    gte_SetGeomOffset(x, y);
}

void TrsProj::SetViewTrsProjEnviro(DRender::tView *Vi)
{
    FUNCTION_MARKER(0x800E1A08, "NFS4.EXE");
    SetProjection(0, 0, 320, GameSetup::gData.commMode == 1 ? 120 : 240);
}

void TrsProj::SetPsxMatrix(matrixtdef *m, coorddef *t)
{
    FUNCTION_MARKER(0x800E1A5C, "NFS4.EXE");
    MATRIX mpsx;
    for (sint32 row = 0; row < 3; ++row)
    {
        sint32 r0 = m->m[row];
        sint32 r1 = m->m[row + 3];
        sint32 r2 = m->m[row + 6];
        mpsx.m[row][0] = (sint16)(r0 >> 4);
        mpsx.m[row][1] = (sint16)(r1 >> 4);
        mpsx.m[row][2] = (sint16)(r2 >> 4);
    }
    gte_SetRotMatrix(&mpsx);
    if (t != 0)
        SetPsxTrans(t);
    else
    {
        mpsx.t[2] = 0;
        mpsx.t[1] = 0;
        mpsx.t[0] = 0;
        gte_SetTransMatrix(&mpsx);
    }
}

void TrsProj::SetPsxTrans(coorddef *t)
{
    FUNCTION_MARKER(0x800E1B4C, "NFS4.EXE");
    MATRIX mpsx;
    mpsx.t[0] = t->x >> 10;
    mpsx.t[1] = t->y >> 10;
    mpsx.t[2] = t->z >> 10;
    gte_SetTransMatrix(&mpsx);
}

void TrsProj::SetPsxTransZero(void)
{
    FUNCTION_MARKER(0x800E1B9C, "NFS4.EXE");
    MATRIX mpsx;
    mpsx.t[2] = 0;
    mpsx.t[1] = 0;
    mpsx.t[0] = 0;
    gte_SetTransMatrix(&mpsx);
}

void TrsProj::TransPt(coorddef *s, coorddef *d)
{
    FUNCTION_MARKER(0x800E1BCC, "NFS4.EXE");
    SVECTOR pt;
    VECTOR tv;
    pt.vx = (sint16)(s->x >> 10);
    pt.vy = (sint16)(s->y >> 10);
    pt.vz = (sint16)(s->z >> 10);
    gte_ldv0(&pt);
    gte_rt();
    gte_stlvnl(&tv);
    d->x = ((uint32)tv.vx << 10);
    d->y = ((uint32)tv.vy << 10);
    d->z = ((uint32)tv.vz << 10);
}

void TrsProj::TransPtN16(RelCoord16 *s, coorddef *d, sint32 n)
{
    FUNCTION_MARKER(0x800E1C58, "NFS4.EXE");
    SVECTOR pt;
    VECTOR tv;
    for (uint32 remaining = n; remaining != 0; --remaining)
    {
        sint16 x = s->x;
        pt.vy = 0;
        pt.vx = x;
        pt.vz = s->z;
        gte_ldv0(&pt);
        gte_rt();
        gte_stlvnl(&tv);
        ++s;
        d->x = tv.vx;
        d->y = tv.vy;
        d->z = tv.vz;
        ++d;
    }
}

void TrsProj::TransformProjectVertex(matrixtdef *m, coorddef *t, sint32 n, coorddef *s, Draw::tVertex *v)
{
    FUNCTION_MARKER(0x800E1CE0, "NFS4.EXE");
    for (sint32 i = 0; i < n; ++i)
    {
        coorddef tmp;
        transform(s, m, &tmp);
        ++s;
        v->sv.x = (sint16)((tmp.x + (uint32)t->x) >> 10);
        v->sv.y = (sint16)((tmp.y + (uint32)t->y) >> 10);
        sint32 z = t->z;
        v->sv.p = 10;
        v->sv.z = (sint16)((tmp.z + (uint32)z) >> 10);
        ++v;
    }
}
