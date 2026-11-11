#ifndef HUDPMX_H
#define HUDPMX_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/psx/draw.h"

class HudPmx
{
public:
    struct tShape;
    static tShape gShapes[175];

    struct tUV
    {
        uint8 u0;
        uint8 v0;
        uint16 clut;
    };

    static tUV gHudNumberUV[10];

    static void LoadShape(char *n, tShape *s);
    static void InitTextures(void);
    static void ReleaseFont(void);
};

struct HudPmx::tShape
{
    Draw::tPixMap pixmap;
    sint16 width;
    sint16 height;
};

#endif
