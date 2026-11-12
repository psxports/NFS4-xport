#include "xport.h"
#include "eaclib/psx/internal.h"

void fastmovfxya(shapetbl *shape, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x80106084, "NFS4.EXE");
    sint32 srcX = shape->shapex;
    sint32 srcY = shape->shapey;
    sint32 width = shape->width;
    sint32 height = shape->height;
    sint32 clip = (currentwindow.clipLeft - x);
    if (clip > 0)
    {
        x = (x + clip);
        srcX = (srcX + clip);
        width = (width - clip);
    }
    clip = ((x + width) - currentwindow.clipRight);
    if (clip > 0)
        width = (width - clip);
    clip = (currentwindow.clipTop - y);
    if (clip > 0)
    {
        y = (y + clip);
        srcY = (srcY + clip);
        height = (height - clip);
    }
    clip = ((y + height) - currentwindow.clipBottom);
    if (clip > 0)
        height = (height - clip);
    if (width <= 0 || height <= 0)
        return;

    PSX_RECT rectangle = { (sint16)srcX, (sint16)srcY, (sint16)width, (sint16)height };
    DR_MOVE *packet = ((DR_MOVE *)(primptr));
    primptr += sizeof(*packet);
    SetDrawMove(packet, &rectangle, ((currentwindow.originX + x)), (currentwindow.originY + y));
    addPrim(nextprim, packet);
    nextprim = &packet->tag;
}
