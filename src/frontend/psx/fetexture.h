#ifndef FETEXTURE_H
#define FETEXTURE_H

#include "xport.h"

struct shapetbl;

class tTexture
{
public:
    struct ShapeInfo
    {
        shapetbl *shpptr;
        sint32 clutID;
        char depth;
    #if defined(_MSC_VER)

        uint8 type : 8;
    #else
        uint32 type : 8;
    #endif
        sint32 next : 24;
        sint16 width;
        sint16 height;
        sint16 centerx;
        sint16 centery;
        sint16 shapex;
        sint16 shapey;
        uint16 tpage;
        uint16 clut;
    };
};

class FETexture
{
public:
    static void LoadPmxAtOffset(char *f, sint32 index, tTexture::ShapeInfo *dest, sint32 yoffset, sint32 xoffset);
    static void LoadPmx(char *f, sint32 index, tTexture::ShapeInfo *dest);
};

#endif
