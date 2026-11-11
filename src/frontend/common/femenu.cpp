#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "frontend/common/feapp.h"
#include "frontend/common/fefades.h"
#include "frontend/common/femenu.h"
#include "frontend/common/fetextrender.h"
#include "game/common/audiocmn.h"
#include "game/common/new.h"
#include "game/common/textsys.h"
#include "game/psx/drawc.h"

tPlayer gMenu_SubMenuPlayer = kPlayerBoth;

void *tListIterator::operator new(size_t size) throw()
{
    return __builtin_new((uint32)size);
}

void tListIterator::operator delete(void *memory) throw()
{
    purgememadr(memory);
}

void *tMenuItem::operator new(size_t size) throw()
{
    return __builtin_new((uint32)size);
}

void tMenuItem::operator delete(void *memory) throw()
{
    purgememadr(memory);
}

void *tMenu::operator new(size_t size) throw()
{
    return __builtin_new((uint32)size);
}

void tMenu::operator delete(void *memory) throw()
{
    purgememadr(memory);
}

tListIterator::tListIterator(sint16 *selection, char *valPtr)
{
    FUNCTION_MARKER(0x80023DD0, "FRONT.BIN");

    fSelectionList = selection;
    fValue = valPtr;
    fMaxValue = 0;
    fMinValue = 0;
}

tListIterator::~tListIterator(void)
{
    FUNCTION_MARKER(0x80023DF4, "FRONT.BIN");
}

char tListIterator::Value(tPlayer)
{
    FUNCTION_MARKER(0x80023E28, "FRONT.BIN");

    return *fValue;
}

sint16 tListIterator::TextValue(tPlayer)
{
    FUNCTION_MARKER(0x80023E3C, "FRONT.BIN");

    char value = Value(kPlayerBoth);
    return fSelectionList[value];
}

void tListIterator::Increment(tPlayer)
{
    FUNCTION_MARKER(0x80023E8C, "FRONT.BIN");

    ++*fValue;
    if (fSelectionList[*fValue] == 0)
        *fValue = 0;
}

void tListIterator::Decrement(tPlayer)
{
    FUNCTION_MARKER(0x80023ED8, "FRONT.BIN");

    if (*fValue != 0)
        --*fValue;
    else
        while (fSelectionList[*fValue + 1] > 0)
            ++*fValue;
}

tListIteratorIndexed::tListIteratorIndexed(sint16 *selection, char *valPtr, char *index) : tListIterator(selection, valPtr)
{
    FUNCTION_MARKER(0x80023F60, "FRONT.BIN");

    fIndex = index;
}

tListIteratorIndexed::~tListIteratorIndexed(void)
{
    FUNCTION_MARKER(0x80023FA4, "FRONT.BIN");
}

tListIteratorDoubleIndexed::~tListIteratorDoubleIndexed(void)
{
    FUNCTION_MARKER(0x80024154, "FRONT.BIN");
}

tListIteratorMultiPlayer::~tListIteratorMultiPlayer(void)
{
    FUNCTION_MARKER(0x80024390, "FRONT.BIN");
}

char tListIteratorIndexed::Value(tPlayer)
{
    FUNCTION_MARKER(0x80023FCC, "FRONT.BIN");

    return fValue[*fIndex];
}

sint16 tListIteratorIndexed::TextValue(tPlayer)
{
    FUNCTION_MARKER(0x80023FF0, "FRONT.BIN");

    char value = Value(kPlayerBoth);
    return fSelectionList[value];
}

void tListIteratorIndexed::Increment(tPlayer)
{
    FUNCTION_MARKER(0x80024040, "FRONT.BIN");

    ++fValue[*fIndex];
    char *value = fValue + *fIndex;
    if (fSelectionList[*value] == 0)
        *value = 0;
}

void tListIteratorIndexed::Decrement(tPlayer)
{
    FUNCTION_MARKER(0x800240A8, "FRONT.BIN");

    char *value = fValue + *fIndex;
    if (*value != 0)
        --*value;
    else
        while (fSelectionList[*value + 1] > 0)
        {
            ++fValue[*fIndex];
            value = fValue + *fIndex;
        }
}

char tListIteratorDoubleIndexed::Value(tPlayer)
{
    FUNCTION_MARKER(0x8002417C, "FRONT.BIN");

    sint32 offset = (sint32)((uint32)*fIndex1 * (uint32)index1multiplier + *fIndex2);
    return fValue[offset];
}

sint16 tListIteratorDoubleIndexed::TextValue(tPlayer)
{
    FUNCTION_MARKER(0x800241B8, "FRONT.BIN");

    char value = Value(kPlayerBoth);
    return fSelectionList[value];
}

void tListIteratorDoubleIndexed::Increment(tPlayer)
{
    FUNCTION_MARKER(0x80024208, "FRONT.BIN");

    sint32 offset = (sint32)((uint32)*fIndex1 * (uint32)index1multiplier + *fIndex2);
    ++fValue[offset];
    offset = (sint32)((uint32)*fIndex1 * (uint32)index1multiplier + *fIndex2);
    char *value = fValue + offset;
    if (fSelectionList[*value] == 0)
        *value = 0;
}

void tListIteratorDoubleIndexed::Decrement(tPlayer)
{
    FUNCTION_MARKER(0x800242A4, "FRONT.BIN");

    sint32 offset = (sint32)((uint32)*fIndex1 * (uint32)index1multiplier + *fIndex2);
    char *value = fValue + offset;
    if (*value != 0)
        --*value;
    else
        while (fSelectionList[*value + 1] > 0)
        {
            offset = (sint32)((uint32)*fIndex1 * (uint32)index1multiplier + *fIndex2);
            ++fValue[offset];
            offset = (sint32)((uint32)*fIndex1 * (uint32)index1multiplier + *fIndex2);
            value = fValue + offset;
        }
}

char tListIteratorMultiPlayer::Value(tPlayer atIndex)
{
    FUNCTION_MARKER(0x800243B8, "FRONT.BIN");

    if (atIndex == kPlayerBoth)
        atIndex = kPlayerOne;
    return fValue[atIndex];
}

sint16 tListIteratorMultiPlayer::TextValue(tPlayer atIndex)
{
    FUNCTION_MARKER(0x800243E0, "FRONT.BIN");

    char value = Value(atIndex);
    return fSelectionList[value];
}

void tListIteratorMultiPlayer::Increment(tPlayer atIndex)
{
    FUNCTION_MARKER(0x80024430, "FRONT.BIN");

    if (atIndex == kPlayerBoth)
        atIndex = kPlayerOne;
    ++fValue[atIndex];
    char *value = fValue + atIndex;
    if (fSelectionList[*value] == 0)
        *value = 0;
}

void tListIteratorMultiPlayer::Decrement(tPlayer atIndex)
{
    FUNCTION_MARKER(0x80024494, "FRONT.BIN");

    if (atIndex == kPlayerBoth)
        atIndex = kPlayerOne;
    char *value = fValue + atIndex;
    if (*value != 0)
        --*value;
    else
        while (fSelectionList[*value + 1] > 0)
        {
            ++*value;
            value = fValue + atIndex;
        }
}

tListIteratorRange::tListIteratorRange(char minValue, char maxValue, char *valPtr) : tListIterator(0, valPtr)
{
    FUNCTION_MARKER(0x80024534, "FRONT.BIN");

    fMinValue = minValue;
    fMaxValue = maxValue;
}

tListIteratorRange::~tListIteratorRange(void)
{
    FUNCTION_MARKER(0x80024590, "FRONT.BIN");
}

char tListIteratorRange::Value(tPlayer)
{
    FUNCTION_MARKER(0x800245B8, "FRONT.BIN");

    return *fValue;
}

sint16 tListIteratorRange::TextValue(tPlayer)
{
    FUNCTION_MARKER(0x800245CC, "FRONT.BIN");

    return 0;
}

void tListIteratorRange::Increment(tPlayer)
{
    FUNCTION_MARKER(0x800245D4, "FRONT.BIN");

    if (*fValue < fMaxValue)
        ++*fValue;
}

void tListIteratorRange::Decrement(tPlayer)
{
    FUNCTION_MARKER(0x800245FC, "FRONT.BIN");

    if (*fValue > fMinValue)
        --*fValue;
}

tListIteratorRangeIndexed::tListIteratorRangeIndexed(char minValue, char maxValue, char *valPtr, char *index) : tListIteratorRange(minValue, maxValue, valPtr)
{
    FUNCTION_MARKER(0x80024624, "FRONT.BIN");

    fIndex = index;
}

tListIteratorRangeIndexed::~tListIteratorRangeIndexed(void)
{
    FUNCTION_MARKER(0x80024670, "FRONT.BIN");
}

char tListIteratorRangeIndexed::Value(tPlayer)
{
    FUNCTION_MARKER(0x80024698, "FRONT.BIN");

    return fValue[*fIndex];
}

void tListIteratorRangeIndexed::Increment(tPlayer)
{
    FUNCTION_MARKER(0x800246BC, "FRONT.BIN");

    char *value = fValue + *fIndex;
    if (*value < fMaxValue)
        ++*value;
}

void tListIteratorRangeIndexed::Decrement(tPlayer)
{
    FUNCTION_MARKER(0x800246F8, "FRONT.BIN");

    char *value = fValue + *fIndex;
    if (*value > fMinValue)
        --*value;
}

tMenuItem::tMenuItem(uint32 textDescription)
{
    FUNCTION_MARKER(0x80024734, "FRONT.BIN");

    fTextDescription = textDescription;
    fFlags = 0;
    fSelFade = 0;
    fButtonImage = -1;
    fNumFrames = 0;
    fNewMenu = 0;
}

tMenuItem::~tMenuItem(void)
{
    FUNCTION_MARKER(0x80024764, "FRONT.BIN");
}

sint32 tMenuItem::DebounceKeys(void)
{
    FUNCTION_MARKER(0x80024798, "FRONT.BIN");

    return -1;
}

void tMenuItem::ProcessInput(tPlayer, tInputKeyType &, tMenuCommand &)
{
    FUNCTION_MARKER(0x800247A0, "FRONT.BIN");
}

void tMenuItem::UpdateTransition(bool selected)
{
    FUNCTION_MARKER(0x800247A8, "FRONT.BIN");

    UpdateSelFade(selected);
}

bool tMenuItem::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x800247C8, "FRONT.BIN");

    return true;
}

void tMenuItem::UpdateSelFade(bool selected)
{
    FUNCTION_MARKER(0x800247D0, "FRONT.BIN");

    fSelFade = (sint16)(fSelFade + (selected ? 64 : -10));
    if (fSelFade > 128)
        fSelFade = 128;
    if (fSelFade < 0)
        fSelFade = 0;
}

void tMenuItem::Draw(sint32, sint32, bool selected)
{
    FUNCTION_MARKER(0x80024828, "FRONT.BIN");

    Draw(selected);
}

void tMenuItem::Draw(sint32 x, sint32 y, sint32, bool selected)
{
    FUNCTION_MARKER(0x80024864, "FRONT.BIN");

    Draw(selected, x, y, false);
}

tMenuItemInteractive::tMenuItemInteractive(uint32 textDescription) : tMenuItem(textDescription)
{
    FUNCTION_MARKER(0x800248A8, "FRONT.BIN");
}

tMenuItemInteractive::~tMenuItemInteractive(void)
{
    FUNCTION_MARKER(0x800248DC, "FRONT.BIN");
}

tMenuItemLeftRightChoice::tMenuItemLeftRightChoice(uint32 textDescription, tListIterator *dataPtr) : tMenuItemInteractive(textDescription)
{
    FUNCTION_MARKER(0x80024904, "FRONT.BIN");

    fData = dataPtr;
    fFlags |= 0x400U;
}

tMenuItemLeftRightChoice::~tMenuItemLeftRightChoice(void)
{
    FUNCTION_MARKER(0x80024954, "FRONT.BIN");
}

void tMenuItemLeftRightChoice::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x8002497C, "FRONT.BIN");

    if ((fFlags & 1U) != 0)
        return;
    sint32 SFXnum;
    if (keyval == kInput_KeyType_Left)
    {
        fData->Decrement(fromPlayer);
        SFXnum = 5;
    }
    else if (keyval == kInput_KeyType_Right)
    {
        fData->Increment(fromPlayer);
        SFXnum = 6;
    }
    else
        return;
    keyval = kInput_KeyType_AlreadyProcessed;
    AudioCmn::PlayFESFX(SFXnum);
}

void tMenuItemLeftRightChoice::Draw(bool selected)
{
    FUNCTION_MARKER(0x80024A2C, "FRONT.BIN");

    sint16 x = (sint16)TextSys::WordX(fTextDescription);
    sint16 y = (sint16)TextSys::WordY(fTextDescription);
    tMenuTextState textState = selected ? textState_Selected : textState_Unselected;
    FETextRender::MenuTextPositioned((sint16)fTextDescription, x, y, textState, textType_Options);
    sint16 index = fData->TextValue(gMenu_SubMenuPlayer);
    FETextRender::MenuTextPositioned(index, (sint16)(x + 180), y, textState, textType_Options);
}

tMenuItemLeftRightSlider::tMenuItemLeftRightSlider(uint32 textDescription, tListIterator *dataPtr) : tMenuItemInteractive(textDescription)
{
    FUNCTION_MARKER(0x80024AF8, "FRONT.BIN");

    fData = dataPtr;
    fFlags |= 0x80U;
    fFlags |= 0x80U;
}

tMenuItemLeftRightSlider::~tMenuItemLeftRightSlider(void)
{
    FUNCTION_MARKER(0x80024B54, "FRONT.BIN");
}

void DrawSlider(sint16 value, sint16 min, sint16 max, sint16 fX, sint16 fY, sint16 fWidth, sint16 fHeight, sint16 rectwidth, sint16 rectspace, bool reverse, bool shadow, sint16 fSelFade, sint16 fFadeVal)
{
    FUNCTION_MARKER(0x80024C2C, "FRONT.BIN");
    sint32 range = sint32(max) - sint32(min);
    sint32 numerator = (sint32(value) - sint32(min)) * sint32(fWidth);
    if (range == 0)
        abort();
    sint16 width = (sint16)(numerator / range);
    sint32 factor = fSelFade == 0 ? 1 : 0;
    sint32 myDarkBlue = 0xC83C1E;
    sint32 end = sint32(fX) + sint32(fWidth);
    sint16 x1 = reverse ? (sint16)(end - 1) : fX;
    if (reverse ? x1 < fX : sint32(x1) >= end)
        return;
    for (;;)
    {
        Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
        POLY_F4 *prim = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(prim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
        setaddr(ot, prim);
        sint16 right = (sint16)(sint32(x1) + sint32(rectwidth));
        sint16 bottom = (sint16)(sint32(fY) + sint32(fHeight));
        prim->p[0].x = x1;
        prim->p[0].y = fY;
        prim->p[1].x = right;
        prim->p[1].y = fY;
        prim->p[2].x = x1;
        prim->p[2].y = bottom;
        prim->p[3].x = right;
        prim->p[3].y = bottom;
        sint32 Col = 0;
        if (!shadow)
        {
            bool filled = reverse ? sint32(x1) >= end - sint32(width) : sint32(x1) < sint32(fX) + sint32(width);
            sint32 color = 0x280F00;
            if (filled)
            {
                color = myDarkBlue;
                if (fSelFade != 0)
                {
                    sint32 distance = reverse ? end - sint32(x1) : sint32(x1) - sint32(fX);
                    if (fWidth == 0)
                        abort();
                    sint32 red = (distance * 190 / sint32(fWidth)) >> factor;
                    sint32 green = (distance * 124 / sint32(fWidth) + 66) >> factor;
                    sint32 blue = (distance * -210 / sint32(fWidth) + 210) >> factor;
                    uint32 packed = (uint32(blue) << 16) |
                        uint32((sint32)(uint32(green) << 16) >> 8) | uint32((sint16)red);
                    color = CalcFadeVal(myDarkBlue, (sint32)packed, fSelFade);
                }
            }
            Col = CalcFadeVal(color, fFadeVal);
        }
        uint32 color = (uint32)Col;
        memcpy(&prim->rgbc, &color, sizeof(color));
        SetPolyF4(prim);
        SetSemiTrans(prim, 0);
        sint32 step = sint32((uint16)rectwidth) + sint32((uint16)rectspace);
        x1 = (sint16)(reverse ? sint32(x1) - step : sint32(x1) + step);
        if (reverse ? x1 < fX : sint32(x1) >= end)
            break;
    }
}

sint32 tMenuItemLeftRightSlider::DebounceKeys(void)
{
    FUNCTION_MARKER(0x80024B7C, "FRONT.BIN");

    return 0x600;
}

void tMenuItemLeftRightSlider::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x80024B84, "FRONT.BIN");

    if ((fFlags & 1U) != 0)
        return;
    if (keyval == kInput_KeyType_Left)
        fData->Decrement(fromPlayer);
    else if (keyval == kInput_KeyType_Right)
        fData->Increment(fromPlayer);
    else
        return;
    keyval = kInput_KeyType_AlreadyProcessed;
    AudioCmn::PlayFESFXVol(21, 64);
}

void tMenuItemLeftRightSlider::Draw(bool selected)
{
    FUNCTION_MARKER(0x800251E4, "FRONT.BIN");

    if (fX == 0 && fY == 0)
    {
        fX = (sint16)TextSys::WordX(fTextDescription);
        fY = (sint16)TextSys::WordY(fTextDescription);
    }
    FETextRender::MenuTextPositioned((sint16)fTextDescription, (sint16)(fX + 8), (sint16)(fY + 3), selected ? textState_Selected : textState_Unselected, textType_Options);
    char value = fData->Value(kPlayerBoth);
    DrawSlider(value, fData->fMinValue, fData->fMaxValue, (sint16)(fX + 115), (sint16)(fY + 4), fWidth, fHeight, 6, 4, false, false, selected ? 128 : 0, 0);
}

void tMenuItemLeftRightSlider::SetDimensions(sint16 x, sint16 y, sint16 width, sint16 height)
{
    FUNCTION_MARKER(0x80025310, "FRONT.BIN");

    fX = x;
    fY = y;
    fWidth = width;
    fHeight = height;
}

tMenuItemGoToMenuButton::tMenuItemGoToMenuButton(uint32 textDescription, tMenu *newMenu, void (*OnButtonPress)(tMenuCommand &)) : tMenuItemInteractive(textDescription)
{
    FUNCTION_MARKER(0x80025328, "FRONT.BIN");

    fNewMenu = newMenu;
    fOnButtonPress = OnButtonPress;
}

tMenuItemGoToMenuButton::~tMenuItemGoToMenuButton(void)
{
    FUNCTION_MARKER(0x8002537C, "FRONT.BIN");
}

void tMenuItemGoToMenuButton::ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x800253A4, "FRONT.BIN");

    if ((fFlags & 1U) != 0 || keyval != kInput_KeyType_Cross)
        return;
    if (fNewMenu != 0)
    {
        command.type = (fFlags & 0x40U) != 0 ? kMenu_Command_GoToMenuOneWay : kMenu_Command_GoToMenu;
        command.nextMenu = fNewMenu;
    }
    if (fOnButtonPress != 0)
        fOnButtonPress(command);
    keyval = kInput_KeyType_AlreadyProcessed;
}

void tMenu::tMenuConstructor(tMenuItem *firstItem, void *ap)
{
    FUNCTION_MARKER(0x80025434, "FRONT.BIN");

    VertHelp = 0;
    fItemList[0] = firstItem;
    tMenuItem **items = (tMenuItem **)ap;
    sint32 i = 0;
    tMenuItem *p;
    do
    {
        p = *items++;
        fItemList[++i] = p;
    } while (p != 0);
}

tMenu::tMenu(uint32 flags, tScreen *screenHandler, tMenu *nextMenu, tMenu *optionsMenu, void (*OnButtonPress)(tMenuCommand &), sint16 title)
{
    FUNCTION_MARKER(0x8002546C, "FRONT.BIN");

    fFlags = flags;
    fCurrentItem = 0;
    fScreen = screenHandler;
    fNextMenu = nextMenu;
    fNeverAnyEnabled = false;
    fChildMenu = 0;
    fOptionsMenu = optionsMenu;
    fOnButtonPress = OnButtonPress;
    fTitle = title;
}

tMenu::~tMenu(void)
{
    FUNCTION_MARKER(0x800254B0, "FRONT.BIN");
}

void tMenu::Initialize(void)
{
    FUNCTION_MARKER(0x800254E4, "FRONT.BIN");

    sint32 original = fCurrentItem;
    if ((fItemList[fCurrentItem]->fFlags & 1U) == 0)
        return;
    do
    {
        if ((fItemList[fCurrentItem]->fFlags & 1U) == 0)
            return;
        fCurrentItem = (sint32)((uint32)fCurrentItem + 1U);
        if (fItemList[fCurrentItem] == 0)
            fCurrentItem = 0;
    } while (fCurrentItem != original);
}

void tMenu::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x80025578, "FRONT.BIN");

    if ((fFlags & 4U) != 0 && keyval == kInput_KeyType_Start)
        keyval = kInput_KeyType_Cross;
    if ((fFlags & 0x10000U) != 0 && keyval == kInput_KeyType_Cross)
        keyval = kInput_KeyType_Start;
    if (fItemList[fCurrentItem] != 0)
        fItemList[fCurrentItem]->ProcessInput(fromPlayer, keyval, command);

    switch (keyval)
    {
    case kInput_KeyType_Up:
    {
        sint32 lastItem = fCurrentItem;
        do
        {
            if (fCurrentItem > 0)
                fCurrentItem = (sint32)((uint32)fCurrentItem - 1U);
            else
                while (fItemList[(sint32)((uint32)fCurrentItem + 1U)] != 0)
                    fCurrentItem = (sint32)((uint32)fCurrentItem + 1U);
        } while ((fItemList[fCurrentItem]->fFlags & 1U) != 0);
        if (fCurrentItem != lastItem)
            AudioCmn::PlayFESFX(3);
        break;
    }
    case kInput_KeyType_Down:
    {
        sint32 lastItem = fCurrentItem;
        do
        {
            fCurrentItem = (sint32)((uint32)fCurrentItem + 1U);
            if (fItemList[fCurrentItem] == 0)
                fCurrentItem = 0;
        } while ((fItemList[fCurrentItem]->fFlags & 1U) != 0);
        if (fCurrentItem != lastItem)
            AudioCmn::PlayFESFX(4);
        break;
    }
    case kInput_KeyType_Cross:
        if (fNextMenu == 0)
            return;
        command.type = kMenu_Command_GoToMenu;
        command.nextMenu = fNextMenu;
        break;
    case kInput_KeyType_Start:
        if (fOnButtonPress != 0)
            fOnButtonPress(command);
        else if ((fFlags & 1U) != 0)
            command.type = kMenu_Command_StartRace;
        else if ((fFlags & 2U) != 0)
            command.type = kMenu_Command_Start2PlayerRace;
        else
            return;
        break;
    case kInput_KeyType_Circle:
        FEApp->DisplayHelp(0);
        return;
    case kInput_KeyType_Square:
        if (fOptionsMenu == 0)
            return;
        command.type = kMenu_Command_GoToMenu;
        command.nextMenu = fOptionsMenu;
        break;
    case kInput_KeyType_Triangle:
        command.type = kMenu_Command_BackupMenu;
        break;
    default:
        return;
    }
    keyval = kInput_KeyType_AlreadyProcessed;
}

sint16 tMenu::GetNumberEnabledItems(void)
{
    FUNCTION_MARKER(0x80025894, "FRONT.BIN");

    sint16 result = 0;
    if (fNeverAnyEnabled)
        return 0;
    for (sint16 i = 0; fItemList[i] != 0; ++i)
        if ((fItemList[i]->fFlags & 1U) == 0)
            ++result;
    return result;
}

void tMenu::Draw(void)
{
    FUNCTION_MARKER(0x800258FC, "FRONT.BIN");

    if (fTitle >= 0)
        FETextRender::Title(fTitle);
    for (sint16 item = 0; fItemList[item] != 0; ++item)
        fItemList[item]->Draw(fCurrentItem == item);
}

void tMenu::UpdateTransition(void)
{
    FUNCTION_MARKER(0x800259A8, "FRONT.BIN");

    for (sint16 item = 0; fItemList[item] != 0; ++item)
        fItemList[item]->UpdateTransition(fCurrentItem == item);
}

void tMenu::TransitionOff(void)
{
    FUNCTION_MARKER(0x80025A3C, "FRONT.BIN");
}

void tMenu::TransitionOn(void)
{
    FUNCTION_MARKER(0x80025A44, "FRONT.BIN");
}

bool tMenu::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x80025A4C, "FRONT.BIN");

    return true;
}

bool tMenu::IsSubMenu(void)
{
    FUNCTION_MARKER(0x80025A54, "FRONT.BIN");

    return false;
}

sint32 tMenu::DebounceKeys(void)
{
    FUNCTION_MARKER(0x80025A5C, "FRONT.BIN");

    return fItemList[fCurrentItem]->DebounceKeys();
}

void tMenuItem::TransitionOn(void)
{
    FUNCTION_MARKER(0x80025AA8, "FRONT.BIN");
}

void tMenuItem::TransitionOff(void)
{
    FUNCTION_MARKER(0x80025AB0, "FRONT.BIN");
}
