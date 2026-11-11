#ifndef CAMERAOBJ_H
#define CAMERAOBJ_H

#include "xport.h"
#include "game/common/camera.h"
#include "game/common/carobj.h"

enum forceFocus_t { FOCUS_NORMAL = 0, FOCUS_AI = 1, FOCUS_COPANDAI = 2 };

struct camera_info
{
    BO::tNewtonObj *anchor,*target;
    coorddef position,relpos,audioPos;
    sint32 TVHeight;
    matrixtdef rotation;
    sint32 twist;
    coorddef wallLeft,wallRight;
    sint16 mode,camNum;
    char modechange:1,pitch:1,jostling:1,tracking:1,checkwalls:1,noLookBack:1,checkcollisions:1,splitscreen:1;
    char intransition,tumbling;
    char direction:1,zooming:2,inCar:1;
    sint16 circleCounter,circleAngle;
    char animNum,animHandle,splineMode;
    forceFocus_t forceFocus;
    char focusOnAICar;
    sint32 POInhibitor;
    BWorldSm::Pos slicePos;
};

#endif
