#ifndef HRZSKU_H
#define HRZSKU_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/matrixtdef.h"
#include "game/common/renderview.h"
#include "game/psx/draw.h"
#include "game/psx/trackspec.h"

class Hrz
{
public:
    static CHorizonSpec *gTrackSpec;
    static SVECTOR gLightningPosInSky;
    static CVECTOR gSaveCol[4];

    static void InitSkyColor(void);
    static void InitSky(void);
    static void Init2DRing(void);
    static void GetHorizonPixMap(Draw::tPixMap *p);
    static void InitHorizon(void);
    static void KillHorizon(void);
    static void LightningAddFork(sint8 direction, sint8 column, sint8 row);
    static void CalculateLightning(void);
    static void TextureQuad(DVECTOR *pt, char type, char bright, Draw::DCache *sd);
    static void SetLightingPosInSky(DRender::tView *Vi);
    static void BuildForkLightning(Draw::DCache *sd);
    static void LightningFlicker(sint32 on);
    static void RotProj16(sint32 n, SVECTOR *s, sint32 *z, DVECTOR *p);
    static void SetDitheringPrim(sint32 dither, sint32 otz);
    static void BuildSky(void);
    static void BuildHorizon(DRender::tView *Vi);
};

struct Draw::HorizonCache
{
    Draw::tCacheHeader head;
    DVECTOR scptTop[17];
    DVECTOR scptMidBot[17];
    DVECTOR scptPmxLeft[17];
    DVECTOR scptPmxRight[17];
    sint32 vertexTop[18];
};

struct Draw::SkyCache
{
    Draw::tCacheHeader head;
    DVECTOR pSkyMesh[85];
    sint32 pSkyZ[85];
};

struct Draw::DCache
{
    Draw::tCacheHeader head;
    MATRIX matB;
    MATRIX matNight;
    MATRIX matCop;
    MATRIX identMat;
    sint32 otz;
    VECTOR tVn0;
    VECTOR tVn1;
    VECTOR tVn2;
    VECTOR tVn3;
    sint16 light;
    sint16 doublelayer;
};

void HrzSetPsxMatrix(matrixtdef *m);
void HrzSetPsxTranslation(coorddef *t);

class Sky
{
public:
    static CSkySpec *gTrackSpec;

    static void InitStars(void);
    static void KillStars(void);
    static void RenderStars(Draw::SkyCache *sd, sint32 otz);
};

#endif
