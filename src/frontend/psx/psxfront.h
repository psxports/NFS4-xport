#ifndef PSXFRONT_H
#define PSXFRONT_H

#include "xport.h"
#include "frontend/psx/fetexture.h"
#include "game/common/car.h"
#include "game/psx/font.h"

struct tDrawShapeExtended
{
    sint16 flip_axis;
    sint32 tint[4];
    tTexture::ShapeInfo *custom_shapes;
};

extern tTexture::ShapeInfo *gHelpShapes;
extern Car::tObj *gCarObj[2];

void Init_PSX_FrontEnd(void);
void InitializeSpinningCars(void);
void PSXExitFrontend(void);
void CleanupSpinningCarsMenu(void);
void LoadAllHelpShapes(void);

class PSXFront
{
public:
    static void AllocateDrawMemory(void);
    static void FreeDrawMemory(void);
    static void ResetOverlayGraphics(void);
};

void DrawShapeExtended(sint32 index, sint32 flags, sint32 x, sint32 y, sint32 fade, sint32 abr, tDrawShapeExtended *extra);
void ScaleShapeExtended(sint32 index, sint32 flags, sint32 x, sint32 y, sint32 fade, sint32 abr, tDrawShapeExtended *extra);

void FontUpsideDownBlit(sint32 x, sint32 y, void *src, sint32 u, sint32 v, charactertbl *ch, sint32 colour);
void PSXDrawSquare(sint32 col, sint32 x, sint32 y, sint32 w, sint32 h);
void PSXDrawGouraudSquare(sint32 x, sint32 y, sint32 w, sint32 h, sint32 c1, sint32 c2, sint32 c3, sint32 c4);
void PSXDrawBrightEndLine(sint32 col, sint32 x, sint32 y, sint32 w, sint32 h, sint32 side, sint32 fade, sint32 darksides);
void PSXTransDrawBrightEndLine(sint32 col, sint32 x, sint32 y, sint32 w, sint32 h, sint32 side, sint32 fade, sint32 darksides, sint32 opacity);
void PSXDrawTransSquare(sint32 col, sint32 x, sint32 y, sint32 w, sint32 h, sint16 opacity);
void PSXDrawTransGouraudSquare(sint32 x, sint32 y, sint32 w, sint32 h, sint32 opacity, sint32 c1, sint32 c2, sint32 c3, sint32 c4);

#endif
