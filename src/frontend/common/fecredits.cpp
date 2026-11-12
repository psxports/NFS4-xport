#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "frontend/common/fecheats.h"
#include "frontend/common/fecredits.h"
#include "frontend/common/fefades.h"
#include "frontend/common/feinput.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/front.h"
#include "frontend/common/screenmain.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "game/common/textsys.h"
#include "game/psx/textpix.h"

struct tCredit
{
    sint16 titleTextID;
    sint16 subTitleTextID;
    sint16 titleX;
    sint16 titleY;
    sint16 titleJustify;
    sint16 titleWidth;
    sint16 subTitleX;
    sint16 subTitleY;
    sint16 subTitleJustify;
    sint16 subTitleWidth;
    sint16 textX;
    sint16 textY;
    sint16 textJustify;
    sint16 creditType;
    sint16 bgNumber;
    sint16 pad;
    char text[292];
};

sint32 CREDFADETICKS = 700;
static sint32 lasttick;

void tCreditManager::Setup(void)
{
    FUNCTION_MARKER(0x80035AE8, "FRONT.BIN");
    fCreditsInitialized = 0;
    fRequestDeInit = 0;
    fTVFade = 0;
}

void tCreditManager::Init(sint32)
{
    FUNCTION_MARKER(0x80035AF8, "FRONT.BIN");
    if (fCreditsInitialized != 0)
        return;
    if (fRequestDeInit != 0)
        fRequestDeInit = 0;
    fShowCreditNum = -1;
    fTextFade = 128;
    fStartTicks = 0;
    StartedTransition = 0;
    StartedLines = 0;
    StartedTextFade = 0;
    fRequestDeInit = 0;
    fCurrCredit = 0;
    char filename[80];
    sprintf(filename, "%szcred%d.dat", Paths_Paths[37], (uint8)frontEnd.language);
    CreditBuffer = static_cast<tCredit *>(reservememadr("records", filesize(filename), 0));
    loadfileatadrz(filename, CreditBuffer);
    uint32 size = filesize(filename);
    fCreditsInitialized = 1;
    fNumCredits = (size / 324U);
}

void tCreditManager::DeInit(void)
{
    FUNCTION_MARKER(0x80035BD8, "FRONT.BIN");
    fRequestDeInit = 1;
}

void tCreditManager::RealDeInit(void)
{
    FUNCTION_MARKER(0x80035BE4, "FRONT.BIN");
    bool initialized = fCreditsInitialized != 0;
    fRequestDeInit = 0;
    if (initialized)
    {
        purgememadr(CreditBuffer);
        fCreditsInitialized = 0;
    }
}

void tCreditManager::Draw(bool selected)
{
    FUNCTION_MARKER(0x80035C24, "FRONT.BIN");
    fTVFade = (fTVFade + (selected ? 4U : 0xFFFFFFFCU));
    if (fTVFade < 0)
        fTVFade = 0;
    if (fTVFade > 92)
        fTVFade = 92;
    if (fTVFade < 92 && fTextFade < 128 - fTVFade)
        fTextFade = 128 - fTVFade;
    tScreenMain *main = screenMain;
    for (sint32 i = 0; i < 16; ++i)
    {
        main->tvConfigs[i].flags |= 2;
        uint32 bright = 128 - (uint32)fTVFade;
        main->tvConfigs[i].tint = (bright << 16) | (bright << 8) | bright;
    }
    if (fTVFade == 0)
    {
        if (fRequestDeInit != 0)
            RealDeInit();
    }
    else if (fCreditsInitialized != 0)
    {
        SetupCurrCredit();
        if (fShowCreditNum != -1)
            DrawCurrCredit();
    }
}

void Front::ResetOverlayCredits(void)
{
    lasttick = 0;
    CREDFADETICKS = 700;
}

void tCreditManager::SetupCurrCredit(void)
{
    FUNCTION_MARKER(0x80035D68, "FRONT.BIN");
    if ((sint32)((uint32)ticks - lasttick) > 12 && fTextFade == 0 && fCurrCredit == fShowCreditNum)
    {
        if (FEInput::GetNoDebounceKey(0x20, 0) != 0 || FEInput::GetNoDebounceKey(0x20, 1) != 0)
        {
            AudioCmn::PlayFESFX(6);
            fStartTicks = 0;
            fCurrCredit = (fShowCreditNum + 1);
            lasttick = ticks;
        }
        if (fCurrCredit >= fNumCredits)
            fCurrCredit = 0;
        if (fCurrCredit < 0)
            fCurrCredit = (fNumCredits - 1);
    }
    if (fStartTicks != 0 && (sint32)((uint32)ticks - fStartTicks) > CREDFADETICKS)
    {
        fCurrCredit = (fCurrCredit + 1);
        if (fCurrCredit >= fNumCredits)
            fCurrCredit = 0;
        if (fCurrCredit < 0)
            fCurrCredit = (fNumCredits - 1);
        fStartTicks = 0;
    }
    if (fShowCreditNum != fCurrCredit)
    {
        fTextFadeDir = 8;
        StartedTransition = 0;
        StartedLines = 0;
        StartedTextFade = 0;
    }
    fTextFade = (fTextFade + fTextFadeDir);
    if (fTextFade <= 0)
        fTextFade = 0;
    if (fTextFade >= 128)
        fTextFade = 128;
    if (fTextFade == 128 && StartedTransition == 0)
    {
        sint32 current = fCurrCredit;
        StartedTransition = 1;
        fShowCreditNum = current;
        if (current % 3 == 0 || CreditBuffer[current].bgNumber != -1)
        {
            sint32 NNNNN = (current + 1);
            if (fNumCredits < NNNNN)
                NNNNN = 0;
            screenMain->SwapBackground(CreditBuffer[NNNNN].bgNumber);
        }
    }
    if (StartedLines == 0 && StartedTransition != 0 && screenMain->DoneLoadingBackground())
    {
        sint32 now = ticks;
        StartedLines = 1;
        fLineTicks = now;
    }
    if (StartedTextFade == 0 && StartedLines != 0 && (sint32)((uint32)ticks - fLineTicks) > 30)
    {
        sint32 now = ticks;
        StartedTextFade = 1;
        fTextFadeDir = -8;
        fStartTicks = now;
    }
}

void tCreditManager::DrawCurrCredit(void)
{
    FUNCTION_MARKER(0x80036084, "FRONT.BIN");
    tDrawShapeExtended drawFlags;
    drawFlags.tint[0] = 0xCEC844;
    DrawShapeExtended((ticks >> 4) % 10 + 230, 0x410, 16, 16, 0, 0, &drawFlags);
    tCredit *fShowCred = &CreditBuffer[fShowCreditNum];
    FETextRender::SetABR(1, true);
    uint32 y = (uint16)fShowCred->subTitleY;
    sint32 ColTextTitle = CalcFadeVal(CalcFadeVal(0xBEBE, fTextFade), 40);
    sint32 ColTextSubTitle = CalcFadeVal(CalcFadeVal(0xBEBE, fTextFade), 40);
    sint32 ColText = CalcFadeVal(0x787878, fTextFade);
    PSX_RECT r;
    if (fShowCred->titleWidth == 0)
        FETextRender::FullTextRGB(TextSys::Word(fShowCred->titleTextID + 1300), fShowCred->titleX, fShowCred->titleY, ColTextTitle, 0, fShowCred->titleJustify);
    else
    {
        r.x = fShowCred->titleX;
        r.y = fShowCred->titleY;
        r.w = fShowCred->titleWidth;
        r.h = 100;
        FETextRender::WordWrapTextRGBJustify(TextSys::Word(fShowCred->titleTextID + 1300), r, ColTextTitle, fShowCred->titleJustify, 0, false);
    }
    if (fShowCred->subTitleWidth == 0)
    {
        y += 8;
        FETextRender::FullTextRGB(TextSys::Word(fShowCred->subTitleTextID + 1300), fShowCred->subTitleX, fShowCred->subTitleY, ColTextSubTitle, 0, fShowCred->subTitleJustify);
    }
    else
    {
        r.x = fShowCred->subTitleX;
        r.y = fShowCred->subTitleY;
        r.w = fShowCred->subTitleWidth;
        r.h = 100;
        y += (uint32)FETextRender::WordWrapTextRGBJustify(TextSys::Word(fShowCred->subTitleTextID + 1300), r, ColTextSubTitle, fShowCred->subTitleJustify, 0, false);
    }
    if (fShowCred->textY != 0)
        y = (uint16)fShowCred->textY;
    sint16 x = fShowCred->textX;
    sint32 width = fShowCred->subTitleWidth;
    if (width == 0)
    {
        FETextRender::SetFont(0);
        width = textpixels(TextSys::Word(fShowCred->subTitleTextID + 1300));
        width = (width - (uint32)strlen(TextSys::Word(fShowCred->subTitleTextID + 1300)));
    }
    if (x == 0)
    {
        if (fShowCred->subTitleJustify == 0)
            x = (sint16)(((uint16)fShowCred->subTitleX) + (uint32)width + 2U);
        else
            x = (sint16)(((uint16)fShowCred->subTitleX) - (uint32)width - 2U);
    }
    char buffer[292];
    strcpy(buffer, fShowCred->text);
    char *p = buffer;
    while (p != 0)
    {
        bool hidden = false;
        bool jaguar = false;
        bool rollthedice = false;
        char *p2 = strchr(p, '\n');
        if (p2 != 0) *p2 = 0;
        while (*p == '\n')
            ++p;
        if (*p == '\t')
        {
            hidden = true;
            do { ++p; } while (*p == '\t');
        }
        if (*p == '*')
        {
            jaguar = true;
            do { ++p; } while (*p == '*');
        }
        if (*p == '^')
        {
            rollthedice = true;
            do { ++p; } while (*p == '^');
        }
        CREDFADETICKS = rollthedice ? 2000 : 700;
        if (jaguar)
        {
            r.x = 180;
            r.y = 85;
            r.w = 280;
            r.h = 100;
            FETextRender::WordWrapTextRGBJustify(TextSys::Word(1430), r, ColText, 0, 0, false);
            y += 8;
        }
        else if (rollthedice)
        {
            for (sint32 rtd = 0; rtd < 25; ++rtd)
            {
                char *text = TextSys::Word(rtd + 1431);
                sint16 textY = (sint16)y;
                y += 8;
                FETextRender::FullTextRGB(text, x, textY, ColText, 0, fShowCred->textJustify);
            }
            y += 8;
        }
        else
        {
            if (!hidden)
                FETextRender::FullTextRGB(p, x, y, ColText, 0, fShowCred->textJustify);
            else if (FECheat::IsCheatEnabled(cheat_MyMomSaysImCool))
                FETextRender::FullTextRGB(p, x, y, CalcFadeVal(0x505050, fTextFade), 0, fShowCred->textJustify);
            y += 8;
        }
        p = p2 != 0 ? p2 + 1 : 0;
    }
    FETextRender::SetABR(0, false);
    if (StartedLines == 0)
        return;
    sint32 dist = (sint32)(((uint32)ticks - fLineTicks) * 520U) / 80;
    sint32 lineX = dist;
    width = 200;
    if (dist < 200)
    {
        width = dist;
        lineX = 200;
    }
    if (lineX > 320)
    {
        width = 320 - lineX;
        if (width < 0)
            width = 0;
        lineX = 520 - width;
    }
    if (width > 0)
        PSXTransDrawBrightEndLine(CalcFadeVal(0x505050, 64), lineX - 37, fShowCred->subTitleY - 2, width, 4, 3, 1, 0, 1);
    dist = (sint32)(((uint32)ticks - fLineTicks) * 268U) / 80;
    sint32 lineY = dist;
    sint32 height = 100;
    if (dist < 100)
    {
        height = dist;
        lineY = 100;
    }
    if (lineY > 168)
    {
        height = 168 - lineY;
        if (height < 0)
            height = 0;

        lineY = 268 - width;
    }
    if (height > 0)
        PSXTransDrawBrightEndLine(CalcFadeVal(0x505050, 64), x, lineY - 58, 2, height, 1, 1, 0, 1);
}
