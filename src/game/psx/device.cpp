#include "xport.h"
#include "eaclib/psx/pad.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/nfs3.h"
#include "game/common/replay.h"
#include "game/common/sim.h"
#include "game/psx/device.h"
#include "game/psx/psxcontroller.h"

sint32 Device::gForcePause;
sint32 Device::gPausePort;
sint32 Device::gPaused;
char Device::gPausePortIndex;
sint32 Device::gToggleTime[2];
sint32 Device::gPrev[2];

Input::tDeviceList Device::gDeviceList[5] =
{
    { "Null", Device::Null, 0 },
    { "Analog", Device::Analog, 0 },
    { "Pad", Device::PSXPad, 0 },
    { "Multi", Device::PSXPadMulti, 0 },
    { 0, 0, 0 },
};

sint32 Device::VerifyType(sint32 port)
{
    FUNCTION_MARKER(0x800BD3C0, "NFS4.EXE");

    if (gUseFrontend != 0)
    {
        PAD_COMMON *pad = &gPadinfo.buf[port];
        if (pad->nopad != 0)
            return 0;
        sint32 player = port != 0;
        if (pad->ID != frontEnd.controlType[player])
        {
            InGame::ResetPSXController(player, GameSetup::gData.controllerData.controllerConfig[player]);
            frontEnd.controlType[player] = pad->ID;
        }
    }
    return 1;
}

sint32 Device::Fail(sint32 port)
{
    FUNCTION_MARKER(0x800BD468, "NFS4.EXE");

    static uint8 failtime[2] = { 0, 0 };
    sint32 index = port != 0;
    if (VerifyType(port) != 0)
        failtime[index] = 0;
    else
    {
        if (failtime[index] >= 9)
            return 1;
        ++failtime[index];
    }
    return 0;
}

void Device::Update(void)
{
    FUNCTION_MARKER(0x800BD4D8, "NFS4.EXE");

    PAD_update();
    if (simVar.pauseSim != 0)
    {
        gPaused = 1;
        gToggleTime[0] = 17;
        gToggleTime[1] = 17;
    }
    else if (gPaused != 0)
    {
        gPaused = 0;
        gPausePort = -1;
        gToggleTime[0] = 0;
        gToggleTime[1] = 0;
    }
    if (simVar.pauseSim == 0)
    {
        if (Fail(0) != 0)
        {
            gForcePause = 1;
            gPausePort = 0;
            gPausePortIndex = 0;
        }
        else if (GameSetup::gData.commMode == 1 && Fail(4) != 0)
        {
            gForcePause = 1;
            gPausePort = 4;
            gPausePortIndex = 1;
        }
        else
            gForcePause = 0;
    }
}

void Device::StartUp(void)
{
    FUNCTION_MARKER(0x800BD5B8, "NFS4.EXE");

    gPaused = 0;
    gForcePause = 0;
    gPausePort = -1;
    gToggleTime[0] = 0;
    gToggleTime[1] = 0;
    gPrev[0] = 0;
    gPrev[1] = 0;
}

void Device::SetHardCodedKeys(void)
{
    FUNCTION_MARKER(0x800BD5E0, "NFS4.EXE");

    Input::gHandler[0x99] = 0x1003;
    Input::gHandler[0x9a] = 0x4003;
    Input::gHandler[0x9b] = 0x8003;
    Input::gHandler[0x9c] = 0x2003;
    Input::gHandler[0x9e] = 0x803;
    Input::gHandler[0x9d] = 0x400003;
    Input::gHandler[0xb0] = 0x100003;
    Input::gHandler[0xb1] = 0x800003;
    Input::gHandler[0xa0] = 0x103;
    Input::gHandler[0x98] = 0;
}

sint32 Device::PSXPad(uint32 param)
{
    FUNCTION_MARKER(0x800BD644, "NFS4.EXE");

    sint32 port = param >> 16;
    if (VerifyType(port) == 0)
        return 0;
    return ((~(uint32)gPadinfo.buf[port].data.standard.state & 0xffff & param) != 0) ? 255 : 0;
}

bool Device::ReadPad(sint32 port, uint32 param)
{
    FUNCTION_MARKER(0x800BD6AC, "NFS4.EXE");

    if (VerifyType(port) == 0)
        return false;
    PAD_COMMON *pad = &gPadinfo.buf[port];
    uint32 key = param & 0xffff;
    if (pad->ID == 0x23)
    {
        if (key == 0x4000 && pad->data.analog.righty >= 64)
            return true;
        if (pad->ID == 0x23 && key == 0x8000 && pad->data.analog.leftx >= 64)
            return true;
    }
    if (pad->ID == 0x73)
    {
        uint8 *analogs = (uint8 *)&pad->data.analog + 2;
        if (key == 0x80 && (analogs[0] <= 64 || analogs[2] <= 64))
            return true;
        if (pad->ID == 0x73 && key == 0x20 && (analogs[0] >= 192 || analogs[2] >= 192))
            return true;
        if (pad->ID == 0x73 && key == 0x10 && (analogs[3] <= 64 || analogs[1] <= 64))
            return true;
        if (pad->ID == 0x73 && key == 0x40 && (analogs[3] >= 192 || analogs[1] >= 192))
            return true;
    }
    return (PAD_state(port) & param) != 0;
}

sint32 Device::PSXPadMulti(uint32 param)
{
    FUNCTION_MARKER(0x800BD8DC, "NFS4.EXE");

    if (simVar.pauseSim != 0)
        return ReadPad(gPausePort, param) ? 255 : 0;
    if (ReadPad(0, param))
    {
        if ((param & 0xffff) == 8)
        {
            gPausePort = 0;
            gPausePortIndex = 0;
        }
        return 255;
    }
    if (Replay::ReplayMode < 2 && GameSetup::gData.commMode != 1)
        return 0;
    if (!ReadPad(4, param))
        return 0;
    if ((param & 0xffff) == 8)
    {
        gPausePortIndex = 1;
        gPausePort = 4;
    }
    return 255;
}

sint32 Device::Analog(uint32 param)
{
    FUNCTION_MARKER(0x800BD9B8, "NFS4.EXE");

    sint32 port = param >> 20;
    if (VerifyType(port) == 0)
        return 0;
    uint8 *analogs = (uint8 *)&gPadinfo.buf[port].data.analog + 2;
    sint32 v = analogs[(param >> 16) & 3];
    sint32 min = (param >> 8) & 255;
    sint32 max = param & 255;
    if (min < max)
    {
        if (v < min)
            return 0;
        if (v > max)
            return 255;
        return ((v - min) * 255) / (max - min);
    }
    if (min == max)
        return 0;
    if (v < max)
        return 255;
    if (v > min)
        return 0;
    return ((min - v) * 255) / (min - max);
}

sint32 Device::Null(uint32 param)
{
    FUNCTION_MARKER(0x800BDAB8, "NFS4.EXE");

    return 0;
}
