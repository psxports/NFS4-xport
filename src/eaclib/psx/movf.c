#include "xport.h"
#include "eaclib/psx/internal.h"

void movfxya(shapetbl *shape, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x800F0738, "NFS4.EXE");
    sint32 depth = shapedepth(shape);
    if ((shape->type & 8) == 0)
    {
        PSX_RECT rectangle = {
            (sint16)((uint32)currentwindow.originX + (uint32)x),
            (sint16)((uint32)currentwindow.originY + (uint32)y),
            shape->width, shape->height
        };
        vramimage(&rectangle, ((uint32 *)(&shape->data)));
        return;
    }
    if ((shape->type & 3) == 2 && (semitrans & 2) == 0)
    {
        fastmovfxya(shape, x, y);
        return;
    }
    if (shape->width <= 0 || shape->height <= 0)
        return;

    sint32 srcX = shape->shapex;
    sint32 srcY = shape->shapey;
    for (sint32 row = 0; row < shape->height; )
    {
        sint32 textureY = srcY + row;
        sint32 v = textureY & 0xFF;
        sint32 rows = 255 - v;
        if (rows > shape->height - row)
            rows = shape->height - row;
        sint32 rowStep = rows > 0 ? rows : 1;

        for (sint32 column = 0; column < shape->width; )
        {
            sint32 textureX = srcX + column * depth / 16;
            sint32 u = srcX * 16 / depth + column - (textureX & ~63) * 16 / depth;
            sint32 columns = 255 - u;
            if (columns > shape->width - column)
                columns = shape->width - column;
            sint32 columnStep = columns > 0 ? columns : 1;

            POLY_FT4 *packet = ((POLY_FT4 *)(primptr));
            primptr += sizeof(*packet);
            setPolyFT4(packet);
            packet->rgbc.code = (uint8)(0x2C | semitrans);
            packet->p[0].attr = (uint16)shapetoclutid(shape);
            packet->p[1].attr = (uint16)(((shape->type & 3) << 7) |
                ((textureY & 0x100) >> 4) | ((textureX & 0x3C0) >> 6));

            sint16 left = (sint16)((uint32)x + (uint32)column);
            sint16 top = (sint16)((uint32)y + (uint32)row);
            sint16 right = (sint16)(left + columnStep);
            sint16 bottom = (sint16)(top + rowStep);
            packet->p[0].xy.x = packet->p[2].xy.x = left;
            packet->p[1].xy.x = packet->p[3].xy.x = right;
            packet->p[0].xy.y = packet->p[1].xy.y = top;
            packet->p[2].xy.y = packet->p[3].xy.y = bottom;
            packet->p[0].uv.u = packet->p[2].uv.u = (uint8)u;
            packet->p[1].uv.u = packet->p[3].uv.u = (uint8)(u + columns);
            packet->p[0].uv.v = packet->p[1].uv.v = (uint8)v;
            packet->p[2].uv.v = packet->p[3].uv.v = (uint8)(v + rows);
            addPrim(nextprim, packet);
            nextprim = &packet->tag;
            column += columnStep;
        }
        row += rowStep;
    }
}
