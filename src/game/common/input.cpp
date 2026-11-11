#include "xport.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/sim.h"
#include "game/common/simqueue.h"
#include "game/psx/device.h"

sint32 Input::gTime;
Input::tResults Input::gSim;
sint32 Input::gHandler[184];
uint32 Input::gDBFlags[8];
uint32 Input::gInterfaceResults[32];
sint32 Input::gPressTime[2][17];
sint32 Input::gLookBehind[2];
sint32 Input::gMode[2];
Input::tResults Input::gResults[2];

sint32 * Input::StartUp(void)
{
    FUNCTION_MARKER(0x8009C344, "NFS4.EXE");

    Device::StartUp();
    sint32 *h = gHandler;
    for (sint32 i = 0; i < 184; ++i)
        *h++ = 0;
    for (sint32 i = 0; i < 2; ++i)
    {
        gDBFlags[i] = 0;
        gMode[i] = 0;
        for (sint32 j = 0; j < 17; ++j)
            gPressTime[i][j] = 0;
    }
    for (sint32 i = 31; i >= 0; --i)
        gInterfaceResults[i] = 0;
    return gHandler;
}

sint32 Input::WingCommandMode(sint32 player)
{
    FUNCTION_MARKER(0x8009C408, "NFS4.EXE");

    sint32 *h = gHandler + (player == 0 ? 45 : 121);
    return (h[0] != 0 || h[1] != 0 || h[2] != 0) && gMode[player & 1] == 1;
}

void Input::Update(void)
{
    FUNCTION_MARKER(0x8009C478, "NFS4.EXE");

    Device::Update();
    char iactive[32];
    for (sint32 i = 31; i >= 0; --i)
        iactive[i] = 1;
    sint32 *h = gHandler;
    uint32 menukeys = 0;
    for (sint32 i = 0; i < 2; ++i)
    {
        tResults *r = &gResults[i];
        sint32 mode = 0;
        for (sint32 j = 0; j < 2; ++j, ++h)
        {
            uint32 code = (uint32)*h;
            if (code != 0 && Device::gDeviceList[code & 255].devicefunc((uint32)((sint32)code >> 8)) > 64)
                mode = j + 1;
        }
        gMode[i] = mode;
        char active[40];
        if (mode != 0)
        {
            for (sint32 j = 39; j >= 0; --j)
                active[j] = 1;
            for (sint32 j = 0; j < 40; ++j)
                for (sint32 k = 0; k < 17; ++k)
                    if (h[j] == h[mode * 17 + 23 + k])
                        active[j] = 0;
            for (sint32 j = 0; j < 32; ++j)
                for (sint32 k = 0; k < 17; ++k)
                    if (gHandler[152 + j] == h[mode * 17 + 23 + k])
                        iactive[j] = 0;
        }
        uint32 code = (uint32)*h++;
        sint32 left = Device::gDeviceList[code & 255].devicefunc((uint32)((sint32)code >> 8));
        code = (uint32)*h++;
        sint32 right = Device::gDeviceList[code & 255].devicefunc((uint32)((sint32)code >> 8));
        if (mode == 0 || (active[0] != 0 && active[1] != 0))
            r->steering = (char)((sint32)((uint32)right - (uint32)left) / 2);
        if (mode == 0 || active[2] != 0)
        {
            code = (uint32)*h;
            r->gas = (uint8)Device::gDeviceList[code & 255].devicefunc((uint32)((sint32)code >> 8));
        }
        ++h;
        if (mode == 0 || active[3] != 0)
        {
            code = (uint32)*h;
            r->brake = (uint8)Device::gDeviceList[code & 255].devicefunc((uint32)((sint32)code >> 8));
        }
        ++h;
        for (sint32 j = 0; j < 2; ++j, ++h)
        {
            if (mode == 0 || active[j + 4] != 0)
            {
                code = (uint32)*h;
                if (code != 0 && Device::gDeviceList[code & 255].devicefunc((uint32)((sint32)code >> 8)) > 64)
                    r->flags = (uint8)(r->flags | (1U << j));
                else
                    r->flags = (uint8)(r->flags & ~(1U << j));
            }
        }
        r->flags &= 7;
        uint32 acc = 0;
        char hactive[17];
        for (sint32 k = 16; k >= 0; --k)
            hactive[k] = 1;
        for (sint32 j = 0; j < 17; ++j, ++h)
        {
            code = (uint32)*h;
            if (code != 0)
            {
                uint32 mask = 1U << j;
                if ((mode == 0 || active[j + 6] != 0) && Device::gDeviceList[code & 255].devicefunc((uint32)((sint32)code >> 8)) > 64)
                {
                    gPressTime[i][j] = (sint32)((uint32)gPressTime[i][j] + 1);
                    if (gPressTime[i][j] >= 6 && (gDBFlags[i] & mask) == 0)
                    {
                        acc = j + 1;
                        gDBFlags[i] |= mask;
                    }
                    for (sint32 k = 0; k < 17; ++k)
                    {
                        if (h[k + 17 - j] == *h)
                        {
                            hactive[k] = 0;
                            gPressTime[i][k] = gPressTime[i][j];
                        }
                    }
                }
                else
                {
                    gDBFlags[i] &= ~mask;
                    gPressTime[i][j] = 0;
                }
            }
        }
        for (sint32 j = 0; j < 17; ++j, ++h)
        {
            code = (uint32)*h;
            if (code != 0)
            {
                uint32 mask = 1U << j;
                if (mode != 0 && active[j + 23] == 0)
                    gDBFlags[i] &= ~mask;
                else if (Device::gDeviceList[code & 255].devicefunc((uint32)((sint32)code >> 8)) > 64)
                {
                    if ((gDBFlags[i] & mask) == 0 && hactive[j] != 0)
                    {
                        acc = j + 1;
                        gDBFlags[i] |= mask;
                    }
                }
                else if ((uint32)gPressTime[i][j] - 1 < 5)
                {
                    if ((gDBFlags[i] & mask) == 0)
                    {
                        acc = j + 1;
                        gDBFlags[i] |= mask;
                    }
                    gPressTime[i][j] = 0;
                }
                else
                    gDBFlags[i] &= ~mask;
            }
        }
        if (mode == 0)
        {
            for (sint32 m = 0; m < 2; ++m)
            {
                for (sint32 k = 0; k < 17; ++k, ++h)
                {
                    code = (uint32)*h;
                    if (code != 0)
                    {
                        uint32 mask = 1U << k;
                        if (Device::gDeviceList[code & 255].devicefunc((uint32)((sint32)code >> 8)) > 64)
                            gDBFlags[i] |= mask;
                        else
                            gDBFlags[i] &= ~mask;
                    }
                }
            }
        }
        else
        {
            h += (mode - 1) * 17;
            for (sint32 k = 0; k < 17; ++k, ++h)
            {
                code = (uint32)*h;
                if (code != 0)
                {
                    uint32 mask = 1U << k;
                    if (Device::gDeviceList[code & 255].devicefunc((uint32)((sint32)code >> 8)) > 64)
                    {
                        if ((gDBFlags[i] & mask) == 0)
                        {
                            acc = k + 1;
                            gDBFlags[i] |= mask;
                        }
                    }
                    else
                        gDBFlags[i] &= ~mask;
                }
            }
            h += (2 - mode) * 17;
        }
        r->flags = (uint8)(r->flags | (acc << 3));
        uint32 command = r->flags >> 3;
        if (command == 15)
        {
            r->flags &= 7;
            menukeys |= 0x200000;
        }
        else if (command == 16 || command == 17)
        {
            r->flags &= 7;
            menukeys |= 1U << (i + (command == 16 ? 26 : 28));
        }
        if (GameSetup::gData.numPlayerRaceCars == 1)
        {
            h += 76;
            ++i;
        }
    }
    for (sint32 k = 0; k < 32; ++k, ++h)
    {
        if (iactive[k] != 0)
        {
            uint32 code = (uint32)*h;
            if (code != 0 && Device::gDeviceList[code & 255].devicefunc((uint32)((sint32)code >> 8)) > 64)
                menukeys |= 1U << k;
        }
    }
    uint32 time = (uint32)gTime;
    gTime = (sint32)(time + 2);
    gInterfaceResults[(time + 2) & 31] = menukeys;
    gInterfaceResults[(time + 3) & 31] = menukeys;
}

void Input::Store(void)
{
    FUNCTION_MARKER(0x8009D208, "NFS4.EXE");

    if (gSimQueue_BlockSelf != 0)
        return;
    if (GameSetup::gData.numPlayerRaceCars < 2)
    {
        if (GameSetup::gData.commMode == 0)
            SimQueue::Put(GameSetup::gData.localCar, &gResults[0]);
    }
    else if (SimQueue::Put(0, &gResults[0]) != 0)
        SimQueue::Put(1, &gResults[1]);
}

void Input::Fetch(sint32 humanIndex)
{
    FUNCTION_MARKER(0x8009D294, "NFS4.EXE");

    SimQueue::GetCurrentInput(humanIndex, &gSim);
}

char Input::Gear(char currentGear, sint32 numGears)
{
    FUNCTION_MARKER(0x8009D2BC, "NFS4.EXE");

    if ((gSim.flags >> 3) == 1)
    {
        if ((uint8)currentGear < (sint32)((uint32)numGears - 1))
            currentGear = (char)((uint8)currentGear + 1);
    }
    else if ((gSim.flags >> 3) == 2 && currentGear != 0)
        currentGear = (char)((uint8)currentGear - 1);
    return currentGear;
}

sint32 Input::Interface(uint32 key, sint32 debounce)
{
    FUNCTION_MARKER(0x8009D318, "NFS4.EXE");

    uint32 mask = (uint32)1 << (key & 31);
    uint32 time = (uint32)simGlobal.time32Hz;
    uint32 keys = gInterfaceResults[time & 31];
    if (debounce != 0)
        keys &= ~gInterfaceResults[(time - 1) & 31];
    return (keys & mask) != 0;
}

sint32 Input::MainExitKey(void)
{
    FUNCTION_MARKER(0x8009D3BC, "NFS4.EXE");

    return gInterfaceResults[(uint32)simGlobal.time32Hz & 31] & 1;
}
