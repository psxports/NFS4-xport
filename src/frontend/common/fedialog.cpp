#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/feapp.h"
#include "frontend/common/fedialog.h"
#include "frontend/common/fefades.h"
#include "frontend/common/feinput.h"
#include "frontend/common/femenu.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetools.h"
#include "frontend/psx/mmeffect.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/textsys.h"
#include "game/psx/textpix.h"

struct helpKeyData
{
    sint16 text;
    sint16 button;
};

struct tHelpData
{
    char autoGenerate;
    helpKeyData items[4];
};

static tDialogBase *DialogVisibilityList[8];
extern sint32 kRGBVals[28];

sint16 tDialogBase::ShouldTimeOut(void)
{
    FUNCTION_MARKER(0x800186C8, "FRONT.BIN");

    if (timeOutTicks <= 0)
        return 0;
    return (sint32)(uint32(ticks) - uint32(startTicks)) >= timeOutTicks ? 1 : 0;
}

void tDialogBase::InitializeClass(void)
{
    FUNCTION_MARKER(0x80018700, "FRONT.BIN");

    HideAllDialogs();
}

void tDialogBase::DrawAllDialogs(void)
{
    FUNCTION_MARKER(0x80018720, "FRONT.BIN");

    for (sint16 i = 0; i < 8 && DialogVisibilityList[i] != 0; ++i)
    {
        if (DialogVisibilityList[i]->ShouldTimeOut() != 0)
        {
            DialogVisibilityList[i]->Hide();
            if (DialogVisibilityList[i] == 0)
                return;
        }
        DialogVisibilityList[i]->Draw();
    }
}

void tDialogBase::HideAllDialogs(void)
{
    FUNCTION_MARKER(0x800187F0, "FRONT.BIN");

    for (sint16 i = 0; i < 8; ++i)
        DialogVisibilityList[i] = 0;
}

tDialogBase *tDialogBase::GetTopMostDialog(void)
{
    FUNCTION_MARKER(0x80018830, "FRONT.BIN");

    return DialogVisibilityList[0];
}

void tDialogBase::Display(void)
{
    FUNCTION_MARKER(0x80018840, "FRONT.BIN");

    if (currentlyOn != 0)
        return;
    currentlyOn = 1;
    fFullyOpen = 0;
    ReturnValue = fDefault;
    for (sint16 i = 7; i > 0; --i)
        DialogVisibilityList[i] = DialogVisibilityList[i - 1];
    DialogVisibilityList[0] = this;
    ShouldTimeOut();
    DialogVisibilityList[0]->ShouldTimeOut();
    startTicks = ticks;
    AudioCmn::PlayFESFX(15);
    fFullyOpen = 0;
    fFadeText = 128;
}

void tDialogBase::Hide(void)
{
    FUNCTION_MARKER(0x80018904, "FRONT.BIN");

    if (currentlyOn == 0)
        return;
    currentlyOn = 0;
    for (sint16 i = 0; i < 8; ++i)
    {
        if (DialogVisibilityList[i] == this)
        {
            currentlyOn = 0;
            DialogVisibilityList[i] = 0;
            memmove(&DialogVisibilityList[i], &DialogVisibilityList[i + 1], (7 - i) * sizeof(DialogVisibilityList[0]));
            DialogVisibilityList[7] = 0;
            AudioCmn::PlayFESFX(18);
        }
    }
}

void tDialogBase::Draw(void)
{
    FUNCTION_MARKER(0x800189B8, "FRONT.BIN");

    PSXDrawTransSquare(0, left, top, 4, height, 4);
    PSXDrawTransSquare(0, left + width - 4, top, 4, height, 4);
    PSXDrawTransSquare(0, left + 4, top, width - 8, 2, 4);
    PSXDrawTransSquare(0, left + 4, top + height - 2, width - 8, 2, 4);
    PSXDrawTransSquare(0, left + 4, top + 2, width - 8, height - 4, 2);
    FeDraw_SetABRMode(0);
}

void tDialogBase::ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x80018AC8, "FRONT.BIN");

    if (keyval != kInput_KeyType_AlreadyProcessed)
    {
        Hide();
        keyval = kInput_KeyType_AlreadyProcessed;
    }
}

void tDialogHelp::AddItem(sint16 textID, sint16 controllerID)
{
    FUNCTION_MARKER(0x80018B0C, "FRONT.BIN");

    if (numItems < 7)
    {
        char *word = TextSys::Word(textID);
        text[numItems] = word;
        cont[numItems] = controllerID;
        numItems = (sint16)((uint16)numItems + 1u);
    }
}

void tDialogHelp::CalculateDimensions(void)
{
    FUNCTION_MARKER(0x80018B90, "FRONT.BIN");
    tHelpData helpArray[1] = { { 1, { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } } } };
    FETextRender::SetFont(0);
    numItems = 0;
    AddItem(0x59, 0);
    tPlayer player = specificPlayer == kPlayerBoth ? kPlayerOne : (tPlayer)specificPlayer;
    if (helpArray[variant].autoGenerate != 0)
    {
        bool showLeftRight = false;
        bool showCross = false;
        tMenu *menu = FEApp->fCurrentMenu[player];
        for (sint16 i = 0; menu->fItemList[i] != 0; ++i)
        {
            uint32 flags = menu->fItemList[i]->fFlags;
            if ((flags & 1) == 0)
            {
                if ((flags & 0x400) != 0)
                    showLeftRight = true;
                else
                    showCross = true;
            }
        }
        if (menu->GetNumberEnabledItems() > 1)
            AddItem(0x52, 0x50);
        if (showLeftRight)
            AddItem(0x53, 0xA0);
        if (showCross)
            AddItem((menu->fFlags & 0x30000) != 0 ? 0x56 : 0x55, 0x4000);
        if (FEApp->backDepth[player] > 0)
            AddItem(0x54, 0x1000);
        if (menu->fOptionsMenu != 0)
            AddItem(0x57, (sint16)0x8000);
        if ((menu->fFlags & 0x800) != 0)
            AddItem(0x58, 8);
        else if (menu->fNextMenu != 0 || (menu->fFlags & 0x400) != 0 || menu->fOnButtonPress != 0 || (menu->fFlags & 4) != 0)
            AddItem(0x56, 8);
    }
    for (sint16 i = 0; helpArray[variant].items[i].text != 0; ++i)
        AddItem(helpArray[variant].items[i].text, helpArray[variant].items[i].button);
    helpcontrollers = 0;
    PAD_update();
    if (gPadinfo.buf[0].nopad == 0)
        helpcontrollers = (sint16)(helpcontrollers | (gPadinfo.buf[0].ID == 0x23 ? 2 : 1));
    if (gPadinfo.buf[4].nopad == 0)
        helpcontrollers = (sint16)(helpcontrollers | (gPadinfo.buf[4].ID == 0x23 ? 2 : 1));
    width = 0;
    for (sint16 i = 0; i < numItems; ++i)
    {
        sint32 newWidth = textpixels(text[i]);
        if (width < newWidth)
            width = (sint16)newWidth;
    }
    lefttext = helpcontrollers == 3 ? 70 : 40;
    width = (sint16)((uint16)width + lefttext);
    height = numItems < 2 ? 0 : (sint16)(numItems * 15);
    left = (sint16)((sint32)(uint32(screenwidth) - uint32((sint32)width)) / 2);
    top = (sint16)((240 - height) / 2);
    if (width < gHelpShapes[3].width + 20)
        width = (sint16)(gHelpShapes[3].width + 20);
    uint32 ticks = uint32(::ticks);
    uint32 elapsed = ticks - uint32(startTicks);
    width = (sint16)((uint16)width + 20);
    height = (sint16)((uint16)height + 10);
    if (elapsed < 50)
    {
        sint32 shapeWidth = gHelpShapes[42].width;
        sint32 widthDifference = width - (sint16)(uint32(shapeWidth) << 1);
        uint32 widthProduct = uint32(widthDifference) * elapsed;
        sint32 shapeHeight = gHelpShapes[42].height;
        sint32 heightDifference = height - (sint16)(uint32(shapeHeight) << 1);
        uint32 heightProduct = uint32(heightDifference) * (ticks - uint32(startTicks));
        width = (sint16)((uint32(shapeWidth) << 1) + widthProduct / 50u);
        height = (sint16)((uint32(shapeHeight) << 1) + heightProduct / 50u);
    }
    top = 20;
    left = (sint16)(505 - (uint16)width);
}

void tDialogHelp::Draw(void)
{
    FUNCTION_MARKER(0x8001912C, "FRONT.BIN");
    CalculateDimensions();
    sint32 ticks = ::ticks;
    sint32 deadline = (sint32)(uint32(startTicks) + 50u);
    if (ticks >= deadline)
    {
        sint32 numLetters = (sint32)(uint32(ticks) - uint32(startTicks) - 50u) / 3;
        char buffer[80];
        for (sint16 i = 0; i < numItems; ++i)
        {
            sint16 y = (sint16)((uint16)top + (i == 0 ? 4 : (i - 1) * 15 + 19));
            if (i > 0)
            {
                sint32 control = cont[i];
                uint8 padType = helpcontrollers == 2 ? 0x23 : 0x41;
                sint32 x = left + 20;
                sint32 buttonY = top + (i - 1) * 15 + 19;
                if (helpcontrollers == 3 && control != 0xA0 && control != 0x50 && control != 0x40)
                {
                    FeTools::DrawPSXButton(0x41, (uint16)control, x, buttonY);
                    padType = 0x23;
                    x = left + 40;
                    buttonY = top + (i - 1) * 15 + 19;
                }
                FeTools::DrawPSXButton(padType, (uint16)control, x, buttonY);
            }
            char *sMenuText;
            if (numLetters < (sint32)strlen(text[i]))
            {
                for (sint16 j = 0; j < numLetters; ++j)
                    buffer[j] = text[i][j];
                buffer[numLetters] = 0;
                sMenuText = buffer;
            }
            else
                sMenuText = text[i];
            FETextRender::FullText(sMenuText, (sint16)((uint16)left + (uint16)lefttext), y,
                i == 0 ? textType_PopUpTitle : textType_PopUpText, textState_Selected, 0);
        }
    }
    tDialogBase::Draw();
}

void tDialogMessageString::CalculateDimensions(void)
{
    FUNCTION_MARKER(0x80019418, "FRONT.BIN");

    sint32 ticks = ::ticks;
    sint32 numerator = (sint32)((uint32(ticks) - 50u - uint32(startTicks)) << 7);
    sint32 fade = (sint32)(128u - uint32(numerator / 100));
    fFadeText = fade;
    if (fade > 128)
        fade = 128;
    if (fade < 0)
        fade = 0;
    fFadeText = fade;
    if (fade != 0)
        fFullyOpen = 0;
    width = MaxW;
    if (MaxH == 0)
    {
        height = (sint16)FETextRender::WordWrapHeight((sint16)(uint32((uint16)MaxW) - 40u), string);
        Centerit = height == 8 ? 1 : 0;
        height = (sint16)((uint16)height + 16u);
    }
    else
    {
        Centerit = 0;
        height = MaxH;
    }
    ticks = ::ticks;
    sint32 elapsed = (sint32)(uint32(ticks) - uint32(startTicks));
    if (elapsed < 50)
    {
        sint32 shapeWidth = gHelpShapes[42].width;
        sint32 widthDifference = width - (sint16)(uint32(shapeWidth) << 1);
        sint32 widthProduct = (sint32)(uint32(widthDifference) * uint32(elapsed));
        sint32 shapeHeight = gHelpShapes[42].height;
        sint32 heightDifference = height - (sint16)(uint32(shapeHeight) << 1);
        sint32 heightTicks = (sint32)(uint32(ticks) - uint32(startTicks));
        sint32 heightProduct = (sint32)(uint32(heightDifference) * uint32(heightTicks));
        fFullyOpen = 0;
        width = (sint16)((uint32(shapeWidth) << 1) + uint32(widthProduct / 50));
        height = (sint16)((uint32(shapeHeight) << 1) + uint32(heightProduct / 50));
    }
    else
        fFullyOpen = 1;
    sint32 horizontal = (sint32)(uint32(screenwidth) - uint32((sint32)width));
    left = (sint16)((uint16)OffsetX + uint32(horizontal / 2));
    top = (sint16)((uint16)OffsetY + uint32((240 - height) / 2));
}

void tDialogMessageString::Draw(void)
{
    FUNCTION_MARKER(0x8001962C, "FRONT.BIN");

    CalculateDimensions();
    sint32 deadline = (sint32)(uint32(startTicks) + 50);
    if (::ticks < deadline)
        fFullyOpen = 0;
    else
    {
        if (fFadeText != 0)
            fFullyOpen = 0;
        sint32 col = CalcFadeVal(kRGBVals[(uint8)textDefinitions[8][4]], fFadeText);
        PSX_RECT r;
        r.x = (sint16)((uint16)left + 17);
        r.y = (sint16)((uint16)top + 10);
        r.w = (sint16)(uint32((uint16)width) - 20);
        r.h = (sint16)(uint32((uint16)height) - (uint32((uint16)reservedheight) + 8));
        FETextRender::SetABR(1, true);
        if (Centerit != 0)
            FETextRender::FullTextRGB(string, (sint16)((uint16)left + uint32(sint32(width) >> 1)), (sint16)((uint16)top + 8), col, 0, 2);
        else
            FETextRender::WordWrapTextRGB(string, r, col);
        FETextRender::SetABR(0, false);
    }
    tDialogBase::Draw();
}

void tDialogBackUpOnly::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x80019790, "FRONT.BIN");

    if (specificPlayer != kPlayerBoth && fromPlayer != specificPlayer)
        return;
    if (keyval == kInput_KeyType_Triangle)
        Hide();
    else if (keyval != kInput_KeyType_Circle)
        keyval = kInput_KeyType_AlreadyProcessed;
}

sint16 tDialogInteractive::Run(void)
{
    FUNCTION_MARKER(0x800197E8, "FRONT.BIN");

    fCurrentlyRunning = 1;
    Display();
    CalculateDimensions();
    ReadyToReturnValue = 0;
    uint32 debounce = 0xFFFFFFFF;
    tInputKeyType keyVal[2];
    tMenuCommand command;
    while (ReadyToReturnValue == 0)
    {
        command.type = kMenu_Command_None;
        for (sint32 i = 0; i < 2; ++i)
        {
            tPlayer player = static_cast<tPlayer>(i);
            keyVal[i] = FEInput::GetKeyFromPlayer(player, (sint32)debounce);
            if (keyVal[i] == kInput_KeyType_NoKey)
                keyVal[i] = kInput_KeyType_AlreadyProcessed;
            if (keyVal[i] == kInput_KeyType_Circle && FEApp->helpPopup.currentlyOn == 0 && FEApp->fCurrentMenu[i] != 0)
            {
                FEApp->fCurrentMenu[i]->ProcessInput(player, keyVal[i], command);
                keyVal[i] = kInput_KeyType_AlreadyProcessed;
            }
            if (keyVal[i] != kInput_KeyType_AlreadyProcessed && FEApp->helpPopup.currentlyOn != 0)
            {
                FEApp->helpPopup.Hide();
                keyVal[i] = kInput_KeyType_AlreadyProcessed;
            }
            if (keyVal[i] != kInput_KeyType_NoKey)
                ProcessInput(player, keyVal[i], command);
        }
        FEApp->Redraw();
    }
    AudioCmn::PlayFESFX(0);
    Hide();
    FEApp->Redraw();
    sint16 result = ReturnValue;
    fCurrentlyRunning = 0;
    return result;
}

void tDialogYesNo::CalculateDimensions(void)
{
    FUNCTION_MARKER(0x800199D4, "FRONT.BIN");

    tDialogMessageString::CalculateDimensions();
    if (MaxH != 0)
        return;
    sint32 elapsed = (sint32)(uint32(ticks) - uint32(startTicks));
    if (elapsed < 50)
    {
        sint32 scaled = (sint32)(uint32(elapsed) * 15u);
        height = (sint16)((uint16)height + uint32(scaled / 50));
    }
    else
        height = (sint16)((uint16)height + 15u);
    reservedheight = 15;
    top = (sint16)((uint16)OffsetY + uint32((240 - height) / 2));
}

tDialogYesNo::tDialogYesNo(void)
{
    FUNCTION_MARKER(0x80019A8C, "FRONT.BIN");

    MaxW = 288;
    specificPlayer = -1;
    fFadeText = 128;
    currentlyOn = 0;
    reservedheight = 0;
    MaxH = 0;
    OffsetY = 0;
    OffsetX = 0;
    height = 0;
    width = 0;
    top = 0;
    left = 0;
    fDefault = 0;
    timeOutTicks = 0;
    Centerit = 0;
    fFullyOpen = 0;
    timeOutTicks = 0;
    timeOutTicks = 0;
    fCurrentlyRunning = 0;
    ReturnValue = 0;
    ReadyToReturnValue = 0;
}

void tDialogYesNo::Draw(void)
{
    FUNCTION_MARKER(0x80019B44, "FRONT.BIN");

    CalculateDimensions();
    if (fFadeText != 0)
        fFullyOpen = 0;
    sint32 elapsed = (sint32)(uint32(ticks) - uint32(startTicks));
    if (elapsed >= 50)
    {
        sint32 x = left + (sint32(width) >> 2);
        for (sint32 i = 0; i < 2; ++i)
        {
            sint32 state = i == ReturnValue ? 1 : 2;
            sint32 col = CalcFadeVal(kRGBVals[(uint8)textDefinitions[8][state + 3]], fFadeText);
            sint32 y = top + height - 11;
            FETextRender::SetABR(1, true);
            char *word = TextSys::Word(yesnowords[i]);
            FETextRender::FullTextRGB(word, (sint16)x, (sint16)y, col, 0, 2);
            FETextRender::SetABR(0, false);
            x += sint32(width) >> 1;
        }
    }
    tDialogMessageString::Draw();
}

void tDialogYesNo::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyVal, tMenuCommand &command)
{
    FUNCTION_MARKER(0x80019CCC, "FRONT.BIN");

    if ((specificPlayer != kPlayerBoth && fromPlayer != specificPlayer) || fFullyOpen != 1)
        return;
    if (keyVal == kInput_KeyType_Left)
    {
        AudioCmn::PlayFESFX(5);
        ReturnValue = 1;
    }
    else if (keyVal == kInput_KeyType_Right)
    {
        AudioCmn::PlayFESFX(6);
        ReturnValue = 0;
    }
    else if (keyVal == kInput_KeyType_Cross || keyVal == kInput_KeyType_Start)
    {
        AudioCmn::PlayFESFX(0);
        ReadyToReturnValue = 1;
        Hide();
    }
}

void tDialogYesNoTri::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyVal, tMenuCommand &command)
{
    FUNCTION_MARKER(0x80019E7C, "FRONT.BIN");

    if (keyVal == kInput_KeyType_Triangle)
    {
        ReadyToReturnValue = 1;
        ReturnValue = -1;
    }
    else
        tDialogYesNo::ProcessInput(fromPlayer, keyVal, command);
}

void tDialogNoInputMessage::ProcessInput(tPlayer, tInputKeyType &, tMenuCommand &)
{
    FUNCTION_MARKER(0x80019EBC, "FRONT.BIN");
}
