#ifndef INPUT_H
#define INPUT_H

#include "xport.h"
#include "game/common/car.h"

class Input
{
public:
    struct tResults
    {
        char steering;
        uint8 gas;
        uint8 brake;
        uint8 flags;
    };

    struct tDeviceList
    {
        char *devicename;
        sint32 (*devicefunc)(uint32);
        sint32 (*startupfunc)(uint32);
    };

    static sint32 gTime;
    static sint32 gHandler[184];
    static uint32 gDBFlags[8];
    static uint32 gInterfaceResults[32];
    static sint32 gPressTime[2][17];
    static sint32 gLookBehind[2];
    static sint32 gMode[2];
    static tResults gSim;
    static tResults gResults[2];

    static sint32 * StartUp(void);
    static sint32 WingCommandMode(sint32 player);
    static void Update(void);
    static void Store(void);
    static void Fetch(sint32 humanIndex);
    static char Gear(char currentGear, sint32 numGears);
    static sint32 Interface(uint32 key, sint32 debounce);
    static sint32 MainExitKey(void);
};

#endif
