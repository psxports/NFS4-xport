#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/feapp.h"
#include "frontend/common/feaudio.h"
#include "frontend/common/fecheats.h"
#include "frontend/common/fefades.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/femenuoptions.h"
#include "frontend/common/fescreen.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/front.h"
#include "frontend/common/screenaudio.h"
#include "frontend/common/screencontroller.h"
#include "frontend/common/screenusername.h"
#include "frontend/common/stattool.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/audiomus.h"
#include "game/common/nfs3.h"
#include "game/common/textsys.h"
#include "game/psx/drawc.h"
#include "game/psx/flare.h"

static char frontMenuMusicBank[] = "amus";
static char frontMenuMusicPattern[] = "zmenu*";
static char frontGameMusicBank[] = "ymus";
static char frontGameMusicPattern[] = "game*";
static char frontAllMusicPattern[] = "*";
static sint32 flareextra;
sint32 PulsateYellow;
sint32 fHelpText;

void CalcPulsateYellow(void)
{
    FUNCTION_MARKER(0x8001BF48, "FRONT.BIN");
    sint32 pulsateval = ticks % 128;
    if (pulsateval > 64)
        pulsateval = 128 - pulsateval;
    PulsateYellow = CalcFadeVal(0xBEBE, pulsateval);
}

void DrawLeftFlare(sint32 y, sint32 fSelFade, sint32 fFadeVal, sint32 &flareextra)
{
    FUNCTION_MARKER(0x8001BFA0, "FRONT.BIN");
    if (fSelFade == 128)
        flareextra = (flareextra + 1);
    else if (flareextra != 0)
        flareextra = (flareextra + 5);
    if (flareextra >= 61)
        flareextra = 0;
    sint32 phase = flareextra < 31 ? flareextra : 60 - flareextra;
    uint32 product = uint32(phase / 2 + 20) * uint32(fSelFade);
    product *= 128u - uint32(fFadeVal);
    sint32 flare_intensity = (sint32)product / 128;
    sint32 glintFade = (128 - fSelFade);
    if (glintFade < fFadeVal)
        glintFade = fFadeVal;
    if (flare_intensity > 0)
    {
        sint32 x = TextSys::WordX(0x1DE);
        Flare::_2DHalo(x, (y + 5), flare_intensity / 2, (flare_intensity * 2) / 3, 23);
        DrawShapeExtended(0, 0, ((x - 3)), (y - 1), glintFade, glintFade != 0 ? 1 : 0, 0);
    }
}

void SubtractiveBox(sint32 x, sint32 y, sint32 w, sint32 h, sint32 col1, sint32 col2, sint32 col3, sint32 col4)
{
    FUNCTION_MARKER(0x8001C0FC, "FRONT.BIN");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_G4 *prim = reinterpret_cast<POLY_G4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    setlen(prim, 8);
    uint32 color = col1;
    memcpy(&prim->p[0].rgbc, &color, sizeof(color));
    color = col2;
    memcpy(&prim->p[1].rgbc, &color, sizeof(color));
    color = col3;
    memcpy(&prim->p[2].rgbc, &color, sizeof(color));
    color = col4;
    memcpy(&prim->p[3].rgbc, &color, sizeof(color));
    setcode(prim, 0x3A);
    sint16 right = (sint16)(uint32(x) + uint32(w));
    sint16 bottom = (sint16)(uint32(y) + uint32(h));
    prim->p[0].xy.x = (sint16)x;
    prim->p[0].xy.y = (sint16)y;
    prim->p[1].xy.x = right;
    prim->p[1].xy.y = (sint16)y;
    prim->p[2].xy.x = (sint16)x;
    prim->p[2].xy.y = bottom;
    prim->p[3].xy.x = right;
    prim->p[3].xy.y = bottom;
    DR_MODE *mode = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
    ot = cprim->LastPrim;
    setaddr(mode, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(mode + 1);
    setaddr(ot, mode);
    uint16 page = GetTPage(2, 2, 0, 256);
    SetDrawMode(mode, 0, 0, page, 0);
}

void tMenuItemGoToMenuButtonFade::TransitionOff(void)
{
    FUNCTION_MARKER(0x8001C25C, "FRONT.BIN");
    fFadeDir = 30;
}

void tMenuItemGoToMenuButtonFade::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001C268, "FRONT.BIN");
    fFadeVal = 128;
    fFadeDir = -30;
    fSelFade = 0;
    fEnableVal = 0;
}

bool tMenuItemGoToMenuButtonFade::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x8001C284, "FRONT.BIN");
    fInTransition = 0;
    if ((fFadeDir < 0 && fFadeVal > 0) || (fFadeDir > 0 && fFadeVal < 128))
        fInTransition = 1;
    return fInTransition == 0;
}

void tMenuItemGoToMenuButtonFade::UpdateTransition(bool selected)
{
    FUNCTION_MARKER(0x8001C2DC, "FRONT.BIN");
    sint32 fade = fFadeVal + fFadeDir;
    if (fade > 128)
        fade = 128;
    if (fade < 0)
        fade = 0;
    fFadeVal = (sint16)fade;
    TransitionIsFinished();
    tMenuItem::UpdateTransition(selected);
}

tMenuItemLeftRightFade::tMenuItemLeftRightFade(uint32 textDescription, tListIterator *dataPtr) : tMenuItemLeftRightChoice(textDescription, dataPtr)
{
    FUNCTION_MARKER(0x8001C358, "FRONT.BIN");
    flareextra = 0;
    fSelFade = 0;
}

void tMenuItemLeftRightFade::TransitionOff(void)
{
    FUNCTION_MARKER(0x8001C394, "FRONT.BIN");
    fInTransition = 1;
    fFadeDir = 30;
    fFadeVal = 0;
}

void tMenuItemLeftRightFade::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001C3AC, "FRONT.BIN");
    fInTransition = 1;
    fFadeDir = -30;
    flareextra = 0;
    fSelFade = 0;
    fFadeVal = 128;
}

bool tMenuItemLeftRightFade::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x8001C3D0, "FRONT.BIN");
    fInTransition = ((fFadeDir < 0 && fFadeVal > 0) || (fFadeDir > 0 && fFadeVal < 128)) ? 1 : 0;
    return fInTransition == 0;
}

void tMenuItemLeftRightFade::UpdateTransition(bool selected)
{
    FUNCTION_MARKER(0x8001C438, "FRONT.BIN");
    sint32 fade = fFadeVal + fFadeDir;
    if (fade > 128)
        fade = 128;
    if (fade < 0)
        fade = 0;
    fFadeVal = (sint16)fade;
    TransitionIsFinished();
    tMenuItem::UpdateTransition(selected);
}

tOptionsMenu::tOptionsMenu(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, sint32 firstframe, sint32 numframes, tMenuItem *firstItem, ...) : tMenu(flags, screenHandler, nextMenu, optionsMenu, OnButtonPress, title)
{
    FUNCTION_MARKER(0x8001C4B4, "FRONT.BIN");
    va_list ap;
    va_start(ap, firstItem);
    tMenuConstructor(firstItem, &ap);
    va_end(ap);
    fScreenFade = 0;
    fPrevItem = 0;
    fFirstFrame = firstframe;
    fNumFrames = numframes;
}

tOptionsMenu::~tOptionsMenu(void)
{
    FUNCTION_MARKER(0x8001C544, "FRONT.BIN");
}

sint32 tOptionsMenu::DebounceKeys(void)
{
    FUNCTION_MARKER(0x8001C56C, "FRONT.BIN");
    tMenuItem *item = fItemList[fCurrentItem];
    if (item != 0 && (item->fFlags & 1u) == 0)
        return item->DebounceKeys();
    return 0;
}

void tOptionsMenu::TransitionOff(void)
{
    FUNCTION_MARKER(0x8001C5D8, "FRONT.BIN");
    fTransitionDirection = 40;
    fScreenFade = 0;
    for (sint32 i = 0; fItemList[(sint16)i] != 0; ++i)
        fItemList[(sint16)i]->TransitionOff();
    fInMenuTransition = 1;
}

void tOptionsMenu::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001C670, "FRONT.BIN");
    fTransitionDirection = -40;
    fInMenuTransition = 1;
    fScreenFade = 552;
    for (sint32 i = 0; fItemList[(sint16)i] != 0; ++i)
        fItemList[(sint16)i]->TransitionOn();
}

bool tOptionsMenu::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x8001C70C, "FRONT.BIN");
    if ((fScreenFade >= 552 && fTransitionDirection > 0) || (fScreenFade <= 0 && fTransitionDirection < 0))
        fInMenuTransition = 0;
    else
        fInMenuTransition = 1;
    if (fInMenuTransition == 0)
    {
        for (sint32 i = 0; fItemList[(sint16)i] != 0; ++i)
            if (!fItemList[(sint16)i]->TransitionIsFinished())
                fInMenuTransition = 1;
    }
    if (!FEApp->fCurrentScreen[0]->TransitionIsFinished())
        fInMenuTransition = 1;
    return fInMenuTransition == 0;
}

void tOptionsMenu::UpdateTransition(void)
{
    FUNCTION_MARKER(0x8001C854, "FRONT.BIN");
    if (fInMenuTransition != 0)
    {
        sint32 direction = fTransitionDirection;
        if (direction < 0)
        {
            if (fScreenFade > 0)
            {
                sint32 fade = (fScreenFade + direction);
                fScreenFade = fade < 0 ? 0 : fade;
            }
            else if (fItemList[0] != 0)
            {
                sint32 i = 0;
                while (fItemList[(sint16)i] != 0)
                {
                    if (!fItemList[(sint16)i]->TransitionIsFinished())
                        break;
                    ++i;
                }
                tMenuItem *item = fItemList[(sint16)i];
                if (item != 0)
                    item->UpdateTransition(false);
            }
        }
        else
        {
            sint32 i = 0;
            while (fItemList[(sint16)i] != 0)
                ++i;
            --i;
            while ((sint16)i != -1)
            {
                if (!fItemList[(sint16)i]->TransitionIsFinished())
                    break;
                --i;
            }
            if ((sint16)i != -1)
                fItemList[(sint16)i]->UpdateTransition(false);
            else if (fScreenFade < 552)
            {
                sint32 fade = (fScreenFade + fTransitionDirection);
                fScreenFade = fade > 552 ? 552 : fade;
            }
        }
    }
    else
    {
        for (sint32 i = 0; fItemList[(sint16)i] != 0; ++i)
        {
            tMenuItem *item = fItemList[(sint16)i];
            bool selected = fInMenuTransition == 0 && (sint16)i == fCurrentItem;
            item->UpdateTransition(selected);
        }
    }
    TransitionIsFinished();
}

void tOptionsMenu::Draw(void)
{
    FUNCTION_MARKER(0x8001CB04, "FRONT.BIN");
    CalcPulsateYellow();
    tMenuItem *item = fItemList[fCurrentItem];
    if (item == 0 || (item->fFlags & 1) != 0)
        fCurrentItem = 0;
    while ((fItemList[fCurrentItem]->fFlags & 1) != 0 && fItemList[fCurrentItem + 1] != 0)
        ++fCurrentItem;
    while ((fItemList[fCurrentItem]->fFlags & 1) != 0 && fCurrentItem > 0)
        --fCurrentItem;
    if (fFirstFrame >= 0 && fNumFrames > 0)
    {
        tDrawShapeExtended drawFlags;
        drawFlags.tint[0] = 0xCEC844;
        sint32 index = (sint32)(uint32(fFirstFrame) + uint32((::ticks >> 4) % fNumFrames));
        DrawShapeExtended(index, 0x410, 16, 16, 0, 0, &drawFlags);
    }
    for (sint16 i = 0; fItemList[i] != 0; ++i)
    {
        bool selected = fInMenuTransition == 0 && i == fCurrentItem;
        fItemList[i]->Draw(0, 0, selected);
    }
}

void tOptionsMenu::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x8001CD08, "FRONT.BIN");
    sint16 enabled = GetNumberEnabledItems();
    if (enabled == 0)
    {
        fCurrentItem = 0;
        if (keyval != kInput_KeyType_Triangle && keyval != kInput_KeyType_Circle)
            keyval = kInput_KeyType_AlreadyProcessed;
    }
    tMenu::ProcessInput(fromPlayer, keyval, command);
}

tInsideBoxMenu::tInsideBoxMenu(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title) : tMenu(flags, screenHandler, nextMenu, optionsMenu, OnButtonPress, title)
{
}

tInsideBoxMenu::tInsideBoxMenu(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, tMenuItem *firstItem, ...) : tMenu(flags, screenHandler, nextMenu, optionsMenu, OnButtonPress, title)
{
    FUNCTION_MARKER(0x8001CD90, "FRONT.BIN");
    va_list ap;
    va_start(ap, firstItem);
    tMenuConstructor(firstItem, &ap);
    va_end(ap);
    fPrevItem = 0;
    fMoving = 0;
    fMovingDir = 0;
}

tInsideBoxMenu::~tInsideBoxMenu(void)
{
    FUNCTION_MARKER(0x8001CE04, "FRONT.BIN");
}

void tInsideBoxMenu::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x8001CE2C, "FRONT.BIN");
    if (fMoving != 0 && (keyval == kInput_KeyType_Up || keyval == kInput_KeyType_Down))
        keyval = kInput_KeyType_AlreadyProcessed;
    if (keyval == kInput_KeyType_Up && fCurrentItem == 0)
        fItemList[0]->ProcessInput(fromPlayer, keyval, command);
    else if (keyval != kInput_KeyType_Down || fItemList[(fCurrentItem + 1)] != 0)
        tMenu::ProcessInput(fromPlayer, keyval, command);
}

void tInsideBoxMenu::Draw(sint16 x, sint16 y, sint16 w, sint16 slideOffset, sint16 maxheight)
{
    FUNCTION_MARKER(0x8001CEFC, "FRONT.BIN");
    if (fCurrentItem != fPrevItem)
    {
        if (fPrevItem < fCurrentItem)
        {
            fMoving = 24;
            fMovingDir = -4;
        }
        else
        {
            fMoving = -24;
            fMovingDir = 4;
        }
        fPrevItem = (sint16)fCurrentItem;
    }
    if (fMoving != 0)
    {
        fMoving = (sint16)(sint32(fMoving) + sint32(fMovingDir));
        if (fMovingDir < 0 && fMoving < 0)
            fMoving = 0;
        if (fMovingDir > 0 && fMoving > 0)
            fMoving = 0;
    }
    for (sint32 i = 0; i < 5; ++i)
    {
        sint32 j = (sint16)(sint32((uint16)fCurrentItem) + i - 2);
        if (i >= 4 && fItemList[j - 1] == 0)
            return;
        if (j >= 0)
        {
            tMenuItem *item = fItemList[j];
            if (item != 0)
            {
                bool selected = fMoving == 0 && j == fCurrentItem;
                item->UpdateTransition(selected);
                item = fItemList[j];
                sint32 rowY = (((fMoving + y) + slideOffset) + ((i - 1) * 24)) + 5;
                selected = fMoving == 0 && j == fCurrentItem;
                item->Draw(sint32(x), rowY, sint32(w), selected);
            }
        }
    }
}

tMenuItemSlidingMenu::tMenuItemSlidingMenu(uint32 textDescription, sint16 width, sint16 height, sint16 diffx, sint16 diffy, bool fillback) : tMenuItem(textDescription)
{
    FUNCTION_MARKER(0x8001D160, "FRONT.BIN");
    currMenu = 0;
    nextMenu = 0;
    fWidth = width;
    fHeight = height;
    fSelFade = 0;
    fFlags |= 0x80U;
    fDiffX = diffx;
    fDiffY = diffy;
    fFillback = fillback;
    fFlags |= 0x80U;
}

tMenuItemSlidingMenu::~tMenuItemSlidingMenu(void)
{
    FUNCTION_MARKER(0x8001D208, "FRONT.BIN");
}

void tMenuItemSlidingMenu::TransitionOff(void)
{
    FUNCTION_MARKER(0x8001D230, "FRONT.BIN");
    tInsideBoxMenu *menu = currMenu;
    fFadeDir = 0;
    fTransitioningOut = 1;
    if (menu != 0)
    {
        fInTransition = 1;
        fFadeDir = 30;
        fFadeVal = 0;
    }
}

void tMenuItemSlidingMenu::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001D25C, "FRONT.BIN");
    sint16 height = fHeight;
    fFadeDir = -30;
    fInTransition = 1;
    fOpenHeight = 0;
    fTransitioningOut = 0;
    fFadeVal = 128;
    fSlideOffset = (sint16)(sint32(height) + (sint32(height) >> 1));
}

bool tMenuItemSlidingMenu::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x8001D294, "FRONT.BIN");
    if (currMenu == 0 || nextMenu == 0)
    {
        fFadeDir = 0;
        fInTransition = 0;
    }
    else
        fInTransition = ((fFadeDir < 0 && fFadeVal > 0) || (fFadeDir > 0 && fFadeVal < 128)) ? 1 : 0;
    return fInTransition == 0;
}

void tMenuItemSlidingMenu::UpdateTransition(bool selected)
{
    FUNCTION_MARKER(0x8001D31C, "FRONT.BIN");
    sint32 fade = fFadeVal + fFadeDir;
    if (fade > 128)
        fade = 128;
    if (fade < 0)
        fade = 0;
    fFadeVal = (sint16)fade;
    TransitionIsFinished();
    tMenuItem::UpdateTransition(selected);
}

sint32 tMenuItemSlidingMenu::DebounceKeys(void)
{
    FUNCTION_MARKER(0x8001D398, "FRONT.BIN");
    tInsideBoxMenu *menu = currMenu;
    return menu != 0 ? (menu->DebounceKeys() | 0x600L) : 0x600L;
}

void tMenuItemSlidingMenu::Draw(bool selected)
{
    FUNCTION_MARKER(0x8001D3E0, "FRONT.BIN");
    Draw(0, 0, selected);
}

void tMenuItemSlidingMenu::UpdatefOpenHeight(bool selected)
{
    FUNCTION_MARKER(0x8001D41C, "FRONT.BIN");
    fClosing = 0;
    if (fOpenHeight == 0)
    {
        sint32 height = fHeight;
        fSlideOffset = (sint16)(height + (height >> 1));
    }
    if (nextMenu != currMenu && fOpenHeight > 0)
    {
        sint32 height = fOpenHeight - 4;
        fOpenHeight = (sint16)(height < 0 ? 0 : height);
        fClosing = 1;
    }
    else if (nextMenu == 0 && fFadeVal < 128)
    {
        if (fInTransition == 0)
        {
            sint32 fade = fFadeVal + 40;
            fFadeVal = (sint16)(fade > 128 ? 128 : fade);
        }
    }
    else if (fFadeVal > 0 && currMenu == 0 && nextMenu != 0)
    {
        if (fInTransition == 0)
        {
            sint32 fade = fFadeVal - 40;
            fFadeVal = (sint16)(fade < 0 ? 0 : fade);
        }
        if (fTransitioningOut == 0)
            currMenu = nextMenu;
    }
    else
    {
        currMenu = nextMenu;
        if (fTransitioningOut == 0 && fInTransition == 0)
        {
            if (nextMenu != 0)
            {
                sint32 fade = fFadeVal - 40;
                fFadeVal = (sint16)(fade < 0 ? 0 : fade);
            }
            else
            {
                sint32 fade = fFadeVal + 40;
                fFadeVal = (sint16)(fade > 128 ? 128 : fade);
            }
        }
    }
    if (currMenu != 0)
    {
        if (selected)
        {
            if (fClosing == 0)
            {
                if (fOpenHeight < fHeight)
                {
                    sint32 height = fOpenHeight + 4;
                    if (height > fHeight)
                        height = fHeight;
                    fOpenHeight = (sint16)height;
                }
                else
                {
                    sint32 offset = fSlideOffset - 6;
                    fSlideOffset = (sint16)(offset < 0 ? 0 : offset);
                }
            }
        }
        else if (fOpenHeight > 0)
        {
            sint32 height = fOpenHeight - 4;
            fOpenHeight = (sint16)(height < 0 ? 0 : height);
        }
    }
}

void Front::ResetOverlayOptions(void)
{
    flareextra = 0;
    PulsateYellow = 0;
    fHelpText = 0;
}

void tMenuItemSlidingMenu::Draw(sint32 offx, sint32 offy, bool selected)
{
    FUNCTION_MARKER(0x8001D63C, "FRONT.BIN");
    UpdatefOpenHeight(selected);
    sint32 ColText = CalcTextFadeSelToHi(textType_Options, fSelFade, fFadeVal);
    sint32 x = (TextSys::WordX(fTextDescription) + offx);
    sint32 y = (TextSys::WordY(fTextDescription) + offy);
    fHelpText = -1;
    tTexture::ShapeInfo *shape = gHelpShapes + 30;
    bool fPlayList = currMenu == &menuDefs->menuPlayListMenu;
    if (fFadeVal != 128 && fPlayList)
        DrawLeftFlare(y, fSelFade, fFadeVal, flareextra);
    if (currMenu != 0 || fPlayList)
    {
        sint32 width = fPlayList ? 220 : fWidth;
        tDrawShapeExtended drawFlags;
        drawFlags.tint[0] = CalcFadeVal(0, 0xBEBE, fSelFade, fFadeVal);
        sint32 right = (x + width);
        sint32 arrowX = ((right - shape->width) - 10);
        sint32 arrowY = (uint32(y) - (fPlayList ? 3u : 2u));
        DrawShapeExtended(57, 0x18, arrowX, arrowY, 0, 1, &drawFlags);
        arrowX = ((right - shape->width) - 10);
        arrowY = (uint32(y) + (fPlayList ? 3u : 4u));
        DrawShapeExtended(58, 0x18, arrowX, arrowY, 0, 1, &drawFlags);
    }
    if (currMenu == 0)
        return;
    sint32 xx = (x + fDiffX);
    sint32 yy = (y + fDiffY);
    sint16 hh = fOpenHeight;
    sint16 ww = fWidth;
    DRAWENV *drenv = Draw::GetDRAWENV(Draw::gPlayer1View, gFlip);
    PSX_RECT full = { 0, drenv->clip.y, 512, (sint16)screenheight };
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    DR_AREA *daprim = reinterpret_cast<DR_AREA *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(daprim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(daprim + 1);
    setaddr(ot, daprim);
    SetDrawArea(daprim, &full);
    sint32 half = sint32(hh) >> 1;
    SubtractiveBox(xx, yy, ww, half, 0x505050, 0x505050, 0, 0);
    SubtractiveBox(xx, ((yy + half)), ww, half, 0, 0, 0x505050, 0x505050);
    tTexture::ShapeInfo *shapetop = gHelpShapes + 31;
    tTexture::ShapeInfo *shapebottom = gHelpShapes + 32;
    currMenu->Draw((sint16)xx, (sint16)yy, ww, (sint16)(uint32((uint16)fSlideOffset) << 1), fHeight);
    if (fFillback != 0 && shapetop->height < hh)
    {
        DrawShapeExtended(31, 12, xx, yy, 0, 0, 0);
        DrawShapeExtended(31, 8, (((xx + ww) - shapetop->width)), yy, 0, 0, 0);
        DrawShapeExtended(32, 12, xx, (((yy + hh) - shapebottom->height)), 0, 0, 0);
        DrawShapeExtended(32, 8, (((xx + ww) - shapebottom->width)), ((yy + hh) - shapebottom->height), 0, 0, 0);
    }
    PSXDrawSquare(0, (((xx + shapetop->width) + 5)), yy, (((ww - shapetop->width) - shapebottom->width) - 10), hh);
    sint32 topHeight = shapetop->height;
    sint32 bottomHeight = shapebottom->height;
    if (topHeight + bottomHeight < hh)
        PSXDrawSquare(0, xx, ((yy + topHeight)), ww, ((hh - topHeight) - bottomHeight));
    PSX_RECT temp = { (sint16)xx, (sint16)(uint32((uint16)drenv->clip.y) + uint32(yy)), ww, hh };
    daprim = reinterpret_cast<DR_AREA *>(cprim->PrimPtr);
    ot = cprim->LastPrim;
    setaddr(daprim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(daprim + 1);
    setaddr(ot, daprim);
    SetDrawArea(daprim, &temp);
    if (fHelpText != -1)
    {
        char *help = TextSys::Word(fHelpText);
        sint16 helpX = (sint16)TextSys::WordX(fHelpText);
        sint16 helpY = (sint16)TextSys::WordY(fHelpText);
        FETextRender::FullTextRGB(help, helpX, helpY, PulsateYellow, 0, 2);
    }
    char *text = TextSys::Word(fTextDescription);
    FETextRender::FullTextRGB(text, x, y, ColText, 0, fPlayList ? 1 : 0);
    sint32 topY = (y - 2);
    DrawShapeExtended(30, 8, (((x + fWidth) - shape->width)), topY, fFadeVal, 0, 0);
    if (fFillback != 0)
        PSXDrawSquare(0, x, topY, (fWidth - shape->width), shape->height);
}

void tMenuItemSlidingMenu::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x8001DC80, "FRONT.BIN");
    if (fOpenHeight == fHeight)
    {
        tInsideBoxMenu *menu = currMenu;
        if (menu != 0)
            menu->ProcessInput(fromPlayer, keyval, command);
    }
    if (keyval == kInput_KeyType_Down)
        keyval = kInput_KeyType_AlreadyProcessed;
    tMenuItem::ProcessInput(fromPlayer, keyval, command);
}

void tMenuItemSlidingMenu::SetMenu(bool bothmenus, tInsideBoxMenu *menu)
{
    FUNCTION_MARKER(0x8001DD28, "FRONT.BIN");
    nextMenu = menu;
    if (bothmenus)
        currMenu = menu;
    if (nextMenu == 0)
        fFlags |= 1u;
    else
        fFlags &= ~1u;
}

void tMenuItemSlidingActivated::UpdatefOpenHeight(bool)
{
    FUNCTION_MARKER(0x8001DD6C, "FRONT.BIN");
    sint32 delta = fActive != 0 ? -3 : 3;
    fSlideOffset = (sint16)(sint32(fSlideOffset) + delta);
    sint32 height = fHeight;
    sint32 limit = height + (height >> 1);
    sint32 offset = fSlideOffset;
    if (offset > limit)
        offset = limit;
    if (offset < 0)
        offset = 0;
    fSlideOffset = (sint16)offset;
}

void tMenuItemSlidingActivated::TransitionOff(void)
{
    FUNCTION_MARKER(0x8001DDD4, "FRONT.BIN");
    tInsideBoxMenu *menu = currMenu;
    fFadeDir = 0;
    fTransitioningOut = 1;
    if (menu != 0)
    {
        fInTransition = 1;
        fFadeDir = 30;
        fFadeVal = 0;
    }
}

void tMenuItemSlidingActivated::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001DE00, "FRONT.BIN");
    sint16 height = fHeight;
    fFadeDir = -30;
    fInTransition = 1;
    fActive = 0;
    fTransitioningOut = 0;
    fFadeVal = 128;
    fSlideOffset = (sint16)(sint32(height) + (sint32(height) >> 1));
    fOpenHeight = height;
}

bool tMenuItemSlidingActivated::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x8001DE40, "FRONT.BIN");
    if (currMenu == 0 || nextMenu == 0)
    {
        fFadeDir = 0;
        fInTransition = 0;
    }
    else
        fInTransition = ((fFadeDir < 0 && fFadeVal > 0) || (fFadeDir > 0 && fFadeVal < 128)) ? 1 : 0;
    return fInTransition == 0;
}

void tMenuItemSlidingActivated::UpdateTransition(bool selected)
{
    FUNCTION_MARKER(0x8001DEC8, "FRONT.BIN");
    sint32 fade = fFadeVal + fFadeDir;
    if (fade > 128)
        fade = 128;
    if (fade < 0)
        fade = 0;
    fFadeVal = (sint16)fade;
    TransitionIsFinished();
    tMenuItem::UpdateTransition(selected);
}

void tMenuItemSlidingActivated::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x8001DF44, "FRONT.BIN");
    if (keyval == kInput_KeyType_Cross)
    {
        if (fActive == 0)
        {
            AudioCmn::PlayFESFX(0);
            fActive = 1;
            keyval = kInput_KeyType_AlreadyProcessed;
            AudioMus::StopSong(20);
            AudioMus::SysCleanUp();
            AudioMus::SysStartUp(0xC000, 0x18000, frontGameMusicBank);
            AudioMus::PlaySong(frontGameMusicPattern);
            AudioMus::tSongList *songs = AudioMus::GetSongList(frontAllMusicPattern, 0);
            screenAudio->songlist = songs;
        }
    }
    else if (keyval == kInput_KeyType_Triangle && fActive != 0)
    {
        AudioCmn::PlayFESFX(1);
        fActive = 0;
        keyval = kInput_KeyType_AlreadyProcessed;
        AudioMus::StopSong(20);
        AudioMus::tSongList *songs = screenAudio->songlist;
        if (songs != 0)
            purgememadr(songs);
        screenAudio->songlist = 0;
        AudioMus::SysCleanUp();
        AudioMus::SysStartUp(0xD800, 0x18000, frontMenuMusicBank);
        AudioMus::PlaySong(frontMenuMusicPattern);
    }
    if (fSlideOffset == 0 && fActive != 0)
    {
        tInsideBoxMenu *menu = currMenu;
        if (menu != 0)
            menu->ProcessInput(fromPlayer, keyval, command);
    }
    if (fActive != 0 && (keyval == kInput_KeyType_Up || keyval == kInput_KeyType_Down))
        keyval = kInput_KeyType_AlreadyProcessed;
    tMenuItem::ProcessInput(fromPlayer, keyval, command);
}

void tMenuItemLeftRightFade::MyLeftRightDraw(sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x8001E13C, "FRONT.BIN");
    tDrawShapeExtended aCol;
    aCol.tint[0] = CalcFadeVal(0xC83C1E, 0xBEBE, fSelFade, fFadeVal);
    DrawLeftFlare(y, fSelFade, fFadeVal, flareextra);
    sint32 ColText = CalcTextFadeSelToHi(textType_Options, fSelFade, fFadeVal);
    char *text = TextSys::Word(fTextDescription);
    FETextRender::FullTextRGB(text, x, y, ColText, 0, 1);
    sint32 arrowY = (y + 1);
    DrawShapeExtended(46, 0x18, ((x + 26)), arrowY, 0, 0, &aCol);
    DrawShapeExtended(47, 0x18, ((x + 200)), arrowY, 0, 0, &aCol);
}

void tMenuItemDisplayLeftRightChoice::Draw(sint32 offx, sint32 offy, bool)
{
    FUNCTION_MARKER(0x8001E230, "FRONT.BIN");
    if (fFadeVal == 128)
        return;
    sint32 x = TextSys::WordX(fTextDescription);
    x = (x + offx);
    sint32 y = TextSys::WordY(fTextDescription);
    y = (sint16)(uint32(y) + uint32(offy));
    sint32 ColText = CalcTextFadeSelToHi(textType_Options, fSelFade, fFadeVal);
    MyLeftRightDraw((sint16)x, y);
    sint16 value = fData->TextValue(gMenu_SubMenuPlayer);
    char *text = TextSys::Word(value);
    FETextRender::FullTextRGB(text, (uint32(x) + 115u), y, ColText, 0, 2);
}

void tMenuItemOnOffLeftRightChoice::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001E320, "FRONT.BIN");
    fOnFade = (uint8)fData->Value(kPlayerBoth) != 0 ? 128 : 0;
    tMenuItemLeftRightFade::TransitionOn();
}

void tMenuItemOnOffLeftRightChoice::Draw(sint32 offx, sint32 offy, bool)
{
    FUNCTION_MARKER(0x8001E384, "FRONT.BIN");
    if (fFadeVal == 128)
        return;
    if ((uint8)fData->Value(kPlayerBoth) != 0)
        fOnFade = (sint16)(sint32(fOnFade) + 32);
    else
        fOnFade = (sint16)(sint32(fOnFade) - 32);
    if (fOnFade > 128)
        fOnFade = 128;
    if (fOnFade < 0)
        fOnFade = 0;
    sint32 x = TextSys::WordX(fTextDescription);
    x = (x + offx);
    sint32 y = TextSys::WordY(fTextDescription);
    y = (sint16)(uint32(y) + uint32(offy));
    sint32 ColTextOn;
    sint32 ColTextOff;
    CalcOnOffFade(textType_Options, fOnFade, fSelFade, fFadeVal, ColTextOn, ColTextOff);
    MyLeftRightDraw((sint16)x, y);
    char *text = TextSys::Word(102);
    FETextRender::FullTextRGB(text, (uint32(x) + 55u), y, ColTextOn, 0, 0);
    text = TextSys::Word(103);
    FETextRender::FullTextRGB(text, (uint32(x) + 158u), y, ColTextOff, 0, 0);
}

tMenuItemLeftRightAudioSlider::tMenuItemLeftRightAudioSlider(uint32 textDescription, tListIterator *dataPtr, sint32 AudioArt) : tMenuItemLeftRightSlider(textDescription, dataPtr)
{
    FUNCTION_MARKER(0x8001E4FC, "FRONT.BIN");
    fAudioArt = (sint16)AudioArt;
    flareextra = 0;
    fSelFade = 0;
}

tMenuItemLeftRightAudioSlider::~tMenuItemLeftRightAudioSlider(void)
{
    FUNCTION_MARKER(0x8001E548, "FRONT.BIN");
}

void tMenuItemLeftRightAudioSlider::Draw(sint32 ox, sint32 oy, bool)
{
    FUNCTION_MARKER(0x8001E570, "FRONT.BIN");
    sint32 x = TextSys::WordX(fTextDescription);
    fX = (sint16)(uint32(x) + uint32(ox));
    sint32 y = TextSys::WordY(fTextDescription);
    fY = (sint16)(uint32(y) + uint32(oy));
    sint32 type = TextSys::WordFlags((sint16)fTextDescription);
    sint32 highlighted = kRGBVals[(uint8)textDefinitions[type][5]];
    type = TextSys::WordFlags((sint16)fTextDescription);
    sint32 selectedColor = kRGBVals[(uint8)textDefinitions[type][4]];
    DrawLeftFlare(fY, fSelFade, fFadeVal, flareextra);
    sint32 coltext = CalcFadeVal(selectedColor, highlighted, fSelFade, fFadeVal);
    if (fFadeVal == 128)
        return;
    char *text = TextSys::Word(fTextDescription);
    FETextRender::FullTextRGB(text, fX, fY, coltext, 0, 1);
    tDrawShapeExtended tCol;
    tCol.tint[0] = CalcFadeVal(0x551E00, 0xBEBE, fSelFade, fFadeVal);
    if (fSelFade != 0)
        DrawShapeExtended((fAudioArt + 1), 0x10, 0, 0, 0, 0, &tCol);
    uint8 value = (uint8)fData->Value(kPlayerBoth);
    DrawSlider(value, (uint8)fData->fMinValue, (uint8)fData->fMaxValue, (sint16)(sint32(fX) + 20), (sint16)(sint32(fY) + 1), fWidth, fHeight, 4, 4, false, false, fSelFade, fFadeVal);
}

sint32 tMenuItemLeftRightAudioSlider::Percentage(void)
{
    FUNCTION_MARKER(0x8001E77C, "FRONT.BIN");
    uint8 value = (uint8)fData->Value(kPlayerBoth);
    tListIterator *data = fData;
    sint32 minValue = (uint8)data->fMinValue;
    sint32 range = ((uint8)data->fMaxValue) - minValue;
    if (range == 0)
        abort();
    sint32 percent = (sint32(value) - minValue) * 100 / range;
    if ((uint8)data->Value(kPlayerBoth) != 0 && percent < 100)
        ++percent;
    return percent;
}

void tMenuItemLeftRightAudioSlider::TransitionOff(void)
{
    FUNCTION_MARKER(0x8001E850, "FRONT.BIN");
    fFadeVal = 0;
    fFadeDir = 60;
}

void tMenuItemLeftRightAudioSlider::TransitionOn(void)
{
    FUNCTION_MARKER(0x8001E860, "FRONT.BIN");
    fFadeVal = 128;
    fFadeDir = -60;
    fSelFade = 0;
}

bool tMenuItemLeftRightAudioSlider::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x8001E878, "FRONT.BIN");
    fInTransition = 0;
    if ((fFadeDir < 0 && fFadeVal > 0) || (fFadeDir > 0 && fFadeVal < 128))
        fInTransition = 1;
    return fInTransition == 0;
}

void tMenuItemLeftRightAudioSlider::UpdateTransition(bool selected)
{
    FUNCTION_MARKER(0x8001E8D4, "FRONT.BIN");
    sint32 fade = fFadeVal + fFadeDir;
    if (fade > 128)
        fade = 128;
    if (fade < 0)
        fade = 0;
    fFadeVal = (sint16)fade;
    tMenuItem::UpdateTransition(selected);
}

tInsideBoxSongMenu::tInsideBoxSongMenu(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title, tMenuItem *firstItem, ...) : tInsideBoxMenu(flags, screenHandler, nextMenu, optionsMenu, OnButtonPress, title)
{
    FUNCTION_MARKER(0x8001E920, "FRONT.BIN");
    for (sint32 j = 0; j < 5; ++j)
    {
        fSelFade[j] = 0;
        fOnOffFade[j] = 0;
    }
    va_list ap;
    va_start(ap, firstItem);
    tMenuConstructor(firstItem, &ap);
    va_end(ap);
    fMoving = 0;
    fMovingDir = 0;

    {
        sint32 j;

        (void)sizeof(j);
    }
}

tInsideBoxSongMenu::~tInsideBoxSongMenu(void)
{
    FUNCTION_MARKER(0x8001E9B0, "FRONT.BIN");
}

void tInsideBoxSongMenu::Draw(sint16 x, sint16 y, sint16 w, sint16 slideOffset, sint16 maxheight)
{
    FUNCTION_MARKER(0x8001E9D8, "FRONT.BIN");
    if (screenAudio->songlist == 0)
        return;
    sint32 drawY = y + ((sint32(maxheight) - 21) >> 1);
    for (sint32 j = 0; j < 5; ++j)
    {
        if (j != 2)
        {
            sint32 fade = fSelFade[j] - 8;
            if (fade < 0)
                fade = 0;
            fSelFade[j] = (sint16)fade;
        }
    }
    sint32 fade = fSelFade[2] + 8;
    if (fade > 128)
        fade = 128;
    fSelFade[2] = (sint16)fade;
    if (fMoving != 0)
    {
        fMoving = (sint16)(sint32(fMoving) + sint32(fMovingDir) * 2);
        if (fMovingDir < 0 && fMoving < 0)
            fMoving = 0;
        if (fMovingDir > 0 && fMoving > 0)
            fMoving = 0;
    }
    for (sint32 j = 0; j < 5; ++j)
    {
        sint32 song = (screenAudio->fSelectedSong + j) - 2;
        if (song >= 0 && song < screenAudio->songlist->numsongs)
        {
            if (frontEnd.FEPlayList[song] != 0)
                fOnOffFade[j] = (sint16)(sint32(fOnOffFade[j]) + 32);
            else
                fOnOffFade[j] = (sint16)(sint32(fOnOffFade[j]) - 32);
            if (fOnOffFade[j] > 128)
                fOnOffFade[j] = 128;
            if (fOnOffFade[j] < 0)
                fOnOffFade[j] = 0;
            DrawOneSong((sint16)song, x, (sint16)(sint32(fMoving) + drawY + j * 21 - 40 + sint32(slideOffset)), w, fOnOffFade[j], fSelFade[j]);
        }
    }
}

void tInsideBoxSongMenu::DrawOneSong(sint16 songnum, sint16 x, sint16 y, sint16 w, sint16 fOnOffFade, sint16 fSelFade)
{
    FUNCTION_MARKER(0x8001EC48, "FRONT.BIN");
    sint32 Col = CalcFadeVal(0x551E00, 40);
    sint32 ColTextOn;
    sint32 ColTextOff;
    CalcOnOffFade(textType_Options, fOnOffFade, fSelFade, 0, ColTextOn, ColTextOff);
    sint32 ColText = CalcTextFadeSelToHi(textType_Options, fSelFade, 0);
    char *text = screenAudio->songlist->song[songnum].title;
    FETextRender::FullTextRGB(text, (sint32(x) + 3), (sint32(y) + 2), ColText, 0, 0);
    text = screenAudio->songlist->song[songnum].artist;
    FETextRender::FullTextRGB(text, (sint32(x) + 2), (sint32(y) + 10), ColText, 0, 0);
    text = TextSys::Word(102);
    FETextRender::FullTextRGB(text, (sint32(x) + sint32(w) - 90), (sint32(y) + 6), ColTextOn, 0, 2);
    text = TextSys::Word(103);
    FETextRender::FullTextRGB(text, (sint32(x) + sint32(w) - 30), (sint32(y) + 6), ColTextOff, 0, 2);
    PSXDrawSquare(Col, ((x + w) - 120), y, 2, 19);
    PSXDrawSquare(Col, ((x + w) - 60), y, 2, 19);
    PSXDrawSquare(Col, x, y, w, 1);
    PSXDrawSquare(Col, x, (y + 19), w, -1);
}

sint32 tInsideBoxSongMenu::DebounceKeys(void)
{
    FUNCTION_MARKER(0x8001EE74, "FRONT.BIN");
    return 0x1E00;
}

void tInsideBoxSongMenu::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x8001EE7C, "FRONT.BIN");
    if (screenAudio->songlist == 0)
        return;
    if (keyval == kInput_KeyType_Left)
        AudioCmn::PlayFESFX(5);
    else if (keyval == kInput_KeyType_Right)
        AudioCmn::PlayFESFX(6);
    switch (keyval)
    {
    case kInput_KeyType_Left:
    case kInput_KeyType_Right:
    {
        sint32 song = screenAudio->fSelectedSong;
        frontEnd.FEPlayList[song] = frontEnd.FEPlayList[song] == 0 ? 1 : 0;
        break;
    }
    case kInput_KeyType_Up:
        if (screenAudio->fSelectedSong > 0 && fMoving == 0)
        {
            fMoving = -21;
            fMovingDir = 4;
            for (sint32 j = 4; j > 0; --j)
            {
                fOnOffFade[j] = fOnOffFade[j - 1];
                fSelFade[j] = fSelFade[j - 1];
            }
            fOnOffFade[0] = 0;
            fSelFade[0] = 0;
            screenAudio->fSelectedSong = (sint16)(sint32(screenAudio->fSelectedSong) - 1);
            AudioCmn::PlayFESFX(3);
        }
        break;
    case kInput_KeyType_Down:
    {
        sint32 lastSong = (screenAudio->songlist->numsongs - 1);
        if (screenAudio->fSelectedSong < lastSong && fMoving == 0)
        {
            fMoving = 21;
            fMovingDir = -4;
            for (sint32 j = 0; j < 4; ++j)
            {
                fOnOffFade[j] = fOnOffFade[j + 1];
                fSelFade[j] = fSelFade[j + 1];
            }
            fOnOffFade[4] = 0;
            fSelFade[4] = 0;
            screenAudio->fSelectedSong = (sint16)(sint32(screenAudio->fSelectedSong) + 1);
            AudioCmn::PlayFESFX(4);
        }
        break;
    }
    default:
        return;
    }
    keyval = kInput_KeyType_AlreadyProcessed;
}

void tMenuItemControllerLeftRightChoice::Draw(sint32 ox, sint32 oy, bool)
{
    FUNCTION_MARKER(0x8001F084, "FRONT.BIN");
    sint32 x = TextSys::WordX(fTextDescription);
    x = (x + ox);
    sint32 y = TextSys::WordY(fTextDescription);
    y = (y + oy);
    tTexture::ShapeInfo *shape = gHelpShapes + 30;
    sint32 Col = CalcFadeVal(0xC83C1E, 0xBEBE, fSelFade, fFadeVal);
    sint32 ColText = CalcTextFadeSelToHi(textType_Options, fSelFade, fFadeVal);
    tDrawShapeExtended drawFlags;
    if (fFadeVal != 128)
    {
        drawFlags.tint[0] = Col;
        char *text = TextSys::Word(fTextDescription);
        FETextRender::FullTextRGB(text, x, y, ColText, 0, 0);
        sint16 value = fData->TextValue(gMenu_SubMenuPlayer);
        text = TextSys::Word(value);
        FETextRender::FullTextRGB(text, (uint32(x) + 151u), y, ColText, 0, 2);
    }
    drawFlags.tint[0] = CalcFadeVal(0, 0xBEBE, fSelFade, fFadeVal);
    x = (sint16)x;
    y = (sint16)y;
    DrawShapeExtended(10, 0x18, x + 131, y, 0, 1, &drawFlags);
    DrawShapeExtended(11, 0x18, x + 161, y, 0, 1, &drawFlags);
    DrawShapeExtended(30, 8, (x - (shape->width - 176)), y - 3, fFadeVal, 0, 0);
    PSXDrawSquare(0, x, y - 3, (176 - shape->width), shape->height);
}

tInsideBoxLeftRightSlider::tInsideBoxLeftRightSlider(uint32 textDescription, tListIterator *dataPtr) : tMenuItemLeftRightSlider(textDescription, dataPtr)
{
    FUNCTION_MARKER(0x8001F288, "FRONT.BIN");
    fSelFade = 0;
    fHeight = 5;
}

tInsideBoxLeftRightSlider::~tInsideBoxLeftRightSlider(void)
{
    FUNCTION_MARKER(0x8001F2C8, "FRONT.BIN");
}

void tInsideBoxLeftRightSlider::Draw(sint32 x, sint32 y, sint32 w, bool selected)
{
    FUNCTION_MARKER(0x8001F2F0, "FRONT.BIN");
    fX = (sint16)x;
    fY = (sint16)y;
    sint32 col = CalcFadeVal(0x551E00, 0xC83C1E, fSelFade);
    sint32 coltext = CalcTextFadeUnselToSel(textType_Options, fSelFade, 0);
    PSXDrawSquare(col, fX, fY, w, 1);
    PSXDrawSquare(col, fX, (fY + 8), w, 1);
    char *text = TextSys::Word(fTextDescription);
    FETextRender::FullTextRGB(text, (sint32(fX) + 4), (sint32(fY) + 10), coltext, 0, 0);
    uint8 value = (uint8)fData->Value(kPlayerBoth);
    DrawSlider(value, (uint8)fData->fMinValue, (uint8)fData->fMaxValue, (sint16)(sint32(fX) + 4), (sint16)(sint32(fY) + 2), (sint16)(uint32(w) - 8u), fHeight, 4, 4, false, false, fSelFade, 0);
}

tInsideBoxTwoWaySlider::tInsideBoxTwoWaySlider(uint32 textDescription, tListIterator *dataPtr, sint32 type) : tMenuItemLeftRightSlider(textDescription, dataPtr)
{
    FUNCTION_MARKER(0x8001F474, "FRONT.BIN");
    fType = (sint16)type;
    fSelFade = 0;
    fHeight = 5;
    fActive = 0;
}

tInsideBoxTwoWaySlider::~tInsideBoxTwoWaySlider(void)
{
    FUNCTION_MARKER(0x8001F4C8, "FRONT.BIN");
}

char GetCurrentStickRange(sint32 player)
{
    FUNCTION_MARKER(0x8001F860, "FRONT.BIN");
    PAD_COMMON *pad = &gPadinfo.buf[player * 4];
    sint32 axis1 = pad->data.analog.rightx;
    sint32 range1 = 128 - axis1;
    if (range1 <= 0)
        range1 = axis1 - 128;
    sint32 axis2 = pad->data.analog.righty;
    sint32 range2 = 128 - axis2;
    if (range2 <= 0)
        range2 = axis2 - 128;
    return (char)(range1 > range2 ? range1 : range2);
}

char GetCurrentStickRange2(sint32 player)
{
    FUNCTION_MARKER(0x8001F8BC, "FRONT.BIN");
    PAD_COMMON *pad = &gPadinfo.buf[player * 4];
    sint32 axis1 = pad->data.analog.leftx;
    sint32 range1 = 128 - axis1;
    if (range1 <= 0)
        range1 = axis1 - 128;
    sint32 axis2 = pad->data.analog.lefty;
    sint32 range2 = 128 - axis2;
    if (range2 <= 0)
        range2 = axis2 - 128;
    return (char)(range1 > range2 ? range1 : range2);
}

bool CheckForCheats(char *fData)
{
    FUNCTION_MARKER(0x8001FCAC, "FRONT.BIN");
    sint32 len = (sint32)strlen(fData);
    for (; len < 8; ++len)
        fData[len] = '\0';
    return FECheat::ActivateCheat(fData) || FECheat::ActivateBonusByCode(fData);
}

sint32 SpecialCharacter(char current)
{
    FUNCTION_MARKER(0x800200EC, "FRONT.BIN");
    switch ((uint8)current)
    {
    case 0xE1: return 0x56;
    case 0xE4: return 0x51;
    case 0xE5: return 0x5B;
    case 0xE8: return 0x55;
    case 0xE9: return 0x54;
    case 0xED: return 0x57;
    case 0xF1: return 0x5A;
    case 0xF3: return 0x58;
    case 0xF6: return 0x52;
    case 0xFA: return 0x59;
    case 0xFC: return 0x53;
    default: return 0;
    }
}

void tInsideBoxTwoWaySlider::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x8001F4F0, "FRONT.BIN");
    if (keyval == kInput_KeyType_Cross)
    {
        AudioCmn::PlayFESFX(0);
        fActive = 1;
        keyval = kInput_KeyType_AlreadyProcessed;
    }
    if (keyval == kInput_KeyType_R1)
    {
        if (fActive == 0)
            return;
        AudioCmn::PlayFESFX(1);
        fActive = 0;
        keyval = kInput_KeyType_AlreadyProcessed;
    }
    if (fActive != 0 || keyval == kInput_KeyType_Left || keyval == kInput_KeyType_Right)
        keyval = kInput_KeyType_AlreadyProcessed;
}

void tInsideBoxTwoWaySlider::Draw(sint32 x, sint32 y, sint32 w, bool selected)
{
    FUNCTION_MARKER(0x8001F5A0, "FRONT.BIN");
    if (fActive != 0)
        Calibrate();
    sint32 halfWidth = w >> 1;
    fX = (sint16)x;
    fY = (sint16)y;
    sint32 col = CalcFadeVal(0x551E00, 0xC83C1E, fSelFade);
    sint32 col2 = CalcFadeVal(0x551E00, 0xBEBE, fSelFade);
    sint32 coltext = CalcTextFadeUnselToSel(textType_Options, fSelFade, 0);
    sint32 ww = halfWidth - 4;
    PSXDrawSquare(col, fX, fY, ww, 1);
    PSXDrawSquare(col, ((fX + ww) + 8), fY, ww, 1);
    PSXDrawSquare(col, fX, (fY + 8), ww, 1);
    PSXDrawSquare(col, ((fX + ww) + 9), (fY + 8), ww, 1);
    PSXDrawSquare(col2, ((fX + ww) + 3), fY, 2, 9);
    char *text = TextSys::Word(fTextDescription);
    FETextRender::FullTextRGB(text, (sint32(fX) + 4), (sint32(fY) + 10), coltext, 0, 0);
    uint8 value = (uint8)fData->Value(kPlayerBoth);
    sint16 sliderWidth = (sint16)(halfWidth - 8);
    DrawSlider(value, (uint8)fData->fMinValue, (uint8)fData->fMaxValue, (sint16)(sint32(fX) + 1), (sint16)(sint32(fY) + 2), sliderWidth, fHeight, 4, 4, true, false, fSelFade, 0);
    value = (uint8)fData->Value(kPlayerBoth);
    DrawSlider(value, (uint8)fData->fMinValue, (uint8)fData->fMaxValue, (sint16)(sint32(fX) + ww + 10), (sint16)(sint32(fY) + 2), sliderWidth, fHeight, 4, 4, false, false, fSelFade, 0);
}

void tInsideBoxTwoWaySlider::Calibrate(void)
{
    FUNCTION_MARKER(0x8001F918, "FRONT.BIN");
    sint32 player = (uint8)FEApp->fInputPlayer;
    PAD_COMMON *pad = &gPadinfo.buf[player * 4];
    fHelpText = screenControllerConfig->GetHelpText();
    sint32 range;
    sint32 limit;
    switch (fType)
    {
    case 0:
        if (pad->ID == 0x23)
        {
            range = 128 - pad->data.analog.rightx;
            if (range < 0)
                range = -range;
            limit = ((uint8)frontEnd.deadSpot[player]) + 10;
            if (range < limit)
                range = limit;
            frontEnd.steeringRange[player] = (char)range;
        }
        else if (pad->ID == 0x53 || pad->ID == 0x73)
        {
            range = (uint8)GetCurrentStickRange(player);
            limit = ((uint8)frontEnd.J1MIN[player]) + 10;
            if (range < limit)
                range = limit;
            frontEnd.J1MAX[player] = (char)range;
        }
        break;
    case 1:
        if (pad->ID == 0x23)
        {
            range = 128 - pad->data.analog.rightx;
            if (range < 0)
                range = -range;
            limit = ((uint8)frontEnd.steeringRange[player]) - 10;
            if (range > limit)
                range = limit;
            if (range < 0)
                range = 0;
            frontEnd.deadSpot[player] = (char)range;
        }
        else if (pad->ID == 0x53 || pad->ID == 0x73)
        {
            range = (uint8)GetCurrentStickRange(player);
            limit = ((uint8)frontEnd.J1MAX[player]) - 10;
            if (range > limit)
                range = limit;
            if (range < 0)
                range = 0;
            frontEnd.J1MIN[player] = (char)range;
        }
        break;
    case 2:
        if (pad->ID == 0x23)
            frontEnd.ImaxRange[player] = (char)pad->data.analog.righty;
        else if (pad->ID == 0x53 || pad->ID == 0x73)
        {
            range = (uint8)GetCurrentStickRange2(player);
            limit = ((uint8)frontEnd.J2MIN[player]) + 10;
            if (range < limit)
                range = limit;
            frontEnd.J2MAX[player] = (char)range;
            frontEnd.J2MAX[player] = (char)range;
        }
        break;
    case 3:
        if (pad->ID == 0x23)
        {
            range = pad->data.analog.leftx;
            if (range < 10)
                range = 10;
            frontEnd.IImaxRange[player] = (char)range;
        }
        else if (pad->ID == 0x53 || pad->ID == 0x73)
        {
            range = (uint8)GetCurrentStickRange2(player);
            limit = ((uint8)frontEnd.J2MAX[player]) - 10;
            if (range > limit)
                range = limit;
            if (range < 0)
                range = 0;
            frontEnd.J2MIN[player] = (char)range;
        }
        break;
    }
}

tUserNameMenuItem::tUserNameMenuItem(uint32 textDescription) : tMenuItem(textDescription)
{
    FUNCTION_MARKER(0x8001FC38, "FRONT.BIN");

    fCurrentRow = 0;
    fCurrentColumn = 0;
    for (sint16 i = 0; i < 10; ++i)
        fRowList[i][0] = '\0';
    fData = 0;
}

void tUserNameMenuItem::ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x8001FD2C, "FRONT.BIN");
    switch (keyval)
    {
    case kInput_KeyType_Up:
        fCurrentRow = (sint16)(sint32(fCurrentRow) - 1);
        if (fCurrentRow < 0)
            fCurrentRow = (sint16)(sint32(menu_kUserNameRows) - 1);
        AudioCmn::PlayFESFX(3);
        if (fCurrentColumn < 0)
            fCurrentColumn = 0;
        keyval = kInput_KeyType_AlreadyProcessed;
        break;
    case kInput_KeyType_Down:
        fCurrentRow = (sint16)(sint32(fCurrentRow) + 1);
        if (fCurrentRow >= menu_kUserNameRows)
            fCurrentRow = 0;
        AudioCmn::PlayFESFX(4);
        if (fCurrentColumn < 0)
            fCurrentColumn = 0;
        keyval = kInput_KeyType_AlreadyProcessed;
        break;
    case kInput_KeyType_Left:
        fCurrentColumn = (sint16)(sint32(fCurrentColumn) - 1);
        if (fCurrentColumn < 0)
            fCurrentColumn = 5;
        AudioCmn::PlayFESFX(3);
        keyval = kInput_KeyType_AlreadyProcessed;
        break;
    case kInput_KeyType_Right:
    {
        fCurrentColumn = (sint16)(sint32(fCurrentColumn) + 1);
        if (fCurrentColumn >= 6)
            fCurrentColumn = 0;
        AudioCmn::PlayFESFX(4);
        keyval = kInput_KeyType_AlreadyProcessed;
        sint32 row = fCurrentRow;
        while (fRowList[row][fCurrentColumn] == '-')
        {
            fCurrentColumn = (sint16)(sint32(fCurrentColumn) + 1);
            if (fCurrentColumn >= 6)
                fCurrentColumn = 0;
        }
        break;
    }
    case kInput_KeyType_Cross:
    {
        sint16 len = (sint16)strlen(fData);
        uint8 current = (uint8)fRowList[fCurrentRow][fCurrentColumn];
        if (current == '#' || current == '$')
        {
            if (len > 0)
            {
                fData[sint32(len) - 1] = '\0';
                AudioCmn::PlayFESFX(21);
            }
            keyval = kInput_KeyType_AlreadyProcessed;
        }
        else if (current == '!' || current == '@')
        {
            if (CheckForCheats(fData))
                fData[0] = '\0';
            else
            {
                keyval = (FEApp->fCurrentMenu[0]->fFlags & 0x100U) != 0 ? kInput_KeyType_Triangle : kInput_KeyType_Start;
                AudioCmn::PlayFESFX(1);
            }
        }
        else if (current == '&' || current == '^')
        {
            if (len < fMaxStringLength)
            {
                fData[len] = ' ';
                fData[sint32(len) + 1] = '\0';
                AudioCmn::PlayFESFX(21);
            }
            else
                AudioCmn::PlayFESFX(1);
        }
        else
        {
            if (len < fMaxStringLength)
            {
                fData[len] = (char)current;
                fData[sint32(len) + 1] = '\0';
                AudioCmn::PlayFESFX(21);
                keyval = kInput_KeyType_AlreadyProcessed;
            }
            else
                AudioCmn::PlayFESFX(1);
        }
        break;
    }
    default:
        break;
    }
    sint32 row = fCurrentRow;
    while (fRowList[row][fCurrentColumn] == '-')
    {
        fCurrentColumn = (sint16)(sint32(fCurrentColumn) - 1);
        if (fCurrentColumn < 0)
            fCurrentColumn = 5;
    }
    Stattool::SamNelsonsUpperLowerStringConverterForRecords(fData);
}

void tUserNameMenuItem::Draw(bool)
{
    FUNCTION_MARKER(0x8002017C, "FRONT.BIN");
    sint32 x = TextSys::WordX(fTextDescription);
    sint32 y = TextSys::WordY(fTextDescription);
    tTexture::ShapeInfo *shape = gHelpShapes + 30;
    if (fFadeVal != 128)
    {
        char output[2];
        output[1] = '\0';
        sint32 ColText = CalcFadeVal(0xBEBE, fFadeVal);
        sint32 Col = CalcFadeVal(0xBEBE, fFadeVal);
        char *text = TextSys::Word(fTextDescription);
        FETextRender::FullTextRGB(text, x, y, ColText, 0, 0);
        sint16 sl = (sint16)strlen(fData);
        sint32 textStartX = (x + 78) - (fMaxStringLength * 10);
        for (sint16 j = 0; j < sl; ++j)
        {
            output[0] = fData[j];
            FETextRender::FullTextRGB(output, (textStartX + sint32(j) * 20), (y + 17), ColText, 1, 0);
        }
        sint16 startx = (sint16)textStartX;
        sint16 j = sl;
        while (j < fMaxStringLength)
        {
            PSXDrawSquare(Col, (startx + (j * 20)), y + 25, 17, 1);
            j = (sint16)(sint32(j) + 1);
        }
        sint32 xx = (fCurrentColumn * 28) + 258;
        sint32 yy = MENUUSERNAME_STARTY + (fCurrentRow * 15);
        if (fFadeVal == 0)
        {
            tDrawShapeExtended fFlags;
            fFlags.tint[0] = PulsateYellow;
            uint8 current = (uint8)fRowList[fCurrentRow][fCurrentColumn];
            sint32 shapetodraw;
            if (current == '!' || current == '@')
                shapetodraw = 78;
            else if (current == '#' || current == '$')
                shapetodraw = 79;
            else if (current == '&' || current == '^')
                shapetodraw = 80;
            else if (current >= 'a' && current <= 'z')
                shapetodraw = current - 55;
            else if (current >= '0' && current <= '9')
                shapetodraw = current + 20;
            else
                shapetodraw = SpecialCharacter((char)current);
            DrawShapeExtended(shapetodraw, 0x10, xx - 5, yy - 3, 0, 0, &fFlags);
        }
    }
    sint32 right = x + 156;
    DrawShapeExtended(30, 8, (right - shape->width), y - 3, fFadeVal, 0, 0);
    PSXDrawSquare(0, x, y - 3, (156 - shape->width), shape->height);
    shape = gHelpShapes + 33;
    DrawShapeExtended(33, 8, (right - shape->width), y + 12, fFadeVal, 0, 0);
    PSXDrawSquare(0, x, y + 12, (156 - shape->width), shape->height);
}

void tUserNameMenuItem::TransitionOff(void)
{
    FUNCTION_MARKER(0x80020574, "FRONT.BIN");
    fFadeDir = 30;
}

void tUserNameMenuItem::TransitionOn(void)
{
    FUNCTION_MARKER(0x80020580, "FRONT.BIN");
    for (sint16 i = 0; i < 10; ++i)
        sprintf(fRowList[i], TextSys::Word(i + 507));
    sint16 NumberOfRows[6] = { 7, 9, 9, 9, 8, 9 };
    menu_kUserNameRows = NumberOfRows[(uint8)frontEnd.language];
    fCurrentRow = 0;
    sint32 i = 0;
    uint8 current = (uint8)fRowList[0][fCurrentColumn];
    while (current != '!' && current != '@' && i < menu_kUserNameRows)
    {
        fCurrentColumn = 0;
        current = (uint8)fRowList[fCurrentRow][0];
        sint32 column = 0;
        if (current != '!' && current != '@')
        {
            while (column < (sint32)strlen(fRowList[fCurrentRow]))
            {
                fCurrentColumn = (sint16)(sint32(fCurrentColumn) + 1);
                column = fCurrentColumn;
                current = (uint8)fRowList[fCurrentRow][column];
                if (current == '!' || current == '@')
                    break;
            }
        }
        fCurrentRow = (sint16)(sint32(fCurrentRow) + 1);
        i = fCurrentRow;
        current = (uint8)fRowList[fCurrentRow][fCurrentColumn];
    }
    fFadeVal = 128;
    fFadeDir = -30;
    fSelFade = 0;
    fCurrentRow = (sint16)(sint32(fCurrentRow) - 1);
    sint32 speech = FEApp->speechToPlay[fPlayer];
    if (speech != -1)
    {
        FeAudio::AsyncPlaySpeech(2, speech);
        FEApp->speechToPlay[fPlayer] = -1;
    }
}

bool tUserNameMenuItem::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x800207B4, "FRONT.BIN");
    fInTransition = 0;
    if (fFadeDir < 0 && fFadeVal > 0)
        fInTransition = 1;
    if (fFadeDir > 0)
    {
        if (fFadeVal < 128)
            fInTransition = 1;
        else
        {
            uint16 speechState;
            memcpy(&speechState, &ginfo.areLoading, sizeof(speechState));
            if (speechState != 0)
                fInTransition = 1;
        }
    }
    return fInTransition == 0;
}

void tUserNameMenuItem::UpdateTransition(bool selected)
{
    FUNCTION_MARKER(0x80020820, "FRONT.BIN");
    sint32 fade = fFadeVal + fFadeDir;
    if (fade > 128)
        fade = 128;
    if (fade < 0)
        fade = 0;
    fFadeVal = (sint16)fade;
    TransitionIsFinished();
    tMenuItem::UpdateTransition(selected);
}

void tInsideBoxControllerLeftRightSlider::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x80020AF4, "FRONT.BIN");

    if (keyval == kInput_KeyType_Left)
        screenControllerConfig->fResetShakeTimeOut = 1;
    else if (keyval == kInput_KeyType_Right)
    {
        uint8 value = (uint8)fData->Value(kPlayerBoth);
        if (value != (uint8)fData->fMaxValue)
            screenControllerConfig->fResetShakeTimeOut = 1;
    }
    tMenuItemLeftRightSlider::ProcessInput(fromPlayer, keyval, command);
}

tBlankMenuItemGoToMenuNFS4Button::tBlankMenuItemGoToMenuNFS4Button(uint32 textDescription, tMenu *newMenu, void (*OnButtonPress)(tMenuCommand &), sint32 firstFrame, sint32 numFrames) : tMenuItemGoToMenuNFS4Button(textDescription, newMenu, OnButtonPress, firstFrame, numFrames)
{
}

tInsideBoxControllerLeftRightSlider::tInsideBoxControllerLeftRightSlider(uint32 textDescription, tListIterator *dataPtr) : tInsideBoxLeftRightSlider(textDescription, dataPtr)
{
}

tMenuItemGoToMenuButtonFade::tMenuItemGoToMenuButtonFade(uint32 textDescription, tMenu *newMenu, void (*OnButtonPress)(tMenuCommand &)) : tMenuItemGoToMenuButton(textDescription, newMenu, OnButtonPress)
{
}

tMenuItemSlidingActivated::tMenuItemSlidingActivated(uint32 textDescription, sint16 width, sint16 height, sint16 diffx, sint16 diffy, bool fillback) : tMenuItemSlidingMenu(textDescription, width, height, diffx, diffy, fillback)
{
}

tMenuItemDisplayLeftRightChoice::tMenuItemDisplayLeftRightChoice(uint32 textDescription, tListIterator *dataPtr) : tMenuItemLeftRightFade(textDescription, dataPtr)
{
}

tMenuItemOnOffLeftRightChoice::tMenuItemOnOffLeftRightChoice(uint32 textDescription, tListIterator *dataPtr) : tMenuItemLeftRightFade(textDescription, dataPtr)
{
}

tMenuItemControllerLeftRightChoice::tMenuItemControllerLeftRightChoice(uint32 textDescription, tListIterator *dataPtr) : tMenuItemLeftRightFade(textDescription, dataPtr)
{
}

void tBlankMenuItemGoToMenuNFS4Button::Draw(bool selected)
{
    FUNCTION_MARKER(0x800321D0, "FRONT.BIN");
}

void tBlankMenuItemGoToMenuNFS4Button::Draw(sint32 x, sint32 y, bool selected)
{
    FUNCTION_MARKER(0x800321C8, "FRONT.BIN");
}

bool tBlankMenuItemGoToMenuNFS4Button::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x800321C0, "FRONT.BIN");

    return true;
}
