#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "frontend/common/feapp.h"
#include "frontend/common/fefades.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetools.h"
#include "frontend/psx/mmeffect.h"
#include "frontend/psx/psxfront.h"
#include "game/common/textsys.h"
#include "game/psx/font.h"
#include "game/psx/textpix.h"

sint32 gSemiTransText;
extern sint32 kRGBVals[28];
char textDefinitions[14][6] = { { 3, 0, 0, 9, 9, 9 }, { 0, 0, 0, 10, 10, 9 }, { 0, 0, 0, 20, 17, 9 }, { 0, 0, 0, 9, 9, 9 }, { 0, 0, 0, 16, 20, 9 }, { 0, 0, 0, 20, 20, 20 }, { 0, 0, 0, 16, 20, 9 }, { 0, 0, 0, 17, 17, 17 }, { 0, 0, 0, 15, 15, 9 }, { 0, 0, 0, 9, 9, 9 }, { 0, 0, 0, 15, 10, 9 }, { 0, 0, 0, 6, 20, 9 }, { 1, 0, 0, 16, 20, 9 }, { 3, 2, 0, 9, 9, 9 } };

void FETextRender::SetFont(sint32 size)
{
    FUNCTION_MARKER(0x80020CD8, "FRONT.BIN");
    if (currentSize == size)
        return;
    sint16 selected = (sint16)size;
    currentSize = selected;
    char *f1;
    if (selected == 0)
        f1 = font12;
    else if (selected == 3)
        f1 = fontTitle;
    else
        f1 = font18;
    Font::SwitchFont(f1);
}

void FETextRender::FullTextRGB(char *sMenuText, sint16 x, sint16 y, sint32 col, char size, sint16 justify)
{
    FUNCTION_MARKER(0x80020D4C, "FRONT.BIN");
    char *str = sMenuText;
    char buffer[128];
    uint32 fontSize = (uint8)size;
    SetFont((sint32)fontSize);
    if (fontSize == 0)
    {
        sprintf(buffer, "%s", sMenuText);
        s_lower(buffer);
        str = buffer;
    }
    sint32 drawX = x;
    if (justify == 1 || justify == 4 || justify == 6)
        drawX = (drawX - textpixels(str));
    else if (justify == 2 || justify == 5)
        drawX = (x - (textpixels(str) / 2));
    Font::TextColor(gSemiTransText != 0 ? 15 : 1);
    Font::TextTint(col);
    sint16 finalX = (sint16)drawX;
    sint16 finalY = y;
    Font::TextXY(str, finalX, finalY);
    if (justify >= 3 && justify <= 5)
    {
        Font::TextTint(0);
        Font::TextXY(str, finalX + 2, finalY + 1);
    }
}

void FETextRender::FullText(char *sMenuText, sint16 x, sint16 y, tMenuTextType textType, tMenuTextState textState, sint16 justify)
{
    FUNCTION_MARKER(0x80020EC0, "FRONT.BIN");
    char size = textDefinitions[textType][0];
    sint32 textcol = kRGBVals[(uint8)textDefinitions[textType][(sint32)textState + 3]];
    FullTextRGB(sMenuText, x, y, textcol, size, justify);
}

void FETextRender::FullTextFade(sint32 fade, char *sMenuText, sint16 x, sint16 y, tMenuTextType textType, tMenuTextState textState, sint16 justify)
{
    FUNCTION_MARKER(0x80020F44, "FRONT.BIN");
    sint32 col = kRGBVals[(uint8)textDefinitions[textType][(sint32)textState + 3]];
    col = CalcFadeVal(col, fade);
    FullTextRGB(sMenuText, x, y, col, textDefinitions[textType][0], justify);
}

void FETextRender::MenuTextPositioned(sint16 index, sint16 x, sint16 y, tMenuTextState textState, tMenuTextType textType)
{
    FUNCTION_MARKER(0x80021014, "FRONT.BIN");
    if (textType == textType_Default)
        textType = static_cast<tMenuTextType>(TextSys::WordFlags(index));
    sint16 justify = (uint8)textDefinitions[textType][1];
    MenuTextPositionedJustify(index, x, y, justify, textState, textType);
}

void FETextRender::MenuTextFade(sint32 fade, sint16 index, tMenuTextState textState, tMenuTextType textType)
{
    FUNCTION_MARKER(0x800210B4, "FRONT.BIN");
    sint32 x = TextSys::WordX(index);
    sint32 y = TextSys::WordY(index);
    if (textType == textType_Default)
        textType = static_cast<tMenuTextType>(TextSys::WordFlags(index));
    sint16 justify = (uint8)textDefinitions[textType][1];
    MenuTextPositionedJustifyFade(fade, index, (sint16)x, (sint16)y, justify, textState, textType);
}

void FETextRender::MenuTextPositionedJustify(sint16 index, sint16 x, sint16 y, sint16 justify, tMenuTextState state, tMenuTextType type)
{
    FUNCTION_MARKER(0x80021180, "FRONT.BIN");
    MenuTextPositionedJustifyFade(0, index, x, y, justify, state, type);
}

void FETextRender::MenuTextPositionedJustifyFade(sint32 fade, sint16 index, sint16 x, sint16 y, sint16 justify, tMenuTextState textState, tMenuTextType textType)
{
    FUNCTION_MARKER(0x800211D4, "FRONT.BIN");
    if (justify == 6 || textType == textType_Title)
    {
        Title(index);
        return;
    }
    if (textType == textType_Default)
        textType = static_cast<tMenuTextType>(TextSys::WordFlags(index));
    char *sMenuText = TextSys::Word(index);
    FullTextFade(fade, sMenuText, x, y, textType, textState, justify);
}

sint32 FETextRender::WordWrapTextRGBJustify(char *str, PSX_RECT &r, sint32 col, sint32 justify, sint32 size, bool JustGrabHeight)
{
    FUNCTION_MARKER(0x800212A4, "FRONT.BIN");
    sint32 OriginalY = r.y;
    SetFont(size);
    sint16 spacing = size == 0 ? 8 : (size == 1 ? 10 : 16);
    uint16 index1 = 0;
    sint32 strLength = (sint32)strlen(str);
    Font::TextColor(gSemiTransText != 0 ? 15 : 1);
    Font::TextTint(col);
    char buffer[128];
    char source[512];
    memcpy(source, str, strLength + 1U);
    if (size == 0)
        s_lower(source);
    if (strLength > 0)
    {
        do
        {
            while (source[index1] == ' ')
                ++index1;
            uint16 index2 = (uint16)(index1 + (sint32(r.w) >> 3) + 10);
            if (index2 > strLength)
                index2 = (uint16)strLength;
            sint32 count = index2 - index1;
            memcpy(buffer, source + index1, count + 1U);
            buffer[count] = 0;
            sint16 width = (sint16)textpixels(buffer);
            if (r.w < width)
            {
                do
                {
                    if (source[index2] != ' ')
                    {
                        while (index2 > index1)
                        {
                            --index2;
                            if (source[index2] == ' ')
                                break;
                        }
                    }
                    while (source[index2] == ' ' && index2 > index1)
                        --index2;
                    buffer[sint32(index2) - sint32(index1) + 1] = 0;
                    width = (sint16)textpixels(buffer);
                } while (r.w < width && index2 > index1);
            }
            if (index2 == index1)
            {
                buffer[1] = source[sint32(index2) + 1];
                while (source[index2] != 0 && source[index2] != ' ')
                    ++index2;
                buffer[sint32(index2) - sint32(index1)] = 0;
            }
            while (source[index2] != 0 && source[index2] != ' ')
                ++index2;
            while (source[index2] == ' ')
                ++index2;
            buffer[sint32(index2) - sint32(index1)] = 0;
            sint32 finalWidth = textpixels(buffer);
            sint16 x = r.x;
            if (justify == 1 || justify == 4)
                x = (sint16)(((uint16)x) - (uint32)finalWidth);
            else if (justify == 2 || justify == 5)
                x = (sint16)(sint32(x) - (sint32((sint16)finalWidth) >> 1));
            index1 = index2;
            if (!JustGrabHeight)
            {
                Font::TextXY(buffer, x, r.y);
                if (justify - 3U < 3)
                {
                    Font::TextTint(0);
                    Font::TextXY(buffer, sint32(x) + 2, sint32(r.y) + 1);
                    Font::TextTint(col);
                }
            }
            r.y = (sint16)((uint16)r.y + (uint16)spacing);
        } while (index1 < strLength);
    }
    return r.y - OriginalY;
}

void FETextRender::WordWrapTextRGB(char *str, PSX_RECT &r, sint32 Col)
{
    FUNCTION_MARKER(0x80021718, "FRONT.BIN");
    WordWrapTextRGBJustify(str, r, Col, 0, 0, false);
}

void FETextRender::WordWrapTextFade(sint32 fade, char *str, PSX_RECT &r, tMenuTextState textState, tMenuTextType textType)
{
    FUNCTION_MARKER(0x80021740, "FRONT.BIN");
    sint32 col = kRGBVals[(uint8)textDefinitions[textType][(sint32)textState + 3]];
    col = CalcFadeVal(col, fade);
    sint32 justify = (uint8)textDefinitions[textType][1];
    sint32 size = (uint8)textDefinitions[textType][0];
    WordWrapTextRGBJustify(str, r, col, justify, size, false);
}

void FETextRender::WordWrapText(char *str, PSX_RECT &r, tMenuTextState textState, tMenuTextType textType)
{
    FUNCTION_MARKER(0x800217E8, "FRONT.BIN");
    WordWrapTextFade(0, str, r, textState, textType);
}

void FETextRender::WordWrapFade(sint32 fade, sint16 index, PSX_RECT &r, tMenuTextState state, tMenuTextType type)
{
    FUNCTION_MARKER(0x80021824, "FRONT.BIN");
    char *str = TextSys::Word(index);
    WordWrapTextFade(fade, str, r, state, type);
}

void FETextRender::WordWrap(sint16 index, PSX_RECT &r, tMenuTextState textState, tMenuTextType textType)
{
    FUNCTION_MARKER(0x8002188C, "FRONT.BIN");
    WordWrapFade(0, index, r, textState, textType);
}

sint32 FETextRender::WordWrapHeight(sint16 width, char *str)
{
    FUNCTION_MARKER(0x800218C8, "FRONT.BIN");
    PSX_RECT r;
    r.h = 500;
    r.w = width;
    r.x = 0;
    r.y = 0;
    return WordWrapTextRGBJustify(str, r, 0, 0, 0, true);
}

void FETextRender::Title(sint16 index)
{
    FUNCTION_MARKER(0x80021914, "FRONT.BIN");
    sint16 offset = (uint8)FEApp->fPlayer == 1 ? 105 : 0;
    char upstr[80];
    strcpy(upstr, TextSys::Word(index));
    s_lower(upstr);
    FullText(upstr, 48, (sint16)(offset | 16), textType_Title, textState_Selected, 0);
    PSX_RECT r;
    r.x = 43;
    r.y = (sint16)(offset + 18);
    r.w = (sint16)(textpixels(upstr) + 10U);
    r.h = 12;
    PSXDrawTransSquare(0, r.x, r.y, r.w, 12, 2);
    FeDraw_SetABRMode(0);
}

void FETextRender::SetABR(sint32 abr, bool trans)
{
    FUNCTION_MARKER(0x800219E8, "FRONT.BIN");
    gSemiTransText = trans;
    Font::SetABR(abr);
}
