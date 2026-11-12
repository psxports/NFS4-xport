#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/globals.h"
#include "frontend/common/fefades.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetv.h"
#include "frontend/common/screencarselect.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/psxfront.h"
#include "game/common/textsys.h"

void tVideoWall::Initialize(tTVConfig *tvs, tTexture::ShapeInfo *shapes, sint16 firstTV, sint16 numTVs, sint16 *tvOrdering, sint16 flip_axis)
{
    FUNCTION_MARKER(0x80036790, "FRONT.BIN");
    fTVs = tvs;
    fFirstTVShape = firstTV;
    fTVShapes = shapes;
    fOffsetY = 0;
    fOffsetX = 0;
    fTransitionDirection = 0;
    fAvailableTextID = -1;
    fNumTVs = numTVs;
    uint32 tick = uint32(ticks);
    fAvailable = 1;
    fAvailableBright = 0;
    fValid = 1;
    fIconShapes = 0;
    tvOrder = tvOrdering;
    fTVTicks = tick;
    fFlipAxis = flip_axis;
    for (sint16 i = 0; i < fNumTVs; ++i)
        fTVs[i].state = tv_StateOff;
    fUpdated = 0;
}

void tVideoWall::UpdateImages(void)
{
    FUNCTION_MARKER(0x80036840, "FRONT.BIN");
    for (sint16 i = 0; i < fNumTVs; ++i)
    {
        InitTV(fTVs[i], fTVShapes, (sint16)((uint16)fFirstTVShape + (uint16)i));
        fTVs[i].x = (sint16)((uint16)fTVs[i].x + (uint16)fOffsetX);
        fTVs[i].y = (sint16)((uint16)fTVs[i].y + (uint16)fOffsetY);
        if (fFlipAxis > 0)
        {
            fTVs[i].flags |= 4;
            fTVs[i].flip_axis = fFlipAxis;
        }
        if (fValid == 0 || fAvailable == 0)
            fTVs[i].flags |= 0x28;
    }
    fUpdated = 1;
}

void tVideoWall::SetAvailableText(sint16 textID, sint16 x, sint16 y)
{
    FUNCTION_MARKER(0x8003698C, "FRONT.BIN");
    fAvailableTextID = textID;
    fAvailableX = x;
    fAvailableY = y;
    fAvailableBright = 0;
    fAvailable = 1;
}

void tVideoWall::SetAvailableIcon(sint16 icon, sint16 numFrames, sint16 x, sint16 y, tTexture::ShapeInfo *iconImages)
{
    FUNCTION_MARKER(0x800369A8, "FRONT.BIN");
    fIcon = icon;
    fIconFrames = numFrames;
    fIconX = x;
    fIconShapes = iconImages;
    fIconY = y;
}

void tVideoWall::SetOffset(sint16 x, sint16 y)
{
    FUNCTION_MARKER(0x800369C8, "FRONT.BIN");
    fOffsetX = x;
    fOffsetY = y;
}

void tVideoWall::SetAvailable(sint16 avail)
{
    FUNCTION_MARKER(0x800369D4, "FRONT.BIN");
    fAvailable = avail;
}

void tVideoWall::SetValid(sint16 valid)
{
    FUNCTION_MARKER(0x800369DC, "FRONT.BIN");
    fValid = valid;
}

void tVideoWall::UpdateTransition(void)
{
    FUNCTION_MARKER(0x800369E4, "FRONT.BIN");
    sint16 j = (sint16)((uint32(ticks) - fTVTicks) >> 3);
    if (fTransitionDirection > 0)
    {
        if (fValid == 0)
            fTVTicks = uint32(ticks);
        else
        {
            for (sint16 i = 0; i < j; ++i)
            {
                if (i >= fNumTVs)
                    return;
                if (fTVs[tvOrder[i]].state == tv_StateOff && fValid != 0 && fAvailable != 0)
                    TurnOnTV(fTVs[tvOrder[i]]);
            }
        }
    }
    else
    {
        for (sint16 i = 0; i < j; ++i)
        {
            if (i >= fNumTVs)
                break;
            if (fTVs[tvOrder[i]].state == tv_StateOn)
                TurnOffTV(fTVs[tvOrder[i]]);
        }
        if (j >= fNumTVs)
            fTransitionDirection = 0;
    }
}

void tVideoWall::Draw(void)
{
    FUNCTION_MARKER(0x80036BB4, "FRONT.BIN");
    fAvailableBright = (sint16)((uint16)fAvailableBright + (fAvailable == 0 && fValid != 0 ? 4 : -4));
    if (fAvailableBright > 64)
        fAvailableBright = 64;
    else if (fAvailableBright < 0)
        fAvailableBright = 0;
    if (fValid == 0 || fAvailable == 0 || fAvailableBright != 0)
    {
        for (sint16 i = 0; i < fNumTVs; ++i)
            DrawTVLines(fTVs[i]);
        if (fAvailableBright > 0)
        {
            sint32 textColor = CalcFadeVal(0xBEBE, 128 - fAvailableBright);
            if (fIconShapes != 0 && fIconFrames > 0)
            {
                tDrawShapeExtended drawFlags;
                drawFlags.tint[0] = 0xBEBE;
                drawFlags.custom_shapes = fIconShapes;
                sint32 index = fIcon + (ticks >> 4) % fIconFrames;
                DrawShapeExtended(index, 0x611, fIconX, fIconY, 128 - fAvailableBright, 1, &drawFlags);
            }
            if (fAvailableTextID >= 0)
            {
                FETextRender::SetABR(1, true);
                char *word = TextSys::Word(fAvailableTextID);
                FETextRender::FullTextRGB(word, fAvailableX, fAvailableY, textColor, 3, 2);
                FETextRender::SetABR(0, false);
            }
        }
    }
    if (fUpdated != 0 || fValid == 0 || fAvailable == 0)
        for (sint16 i = 0; i < fNumTVs; ++i)
            DrawTV(fTVs[i]);
}

void tVideoWall::TurnOff(void)
{
    FUNCTION_MARKER(0x80036E58, "FRONT.BIN");
    if (fTransitionDirection != -1)
    {
        uint32 tick = uint32(ticks);
        fTransitionDirection = -1;
        fTVTicks = tick;
    }
}

void tVideoWall::TurnOffInstant(void)
{
    FUNCTION_MARKER(0x80036E7C, "FRONT.BIN");
    TurnOff();
    for (sint16 i = 0; i < fNumTVs; ++i)
        TurnOffTV(fTVs[i]);
}

void tVideoWall::TurnOn(void)
{
    FUNCTION_MARKER(0x80036EF8, "FRONT.BIN");
    if (fTransitionDirection != 1)
    {
        uint32 tick = uint32(ticks);
        fTransitionDirection = 1;
        fTVTicks = tick;
    }
}
