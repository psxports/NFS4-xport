#ifndef PSXCONTROLLER_H
#define PSXCONTROLLER_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"

extern sint32 mappings[3][13][3];

class InGame
{
public:
    static void ResetPSXController(sint32 player, sint32 config);
    static sint32 GetPSXPadValue(sint32 value, sint32 player);
    static sint32 GetDevice(sint32 control);
    static void SetRamp(void);
};

#endif
