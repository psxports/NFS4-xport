#ifndef FEINPUT_H
#define FEINPUT_H

#include "xport.h"
#include "frontend/common/femenu.h"

class FEInput
{
public:
    static void VerifyControllerValues(sint32 controller);
    static sint32 GetNoDebounceKey(sint32 key, sint32 controller);
    static sint32 GetDebounceKey(sint32 key, sint32 controller);
    static tInputKeyType GetKeyFromPlayer(tPlayer player, sint32 debounce);
    static void ResetOverlayInput(void);
};

#endif
