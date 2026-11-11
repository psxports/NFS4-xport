#include "xport.h"
#include "eaclib/psx/internal.h"

void fastmovfxya(shapetbl *shape, sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x80106084, "NFS4.EXE");
    sint32 srcX = shape->shapex;
    sint32 srcY = shape->shapey;
    sint32 width = shape->width;
    sint32 height = shape->height;
    sint32 clip = (sint32)((uint32)currentwindow.clipLeft - (uint32)x);
    if (clip > 0)
    {
        x = (sint32)((uint32)x + (uint32)clip);
        srcX = (sint32)((uint32)srcX + (uint32)clip);
        width = (sint32)((uint32)width - (uint32)clip);
    }
    clip = (sint32)((uint32)x + (uint32)width - (uint32)currentwindow.clipRight);
    if (clip > 0)
        width = (sint32)((uint32)width - (uint32)clip);
    clip = (sint32)((uint32)currentwindow.clipTop - (uint32)y);
    if (clip > 0)
    {
        y = (sint32)((uint32)y + (uint32)clip);
        srcY = (sint32)((uint32)srcY + (uint32)clip);
        height = (sint32)((uint32)height - (uint32)clip);
    }
    clip = (sint32)((uint32)y + (uint32)height - (uint32)currentwindow.clipBottom);
    if (clip > 0)
        height = (sint32)((uint32)height - (uint32)clip);
    if (width <= 0 || height <= 0)
        return;

    PSX_RECT rectangle = { (sint16)srcX, (sint16)srcY, (sint16)width, (sint16)height };
    DR_MOVE *packet = ((DR_MOVE *)(primptr));
    primptr += sizeof(*packet);
    SetDrawMove(packet, &rectangle,
        (sint32)((uint32)currentwindow.originX + (uint32)x),
        (sint32)((uint32)currentwindow.originY + (uint32)y));
    addPrim(nextprim, packet);
    nextprim = &packet->tag;
}
