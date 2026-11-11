#include <stdarg.h>
#include <stddef.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/audiocmn.h"
#include "game/common/gmesetup.h"
#include "game/common/mpause.h"
#include "game/common/pausemenu.h"
#include "game/common/textsys.h"
#include "game/psx/font.h"
#include "game/psx/hud.h"
#include "game/psx/rendercache.h"
#include "game/psx/textpix.h"

static sint32 gPause_CurrentY;
sint32 ChangedEnabling;

void *tPMenuItem::operator new(size_t size) throw()
{
    return reservememadr(0, (sint32)size, 0);
}

void tPMenuItem::operator delete(void *memory) throw()
{
    purgememadr(memory);
}

void *tPMenu::operator new(size_t size) throw()
{
    return reservememadr(0, (sint32)size, 0);
}

void tPMenu::operator delete(void *memory) throw()
{
    purgememadr(memory);
}

void *tPListIterator::operator new(size_t size) throw()
{
    return reservememadr(0, (sint32)size, 0);
}

void tPListIterator::operator delete(void *memory) throw()
{
    purgememadr(memory);
}

void PauseMenu::FullText(char *sMenuText, sint16 x, sint16 flags, sint16 color)
{
    FUNCTION_MARKER(0x800A6E60, "NFS4.EXE");
    if (gPause_CurrentY == 98)
    {
        x = 160;
        flags = 2;
        color = 6;
    }
    char *str = sMenuText;
    sint32 position = x;
    if (flags == 1)
        position = (sint32)((uint32)position - (uint32)textpixels(str));
    else if (flags == 2)
    {
        sint32 width = textpixels(str);
        position = (sint32)((uint32)position - (uint32)(width / 2));
    }
    Font::TextColor(color);
    Font::TextXY(str, (sint16)position, gPause_CurrentY);
}

void PauseMenu::MenuTextPositioned(sint16 index, sint16 selected, sint16 disabled, sint16 x)
{
    FUNCTION_MARKER(0x800A6F14, "NFS4.EXE");
    (void)disabled;
    sint32 flags = TextSys::WordFlags(index);
    char *str = TextSys::Word(index);
    sint16 color = selected != 0 ? 3 : 4;
    FullText(str, x, (sint16)flags, color);
}

void PauseMenu::MenuText(sint16 index, bool selected, bool disabled)
{
    FUNCTION_MARKER(0x800A6F8C, "NFS4.EXE");
    sint32 x = TextSys::WordX(index);
    MenuTextPositioned(index, (sint16)selected, (sint16)disabled, (sint16)x);
}

tPListIterator::tPListIterator(sint16 *selection, sint32 *valPtr)
{
    FUNCTION_MARKER(0x800A6FF0, "NFS4.EXE");
    fSelectionList = selection;
    fValue = valPtr;
}

tPListIterator::~tPListIterator(void)
{
    FUNCTION_MARKER(0x800A700C, "NFS4.EXE");
}

char tPListIterator::Value(tPlayer)
{
    FUNCTION_MARKER(0x800A7040, "NFS4.EXE");
    return (char)*fValue;
}

sint16 tPListIterator::TextValue(tPlayer)
{
    FUNCTION_MARKER(0x800A7054, "NFS4.EXE");
    uint8 value = (uint8)Value(kPlayerBoth);
    return fSelectionList[value];
}

void tPListIterator::Increment(tPlayer)
{
    FUNCTION_MARKER(0x800A70A4, "NFS4.EXE");
    sint32 *value = fValue;
    *value = (sint32)((uint32)*value + 1U);
    value = fValue;
    if (fSelectionList[*value] == 0)
        *value = 0;
    AudioCmn::PlayPauseSound(5);
    gMPauseUpdateNextTime = 1;
}

void tPListIterator::Decrement(tPlayer)
{
    FUNCTION_MARKER(0x800A7110, "NFS4.EXE");
    sint32 *value = fValue;
    sint32 current = *value;
    if (current == 0)
    {
        sint16 next = fSelectionList[1];
        while (next > 0)
        {
            *value = (sint32)((uint32)*value + 1U);
            value = fValue;
            next = fSelectionList[*value + 1];
        }
    }
    else
        *value = (sint32)((uint32)current - 1U);
    AudioCmn::PlayPauseSound(5);
    gMPauseUpdateNextTime = 1;
}

tPListIteratorIndexed::tPListIteratorIndexed(sint16 *selection, sint32 *valPtr, char *index) : tPListIterator(selection, valPtr)
{
    FUNCTION_MARKER(0x800A71B0, "NFS4.EXE");
    fIndex = index;
}

tPListIteratorIndexed::~tPListIteratorIndexed(void)
{
    FUNCTION_MARKER(0x800A71F4, "NFS4.EXE");
}

char tPListIteratorIndexed::Value(tPlayer)
{
    FUNCTION_MARKER(0x800A721C, "NFS4.EXE");
    uint8 index = (uint8)*fIndex;
    return (char)fValue[index];
}

sint16 tPListIteratorIndexed::TextValue(tPlayer)
{
    FUNCTION_MARKER(0x800A7240, "NFS4.EXE");
    uint8 value = (uint8)Value(kPlayerBoth);
    return fSelectionList[value];
}

void tPListIteratorIndexed::Increment(tPlayer)
{
    FUNCTION_MARKER(0x800A7290, "NFS4.EXE");
    uint8 index = (uint8)*fIndex;
    sint32 *value = &fValue[index];
    *value = (sint32)((uint32)*value + 1U);
    index = (uint8)*fIndex;
    value = &fValue[index];
    if (fSelectionList[*value] == 0)
        *value = 0;
    AudioCmn::PlayPauseSound(5);
    gMPauseUpdateNextTime = 1;
}

void tPListIteratorIndexed::Decrement(tPlayer)
{
    FUNCTION_MARKER(0x800A731C, "NFS4.EXE");
    char *index = fIndex;
    sint32 *values = fValue;
    sint32 *value = &values[(uint8)*index];
    sint32 current = *value;
    if (current == 0)
    {
        sint16 next = fSelectionList[1];
        while (next > 0)
        {
            value = &values[(uint8)*index];
            *value = (sint32)((uint32)*value + 1U);
            index = fIndex;
            values = fValue;
            next = fSelectionList[values[(uint8)*index] + 1];
        }
    }
    else
        *value = (sint32)((uint32)current - 1U);
    AudioCmn::PlayPauseSound(5);
    gMPauseUpdateNextTime = 1;
}

tPMenuItem::tPMenuItem(uint32 textDescription)
{
    FUNCTION_MARKER(0x800A73E8, "NFS4.EXE");
    fTextDescription = textDescription;
    fFlags = 0;
}

tPMenuItem::~tPMenuItem(void)
{
    FUNCTION_MARKER(0x800A7404, "NFS4.EXE");
}

tPMenu *tPMenuItem::NextMenu(void)
{
    FUNCTION_MARKER(0x800A7438, "NFS4.EXE");
    return 0;
}

bool tPMenuItem::Debounce(void)
{
    FUNCTION_MARKER(0x800A7440, "NFS4.EXE");
    return true;
}

void tPMenuItem::ProcessInput(tInputKeyType &, tPMenuCommand &)
{
    FUNCTION_MARKER(0x800A7448, "NFS4.EXE");
}

tPMenuItemNonInteractiveText::tPMenuItemNonInteractiveText(uint32 textDescription) : tPMenuItem(textDescription)
{
    FUNCTION_MARKER(0x800A7450, "NFS4.EXE");
}

tPMenuItemNonInteractiveText::~tPMenuItemNonInteractiveText(void)
{
    FUNCTION_MARKER(0x800A7484, "NFS4.EXE");
}

void tPMenuItemNonInteractiveText::Draw(bool)
{
    FUNCTION_MARKER(0x800A74AC, "NFS4.EXE");
    PauseMenu::MenuText((sint16)fTextDescription, false, false);
}

bool tPMenuItemNonInteractiveText::IsNavigable(void)
{
    FUNCTION_MARKER(0x800A86A0, "NFS4.EXE");
    return false;
}

tPMenuItemInteractive::tPMenuItemInteractive(uint32 textDescription) : tPMenuItem(textDescription)
{
    FUNCTION_MARKER(0x800A74D4, "NFS4.EXE");
}

tPMenuItemInteractive::~tPMenuItemInteractive(void)
{
    FUNCTION_MARKER(0x800A7508, "NFS4.EXE");
}

bool tPMenuItemInteractive::IsNavigable(void)
{
    FUNCTION_MARKER(0x800A8698, "NFS4.EXE");
    return true;
}

void tPMenuItemInteractive::Draw(bool selected)
{
    FUNCTION_MARKER(0x800A7530, "NFS4.EXE");
    bool disabled = (fFlags & 1) != 0;
    PauseMenu::MenuText((sint16)fTextDescription, selected, disabled);
}

tPMenuItemLeftRightChoice::tPMenuItemLeftRightChoice(uint32 textDescription, tPListIterator *dataPtr) : tPMenuItemInteractive(textDescription)
{
    FUNCTION_MARKER(0x800A7558, "NFS4.EXE");
    fData = dataPtr;
}

tPMenuItemLeftRightChoice::~tPMenuItemLeftRightChoice(void)
{
    FUNCTION_MARKER(0x800A759C, "NFS4.EXE");
}

void tPMenuItemLeftRightChoice::ProcessInput(tInputKeyType &keyval, tPMenuCommand &)
{
    FUNCTION_MARKER(0x800A75C4, "NFS4.EXE");
    if (keyval == kInput_KeyType_Left)
        fData->Decrement(kPlayerBoth);
    else if (keyval == kInput_KeyType_Right)
        fData->Increment(kPlayerBoth);
    else
        return;
    keyval = kInput_KeyType_AlreadyProcessed;
}

void tPMenuItemLeftRightChoice::Draw(bool selected)
{
    FUNCTION_MARKER(0x800A7650, "NFS4.EXE");
    sint32 position = TextSys::WordX((sint32)fTextDescription);
    PauseMenu::MenuTextPositioned((sint16)fTextDescription, (sint16)selected, (sint16)(fFlags & 1), (sint16)position);
    sint16 index = fData->TextValue(kPlayerBoth);
    sint16 x = (sint16)TextSys::WordX(index);
    PauseMenu::MenuTextPositioned(index, (sint16)selected, (sint16)(fFlags & 1), x);
    sint32 y = gPause_CurrentY;
    if (!selected || GameSetup::gData.userSetting.language != 0)
        return;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_GT4 *prim = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    char *string = TextSys::Word(index);
    sint32 width = textpixels(string);
    sint32 left = (sint32)((uint32)(sint32)x - (uint32)width - 8U);
    y = (sint32)((uint32)y + 5U);
    Hud::BuildGT4(prim, &HudPmx::gShapes[18], left, y, 0xBEBEUL);
    prim = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
    ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    Hud::BuildGT4(prim, &HudPmx::gShapes[19], (sint32)x + 4, y, 0xBEBEUL);
}

tPMenuItemLeftRightSlider::tPMenuItemLeftRightSlider(uint32 textDescription, sint32 *dataPtr, char maxVal) : tPMenuItemInteractive(textDescription)
{
    FUNCTION_MARKER(0x800A7828, "NFS4.EXE");
    fData = dataPtr;
    fMaxVal = maxVal;
}

tPMenuItemLeftRightSlider::~tPMenuItemLeftRightSlider(void)
{
    FUNCTION_MARKER(0x800A787C, "NFS4.EXE");
}

bool tPMenuItemLeftRightSlider::Debounce(void)
{
    FUNCTION_MARKER(0x800A78A4, "NFS4.EXE");
    return false;
}

void tPMenuItemLeftRightSlider::ProcessInput(tInputKeyType &keyval, tPMenuCommand &)
{
    FUNCTION_MARKER(0x800A78AC, "NFS4.EXE");
    bool sound = false;
    if (keyval == kInput_KeyType_Left)
    {
        if (*fData > 0)
        {
            gMPauseUpdateNextTime = 1;
            uint32 maximum = (uint8)fMaxVal;
            sint32 *data = fData;
            sint32 value = (sint32)((uint32)*data - maximum / 30U);
            if (value < 0)
                value = 0;
            *data = value;
            sound = true;
        }
    }
    else if (keyval == kInput_KeyType_Right)
    {
        if (*fData < (sint32)(uint8)fMaxVal)
        {
            gMPauseUpdateNextTime = 1;
            uint32 maximum = (uint8)fMaxVal;
            sint32 *data = fData;
            sint32 value = (sint32)((uint32)*data + maximum / 30U);
            sint32 result = (sint32)maximum;
            if (value <= result)
                result = value;
            *data = result;
            sound = true;
        }
    }
    else
        return;
    keyval = kInput_KeyType_AlreadyProcessed;
    if (sound)
        AudioCmn::PlayPauseSound(5);
}

void tPMenuItemLeftRightSlider::Draw(bool selected)
{
    FUNCTION_MARKER(0x800A79C8, "NFS4.EXE");
    sint16 x = (sint16)TextSys::WordX((sint32)fTextDescription);
    sint16 y = (sint16)((uint16)gPause_CurrentY + 4U);
    PauseMenu::MenuTextPositioned((sint16)fTextDescription, (sint16)selected, (sint16)(fFlags & 1), x);
    for (sint32 i = 0; i < 15; ++i)
    {
        sint32 value = *fData;
        uint32 maximum = (uint8)fMaxVal;
        if (maximum == 0)
            abort();
        sint32 scaled = (sint32)((uint32)value * 15U);
        sint32 filled = scaled / (sint32)maximum;
        uint32 col = 0x323232UL;
        if (i < filled)
            col = selected ? 0xBEBEUL : 0x808080UL;
        if (selected && GameSetup::gData.userSetting.language == 0)
        {
            Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
            POLY_GT4 *prim = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
            uint32 *ot = cprim->LastPrim;
            setaddr(prim, getaddr(ot));
            cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
            setaddr(ot, prim);
            Hud::BuildGT4(prim, &HudPmx::gShapes[18], (sint32)x + 53, (sint32)y + 2, 0xBEBEUL);
            prim = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
            ot = cprim->LastPrim;
            setaddr(prim, getaddr(ot));
            cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
            setaddr(ot, prim);
            Hud::BuildGT4(prim, &HudPmx::gShapes[19], (sint32)x + 144, (sint32)y + 2, 0xBEBEUL);
        }
        sint32 xpos = (sint32)x + 66 + i * 5;
        Hud::FBuildF4(0, xpos, (sint32)y + 2, 3, 5, col, 0, 0);
    }
    Hud::FBuildF4(0, (sint32)x + 63, (sint32)y + 1, 79, 7, 0, 0, 0);
}

tPMenuItemLeftRightSliderIndexed::tPMenuItemLeftRightSliderIndexed(uint32 textDescription, sint32 *dataPtr, char maxVal, char *index) : tPMenuItemLeftRightSlider(textDescription, dataPtr, maxVal)
{
    FUNCTION_MARKER(0x800A7C6C, "NFS4.EXE");
    fIndex = index;
}

tPMenuItemLeftRightSliderIndexed::~tPMenuItemLeftRightSliderIndexed(void)
{
    FUNCTION_MARKER(0x800A7CB4, "NFS4.EXE");
}

void tPMenuItemLeftRightSliderIndexed::ProcessInput(tInputKeyType &keyval, tPMenuCommand &command)
{
    FUNCTION_MARKER(0x800A7CDC, "NFS4.EXE");
    uint8 index = (uint8)*fIndex;
    sint32 *orgdata = fData;
    fData = orgdata + index;
    tPMenuItemLeftRightSlider::ProcessInput(keyval, command);
    fData = orgdata;
}

void tPMenuItemLeftRightSliderIndexed::Draw(bool selected)
{
    FUNCTION_MARKER(0x800A7D28, "NFS4.EXE");
    uint8 index = (uint8)*fIndex;
    sint32 *orgdata = fData;
    fData = orgdata + index;
    tPMenuItemLeftRightSlider::Draw(selected);
    fData = orgdata;
}

tPMenuItemGoToMenuButton::tPMenuItemGoToMenuButton(uint32 textDescription, tPMenu *newMenu, void (*OnButtonPress)(tPMenuCommand &)) : tPMenuItemInteractive(textDescription)
{
    FUNCTION_MARKER(0x800A7D74, "NFS4.EXE");
    fNewMenu = newMenu;
    fOnButtonPress = OnButtonPress;
}

tPMenuItemGoToMenuButton::~tPMenuItemGoToMenuButton(void)
{
    FUNCTION_MARKER(0x800A7DC8, "NFS4.EXE");
}

tPMenu *tPMenuItemGoToMenuButton::NextMenu(void)
{
    FUNCTION_MARKER(0x800A7DF0, "NFS4.EXE");
    return fNewMenu;
}

void tPMenuItemGoToMenuButton::ProcessInput(tInputKeyType &keyval, tPMenuCommand &command)
{
    FUNCTION_MARKER(0x800A7DFC, "NFS4.EXE");
    if (keyval != kInput_KeyType_Cross)
        return;
    AudioCmn::PlayPauseSound(4);
    if (NextMenu() != 0)
    {
        command.type = kMPause_GoToMenu;
        command.nextMenu = NextMenu();
    }
    void (*callback)(tPMenuCommand &) = fOnButtonPress;
    if (callback != 0)
        callback(command);
    keyval = kInput_KeyType_AlreadyProcessed;
}

tPMenuItemCommandButton::tPMenuItemCommandButton(uint32 textDescription, tPMenuCommandType command) : tPMenuItemInteractive(textDescription)
{
    FUNCTION_MARKER(0x800A7EB0, "NFS4.EXE");
    fCommand = command;
}

tPMenuItemCommandButton::~tPMenuItemCommandButton(void)
{
    FUNCTION_MARKER(0x800A7EF4, "NFS4.EXE");
}

void tPMenuItemCommandButton::ProcessInput(tInputKeyType &keyval, tPMenuCommand &command)
{
    FUNCTION_MARKER(0x800A7F1C, "NFS4.EXE");
    if (keyval != kInput_KeyType_Cross)
        return;
    AudioCmn::PlayPauseSound(4);
    command.type = fCommand;
    keyval = kInput_KeyType_AlreadyProcessed;
}

void tPMenu::tPMenuConstructor(tPMenuItem *firstItem, void *ap)
{
    FUNCTION_MARKER(0x800A7F7C, "NFS4.EXE");
    fItemList[0] = firstItem;
    fNumItems = 0;
    va_list *cursor = static_cast<va_list *>(ap);
    tPMenuItem *p = va_arg(*cursor, tPMenuItem *);
    fItemList[1] = p;
    sint32 index = 1;
    while (p != 0)
    {
        fNumItems = fNumItems + 1;
        p = va_arg(*cursor, tPMenuItem *);
        index = index + 1;
        fItemList[index] = p;
    }
}

tPMenu::tPMenu(tPMenuItem *firstItem, ...)
{
    FUNCTION_MARKER(0x800A7FC8, "NFS4.EXE");
    fCurrentItem = 0;
    va_list ap;
    va_start(ap, firstItem);
    tPMenuConstructor(firstItem, &ap);
    va_end(ap);
}

tPMenu::~tPMenu(void)
{
    FUNCTION_MARKER(0x800A8014, "NFS4.EXE");
}

void tPMenu::Initialize(void)
{
    FUNCTION_MARKER(0x800A8048, "NFS4.EXE");
    fCurrentItem = 0;
    fHighlight = true;
    tPMenuItem *entry = fItemList[fCurrentItem];
    bool available = false;
    if ((entry->fFlags & 1) == 0)
        available = entry->IsNavigable();
    if (available)
        return;
    while ((entry = fItemList[fCurrentItem]) != 0)
    {
        bool skip = (entry->fFlags & 1) != 0;
        if (!skip)
            skip = !entry->IsNavigable();
        if (!skip)
            return;
        fCurrentItem = fCurrentItem + 1;
    }
}

bool tPMenu::Debounce(void)
{
    FUNCTION_MARKER(0x800A8148, "NFS4.EXE");
    return fItemList[fCurrentItem]->Debounce();
}

void tPMenu::CheckForDisabled(void)
{
    FUNCTION_MARKER(0x800A8194, "NFS4.EXE");
    for (;;)
    {
        tPMenuItem *entry = fItemList[fCurrentItem];
        bool retry = !entry->IsNavigable();
        if (!retry)
        {
            entry = fItemList[fCurrentItem];
            retry = (entry->fFlags & 1) != 0;
        }
        if (!retry)
            return;
        sint32 current = fCurrentItem;
        if (current > 0)
            fCurrentItem = current - 1;
        else
        {
            tPMenuItem *next = fItemList[current + 1];
            while (next != 0)
            {
                current = fCurrentItem;
                fCurrentItem = current + 1;
                next = fItemList[current + 2];
            }
        }
    }
}

void tPMenu::ProcessInput(tInputKeyType &keyval, tPMenuCommand &command)
{
    FUNCTION_MARKER(0x800A829C, "NFS4.EXE");
    tPMenuItem *entry = fItemList[fCurrentItem];
    if (entry != 0)
        entry->ProcessInput(keyval, command);
    switch (keyval)
    {
    case kInput_KeyType_Up:
        AudioCmn::PlayPauseSound(3);
        for (;;)
        {
            sint32 current = fCurrentItem;
            if (current > 0)
                fCurrentItem = current - 1;
            else
            {
                tPMenuItem *next = fItemList[current + 1];
                while (next != 0)
                {
                    current = fCurrentItem;
                    fCurrentItem = current + 1;
                    next = fItemList[current + 2];
                }
            }
            entry = fItemList[fCurrentItem];
            bool retry = !entry->IsNavigable();
            if (!retry)
            {
                entry = fItemList[fCurrentItem];
                retry = (entry->fFlags & 1) != 0;
            }
            if (!retry)
                break;
        }
        keyval = kInput_KeyType_AlreadyProcessed;
        return;
    case kInput_KeyType_Down:
        AudioCmn::PlayPauseSound(3);
        for (;;)
        {
            sint32 next = fCurrentItem + 1;
            fCurrentItem = next;
            if (fItemList[next] == 0)
                fCurrentItem = 0;
            entry = fItemList[fCurrentItem];
            bool retry = !entry->IsNavigable();
            if (!retry)
            {
                entry = fItemList[fCurrentItem];
                retry = (entry->fFlags & 1) != 0;
            }
            if (!retry)
                break;
        }
        keyval = kInput_KeyType_AlreadyProcessed;
        return;
    case kInput_KeyType_Start:
        AudioCmn::PlayPauseSound(4);
        command.type = kMPause_Continue;
        keyval = kInput_KeyType_AlreadyProcessed;
        return;
    case kInput_KeyType_Triangle:
        AudioCmn::PlayPauseSound(4);
        command.type = kMPause_BackupMenu;
        keyval = kInput_KeyType_AlreadyProcessed;
        return;
    default:
        return;
    }
}

void tPMenu::Draw(void)
{
    FUNCTION_MARKER(0x800A8510, "NFS4.EXE");
    CheckForDisabled();
    tPMenuItem *heading = fItemList[0];
    gPause_CurrentY = 98;
    heading->Draw(false);
    gPause_CurrentY = 117;
    for (sint16 item = 1; ; item = (sint16)(item + 1))
    {
        tPMenuItem *entry = fItemList[item];
        if (entry == 0)
            break;
        if ((entry->fFlags & 1) == 0)
        {
            bool selected = fHighlight && (sint32)item == fCurrentItem;
            entry->Draw(selected);
            gPause_CurrentY = (sint32)((uint32)gPause_CurrentY + 13U);
        }
    }
}

sint32 tPMenu::NumEnabledItems(void)
{
    FUNCTION_MARKER(0x800A8600, "NFS4.EXE");
    sint32 count = fNumItems;
    sint32 ret = count;
    for (sint32 i = 1; i <= count; ++i)
    {
        if ((fItemList[i]->fFlags & 1) != 0)
            --ret;
    }
    return ret;
}

sint32 tPMenu::ItemEnabledNum(sint32 num)
{
    FUNCTION_MARKER(0x800A8650, "NFS4.EXE");
    sint32 ret = num;
    for (sint32 i = 0; i < num; ++i)
    {
        if ((fItemList[i]->fFlags & 1) != 0)
            --ret;
    }
    return ret;
}
