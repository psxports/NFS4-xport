#ifndef DRAWSHP_H
#define DRAWSHP_H

#include "xport.h"

class DrawShape
{
public:
    static void SubtractNFS4RectEdges(PSX_RECT &rect);
    static void NFS4RoundRectangle(sint32 textID, PSX_RECT &position, sint16 selected);
    static void NFS4Rectangle(PSX_RECT &position);
    static void NFS4TransRectangle(PSX_RECT &r, sint16 opacity);
};

#endif
