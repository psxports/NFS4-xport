#include "xport.h"
#include "eaclib/psx/internal.h"

void checkrect(PSX_RECT *rectangle)
{
    FUNCTION_MARKER(0x800F6934, "NFS4.EXE");
    if ((rectangle->w & 1) != 0)
        rectangle->h |= 1;
}

void vramimage(PSX_RECT *rectangle, uint32 *pixels)
{
    FUNCTION_MARKER(0x800F6960, "NFS4.EXE");
    checkrect(rectangle);
    LoadImagePSX(rectangle, pixels);
    drawpending = 1;
}

void vramfxya(shapetbl *shape, sint32 x, sint32 y, sint32 clutx, sint32 cluty)
{
    FUNCTION_MARKER(0x800F69A8, "NFS4.EXE");
    union
    {
        uint32 words[256];
        uint16 pixels[512];
    } converted;
    while (shape != 0)
    {
        uint32 type = shape->type & 0xF7u;
        if (type >= 0x40 && type <= 0x43)
        {
            shape->shapex = ((sint32)(((uint32)(x)) & 0xFFFu));
            shape->shapey = ((sint32)(((uint32)(y)) & 0xFFFu));
            shape->type |= 8u;
            PSX_RECT rectangle;
            rectangle.x = (sint16)x;
            rectangle.y = (sint16)y;
            sint32 depth = shapedepth(shape);
            sint32 product = shape->width * depth;
            sint32 rounded = product + 15;
            if (rounded < 0)
                rounded = product + 30;
            rectangle.w = (sint16)(rounded >> 4);
            rectangle.h = shape->height;
            vramimage(&rectangle, ((uint32 *)(&shape->data)));
        }
        else if (type == 0x22 || type == 0x23 || type == 0x24)
        {
            uint32 *pixels;
            if (type == 0x23)
                pixels = ((uint32 *)(&shape->data));
            else
            {
                uint16 *output = converted.pixels + (type == 0x24 ? 256 : 0);
                const uint8 *source = ((const uint8 *)(&shape->data));
                sint32 shift = type == 0x22 ? 1 : 3;
                for (sint32 i = 0; i < shape->width; ++i)
                {
                    uint32 blue = source[2];
                    uint32 green = source[1];
                    uint32 red = source[0];
                    source += 3;
                    *output++ = (uint16)(((blue >> shift) << 10) | ((green >> shift) << 5) | (red >> shift));
                }
                pixels = converted.words + (type == 0x24 ? 128 : 0);
            }
            shape->shapex = ((sint32)(((uint32)(clutx)) & 0xFFFu));
            shape->shapey = ((sint32)(((uint32)(cluty)) & 0xFFFu));
            shape->type |= 8u;
            PSX_RECT rectangle;
            rectangle.x = (sint16)clutx;
            rectangle.y = (sint16)cluty;
            rectangle.h = 1;
            rectangle.w = shape->width;
            vramimage(&rectangle, pixels);
        }
        sint32 next = shape->next;
        shape = next == 0 ? 0 : ((shapetbl *)(((char *)(shape)) + next));
    }
}
