#ifndef RENDERVIEW_H
#define RENDERVIEW_H

#include "xport.h"
#include "game/common/mathnfs.h"

class DRender
{
public:
    struct tCalcView;

    struct tView;
};

struct DRender::tCalcView
{
    coorddef translation;
    matrixtdef mrotation;
    coorddef translationInv;
    matrixtdef mrotationInv;
    matrixtdef mrotationInvRaw;
};

struct DRender::tView
{
    sint32 id;
    sint32 player;
    DRender::tCalcView cview;
};

#endif
