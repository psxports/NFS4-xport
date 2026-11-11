#ifndef TEXTCRNT_H
#define TEXTCRNT_H

#include "xport.h"
#include "game/psx/font.h"

typedef struct shapetbl shapetbl;
typedef struct NativeCurrentFont NativeCurrentFont;
typedef struct OriginalFontHeader OriginalFontHeader;

struct shapetbl;

typedef void (*NativeTextDraw)(sint32, sint32, void *, sint32, sint32, sint32, sint32, sint32);

typedef sint32 (*NativeFontDecoder)(char **cursor);

struct NativeCurrentFont
{
    sint32 unknown00[3];
    sint32 originX;
    sint32 originY;
    sint32 depth;
    sint32 active;
    sint32 metric1C;
    sint32 metric20;
    sint32 metric24;
    sint32 metric28;
    sint32 penX;
    sint32 penY;
    uint8 controls[64];
    uint32 characterCount;
    sint32 rowBytes;
    sint32 scale;
    char *file;
    char *characters;
    shapetbl *shape;
    sint32 unknown8C[2];
    NativeTextDraw draw;
    uint32 unknown98;
    uint32 unknown9C;
    NativeFontDecoder decoder;
    uint32 unknownA4;
    uint32 unknownA8;
    uint32 unknownAC;
    uint32 unknownB0;
    uint32 unknownB4;
};

struct OriginalFontHeader
{
    uint8 prefix[10];
    uint16 characterCount;
    uint16 unknown0C;
    uint16 flags;
    sint8 originX;
    sint8 originY;
    sint8 metric1C;
    sint8 metric20;
    sint32 charactersOffset;
    sint32 unknown18;
    sint32 shapeOffset;
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern NativeCurrentFont currentfont;
#ifdef __cplusplus
}
#endif

void setfont(char *file);
void inittextdraw(void);
void restoretextdraw(void);
void fortext4(sint32 x, sint32 y, void *source, sint32 u, sint32 v, sint32 width, sint32 height, sint32 rowbytes);
sint32 decodeansi(char **cursor);
sint32 decodeshiftjis(char **cursor);
uint32 remapshiftjiscode(uint32 code);
charactertbl *textbsearch(sint32 key, char *base, uint32 nmemb, uint32 size);
charactertbl *getcharacter(sint32 code);

#ifdef __cplusplus
}
#endif

#endif
