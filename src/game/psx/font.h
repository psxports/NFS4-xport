#ifndef FONT_H
#define FONT_H

#include "xport.h"
#include "psx.h"
#ifdef __cplusplus
#include "game/common/car.h"
#endif

typedef struct charactertbl charactertbl;

struct charactertbl
{
    uint8 index[2];
    uint8 width;
    uint8 height;
    uint8 u[2];
    uint8 v[2];
    char advance;
    char xoffset;
    char yoffset;
};

#ifdef __cplusplus
class font
{
public:
    static uint16 currentTPage;
    static sint32 clutx;
    static sint32 cluty;
    static uint32 tint;
    static uint32 abr;
};
#endif

typedef void (*fontblit)(sint32, sint32, void *, sint32, sint32, charactertbl *, sint32);

#ifdef __cplusplus
class Font
{
public:
    static void TextColor(sint32 color);
    static void TextTint(sint32 rgb);
    static void SetABR(sint32 abr);
    static void Blit(sint32 x, sint32 y, void *src, sint32 u, sint32 v, charactertbl *ch, sint32);
    static void ComputeColors(sint32 colour, sint32 forecolour, sint32 backcolour, char in_game);
    static charactertbl * textbsearch(sint32 key, char *base, uint32 nmemb, uint32 size);
    static charactertbl * Getcharacter(sint32 targetindex);
    static void SetBlitter(fontblit blitter);
    static void ReSetBlitter(void);
    static void SwitchFont(char *f1);
    static void DeInit(void);
    static void ExitFromGame(void);
    static sint32 LoadFont(char *f1, sint32 x, sint32 y, char in_game);
    static void TextXY(char *string, sint32 x, sint32 y);
    static void GetUVWH(char code, sint32 *u, sint32 *v, sint32 *w, sint32 *h, sint32 *yoff);
};

#endif

extern uint16 gFontClut;
extern uint8 gFontSpaceWidth;

#endif
