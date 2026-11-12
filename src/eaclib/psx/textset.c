#include "xport.h"
#include "eaclib/psx/internal.h"

void setfont(char *file)
{
    FUNCTION_MARKER(0x800F2E94, "NFS4.EXE");
    OriginalFontHeader *header = ((OriginalFontHeader *)(file));
    sint32 shapeOffset = header->shapeOffset;
    currentfont.scale = 100;
    currentfont.originX = header->originX;
    currentfont.originY = header->originY;
    currentfont.metric1C = header->metric1C;
    currentfont.metric20 = header->metric20;
    currentfont.metric24 = header->metric20 + header->metric1C;
    shapetbl *shape = ((shapetbl *)(file + shapeOffset));
    currentfont.metric28 = header->metric20 + header->metric1C;
    uint32 count = header->characterCount;
    currentfont.active = 1;
    currentfont.unknownB0 = 0;
    currentfont.unknownAC = 0;
    currentfont.file = file;
    currentfont.shape = shape;
    currentfont.characterCount = count;
    currentfont.characters = file + header->charactersOffset;
    currentfont.depth = shapedepth(shape);
    sint32 depth = shapedepth(shape);
    uint32 bits = (shape->width) * (uint32)depth;
    currentfont.rowBytes = (sint32)((bits + 31) & ~31U) >> 3;
    if ((header->flags & 3) == 2)
        currentfont.decoder = decodeshiftjis;
    else if ((sint32)currentfont.characterCount >= 256)
        currentfont.decoder = decodeshiftjis;
    else if (geti(file + 32, 2) >= 256U)
        currentfont.decoder = decodeshiftjis;
    else
        currentfont.decoder = decodeansi;
    currentfont.penX = 0;
    currentfont.penY = 0;
    currentfont.unknownB4 = 0;
    memset(currentfont.controls, 0, sizeof(currentfont.controls));
    inittextdraw();
    currentfont.unknownA4 = 0;
}
