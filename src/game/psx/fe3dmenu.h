#ifndef FE3DMENU_H
#define FE3DMENU_H

#include "xport.h"
#include "game/common/coorddef.h"

class Fe3D
{
public:
    static COORD16 spotVertex[33];
    static COORD16 lightsVertex[64];

    static void InitShowroom(void);
};

extern sint32 showRoomFlag;
extern sint32 gShowroomLights;
extern sint32 gMenuRotate[2];

#endif
