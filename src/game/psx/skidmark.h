#ifndef SKIDMARK_H
#define SKIDMARK_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/mathnfs.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"

struct tSkid;

class Skidmark
{
public:
    struct Segment
    {
        SVECTOR svx[2];
        CVECTOR rgb;
        sint32 type;
        Segment *next;
    };

    struct Chunk;

    static Chunk * CheckChunk(coorddef *skidpt, sint32 newsegs, sint32 slice);
    static void Add(tSkid *prevskid, coorddef *skidpt, CVECTOR *color, sint32 tireWidth, sint32 type, sint32 slice);
    static void AddStretch(Segment **save, sint32 *savechunk, tSkid *prevskid, coorddef *skidpt, CVECTOR *color, sint32 tireWidth, sint32 type, sint32 slice);
    static void Stretch(Segment *save, sint32 savechunk, tSkid *prevskid, coorddef *skidpt, CVECTOR *color, sint32 tireWidth, sint32 type);
    static void EndStretch(Segment *save, sint32 savechunk, tSkid *prevskid, coorddef *skidpt, CVECTOR *color, sint32 tireWidth, sint32 type);
    static void OnyxBuildFacets(DRender::tView *Vi);
    static void InitSkidmark(void);
    static void RestartSkidmark(void);
    static void KillSkidmark(void);
};

struct Skidmark::Chunk
{
    coorddef cp;
    sint16 n;
    sint16 slice;
    Skidmark::Segment seg[24];
};

struct Draw::tCtrlSkidmark
{
    matrixtdef m;
    coorddef t;
    sint32 count;
    Skidmark::Chunk *smp;
};

#endif
