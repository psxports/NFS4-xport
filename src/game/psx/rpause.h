#ifndef RPAUSE_H
#define RPAUSE_H

#include "psx.h"
#include "game/common/car.h"

class RPause
{
public:
    static void CopyBackToFrontBuffer(void);
    static void StartPauseMenu(void);
    static void StopPauseMenu(void);
};

#endif
