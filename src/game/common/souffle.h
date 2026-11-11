#ifndef SOUFFLE_H
#define SOUFFLE_H

#include "xport.h"
#include "psx.h"
#include "game/common/mathnfs.h"
#include "game/common/renderview.h"

class Souffle
{
public:
    struct tISouffle
    {
        char type;
        char id;
        char wind;
        char cycle;
        sint32 aspeed;
        sint32 angle;
        coorddef source;
        coorddef motion;
        coorddef extramotion;
        SVECTOR trans;
        sint32 rndpixmap;
        sint32 ground;
        sint32 colour;
    };

    static tISouffle *Add(coorddef *soufflept, sint32 type, coorddef *vec, sint32 velVX, sint32 ground, sint32 colour);
    static void DoSouffle(void);
    static sint32 CircleClip(coorddef *pt1, coorddef *pt2, sint32 r);
    static void InsertFacet(DRender::tView *Vi);
    static void InitTrackSouffle(void);
    static void RestartTrackSouffle(void);
    static void KillTrackSouffle(void);
};

void GetGustWind(Souffle::tISouffle *is);
void GetAngleWind(Souffle::tISouffle *is);
void MoveAngleWind(Souffle::tISouffle *is);

extern sint32 gCISouffle;
extern sint32 gTMoveSouffle;
extern sint32 gWindDir;
extern Souffle::tISouffle *gISouffle;

#endif
