#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/globals.h"
#include "frontend/common/feapp.h"
#include "frontend/common/fefades.h"
#include "frontend/common/femenuextended.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetools.h"
#include "frontend/psx/drawshp.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/mmeffect.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/textsys.h"
#include "game/psx/drawc.h"
#include "game/psx/font.h"
#include "game/psx/textpix.h"

static PSX_RECT gHelpPos;

void MenuNFS4::SetHelpPos(PSX_RECT &r)
{
    FUNCTION_MARKER(0x80019FE8, "FRONT.BIN");
    gHelpPos.x = r.x;
    gHelpPos.y = r.y;
    gHelpPos.w = r.w;
    gHelpPos.h = r.h;
}

void MenuNFS4::DrawTextBox(sint32 helpText, PSX_RECT &r, sint32 initialWidth, sint16 drawOffset, sint16 fSelFade, bool drawArrows, bool reflected)
{
    FUNCTION_MARKER(0x8001A018, "FRONT.BIN");
    DRAWENV *drenv = Draw::GetDRAWENV(Draw::gPlayer1View, gFlip);
    tDrawShapeExtended drawFlags;
    drawFlags.tint[0] = CalcFadeVal(0xB54200, 0xBEBE, fSelFade);
    if (reflected)
        drawFlags.tint[0] = CalcFadeVal(0, drawFlags.tint[0], 224 - r.y);
    DrawShape::SubtractNFS4RectEdges(r);
    PSX_RECT temp;
    if (helpText >= 0)
    {
        temp.x = 0;
        temp.y = drenv->clip.y;
        temp.w = 512;
        temp.h = 240;
        Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
        DR_AREA *daprim = reinterpret_cast<DR_AREA *>(cprim->PrimPtr);
        setaddr(daprim, getaddr(cprim->LastPrim));
        cprim->PrimPtr = reinterpret_cast<char *>(daprim + 1);
        setaddr(cprim->LastPrim, daprim);
        SetDrawArea(daprim, &temp);
        FETextRender::SetFont(0);
        char buffer[64];
        sprintf(buffer, "%s", TextSys::Word(helpText));
        s_upper(buffer);
        sint32 textpix = textpixels(buffer);
        sint32 length = (sint32)strlen(buffer);
        textpix = (textpix - length);
        sint32 dist = textpix < initialWidth ? initialWidth : textpix;
        dist = (dist + 25);
        sint32 col = CalcTextFadeSelToHi(textType_FlybyHelp, fSelFade, 0);
        if (reflected)
            col = CalcFadeVal(0, col, 240 - r.y);
        char *word = TextSys::Word(helpText);
        FETextRender::FullTextRGB(word, ((uint16)r.x + (uint16)drawOffset), ((uint16)r.y + 4), col, 0, 0);
        word = TextSys::Word(helpText);
        FETextRender::FullTextRGB(word, (uint32((uint16)r.x) + uint32((uint16)drawOffset) - uint32(dist)), ((uint16)r.y + 4), col, 0, 0);
        if (drawArrows)
        {
            sint32 ypos = r.y + (sint32(r.h) >> 1);
            if (!reflected)
                ypos += 2;
            DrawShapeExtended(10, 0x118, r.x + drawOffset - 10, ypos, 0, 0, &drawFlags);
            DrawShapeExtended(11, 0x118, ((((r.x + drawOffset) + textpix) + 8)), ypos, 0, 0, &drawFlags);
            DrawShapeExtended(10, 0x118, ((((r.x + drawOffset) - dist) - 10)), ypos, 0, 0, &drawFlags);
            DrawShapeExtended(11, 0x118, (((((r.x + drawOffset) - dist) + textpix) + 8)), ypos, 0, 0, &drawFlags);
        }
        temp = r;
        temp.y = (sint16)((uint16)temp.y + (uint16)drenv->clip.y);
        temp.x = (sint16)((uint16)temp.x + 2);
        temp.w = (sint16)((uint16)temp.w - 4);
        daprim = reinterpret_cast<DR_AREA *>(cprim->PrimPtr);
        setaddr(daprim, getaddr(cprim->LastPrim));
        cprim->PrimPtr = reinterpret_cast<char *>(daprim + 1);
        setaddr(cprim->LastPrim, daprim);
        SetDrawArea(daprim, &temp);
    }
    temp = r;
    tTexture::ShapeInfo *shape = &gHelpShapes[30];
    temp.y = (sint16)((uint16)temp.y + 1);
    temp.h = (sint16)((uint16)temp.h - 2);
    temp.w = (sint16)((uint16)temp.w - 1 - (sint32(shape->width) >> 1));
    DrawShapeExtended(30, 8, temp.x + temp.w, temp.y, 0, 0, 0);
    PSXDrawSquare(0, temp.x, temp.y, temp.w, shape->height);
}

tMenuItemGoToMenuNFS4Button::tMenuItemGoToMenuNFS4Button(uint32 textDescription, tMenu *newMenu, void (*OnButtonPress)(tMenuCommand &), sint32 firstFrame, sint32 numFrames) : tMenuItemGoToMenuButton(textDescription, newMenu, OnButtonPress)
{
    FUNCTION_MARKER(0x8001A4AC, "FRONT.BIN");
    fOffset = 14;
    fTransitionVal = 0;
    fTransitionSpeed = 0;
    fEnabledTransitionVal = 0;
    fNumFrames = numFrames;
    fButtonImage = firstFrame;
}

tMenuItemGoToMenuNFS4Button::~tMenuItemGoToMenuNFS4Button(void)
{
    FUNCTION_MARKER(0x8001A514, "FRONT.BIN");
}

void tMenuItemGoToMenuNFS4Button::Draw(sint32 x, sint32 y, bool selected)
{
    FUNCTION_MARKER(0x8001A53C, "FRONT.BIN");
    if (selected || fOffset > 15)
    {
        FETextRender::SetFont(0);
        char buffer[64];
        sprintf(buffer, "%s", TextSys::Word(fTextDescription));
        s_upper(buffer);
        sint32 pixels = textpixels(buffer);
        sint32 length = (sint32)strlen(buffer);
        sint16 measured = (sint16)(uint32(pixels) - uint32(length));
        sint16 dist = measured >= 140 ? (sint16)((uint16)measured + 25) : 165;
        uint16 oldOffset = fOffset;
        fOffset = (sint16)(oldOffset - 2);
        if (!selected)
            fOffset = (sint16)(oldOffset - 4);
        if (fOffset < 0)
            fOffset = (sint16)((uint16)fOffset + (uint16)dist);
    }
    else
        fOffset = 14;
    if ((fFlags & 1) == 0)
        fEnabledTransitionVal = (sint16)((uint16)fEnabledTransitionVal + 12);
    else
        fEnabledTransitionVal = (sint16)((uint16)fEnabledTransitionVal - 12);
    if (fEnabledTransitionVal < 0)
        fEnabledTransitionVal = 0;
    else if (fEnabledTransitionVal > 128)
        fEnabledTransitionVal = 128;
    PSX_RECT rect;
    rect.x = (sint16)x;
    rect.y = (sint16)y;
    rect.h = 11;
    sint32 phase = fTransitionVal < fEnabledTransitionVal ? fTransitionVal : fEnabledTransitionVal;
    rect.w = (sint16)(phase * 115 / 128);
    MenuNFS4::DrawTextBox(fTextDescription, rect, 140, fOffset, fSelFade, false, false);
    if ((fFlags & 0x200) != 0)
    {
        Font::SetBlitter(FontUpsideDownBlit);
        rect.x = (sint16)x;
        rect.y = (sint16)(280 - (uint16)y);
        rect.h = 11;
        phase = fTransitionVal < fEnabledTransitionVal ? fTransitionVal : fEnabledTransitionVal;
        rect.w = (sint16)(phase * 115 / 128);
        MenuNFS4::DrawTextBox(fTextDescription, rect, 140, fOffset, fSelFade, false, true);
        Font::ReSetBlitter();
    }
}

void tMenuItemGoToMenuNFS4Button::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001A7B8, "FRONT.BIN");
    fEnabledTransitionVal = 0;
    fTransitionSpeed = 12;
}

void tMenuItemGoToMenuNFS4Button::TransitionOff(void)
{
    FUNCTION_MARKER(0x8001A7C8, "FRONT.BIN");
    fTransitionSpeed = -12;
}

bool tMenuItemGoToMenuNFS4Button::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x8001A7D4, "FRONT.BIN");
    return fTransitionSpeed == 0;
}

void tMenuItemGoToMenuNFS4Button::UpdateTransition(bool selected)
{
    FUNCTION_MARKER(0x8001A7E0, "FRONT.BIN");
    fTransitionVal = (sint16)((uint16)fTransitionVal + (uint16)fTransitionSpeed);
    if (fTransitionVal < 0)
    {
        fTransitionVal = 0;
        fTransitionSpeed = 0;
    }
    else if (fTransitionVal > 128)
    {
        fTransitionVal = 128;
        fTransitionSpeed = 0;
    }
    tMenuItem::UpdateTransition(selected);
}

tMenuItemNFS4LeftRightChoice::tMenuItemNFS4LeftRightChoice(uint32 textDescription, tListIterator *dataPtr, sint32 firstFrame, sint32 numFrames) : tMenuItemLeftRightChoice(textDescription, dataPtr)
{
    FUNCTION_MARKER(0x8001A83C, "FRONT.BIN");
    fButtonImage = firstFrame;
    fOffset = 14;
    fTransitionVal = 0;
    fTransitionSpeed = 0;
    fNumFrames = numFrames;
}

tMenuItemNFS4LeftRightChoice::~tMenuItemNFS4LeftRightChoice(void)
{
    FUNCTION_MARKER(0x8001A8A0, "FRONT.BIN");
}

void tMenuItemNFS4LeftRightChoice::Draw(sint32 x, sint32 y, bool selected)
{
    FUNCTION_MARKER(0x8001A8C8, "FRONT.BIN");
    if (selected || fOffset > 15)
    {
        FETextRender::SetFont(0);
        sint16 measured = (sint16)textpixels(TextSys::Word(fTextDescription));
        sint16 dist = measured >= 140 ? (sint16)((uint16)measured + 25) : 165;
        uint16 oldOffset = fOffset;
        fOffset = (sint16)(oldOffset - 2);
        if (!selected)
            fOffset = (sint16)(oldOffset - 4);
        if (fOffset < 0)
            fOffset = (sint16)((uint16)fOffset + (uint16)dist);
    }
    else
        fOffset = 14;
    if ((fFlags & 1) == 0)
        fEnabledTransitionVal = (sint16)((uint16)fEnabledTransitionVal + 12);
    else
        fEnabledTransitionVal = (sint16)((uint16)fEnabledTransitionVal - 12);
    if (fEnabledTransitionVal < 0)
        fEnabledTransitionVal = 0;
    else if (fEnabledTransitionVal > 128)
        fEnabledTransitionVal = 128;
    PSX_RECT rect;
    rect.x = (sint16)x;
    rect.y = (sint16)y;
    rect.h = 11;
    sint32 phase = fTransitionVal < fEnabledTransitionVal ? fTransitionVal : fEnabledTransitionVal;
    rect.w = (sint16)(phase * 115 / 128);
    MenuNFS4::DrawTextBox(fTextDescription, rect, 140, fOffset, fSelFade, selected || fOffset != 14, false);
    if ((fFlags & 0x200) != 0)
    {
        Font::SetBlitter(FontUpsideDownBlit);
        rect.x = (sint16)x;
        rect.y = (sint16)(280 - (uint16)y);
        rect.h = 11;
        phase = fTransitionVal < fEnabledTransitionVal ? fTransitionVal : fEnabledTransitionVal;
        rect.w = (sint16)(phase * 115 / 128);
        MenuNFS4::DrawTextBox(fTextDescription, rect, 140, fOffset, fSelFade, selected || fOffset != 14, true);
        Font::ReSetBlitter();
    }
}

void tMenuItemNFS4LeftRightChoice::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001AB5C, "FRONT.BIN");
    fEnabledTransitionVal = 0;
    fTransitionSpeed = 12;
}

void tMenuItemNFS4LeftRightChoice::TransitionOff(void)
{
    FUNCTION_MARKER(0x8001AB6C, "FRONT.BIN");
    fTransitionSpeed = -12;
}

bool tMenuItemNFS4LeftRightChoice::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x8001AB78, "FRONT.BIN");
    return fTransitionSpeed == 0;
}

void tMenuItemNFS4LeftRightChoice::UpdateTransition(bool selected)
{
    FUNCTION_MARKER(0x8001AB84, "FRONT.BIN");
    fTransitionVal = (sint16)((uint16)fTransitionVal + (uint16)fTransitionSpeed);
    if (fTransitionVal < 0)
    {
        fTransitionVal = 0;
        fTransitionSpeed = 0;
    }
    else if (fTransitionVal > 128)
    {
        fTransitionVal = 128;
        fTransitionSpeed = 0;
    }
    tMenuItem::UpdateTransition(selected);
}

tMenuItemOptionsLeftRightChoice::tMenuItemOptionsLeftRightChoice(uint32 textDescription, tListIterator *dataPtr) : tMenuItemLeftRightChoice(textDescription, dataPtr)
{
}

tBlankMenuItemNFS4LeftRightChoice::tBlankMenuItemNFS4LeftRightChoice(uint32 textDescription, tListIterator *dataPtr, sint32 firstFrame, sint32 numFrames) : tMenuItemNFS4LeftRightChoice(textDescription, dataPtr, firstFrame, numFrames)
{
}

void tMenuItemOptionsLeftRightChoice::Draw(sint32 x, sint32 y, bool selected)
{
    FUNCTION_MARKER(0x8001ABE0, "FRONT.BIN");
    tTexture::ShapeInfo *left = &gHelpShapes[41];
    sint32 col = CalcTextFadeSelToHi(textType_Options, fSelFade, 0);
    char *word = TextSys::Word(fTextDescription);
    sint16 textY = (sint16)((uint16)y + 3);
    FETextRender::FullTextRGB(word, ((uint16)x + 148), textY, col, 0, 1);
    sint16 textID = fData->TextValue(kPlayerBoth);
    word = TextSys::Word(textID);
    FETextRender::FullTextRGB(word, ((uint16)x + 217 + (sint32(left->width) >> 1)), textY, col, 0, 2);
    tDrawShapeExtended drawFlags;
    drawFlags.tint[0] = CalcFadeVal(0xB54200, 0xBEBE, fSelFade);
    DrawShapeExtended(10, 0x118, ((x + 166)), (y + 6), 0, selected ? 0 : 1, &drawFlags);
    DrawShapeExtended(11, 0x118, (((x - left->width) + 303)), (y + 6), 0, selected ? 0 : 1, &drawFlags);
    PSX_RECT r;
    r.x = (sint16)x;
    r.y = (sint16)y;
    r.w = 297;
    r.h = left->height;
    DrawShape::NFS4RoundRectangle(-1, r, (sint16)selected);
}

tMenuItemOptionsTwoItemChoice::tMenuItemOptionsTwoItemChoice(uint32 textDescription, tListIterator *dataPtr) : tMenuItemLeftRightChoice(textDescription, dataPtr), fOnOffFade(128)
{
}

void tMenuItemOptionsTwoItemChoice::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001ADB8, "FRONT.BIN");
    fOnOffFade = (uint8)fData->Value(kPlayerBoth) != 0 ? 128 : 0;
}

void tMenuItemOptionsTwoItemChoice::Draw(sint32 x, sint32 y, bool selected)
{
    FUNCTION_MARKER(0x8001AE14, "FRONT.BIN");
    tTexture::ShapeInfo *left = &gHelpShapes[41];
    if ((uint8)fData->Value(kPlayerBoth) != 0)
        fOnOffFade = (sint16)((uint16)fOnOffFade + 64);
    else
        fOnOffFade = (sint16)((uint16)fOnOffFade - 64);
    if (fOnOffFade < 0)
        fOnOffFade = 0;
    else if (fOnOffFade > 128)
        fOnOffFade = 128;
    sint32 Col = CalcTextFadeSelToHi(textType_Options, fSelFade, 0);
    sint32 ColTextOn;
    sint32 ColTextOff;
    CalcOnOffFade(textType_Options, fOnOffFade, fSelFade, 0, ColTextOn, ColTextOff);
    char *word = TextSys::Word(fTextDescription);
    sint16 textY = (sint16)((uint16)y + 3);
    FETextRender::FullTextRGB(word, ((uint16)x + 148), textY, Col, 0, 1);
    word = TextSys::Word(fData->fSelectionList[0]);
    FETextRender::FullTextRGB(word, ((uint16)x + 176), textY, ColTextOff, 0, 0);
    word = TextSys::Word(fData->fSelectionList[1]);
    FETextRender::FullTextRGB(word, ((uint16)x - (uint16)left->width + 294), textY, ColTextOn, 0, 1);
    tDrawShapeExtended drawFlags;
    drawFlags.tint[0] = CalcFadeVal(0xB54200, 0xBEBE, fSelFade);
    DrawShapeExtended(10, 0x118, ((x + 166)), (y + 6), 0, selected ? 0 : 1, &drawFlags);
    DrawShapeExtended(11, 0x118, (((x - left->width) + 303)), (y + 6), 0, selected ? 0 : 1, &drawFlags);
    PSX_RECT r;
    r.x = (sint16)x;
    r.y = (sint16)y;
    r.w = 297;
    r.h = left->height;
    DrawShape::NFS4RoundRectangle(-1, r, (sint16)selected);
}

tMenuNFS4::tMenuNFS4(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, tMenuItem *firstItem, ...) : tMenu(flags, screenHandler, nextMenu, optionsMenu, OnButtonPress, title)
{
    FUNCTION_MARKER(0x8001B098, "FRONT.BIN");
    va_list ap;
    va_start(ap, firstItem);
    tMenuConstructor(firstItem, &ap);
    va_end(ap);
}

tMenuNFS4::tMenuNFS4(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title) : tMenu(flags, screenHandler, nextMenu, optionsMenu, OnButtonPress, title)
{
    FUNCTION_MARKER(0x8001B100, "FRONT.BIN");
}

tMenuNFS4::~tMenuNFS4(void)
{
    FUNCTION_MARKER(0x8001B158, "FRONT.BIN");
}

void tMenuNFS4::Initialize(void)
{
    FUNCTION_MARKER(0x8001B180, "FRONT.BIN");
    tMenu::Initialize();
    fInItemTransition = 0;
    fInMenuTransition = 0;
    fNumItems = 0;
    fLastItem = (char)fCurrentItem;
    while (fItemList[(uint8)fNumItems] != 0)
        fNumItems = (char)((uint8)fNumItems + 1);
    if ((fFlags & 0x200) != 0)
    {
        for (sint16 item = 0; fItemList[item] != 0; ++item)
            fItemList[item]->fFlags |= 0x200;
    }
}

void tMenuNFS4::TransitionOff(void)
{
    FUNCTION_MARKER(0x8001B250, "FRONT.BIN");
    for (sint16 i = 0; fItemList[i] != 0; ++i)
        fItemList[i]->TransitionOff();
}

void tMenuNFS4::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001B2D8, "FRONT.BIN");
    for (sint16 i = 0; fItemList[i] != 0; ++i)
        fItemList[i]->TransitionOn();
}

bool tMenuNFS4::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x8001B360, "FRONT.BIN");
    bool result = true;
    for (sint16 i = 0; fItemList[i] != 0; ++i)
        result = (result & fItemList[i]->TransitionIsFinished()) != 0;
    return result;
}

void tMenuNFS4::UpdateTransition(void)
{
    FUNCTION_MARKER(0x8001B400, "FRONT.BIN");
    for (sint16 i = 0; fItemList[i] != 0; ++i)
        fItemList[i]->UpdateTransition(i == fCurrentItem);
}

void tMenuNFS4::DrawItem(sint32 item)
{
    FUNCTION_MARKER(0x8001B494, "FRONT.BIN");
    fItemList[item]->Draw(10, 18 * item + 43, item == fCurrentItem);
}

void tMenuNFS4::Draw(void)
{
    FUNCTION_MARKER(0x8001B4F4, "FRONT.BIN");
    if (fTitle >= 0)
        FETextRender::Title(fTitle);
    tMenu::Initialize();
    sint32 buttonImage = fItemList[fCurrentItem]->fButtonImage;
    sint32 numFrames = fItemList[fCurrentItem]->fNumFrames;
    if (buttonImage >= 0 && numFrames > 0)
    {
        sint32 index = (sint32)(uint32(buttonImage) + uint32((ticks >> 4) % numFrames));
        tDrawShapeExtended drawFlags;
        drawFlags.tint[0] = 0xCEC844;
        sint32 y = FEApp->fPlayer != 0 ? 121 : 16;
        DrawShapeExtended(index, 0x410, 16, y, 0, 0, &drawFlags);
    }
    for (sint16 i = 0; fItemList[i] != 0; ++i)
        DrawItem(i);
}

tMenuNFS4TwoPlayer::tMenuNFS4TwoPlayer(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, tMenuItem *firstItem, ...) : tMenuNFS4(flags, screenHandler, nextMenu, optionsMenu, OnButtonPress, title)
{
    FUNCTION_MARKER(0x8001B63C, "FRONT.BIN");
    fChildMenu = 0;
    va_list ap;
    va_start(ap, firstItem);
    tMenuConstructor(firstItem, &ap);
    va_end(ap);
}

tMenuNFS4TwoPlayer::~tMenuNFS4TwoPlayer(void)
{
    FUNCTION_MARKER(0x8001B6A8, "FRONT.BIN");
}

void tMenuNFS4TwoPlayer::DrawItem(sint32 item)
{
    FUNCTION_MARKER(0x8001B6D0, "FRONT.BIN");
    sint16 y = FEApp->fPlayer == 1 ? 148 : 43;
    fItemList[item]->Draw(10, y + 18 * item, item == fCurrentItem);
}

tMenuNFS4Bottom::tMenuNFS4Bottom(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, tMenuItem *firstItem, ...) : tMenuNFS4(flags, screenHandler, nextMenu, optionsMenu, OnButtonPress, title)
{
    FUNCTION_MARKER(0x8001B748, "FRONT.BIN");
    va_list ap;
    va_start(ap, firstItem);
    tMenuConstructor(firstItem, &ap);
    va_end(ap);
}

tMenuNFS4Bottom::~tMenuNFS4Bottom(void)
{
    FUNCTION_MARKER(0x8001B7B0, "FRONT.BIN");
}

void tMenuNFS4Bottom::Draw(void)
{
    FUNCTION_MARKER(0x8001B7D8, "FRONT.BIN");
    PSX_RECT r;
    r.x = 57;
    r.y = 198;
    r.w = 114;
    r.h = 11;
    MenuNFS4::SetHelpPos(r);
    for (sint16 i = 0; fItemList[i] != 0; ++i)
        fItemList[i]->Draw(0, 0, i == fCurrentItem);
}

tMenuBlank::tMenuBlank(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title) : tMenuNFS4(flags, screenHandler, nextMenu, optionsMenu, OnButtonPress, title)
{
    FUNCTION_MARKER(0x8001B898, "FRONT.BIN");
    fNeverAnyEnabled = true;
    VertHelp = 0;
}

tMenuBlank::~tMenuBlank(void)
{
    FUNCTION_MARKER(0x8001B8FC, "FRONT.BIN");
}

void tMenuBlank::Draw(void)
{
    FUNCTION_MARKER(0x8001B924, "FRONT.BIN");
    if (fInMenuTransition != 0)
    {
        TransitionIsFinished();
        fTransitionVal = (sint16)((uint16)fTransitionVal + fTransitionDirection);
    }
}

void tMenuBlank::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x8001B988, "FRONT.BIN");
    if (keyval == kInput_KeyType_Up || keyval == kInput_KeyType_Down)
        keyval = kInput_KeyType_AlreadyProcessed;
    else
    {
        fItemList[0] = 0;
        tMenu::ProcessInput(fromPlayer, keyval, command);
    }
}

void tMenuBlank::Initialize(void)
{
    FUNCTION_MARKER(0x8001B9CC, "FRONT.BIN");
}

sint32 tMenuBlank::DebounceKeys(void)
{
    FUNCTION_MARKER(0x8001B9D4, "FRONT.BIN");
    return -1;
}

void tMenuBlank::TransitionOff(void)
{
    FUNCTION_MARKER(0x8001B9DC, "FRONT.BIN");
    fTransitionDirection = 8;
    fInMenuTransition = 1;
    fTransitionVal = -112;
}

void tMenuBlank::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001B9F8, "FRONT.BIN");
    fTransitionDirection = -8;
    fInMenuTransition = 1;
    fTransitionVal = 0;
}

bool tMenuBlank::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x8001BA10, "FRONT.BIN");
    if (fTransitionDirection > 0)
        fInMenuTransition = fTransitionVal < 0 ? 1 : 0;
    else
        fInMenuTransition = fTransitionVal >= -111 ? 1 : 0;
    return fInMenuTransition == 0;
}

tMenuOptions::tMenuOptions(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, sint16 player, tMenuItem *firstItem, ...) : tMenuNFS4(flags, screenHandler, nextMenu, optionsMenu, OnButtonPress, title)
{
    FUNCTION_MARKER(0x8001BA4C, "FRONT.BIN");
    va_list ap;
    va_start(ap, firstItem);
    tMenuConstructor(firstItem, &ap);
    va_end(ap);
    fPlayer = player;
}

tMenuOptions::~tMenuOptions(void)
{
    FUNCTION_MARKER(0x8001BAC4, "FRONT.BIN");
}

void tMenuOptions::Draw(void)
{
    FUNCTION_MARKER(0x8001BAEC, "FRONT.BIN");
    sint16 numItems = GetNumberEnabledItems();
    TransitionIsFinished();
    sint32 w = 320;
    sint32 h = numItems * 18;
    if (fInMenuTransition != 0)
    {
        uint32 deltaTicks = uint32(ticks) - fMenuEnterTicks;
        if (deltaTicks > 32)
        {
            deltaTicks = 32;
            fInMenuTransition = 0;
            if (fTransitionDirection < 0)
                return;
        }
        sint32 scaledWidth = (uint32(w) * deltaTicks) >> 5;
        sint32 scaledHeight = (uint32(h) * deltaTicks) >> 5;
        if (fTransitionDirection < 0)
        {
            w -= scaledWidth;
            h -= scaledHeight;
        }
        else
        {
            w = scaledWidth;
            h = scaledHeight;
        }
    }
    else if (fTransitionDirection < 0)
        return;
    h += 18;
    sint32 remaining = 240 - h;
    sint32 y = remaining >> 1;
    sint32 x = (screenwidth - w) >> 1;
    if (fPlayer == 0)
        y -= remaining >> 2;
    else if (fPlayer == 1)
        y += remaining >> 2;
    if (fInMenuTransition == 0)
    {
        if (fTitle >= 0)
            FETextRender::MenuTextPositionedJustify(fTitle, (x + (w >> 1)), (y + 2), 2, textState_Hilighted, textType_PopUpTitle);
        sint32 itemY = y + 18;
        for (sint16 i = 0; fItemList[i] != 0; ++i)
        {
            if ((fItemList[i]->fFlags & 1) == 0)
            {
                fItemList[i]->Draw(x + 10, itemY, i == fCurrentItem);
                itemY += 18;
            }
        }
    }
    PSXDrawSquare(0, x, y, w, 12);
    PSXDrawTransSquare(0, x, y + h, w, -2, 1);
    PSXDrawTransSquare(0, x, y + 12, 4, h - 14, 1);
    PSXDrawTransSquare(0, x + w, y + 12, -4, h - 14, 1);
    PSXDrawTransSquare(0, x, y, w, h, 1);
    FeDraw_SetABRMode(0);
}

void tMenuOptions::TransitionOff(void)
{
    FUNCTION_MARKER(0x8001BDC4, "FRONT.BIN");
    fTransitionDirection = -1;
    uint32 tick = uint32(ticks);
    fInMenuTransition = 1;
    fMenuEnterTicks = tick;
    AudioCmn::PlayFESFX(18);
}

void tMenuOptions::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001BE00, "FRONT.BIN");
    for (sint16 i = 0; fItemList[i] != 0; ++i)
        if ((fItemList[i]->fFlags & 1) == 0)
            fItemList[i]->TransitionOn();
    fTransitionDirection = 1;
    uint32 tick = uint32(ticks);
    fInMenuTransition = 1;
    fMenuEnterTicks = tick;
    AudioCmn::PlayFESFX(15);
}

bool tMenuOptions::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x8001BE98, "FRONT.BIN");
    fInMenuTransition = uint32(ticks) - fMenuEnterTicks < 32 ? 1 : 0;
    return fInMenuTransition == 0;
}

void tMenuOptions::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x8001BEBC, "FRONT.BIN");
    if (keyval == kInput_KeyType_Square)
        keyval = kInput_KeyType_Triangle;
    tMenuNFS4::ProcessInput(fromPlayer, keyval, command);
}

bool tMenuOptions::IsSubMenu(void)
{
    FUNCTION_MARKER(0x8001BEF0, "FRONT.BIN");
    return true;
}
