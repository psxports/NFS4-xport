#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/feinput.h"
#include "frontend/common/femenu.h"
#include "frontend/common/fetools.h"
#include "frontend/common/front.h"
#include "game/common/nfs3.h"

struct tPSXToFEMapping
{
    sint32 PSXKey;
    tInputKeyType FEKey;
};

tPSXToFEMapping getKeyMappings[16] =
{
    {16, kInput_KeyType_Up},
    {128, kInput_KeyType_Left},
    {32, kInput_KeyType_Right},
    {64, kInput_KeyType_Down},
    {1024, kInput_KeyType_L1},
    {256, kInput_KeyType_L2},
    {2048, kInput_KeyType_R1},
    {512, kInput_KeyType_R2},
    {1048576, kInput_KeyType_Up},
    {8388608, kInput_KeyType_Left},
    {2097152, kInput_KeyType_Right},
    {4194304, kInput_KeyType_Down},
    {268435456, kInput_KeyType_Up},
    {(-2147483647 - 1), kInput_KeyType_Left},
    {536870912, kInput_KeyType_Right},
    {1073741824, kInput_KeyType_Down},
};

static sint32 nextTick;
static sint32 debounce[2];

void FEInput::ResetOverlayInput(void)
{
    debounce[0] = debounce[1] = 0;
    nextTick = 0;
}

void FEInput::VerifyControllerValues(sint32 controller)
{
    FUNCTION_MARKER(0x80023898, "FRONT.BIN");
    PAD_COMMON *pad = &gPadinfo.buf[controller * 4];
    if (pad->nopad == 0 && frontEnd.controlType[controller] != pad->ID)
        Front::ResetPSXController(controller, 0);
}

sint32 FEInput::GetNoDebounceKey(sint32 key, sint32 controller)
{
    FUNCTION_MARKER(0x800238F8, "FRONT.BIN");
    PAD_update();
    PAD_COMMON *pad = &gPadinfo.buf[controller * 4];
    if (pad->nopad != 0)
        return 0;
    VerifyControllerValues(controller);
    uint8 *analogs = reinterpret_cast<uint8 *>(&pad->data.analog) + 2;
    if (pad->ID == 0x73 || pad->ID == 0x53)
    {
        switch (uint32(key))
        {
        case 0x00800000: return analogs[0] < 64;
        case 0x00200000: return analogs[0] >= 193;
        case 0x00100000: return analogs[1] < 64;
        case 0x00400000: return analogs[1] >= 193;
        case 0x80000000: return analogs[2] < 64;
        case 0x20000000: return analogs[2] >= 193;
        case 0x10000000: return analogs[3] < 64;
        case 0x40000000: return analogs[3] >= 193;
        }
        return (~uint32(pad->data.standard.state) & uint32(key)) != 0;
    }
    if (pad->ID == 0x23)
    {
        switch (uint32(key))
        {
        case 0x00008000: return analogs[2] >= 65;
        case 0x00004000: return analogs[1] >= 65;
        case 0x00200000: return analogs[0] >= 161;
        case 0x00800000: return analogs[0] < 98;
        }
    }
    return (uint32(PAD_state(controller * 4)) & uint32(key)) != 0;
}

sint32 FEInput::GetDebounceKey(sint32 key, sint32 controller)
{
    FUNCTION_MARKER(0x80023B74, "FRONT.BIN");
    if (GetNoDebounceKey(key, controller) == 0)
    {
        debounce[controller] = (sint32)(uint32(debounce[controller]) & ~uint32(key));
        return 0;
    }
    uint32 previous = uint32(debounce[controller]);
    if ((previous & uint32(key)) == 0)
    {
        debounce[controller] = (sint32)(previous | uint32(key));
        nextTick = 0;
        return 1;
    }
    sint32 tick = ticks;
    if (key != 0x10 && key != 0x80 && key != 0x20 && key != 0x40)
        return 0;
    if (nextTick == 0)
        nextTick = (sint32)(uint32(tick) + uint32(FeTools::gScrollTicksOut) + 10);
    if (tick < nextTick)
        return 0;
    nextTick = (sint32)(uint32(tick) + uint32(FeTools::gScrollTicksOut));
    return 1;
}

tInputKeyType FEInput::GetKeyFromPlayer(tPlayer player, sint32 debounce)
{
    FUNCTION_MARKER(0x80023C88, "FRONT.BIN");
    for (uint16 i = 0; i < 16; ++i)
    {
        if (GetDebounceKey(getKeyMappings[i].PSXKey, player) != 0)
            return getKeyMappings[i].FEKey;
        if ((uint32(debounce) & uint32(getKeyMappings[i].FEKey)) == 0 && GetNoDebounceKey(getKeyMappings[i].PSXKey, player) != 0)
            return getKeyMappings[i].FEKey;
    }
    if (GetDebounceKey(0x4000, player) != 0)
        return kInput_KeyType_Cross;
    if (GetDebounceKey(0x1000, player) != 0)
        return kInput_KeyType_Triangle;
    if (GetDebounceKey(0x2000, player) != 0)
        return kInput_KeyType_Circle;
    if (GetDebounceKey(0x8000, player) != 0)
        return kInput_KeyType_Square;
    return GetDebounceKey(8, player) != 0 ? kInput_KeyType_Start : kInput_KeyType_NoKey;
}
