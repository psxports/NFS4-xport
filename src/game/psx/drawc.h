#ifndef DRAWC_H
#define DRAWC_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"
#include "game/common/matrixtdef.h"
#include "game/common/transformer.h"
#include "game/psx/draw.h"
#include "game/psx/draww.h"

struct Draw::CarVertex
{
    COORD16 vt;
    char u;
    char v;
};

struct Draw::CarCache
{
    Draw::tCacheHeader head;
    MATRIX matB;
    sint32 pad;
    uint32 *sub_ot;
    sint32 otz;
    sint32 sub_otz;
    sint32 bfct;
    DR_MODE drawModeOn;
    DR_MODE drawModeOff;
    Draw::tPixMap *pmxStart;
    Draw::tPixMap ePmx0;
    Draw::tPixMap ePmx1;
    sint32 eAddZ;
    sint32 sub_otSize;
    sint32 color;
    sint32 eColor0;
    sint32 eColor1;
    sint32 eColor2;
    VECTOR tv;
    COORD16 vt0;
    uint8 u0;
    uint8 v0;
    COORD16 vt1;
    uint8 u1;
    uint8 v1;
    COORD16 vt2;
    uint8 u2;
    uint8 v2;
    sint16 dvx0;
    sint16 dvy0;
    sint16 dvx1;
    sint16 dvy1;
    sint16 dvx2;
    sint16 dvy2;
    Draw::CarVertex tV[96];
    COORD16 vt3;
    uint8 u3;
    uint8 v3;
    COORD16 vt4;
    uint8 u4;
    uint8 v4;
    COORD16 vt5;
    uint8 u5;
    uint8 v5;
    COORD16 vt6;
    uint8 offsetU0;
    uint8 offsetV0;
    COORD16 vt7;
    uint8 offsetU1;
    uint8 offsetV1;
    COORD16 vt8;
    uint8 offsetU2;
    uint8 offsetV2;
};

class DrawC
{
public:
    static Draw::CarCache &carCache;
    static MATRIX gScreenMat;
    static MATRIX gMatA;

    static sint32 gEnvMapMax;
    static sint32 gShadowMax;
    static sint32 gMenuLights;
    static sint32 gMenuLightsDirection;
    static sint32 gWetRoad;
    static sint16 gReflectOffset;
    static sint16 gOverlay[51];
    static sint16 SirenFlash[16];
    static sint16 HeadLightFlash[16];
    static sint16 TailLightFlash[16];
    static sint32 gMenuColor[2];
    static sint16 gEnvMapOffset[4];

    struct tEnvMap

    {
        sint16 slice;

        sint16 tex;

        sint16 extra;
    };

    static tEnvMap *gEnvMap;
    static tEnvMap *gShadow;

    static void SetEnviroment(void);
    static void ReadLightingData(void);
    static void ReadeMapData(void);
    static void BuildRenderingData(void);
    static void KillRenderingData(void);
    static void NightHeadlight(Car::tObj *carObj);
    static void MenuColorData(sint32 color, Car::tObj *carObj, sint32 player);
    static sint32 PrimStart(Draw::tVertex *center, Car::tObj *carObj, sint32 lightAvg, Draw::CarCache *sd);
    static void PrimStop(Car::tObj *carObj, Draw::CarCache *sd);
    static void Prim(matrixtdef *m, coorddef *t, Transformer::zObj *obj, Transformer::zOverlay *overlay, sint32 envmap, Draw::CarCache *sd);
    static void DividePrim(COORD16 *vt0, COORD16 *vt1, COORD16 *vt2, uint16 *u0, uint16 *u1, uint16 *u2, Draw::tPixMap *pmx, Draw::CarCache *sd);
    static void PrimClip(matrixtdef *m, coorddef *t, Transformer::zObj *obj, Transformer::zOverlay *overlay, sint32 envmap, Draw::CarCache *sd);
    static void PrimMenu(matrixtdef *m, coorddef *t, Transformer::zObj *obj, Transformer::zOverlay *overlay, sint32 envmap, Draw::CarCache *sd);
    static void PrimHalo(matrixtdef *m, coorddef *t, Transformer::zObj *obj, sint32 type, sint32 index, sint32 reflect, Draw::CarCache *sd);
    static void ShadowPrim(Draw::tVertex *shadowVT, Draw::CarCache *sd);
    static void DivideShadowPrim(COORD16 *vt0, COORD16 *vt1, COORD16 *vt2, COORD16 *vt3, uint16 *u0, uint16 *u1, uint16 *u2, uint16 *u3, Draw::tPixMap *pmx, Draw::CarCache *sd);
    static void ShadowPrimClip(Draw::tVertex *shadowVT, Draw::CarCache *sd);
    static void SpotPrims(matrixtdef *m, coorddef *t, Draw::CarCache *sd);
    static void ShowroomPrims(matrixtdef *m, coorddef *t, Draw::CarCache *sd);
};

void ChangeTPage(uint16 *tpage, sint32 nabr);

#endif
