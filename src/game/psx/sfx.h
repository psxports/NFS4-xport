#ifndef SFX_H
#define SFX_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/renderview.h"
#include "game/common/souffle.h"
#include "game/psx/draw.h"

struct sfxsouffle;

class Sfx
{
public:
    struct tCache
    {
        Draw::tCacheHeader head;
        sint32 otz;
    };

    static void Transform(coorddef *worldpt, SVECTOR *campt, coorddef *t);
    static void BuildSmokeFacet(Souffle::tISouffle *is, sfxsouffle *dSouffle, Draw::tPixMap *cpixmap);
    static void ThickenXZ(SVECTOR *d, coorddef *pt1, coorddef *pt2, coorddef *cp);
    static void BuildFastDisolveFacet(Souffle::tISouffle *is, sfxsouffle *dSouffle, Draw::tPixMap *t, Draw::tPixMap *c);
    static void AdditivePrim(Draw::tPixMap *pmx, SVECTOR *pt, sint32 mode, sint32 offset, tCache *sd);
    static void BuildSouffleFacet(DRender::tView *Vi, Souffle::tISouffle *is);
    static void Add(Souffle::tISouffle *is);
};

#endif
