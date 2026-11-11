#ifndef LOADING_H
#define LOADING_H

#include "xport.h"
#include "psx.h"
#include "game/common/car.h"

class Loading
{
public:
    static void DrawLoadingScreen(void);
    static void UpdateLoadingScreen(sint32 checkpoint);
    static void GetInitialMemory(void);
};

extern sint32 totalAvailMem;

#endif
