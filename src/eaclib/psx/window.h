#ifndef WINDOW_H
#define WINDOW_H

#include "xport.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ShapeBlitWindow
{
    sint32 originX;
    sint32 originY;
    sint32 clipLeft;
    sint32 clipTop;
    sint32 clipRight;
    sint32 clipBottom;
} ShapeBlitWindow;

extern const ShapeBlitWindow currentwindow;

#ifdef __cplusplus
}
#endif

#endif
