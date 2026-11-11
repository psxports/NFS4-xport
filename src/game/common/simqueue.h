#ifndef SIMQUEUE_H
#define SIMQUEUE_H

#include "xport.h"
#include "game/common/car.h"
#include "game/common/input.h"

class SimQueue
{
public:
    static void StartUp(void);
    static void Reset(void);
    static void CleanUp(void);
    static sint32 Put(sint32 pIndex, Input::tResults *val);
    static void SetCurrentInput(sint32 time);
    static void GetCurrentInput(sint32 pIndex, Input::tResults *out);
    static void SetLag(void);
    static sint32 IsBlocking(sint32 pIndex);
};

extern sint32 gSimQueue_BlockSelf;
extern sint32 gSimQueue_Ticker;

#endif
