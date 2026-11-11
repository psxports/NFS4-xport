#ifndef GENERICPMX_H
#define GENERICPMX_H

#include "game/psx/draw.h"

class GenericPMX
{
public:
    static void LoadTexture(void);
};

extern Draw::tPixMap *gSMokePalette;
extern Draw::tPixMap *gDirtPalette;
extern Draw::tPixMap *gGrassPalette;
extern Draw::tPixMap *gSnowPalette;
extern Draw::tPixMap *gLeafPixmap;
extern Draw::tPixMap *gDPixmap;
extern Draw::tPixMap *gSpikeBeltPixmap;
extern Draw::tPixMap *gGravelPalette;
extern Draw::tPixMap *gDamagePixmap;
extern Draw::tPixMap *gSMokePixmap[2];
extern Draw::tPixMap *gSparkHPixmap[6];
extern Draw::tPixMap *gGravelPixmap[2];
extern Draw::tPixMap *gDLPixmap[10];
extern Draw::tPixMap *gWeatherPixmap[3];
extern Draw::tPixMap *gFlarePixmap[3];
extern Draw::tPixMap *gLightningPixmap[16];
extern Draw::tPixMap *gShadowPixmap[2];
extern Draw::tPixMap *gSkidMarkPixmap[2];
extern Draw::tPixMap gPixmaps[71];

#endif
