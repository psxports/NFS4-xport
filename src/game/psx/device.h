#ifndef DEVICE_H
#define DEVICE_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/input.h"

class Device
{
public:
    static Input::tDeviceList gDeviceList[5];

    static sint32 gForcePause;
    static sint32 gPausePort;
    static sint32 gPaused;
    static char gPausePortIndex;
    static sint32 gToggleTime[2];
    static sint32 gPrev[2];

    static sint32 VerifyType(sint32 port);
    static sint32 Fail(sint32 port);
    static void Update(void);
    static void StartUp(void);
    static void SetHardCodedKeys(void);
    static sint32 PSXPad(uint32 param);
    static bool ReadPad(sint32 port, uint32 param);
    static sint32 PSXPadMulti(uint32 param);
    static sint32 Analog(uint32 param);
    static sint32 Null(uint32 param);
};

#endif
