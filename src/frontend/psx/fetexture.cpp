#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "frontend/psx/fetexture.h"
#include "game/psx/texture.h"
#include "game/psx/unpack.h"
#include "lib/psx/blkmov.h"
#include "lib/psx/shpdepth.h"
#include "lib/psx/shpsubs.h"

void FETexture::LoadPmxAtOffset(char *f, sint32 index, tTexture::ShapeInfo *dest, sint32 yoffset, sint32 xoffset)
{
    FUNCTION_MARKER(0x8004F100, "FRONT.BIN");
    sint32 xclut = 0;
    sint32 yclut = 0;
    shapetbl *shpptr = static_cast<shapetbl *>(shapepointer(f, index));
    if (shpptr == 0)
        return;
    char is_compressed = (shpptr->type & 0x80) != 0;
    shapetbl *unpacked = shpptr;
    if (is_compressed)
    {
        sint32 length = (sint32)((uint32)unpacksizez(&shpptr->data) + 16);
        unpacked = static_cast<shapetbl *>(reservememadr("unpacked", length, 0));
        blockmove(shpptr, unpacked, 16);
        unpackz(&shpptr->data, &unpacked->data);
        unpacked->type &= 0x7F;
        uint32 oldptr = reinterpret_cast<uintptr_t>(shpptr) + (uint32)shpptr->next;
        uint32 newptr = oldptr - reinterpret_cast<uintptr_t>(unpacked);
        unpacked->next = (sint32)(newptr << 8) >> 8;
    }
    sint32 bpp = unpacked->type & 3;
    if (bpp != 2)
    {
        Texture::GetClutId(bpp, &xclut, &yclut);
        dest->clutID = (sint32)(((uint32)yclut << 6) | ((xclut >> 4) & 63));
    }
    shapetbl old_shape;
    memcpy(&old_shape, unpacked, 20);
    sint32 x = (sint32)((uint32)unpacked->shapex + (uint32)xoffset);
    sint32 y = (sint32)((uint32)unpacked->shapey + (uint32)yoffset);
    vramfxya(unpacked, x, y, xclut, yclut);
    dest->shpptr = shpptr;
    dest->type = unpacked->type;
    dest->next = unpacked->next;
    dest->width = unpacked->width;
    dest->height = unpacked->height;
    dest->centerx = unpacked->centerx;
    dest->centery = unpacked->centery;
    dest->shapex = unpacked->shapex;
    dest->shapey = unpacked->shapey;
    dest->depth = (char)shapedepth(unpacked);
    dest->tpage = (uint16)(((dest->type & 3) << 7) |
        ((dest->shapey & 0x100) >> 4) | ((dest->shapex & 0x3FF) >> 6) |
        ((dest->shapey & 0x200) << 2));
    dest->clut = GetClut((dest->clutID & 63) << 4, dest->clutID >> 6);
    if (yoffset != 0 || xoffset != 0)
        memcpy(unpacked, &old_shape, 20);
    if (is_compressed)
        purgememadr(unpacked);
}

void FETexture::LoadPmx(char *f, sint32 index, tTexture::ShapeInfo *dest)
{
    FUNCTION_MARKER(0x8004F3B8, "FRONT.BIN");
    LoadPmxAtOffset(f, index, dest, 0, 0);
}
