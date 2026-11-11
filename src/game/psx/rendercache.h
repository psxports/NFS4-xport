#ifndef RENDERCACHE_H
#define RENDERCACHE_H

#include <stddef.h>
#include "game/psx/drawc.h"
#include "game/psx/flare.h"
#include "game/psx/hrzsku.h"
#include "game/psx/night.h"
#include "game/psx/sfx.h"

union NativeRenderCache
{
    Draw::CarCache car;
    Draw::tGiveShelbyMoreCache world;
    Sfx::tCache sfx;
    tNightInitCache night;
    Draw::SkyCache sky;
    Draw::HorizonCache horizon;
    Draw::DCache horizonDraw;
    Draw::FlareCache flare;
};

extern NativeRenderCache gNativeRenderCache;

#endif
