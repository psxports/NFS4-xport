#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "frontend/common/fetextrender.h"
#include "frontend/psx/drawshp.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/mmeffect.h"
#include "frontend/psx/psxfront.h"
#include "game/psx/drawc.h"

sint32 kNoColor = 8421504;

void DrawShape::SubtractNFS4RectEdges(PSX_RECT &rect)
{
    FUNCTION_MARKER(0x8004BE00, "FRONT.BIN");
    sint16 y1 = (sint16)((uint16)rect.y + 1);
    sint16 y2 = (sint16)((uint16)rect.y + (uint16)rect.h);
    sint16 x1 = (sint16)((uint16)rect.x + 2);
    sint16 x2 = (sint16)((uint16)rect.x + (sint32(rect.w) >> 3));
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    for (sint16 i = 0; i < 2; ++i)
    {
        POLY_G4 *prim = reinterpret_cast<POLY_G4 *>(cprim->PrimPtr);
        setaddr(prim, getaddr(cprim->LastPrim));
        cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
        setaddr(cprim->LastPrim, prim);
        uint32 color = 0x808080;
        memcpy(&prim->p[0].rgbc, &color, sizeof(color));
        setcode(prim, 0x3A);
        setlen(prim, 8);
        prim->p[0].xy.y = (sint16)((uint16)y1 + 2);
        memcpy(&prim->p[2].rgbc, &color, sizeof(color));
        color = 0;
        memcpy(&prim->p[3].rgbc, &color, sizeof(color));
        memcpy(&prim->p[1].rgbc, &color, sizeof(color));
        prim->p[0].xy.x = x1;
        prim->p[1].xy.x = x2;
        prim->p[1].xy.y = y1;
        prim->p[2].xy.x = x1;
        prim->p[2].xy.y = (sint16)((uint16)y2 - 2);
        prim->p[3].xy.x = x2;
        prim->p[3].xy.y = y2;
        x1 = (sint16)((uint16)rect.x + (uint16)rect.w - 2);
        x2 = (sint16)((uint16)x1 - (sint32(rect.w) >> 3));
    }
    DR_MODE *dr_mode = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
    setaddr(dr_mode, getaddr(cprim->LastPrim));
    cprim->PrimPtr = reinterpret_cast<char *>(dr_mode + 1);
    setaddr(cprim->LastPrim, dr_mode);
    uint16 page = GetTPage(2, 2, 0, 256);
    SetDrawMode(dr_mode, 0, 0, page, 0);
}

void DrawShape::NFS4RoundRectangle(sint32 textID, PSX_RECT &position, sint16 selected)
{
    FUNCTION_MARKER(0x8004BFB0, "FRONT.BIN");
    tTexture::ShapeInfo *left = &gHelpShapes[41];
    tDrawShapeExtended drawFlags;
    if (textID >= 0)
        FETextRender::MenuTextPositionedJustify((sint16)textID, (sint16)((uint16)position.x + (sint32(position.w) >> 1)), (sint16)((uint16)position.y + 4), 2, textState_Selected, textType_FramedInfo);
    sint16 flags = selected == 0 ? 1 : 0;
    DrawShapeExtended(41, flags | 8, position.x, position.y, 0, 0, &drawFlags);
    DrawShapeExtended(41, flags | 12, position.x + position.w - left->width, position.y, 0, 0, &drawFlags);
    if (selected != 0)
    {
        PSXDrawSquare(0x841D08, position.x + left->width, position.y + left->height - 1, position.w - 2 * left->width + 1, 1);
        PSXDrawSquare(0, position.x + left->width, position.y, position.w - 2 * left->width + 1, left->height - 1);
    }
    else
    {
        PSXDrawTransSquare(0x841D08, position.x + left->width, position.y + left->height - 1, position.w - 2 * left->width + 1, 1, 1);
        PSXDrawTransSquare(0, position.x + left->width, position.y, position.w - 2 * left->width + 1, left->height - 1, 1);
        FeDraw_SetABRMode(0);
    }
}

void DrawShape::NFS4Rectangle(PSX_RECT &position)
{
    FUNCTION_MARKER(0x8004C1BC, "FRONT.BIN");
    tTexture::ShapeInfo *topleft = &gHelpShapes[42];
    tTexture::ShapeInfo *topright = &gHelpShapes[43];
    tTexture::ShapeInfo *bottomleft = &gHelpShapes[44];
    tTexture::ShapeInfo *bottomright = &gHelpShapes[45];
    tDrawShapeExtended drawFlags;
    drawFlags.tint[1] = 0x150800;
    drawFlags.tint[3] = 0x4A1900;
    drawFlags.tint[0] = 0x7B2908;
    drawFlags.tint[2] = 0x291000;
    DrawShapeExtended(42, 9, position.x, position.y, 0, 0, &drawFlags);
    DrawShapeExtended(43, 9, position.x + position.w - topright->width, position.y, 0, 0, &drawFlags);
    DrawShapeExtended(44, 9, position.x, position.y + position.h - bottomleft->height, 0, 0, &drawFlags);
    DrawShapeExtended(45, 9, position.x + position.w - bottomright->width, position.y + position.h - bottomright->height, 0, 0, &drawFlags);
    PSXDrawTransSquare(drawFlags.tint[1], position.x + topleft->width, position.y, position.w - topleft->width - topright->width, 1, 1);
    PSXDrawTransSquare(drawFlags.tint[0], position.x + bottomleft->width, position.y + position.h - 1, position.w - bottomleft->width - bottomright->width, 1, 1);
    PSXDrawTransGouraudSquare(position.x, position.y + topleft->height, 2, position.h - topleft->height - bottomleft->height, 1, drawFlags.tint[2], drawFlags.tint[2], drawFlags.tint[3], drawFlags.tint[3]);
    PSXDrawTransGouraudSquare(position.x + position.w - 2, position.y + topright->height, 2, position.h - topright->height - bottomright->height, 1, drawFlags.tint[2], drawFlags.tint[2], drawFlags.tint[3], drawFlags.tint[3]);
    PSXDrawTransSquare(0, position.x + 2, position.y + topleft->height, topleft->width - 2, position.h - topleft->height - bottomleft->height, 1);
    PSXDrawTransSquare(0, position.x + position.w - 2, position.y + topright->height, 2 - topright->width, position.h - topright->height - bottomright->height, 1);
    PSXDrawTransSquare(0, position.x + topleft->width, position.y + 1, position.w - topleft->width - topright->width, position.h - 2, 1);
    FeDraw_SetABRMode(0);
}

void DrawShape::NFS4TransRectangle(PSX_RECT &r, sint16 opacity)
{
    FUNCTION_MARKER(0x8004C4DC, "FRONT.BIN");
    sint16 borderOpacity = (sint16)((uint16)opacity * 2);
    PSXDrawTransSquare(0, r.x, r.y, 4, r.h, borderOpacity);
    PSXDrawTransSquare(0, r.x + r.w - 4, r.y, 4, r.h, borderOpacity);
    PSXDrawTransSquare(0, r.x + 4, r.y, r.w - 8, 2, borderOpacity);
    PSXDrawTransSquare(0, r.x + 4, r.y + r.h - 2, r.w - 8, 2, borderOpacity);
    PSXDrawTransSquare(0, r.x + 4, r.y + 2, r.w - 8, r.h - 4, opacity);
    FeDraw_SetABRMode(0);
}
