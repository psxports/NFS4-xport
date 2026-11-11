#ifndef FLARE_H
#define FLARE_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"

struct FLARE_PIECE_DEF
{
    sint32 distance;
    sint32 size;
    CVECTOR color;
    char type;
};

struct FLARE_DEF
{
    DVECTOR pos[2];
    DVECTOR oldpos[2];
    uint16 screenData[2][5][5];
    sint32 size;
    FLARE_PIECE_DEF *piece;
    char isDrawn[2];
};

struct Draw::FlareCache
{
    Draw::tCacheHeader head;
};

class Flare
{
public:
    static FLARE_PIECE_DEF SunFlarePieces[9];

    static SVECTOR gSpikes[4];
    static SVECTOR gOct[8];
    static SVECTOR gSmallOct[8];
    static SVECTOR gHex[6];
    static SVECTOR gReflectHex[6];
    static SVECTOR gQuad[4];
    static SVECTOR gLensOct[8];

    struct tInfo
    {
        uint32 chalo;
        uint32 cbeam;
        sint32 scale;
        sint32 flags;
    };

    static tInfo gType[34];

    static void Tri(sint32 *cp, sint32 *p1, sint32 *p2, sint32 otz);
    static void SetMatrix(matrixtdef *m);
    static void IdentMatrix(MATRIX *mtx);
    static void OctFlare(sint32 *center, sint32 otz);
    static void OctFlareSpikes(sint32 *center, sint32 otz);
    static void Spikes(sint32 *center, sint32 otz);
    static void HexFlare(sint32 *center, sint32 otz);
    static void ReflectHexFlare(sint32 *center, sint32 otz);
    static void QuadFlare(sint32 *center, sint32 otz);
    static void CarShapedHalo(sint32 type, COORD16 *ptCenter, COORD16 *pt1, COORD16 *pt2, sint16 flag, sint32 otz, Draw::tCacheHeader *sd);
    static void Halo2(DRender::tView *Vi, sint32 scale, sint32 type, coorddef *fpt, coorddef *fpt2, Draw::FlareCache *);
    static void Halo(DRender::tView *Vi, sint32 scale, sint32 type, coorddef *fpt, Draw::FlareCache *sd);
    static void _2DSpike(sint32 *center, sint32 *end, sint32 otz);
    static void _2DHalo(sint32 x, sint32 y, sint32 scalex, sint32 scaley, sint32 type);
    static void PreCalcHexLightBeam(sint32 *center, sint32 otz);
    static void Quad(sint32 *pt, CVECTOR *color, sint32 otz);
    static void QuadNotTransparent(sint32 *pt, CVECTOR *color, sint32 otz);
    static void QuadRing(sint32 *pt, CVECTOR *color, sint32 otz);
    static void TextureQuad(sint32 *pt, CVECTOR *color, char type, sint32 otz);
    static void SingleColorTex(DVECTOR *xy, CVECTOR *color, sint32 width, sint32 height, char type, sint32 otz);
    static void SingleColorHex(DVECTOR *xy, CVECTOR *color, sint32 width, sint32 height, sint32 otz);
    static void SingleColorOct(DVECTOR *xy, CVECTOR *color, sint32 width, sint32 height, sint32 otz);
    static void SingleColorOctRing(DVECTOR *xy, CVECTOR *color, sint32 width, sint32 height, sint32 otz);
    static void InitLensFlare(void);
    static void LensFlare(DVECTOR *screenPos, Draw::FlareCache *sd);
    static void Sun(SVECTOR *worldPos, Draw::FlareCache *sd);
    static void Moon(SVECTOR *worldPos, Draw::FlareCache *sd);
};

extern FLARE_DEF gFlare_LensFlare;

#endif
