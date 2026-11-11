#ifndef CAMERASLOT_H
#define CAMERASLOT_H

#include "xport.h"
#include "game/common/camera.h"
#include "game/common/mathnfs.h"
#include "game/psx/draww.h"

struct Camera::tCamSlot
{
    char mode;
    char track : 1;
    char zoom : 2;
    char splineMode : 3;
    sint16 fov;
    coorddef pos;
    sint32 height;
    sint32 splineOffset;
    COORD16 euler;
    sint16 slice;
};

#endif
