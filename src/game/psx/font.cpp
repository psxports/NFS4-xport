#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/textcrnt.h"
#include "game/psx/font.h"
#include "game/psx/rendercache.h"
#include "game/psx/systemmemory.h"
#include "game/psx/texture.h"
#include "lib/psx/getm.h"

struct shapetbl;

typedef struct charactertbl FONTFILECHAR;

static fontblit gCurrentBlitter;
uint16 gFontClut;
uint8 gFontSpaceWidth;
sint32 colourRGB[16] = { 0, 8750469, 11184810, 12500480, 10526880, 17077, -4276736, 12521216, 10872411, 10838858, -5592406, 11494, 1158354, 797538, 6233856, 7102560 };
cluttbl &shpfontclut = gSystemMemoryOwner.fontPalette;
uint16 font::currentTPage;
sint32 font::clutx = -1;
sint32 font::cluty = -1;
uint32 font::tint = 1719697536;
uint32 font::abr;

void Font::TextColor(sint32 color)
{
    FUNCTION_MARKER(0x800CB20C, "NFS4.EXE");
    uint32 x = (uint16)font::clutx + ((uint32)color << 4);
    uint32 y = (uint16)shpfontclut.shapey;
    shpfontclut.shapex = (sint16)x;
    gFontClut = (uint16)((y << 6) | ((x >> 4) & 63));
}

void Font::TextTint(sint32 rgb)
{
    FUNCTION_MARKER(0x800CB244, "NFS4.EXE");
    font::tint = (rgb & 0xFFFFFFU) | 0x66000000U;
}

void Font::SetABR(sint32 abr)
{
    FUNCTION_MARKER(0x800CB264, "NFS4.EXE");
    shapetbl *shape = currentfont.shape;
    font::abr = abr;
    if (shape)
        font::currentTPage = GetTPage(shape->type & 3U, abr, shape->shapex, shape->shapey);
}

void Font::Blit(sint32 x, sint32 y, void *src, sint32 u, sint32 v, charactertbl *ch, sint32)
{
    FUNCTION_MARKER(0x800CB2AC, "NFS4.EXE");
    uint32 xy = ((uint32)y << 16) | x;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    SPRT *sprt = reinterpret_cast<SPRT *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    uint32 packed;
    memcpy(&packed, static_cast<const char *>(src) + 12, sizeof(packed));
    sint32 dv = (sint32)(packed << 4) >> 20;
    sint32 width = ch->width;
    sint32 height = ch->height;
    setaddr(sprt, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(sprt + 1);
    setaddr(ot, sprt);
    uint32 row = (dv + (uint32)v) & 255U;
    setlen(sprt, 4);
    uint32 tint = font::tint;
    memcpy(&sprt->p[0].xy, &xy, sizeof(xy));
    memcpy(&sprt->rgbc, &tint, sizeof(tint));
    uint32 clut = gFontClut;
    sprt->w = (sint16)width;
    sprt->h = (sint16)height;
    uint32 uv = (clut << 16) | (row << 8) | u;
    sprt->p[0].uv_attr = uv;
    SetSemiTrans(sprt, 1);
}

void Font::ComputeColors(sint32 colour, sint32 forecolour, sint32 backcolour, char in_game)
{
    FUNCTION_MARKER(0x800CB388, "NFS4.EXE");
    sint32 fr = forecolour & 255U;
    sint32 fg = (forecolour >> 8) & 255U;
    sint32 fb = (forecolour >> 16) & 255U;
    sint32 br = backcolour & 255U;
    sint32 bg = (backcolour >> 8) & 255U;
    sint32 bb = (backcolour >> 16) & 255U;
    uint32 opaque = forecolour & 0xFF000000;
    shpfontclut.type = 0x23;
    shpfontclut.width = 16;
    shpfontclut.height = 1;
    shpfontclut.centerx = 0;
    shpfontclut.centery = 0;
    shpfontclut.next = 0;
    shpfontclut.shapex = (sint16)font::clutx;
    shpfontclut.shapey = (sint16)font::cluty;
    sint32 solid = ((fr * 31 / 255) << 10) | ((fg * 31 / 255) << 5) | (fb * 31 / 255);
    for (sint32 i = 0; i < 16; ++i)
    {
        sint32 rgb = 0;
        if ((uint8)in_game == 0 || opaque != 0)
        {
            if (i >= 4)
            {
                sint32 r = fr * i * 31 / 3825 + br * (15 - i) * 31 / 3825;
                sint32 g = fg * i * 31 / 3825 + bg * (15 - i) * 31 / 3825;
                sint32 b = fb * i * 31 / 3825 + bb * (15 - i) * 31 / 3825;
                rgb = (r << 10) | (g << 5) | b;
                if (colour == 15)
                    rgb |= 0x8000;
                if (rgb == 0)
                    rgb = 0x400;
            }
        }
        else if (i >= 8)
        {
            rgb = solid | 0x8000;
            if (i >= 12)
            {
                rgb = solid;
                if (rgb == 0)
                    rgb = 0x400;
            }
        }
        shpfontclut.data[i] = (sint16)rgb;
    }
    sint32 clutx = font::clutx;
    sint32 cluty = font::cluty;
    sint32 destinationX = (clutx + ((uint32)colour << 4));
    Texture::Vramf(reinterpret_cast<shapetbl *>(&shpfontclut), clutx, cluty, destinationX, cluty);
    DrawSync(0);
}

charactertbl * Font::textbsearch(sint32 key, char *base, uint32 nmemb, uint32 size)
{
    FUNCTION_MARKER(0x800CB6A0, "NFS4.EXE");
    sint32 lim = nmemb;
    while (lim != 0)
    {
        charactertbl *ch = reinterpret_cast<charactertbl *>(base + (lim >> 1) * size);
        uint32 index = ch->index[0] | ((uint32)ch->index[1] << 8);
        sint32 cmp = (key - index);
        if (cmp == 0)
            return ch;
        if (cmp > 0)
        {
            base = reinterpret_cast<char *>(ch) + size;
            lim = (lim - 1);
        }
        lim >>= 1;
    }
    return 0;
}

charactertbl * Font::Getcharacter(sint32 targetindex)
{
    FUNCTION_MARKER(0x800CB740, "NFS4.EXE");
    char *base = currentfont.characters;
    uint32 count = currentfont.characterCount;
    uint32 index = targetindex - 32U;
    if (index < count) {
        charactertbl *ch = reinterpret_cast<charactertbl *>(base + index * 11U);
        if (geti(ch, 2) == (uint32)targetindex)
            return ch;
    }
    return textbsearch(targetindex, base, count, 11);
}

void Font::SetBlitter(fontblit blitter)
{
    FUNCTION_MARKER(0x800CB7CC, "NFS4.EXE");
    gCurrentBlitter = blitter;
}

void Font::ReSetBlitter(void)
{
    FUNCTION_MARKER(0x800CB7D8, "NFS4.EXE");
    gCurrentBlitter = Blit;
}

void Font::SwitchFont(char *f1)
{
    FUNCTION_MARKER(0x800CB7EC, "NFS4.EXE");
    setfont(f1);
    shapetbl *shape = currentfont.shape;
    uint32 abr = font::abr;
    currentfont.draw = 0;
    currentfont.unknown98 = 0;
    currentfont.unknown9C = 0;
    font::currentTPage = GetTPage(shape->type & 3U, abr, shape->shapex, shape->shapey);
    charactertbl *ch = Getcharacter(32);
    gFontSpaceWidth = (uint8)ch->advance;
}

void Font::DeInit(void)
{
    FUNCTION_MARKER(0x800CB858, "NFS4.EXE");
    if (font::clutx != -1)
    {
        uint32 id = (((uint16)font::cluty) << 6) | (((uint32)font::clutx >> 4) & 63U);
        Texture::MenuReleaseClutId(id);
    }
    font::clutx = -1;
    font::cluty = -1;
}

void Font::ExitFromGame(void)
{
    FUNCTION_MARKER(0x800CB8A8, "NFS4.EXE");
    font::clutx = -1;
    font::cluty = -1;
}

sint32 Font::LoadFont(char *f1, sint32 x, sint32 y, char in_game)
{
    FUNCTION_MARKER(0x800CB8BC, "NFS4.EXE");
    setfont(f1);
    shapetbl *shp = currentfont.shape;
    sint32 l = (shp->width * shp->height) / 2;
    uint8 *data = reinterpret_cast<uint8 *>(&shp->data);
    for (sint32 i = 0; i < l; ++i)
        data[i] = (uint8)((data[i] << 4) | (data[i] >> 4));
    if (font::clutx == -1)
        Texture::GetClutId(1, &font::clutx, &font::cluty);
    Texture::Vramf(shp, x, y, font::clutx, font::cluty);
    waitdraw();
    shp->next = 0;
    for (sint32 i = 0; i < l; ++i)
        data[i] = (uint8)((data[i] << 4) | (data[i] >> 4));
    for (sint32 i = 0; i < 16; ++i)
        ComputeColors(i, colourRGB[i], 0, in_game);
    ReSetBlitter();
    SwitchFont(f1);
    ptrdiff_t length = reinterpret_cast<char *>(currentfont.shape) - f1 + 16;
    resizememadr(f1, (sint32)length);
    TextTint(0x808080);
    TextColor(2);
    return (y + shp->height);
}

void Font::TextXY(char *string, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x800CBA90, "NFS4.EXE");
    char *str = string;
    sint32 code = -1;
    while (code != 0)
    {
        code = (uint8)*str++;
        if (code < 33)
            x = (x + gFontSpaceWidth);
        else
        {
            charactertbl *ch = Getcharacter(code);
            if (ch != 0)
            {
                sint32 u = geti(ch->u, 2);
                sint32 v = geti(ch->v, 2);
                shapetbl *shape = currentfont.shape;
                sint32 glyphX = (x + ((sint8)ch->xoffset));
                sint32 glyphY = (y + ((sint8)ch->yoffset));
                sint32 rowbytes = currentfont.rowBytes;
                fontblit blitter = gCurrentBlitter;
                blitter(glyphX, glyphY, shape, u, v, ch, rowbytes);
                x = (x + ((sint8)ch->advance));
            }
        }
    }
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    DR_MODE *dr_mode = reinterpret_cast<DR_MODE *>(cprim->PrimPtr);
    uint16 tpage = font::currentTPage;
    uint32 *ot = cprim->LastPrim;
    setaddr(dr_mode, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(dr_mode + 1);
    setaddr(ot, dr_mode);
    SetDrawMode(dr_mode, 0, 0, tpage, 0);
}

void Font::GetUVWH(char code, sint32 *u, sint32 *v, sint32 *w, sint32 *h, sint32 *yoff)
{
    FUNCTION_MARKER(0x800CBBE8, "NFS4.EXE");
    charactertbl *ch = Getcharacter((uint8)code);
    uint32 value = geti(ch->u, 2);
    *u = value;
    value = geti(ch->v, 2);
    *v = (sint32)((uint32)(sint32)currentfont.shape->shapex + value);
    *w = ch->width;
    *h = ch->height;
    *yoff = (sint8)ch->yoffset;
}
