#include <stdio.h>
#include "xport.h"
#include "frontend/common/fefades.h"
#include "frontend/common/femenuoptions.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/screenusername.h"
#include "frontend/psx/psxfront.h"
#include "game/common/nfs3.h"
#include "game/common/textsys.h"

sint16 menu_kUserNameRows;
sint16 MENUUSERNAME_STARTY;
tScreenUserName *screenUserName;
char UserPermFileName[20];

void tScreenUserName::Initialize(void)
{
    FUNCTION_MARKER(0x8004AF88, "FRONT.BIN");
    sint32 rows = menu_kUserNameRows;
    fTextFade = 128;
    MENUUSERNAME_STARTY = (sint16)(130 - ((rows * 15) >> 1));
    for (sint16 i = 0; i < menu_kUserNameRows; ++i)
    {
        char *format = TextSys::Word(i + 0x1FB);
        sprintf(fRowList[i], format);
    }
    tScreen::Initialize();
}

void tScreenUserName::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x8004B048, "FRONT.BIN");
    *swapFileName = 0;
    numSwapShapes = 0;
    numPermShapes = 92;
    sprintf(UserPermFileName, "zUser%d", (uint8)frontEnd.language);
    *permFileName = UserPermFileName;
}

void tScreenUserName::DrawVerticalLine(sint16 x, sint16 y, sint16 gridpos)
{
    FUNCTION_MARKER(0x8004B0A8, "FRONT.BIN");
    if (gridpos < 0)
        gridpos = 0;
    else if (gridpos >= 64)
        gridpos = 64;
    PSXDrawBrightEndLine(0x785A5A, x, y, 2, 162, 1, gridpos * 2, 0);
}

void tScreenUserName::DrawHorizontalLine(sint16 x, sint16 y, sint16 gridpos)
{
    FUNCTION_MARKER(0x8004B128, "FRONT.BIN");
    if (gridpos < 0)
        gridpos = 0;
    else if (gridpos >= 64)
        gridpos = 64;
    PSXDrawBrightEndLine(0x785A5A, x, y, 192, 1, 2, gridpos * 2, 12);
}

void tScreenUserName::DrawBackground(void)
{
    FUNCTION_MARKER(0x8004B1AC, "FRONT.BIN");
    sint16 fade = (sint16)callingMenu->fScreenFade;
    sint16 fadebox = (sint16)((fade >> 1) - 128);
    if (fadebox < 0)
        fadebox = 0;
    else if (fadebox > 128)
        fadebox = 128;
    sint16 gridpos = (sint16)(fade >> 2);
    if (gridpos < 0)
        gridpos = 0;
    else if (gridpos > 128)
        gridpos = 128;
    if (fade < 0)
        fTextFade = 0;
    else if (fade > 128)
        fTextFade = 128;
    else
        fTextFade = fade;
    SubtractiveBox(240, 42, 194, 85, 0x080808, 0x080808, 0, 0);
    SubtractiveBox(240, 127, 194, 85, 0, 0, 0x080808, 0x080808);
    char output[2] = { ' ', 0 };
    sint16 rowY = MENUUSERNAME_STARTY;
    for (sint16 row = 0; row < menu_kUserNameRows; ++row)
    {
        sint16 x = 258;
        sint16 y = (sint16)(rowY - 1);
        for (sint16 col = 0; fRowList[row][col] != 0; ++col)
        {
            output[0] = fRowList[row][col];
            sint32 colText = CalcFadeVal(0xB54200, fTextFade);
            sint32 word = 0;
            sint32 offset = 22;
            sint32 separators = 1;
            switch ((uint8)output[0])
            {
            case '!': word = 0x205; break;
            case '#': word = 0x206; break;
            case '&': word = 0x207; break;
            case '@': word = 0x205; offset = 36; separators = 2; break;
            case '$': word = 0x206; offset = 36; separators = 2; break;
            case '^': word = 0x207; offset = 78; separators = 5; break;
            case '-': break;
            default:
                FETextRender::FullTextRGB(output, (sint16)(x + 8), y, colText, 1, 2);
                break;
            }
            if (word != 0)
            {
                char *text = TextSys::Word(word);
                FETextRender::FullTextRGB(text, (sint16)(x + offset), y, colText, 1, 2);
                for (sint16 k = 1; k <= separators; ++k)
                    PSXDrawSquare(0, x + k * 28 - 6, rowY - 2, 2, 14);
            }
            x = (sint16)(x + 28);
        }
        rowY = (sint16)(rowY + 15);
    }
    SubtractiveBox(240, 42, 194, 85, 0x505050, 0x505050, 0, 0);
    SubtractiveBox(240, 127, 194, 85, 0, 0, 0x505050, 0x505050);
    sint16 x = 252;
    for (sint16 i = 0; i < 7; ++i)
    {
        DrawVerticalLine(x, 46, (sint16)(gridpos - i * 2));
        x = (sint16)(x + 28);
    }
    sint16 y = (sint16)(MENUUSERNAME_STARTY - 3);
    for (sint16 i = 0; i < menu_kUserNameRows + 1; ++i)
    {
        DrawHorizontalLine(240, y, (sint16)(gridpos - (menu_kUserNameRows - i) * 2));
        y = (sint16)(y + 15);
    }
    for (sint16 i = 0; i < 32; ++i)
        DrawShapeExtended(i, 0, 0, 0, fadebox, 0, 0);
}
