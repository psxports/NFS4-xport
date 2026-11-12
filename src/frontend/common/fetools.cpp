#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "frontend/common/fetools.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/psxfront.h"
#include "game/common/paths.h"
#include "game/common/textsys.h"
#include "game/psx/font.h"

struct tOldMenu;

struct tItemButton
{
    sint16 type;
    sint16 text;
    tOldMenu *nextMenu;
    char *value;
    sint16 *selection;
    char defaultvalue;
    uint8 flags;
    uint8 cases;
};

struct tOldMenu
{
    char *artFileName;
    char *backgrName;
    sint16 driver;
    sint16 current;
    sint16 flags;
    sint16 helpvariant;
    tItemButton *item[16];
};

sint16 currentSize;
char *font12;
char *font18;
char *fontTitle;
sint32 FeTools::gScrollTicksOut = 30;

void s_upper(char *string)
{
    FUNCTION_MARKER(0x800268F0, "FRONT.BIN");
    sint32 len = (sint32)strlen(string);
    for (sint32 n = 0; n < len; ++n) {
        uint8 value = (uint8)string[n];
        if (value >= 97 && value <= 122)
            string[n] = value - 32;
    }
}

void s_lower(char *string)
{
    FUNCTION_MARKER(0x80026950, "FRONT.BIN");
    sint32 len = (sint32)strlen(string);
    for (sint32 n = 0; n < len; ++n) {
        uint8 value = (uint8)string[n];
        if (value >= 65 && value <= 90)
            string[n] = value + 32;
    }
}

void FeTools::DrawPSXButton(uint8 padType, uint16 button, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x800269B0, "FRONT.BIN");
    tDrawShapeExtended drawFlags;
    drawFlags.tint[0] = 0xBEBE;
    sint32 type = 0;
    y = (y + 1);
    if (button == 0x50)
    {
        DrawShapeExtended(26, 0x18, ((x + 3)), (y - 2), 0, 0, &drawFlags);
        type = 27;
        x = (x + 4);
        y = (y + 4);
    }
    else if (button == 0xA0)
    {
        y = (y - 1);
        DrawShapeExtended(28, 0x18, x, y, 0, 0, &drawFlags);
        type = 29;
        x = (x + 10);
    }
    else if (padType == 0x23)
    {
        switch (button)
        {
        case 0x1000:
            type = 18;
            y = (y - 3);
            break;
        case 0x4000:
            type = 19;
            y = (y - 4);
            break;
        case 0x8000:
            type = 20;
            y = (y - 2);
            break;
        case 8:
            type = 21;
            y = (y - 1);
            break;
        }
    }
    else
    {
        switch (button)
        {
        case 0x1000:
            type = 15;
            y = (y - 3);
            break;
        case 0x4000:
            type = 12;
            y = (y - 3);
            break;
        case 0x8000:
            type = 14;
            y = (y - 3);
            break;
        case 8:
            type = 16;
            x = (x + 2);
            break;
        }
    }
    DrawShapeExtended(type, 0x18, x, y, 0, 0, &drawFlags);
}

void FeTools::init(void)
{
    FUNCTION_MARKER(0x80026BA8, "FRONT.BIN");
    char filename[256];
    Font::ExitFromGame();
    sprintf(filename, "%stiny.pfn", Paths_Paths[33]);
    font12 = static_cast<char *>(loadfileadrz(filename, 0));
    Font::LoadFont(font12, 960, 385, 0);
    sprintf(filename, "%ssmall.pfn", Paths_Paths[33]);
    font18 = static_cast<char *>(loadfileadrz(filename, 0));
    Font::LoadFont(font18, 896, 256, 0);
    sprintf(filename, "%stitle.pfn", Paths_Paths[33]);
    fontTitle = static_cast<char *>(loadfileadrz(filename, 0));
    Font::LoadFont(fontTitle, 960, 256, 0);
    currentSize = 3;
}

void FeTools::deinit(void)
{
    FUNCTION_MARKER(0x80026C90, "FRONT.BIN");
    purgememadr(font18);
    purgememadr(fontTitle);
    purgememadr(font12);
    Font::DeInit();
}

void FeTools::FormatMoney(char *string, sint32 amount)
{
    FUNCTION_MARKER(0x80026CE0, "FRONT.BIN");
    sint32 absnum = amount;
    char neg = ' ';
    if (amount < 0)
    {
        absnum = (-amount);
        neg = '-';
    }
    if (absnum < 1000)
    {
        char *format = TextSys::Word(131);
        sprintf(string, format, neg, (sint32)absnum);
    }
    else if (absnum <= 999999)
    {
        char *format = TextSys::Word(132);
        sprintf(string, format, neg, (sint32)(absnum / 1000), (sint32)(absnum % 1000));
    }
    else
    {
        char *format = TextSys::Word(133);
        sprintf(string, format, neg, (sint32)(absnum / 1000000), (sint32)((absnum % 1000000) / 1000), (sint32)(absnum % 1000));
    }
}
