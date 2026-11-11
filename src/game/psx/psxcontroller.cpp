#include "xport.h"
#include "eaclib/psx/pad.h"
#include "game/common/carobj.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/nfs3.h"
#include "game/common/replay.h"
#include "game/psx/psxcontroller.h"

struct PAD_NEGCON
{
    uint8 state : 4;
    uint8 start : 4;
    uint8 buttonAB : 4;
    uint8 rightshift : 4;
    uint8 twist;
    uint8 buttonI;
    uint8 buttonII;
    uint8 leftshift;
};

struct PAD_MOUSE
{
    uint8 unused;
    uint8 state;
    char horizontalvalue;
    char verticalvalue;
    uint8 unused2[2];
};

sint32 mappings[3][13][3] = { { { 8388608, (-2147483647 - 1), 128 }, { 2097152, 536870912, 32 }, { 16384, 16384, 16384 }, { 32768, 32768, 32768 }, { 8192, 8192, 8192 }, { 16, 2048, 2048 }, { 64, 512, 512 }, { 4096, 4096, 4096 }, { 2048, 256, 256 }, { 128, 16, 16 }, { 16, 16, 16 }, { 1024, 1024, 1024 }, { 32, 1, 1 } }, { { 8388608, (-2147483647 - 1), 128 }, { 2097152, 536870912, 32 }, { 16384, 268435456, 32768 }, { 32768, 1073741824, 16384 }, { 8192, 8192, 8192 }, { 16, 1048576, 2048 }, { 64, 4194304, 512 }, { 4096, 4096, 4096 }, { 32, 256, 256 }, { 128, 16, 16 }, { 16, 16, 16 }, { 1024, 1024, 1024 }, { 2048, 1, 1 } }, { { 8388608, (-2147483647 - 1), 128 }, { 2097152, 536870912, 32 }, { 32768, 16384, 16384 }, { 16384, 32768, 32768 }, { 8192, 8192, 8192 }, { 16, 512, 512 }, { 64, 256, 256 }, { 4096, 4096, 4096 }, { 2048, 2048, 2048 }, { 128, 16, 16 }, { 16, 16, 16 }, { 1024, 1024, 1024 }, { 32, 1, 1 } } };
static sint32 hoff[2] = { 77, 1 };

void InGame::ResetPSXController(sint32 player, sint32 config)
{
    FUNCTION_MARKER(0x800DC4CC, "NFS4.EXE");

    sint32 *h = Input::gHandler;
    sint32 type = gPadinfo.buf[player * 4].ID;
    if (type == 0x23)
        type = 0;
    else if (type == 0x53 || type == 0x73)
        type = 1;
    else
        type = 2;
    if (frontEnd.controlType[player] != gPadinfo.buf[player * 4].ID)
        frontEnd.controlType[player] = gPadinfo.buf[player * 4].ID;
    GameSetup::gData.controllerData.controllerConfig[player] = config;
    h[0x4f - hoff[player]] = GetPSXPadValue(mappings[config][0][type], player);
    h[0x50 - hoff[player]] = GetPSXPadValue(mappings[config][1][type], player);
    h[0x51 - hoff[player]] = GetPSXPadValue(mappings[config][2][type], player);
    h[0x52 - hoff[player]] = GetPSXPadValue(mappings[config][3][type], player);
    h[player + 0xae] = GetPSXPadValue(mappings[config][8][type], player);
    h[0x75 - hoff[player]] = GetPSXPadValue(mappings[config][7][type], player);
    h[0x65 - hoff[player]] = GetPSXPadValue(mappings[config][7][type], player);
    h[0x53 - hoff[player]] = GetPSXPadValue(mappings[config][4][type], player);
    sint32 value;
    sint32 index;
    const Car::tObj *car = Cars::gHumanRaceCarList[player];
    if (!car || (car->carFlags & 0x200) == 0)
    {
        h[0x7d - hoff[player]] = GetPSXPadValue(mappings[config][0][type], 0);
        h[0x7e - hoff[player]] = GetPSXPadValue(mappings[config][1][type], 0);
        index = 0x82;
    }
    else
        index = 0x81;
    value = mappings[config][10][type];
    if (type == 1)
        value |= 6;
    h[index - hoff[player]] = GetPSXPadValue(value, player);
    if (GameSetup::gData.Time == 0)
    {
        h[0x73 - hoff[player]] = GetPSXPadValue(0, player);
        value = mappings[config][9][type];
        if (type == 1)
            value |= 6;
    }
    else
    {
        h[0x73 - hoff[player]] = GetPSXPadValue(mappings[config][9][type], player);
        value = type == 1 ? 6 : 0;
    }
    h[0x54 - hoff[player]] = GetPSXPadValue(value, player);
    h[0x66 - hoff[player]] = GetPSXPadValue(mappings[config][5][type], player);
    h[0x67 - hoff[player]] = GetPSXPadValue(mappings[config][6][type], player);
    h[0x68 - hoff[player]] = GetPSXPadValue(mappings[config][12][type], player);
    h[0x4d - hoff[player]] = GetPSXPadValue(mappings[config][11][type], player);
}

sint32 InGame::GetPSXPadValue(sint32 value, sint32 player)
{
    FUNCTION_MARKER(0x800DC990, "NFS4.EXE");

    PAD_update();
    PAD_COMMON *pad = &gPadinfo.buf[player * 4];
    sint32 type = pad->nopad == 0 ? pad->ID : 0;
    GameSetup::tControllerData &data = GameSetup::gData.controllerData;
    uint32 analogPlayer = (uint32)player << 30;
    sint32 c = value;
    if (type == 0x23)
    {
        if (c == 0x4000)
            return (sint32)(analogPlayer | ((uint32)data.ImaxRange[player] << 8) | 0x1000001U);
        if (c == 0x8000)
            return (sint32)(analogPlayer | ((uint32)data.IImaxRange[player] << 8) | 0x2000001U);
        if (c == 0x400)
            return (sint32)(analogPlayer | 0x30aff01U);
        if (c == 0x800000)
            return (sint32)(analogPlayer | ((128U - (uint32)data.deadSpot[player]) << 16) | ((128U - (uint32)data.steeringRange[player]) << 8) | 1U);
        if (c == 0x200000)
            return (sint32)(analogPlayer | (((uint32)data.deadSpot[player] + 128U) << 16) | (((uint32)data.steeringRange[player] + 128U) << 8) | 1U);
    }
    else if (type == 0x53 || type == 0x73)
    {
        switch ((uint32)c)
        {
        case 0x800000U:
            return (sint32)(analogPlayer | ((128U - (uint32)data.J1MIN[player]) << 16) | ((128U - (uint32)data.J1MAX[player]) << 8) | 1U);
        case 0x200000U:
            return (sint32)(analogPlayer | (((uint32)data.J1MIN[player] + 128U) << 16) | (((uint32)data.J1MAX[player] + 128U) << 8) | 1U);
        case 0x100000U:
            return (sint32)(analogPlayer | ((128U - (uint32)data.J1MIN[player]) << 16) | ((128U - (uint32)data.J1MAX[player]) << 8) | 0x1000001U);
        case 0x400000U:
            return (sint32)(analogPlayer | (((uint32)data.J1MIN[player] + 128U) << 16) | (((uint32)data.J1MAX[player] + 128U) << 8) | 0x1000001U);
        case 0x80000000U:
            return (sint32)(analogPlayer | ((128U - (uint32)data.J2MIN[player]) << 16) | ((128U - (uint32)data.J2MAX[player]) << 8) | 0x2000001U);
        case 0x20000000U:
            return (sint32)(analogPlayer | (((uint32)data.J2MIN[player] + 128U) << 16) | (((uint32)data.J2MAX[player] + 128U) << 8) | 0x2000001U);
        case 0x10000000U:
            return (sint32)(analogPlayer | ((128U - (uint32)data.J2MIN[player]) << 16) | ((128U - (uint32)data.J2MAX[player]) << 8) | 0x3000001U);
        case 0x40000000U:
            return (sint32)(analogPlayer | (((uint32)data.J2MIN[player] + 128U) << 16) | (((uint32)data.J2MAX[player] + 128U) << 8) | 0x3000001U);
        }
    }
    sint32 newControl = (sint32)(((uint32)player << 26) | ((uint32)value << 8) | 2U);
    return newControl;
}

sint32 InGame::GetDevice(sint32 control)
{
    FUNCTION_MARKER(0x800DCD34, "NFS4.EXE");

    return control & 255;
}

void InGame::SetRamp(void)
{
    FUNCTION_MARKER(0x800DCD3C, "NFS4.EXE");

    if (Replay::ReplayMode >= 2 || Cars::gNumHumanRaceCars <= 0)
        return;
    for (sint16 i = 0; i < Cars::gNumHumanRaceCars; i = (sint16)((uint16)i + 1))
    {
        GameSetup::tCarData *carInfo = Cars::gHumanRaceCarList[i]->carInfo;
        carInfo->RampGas = 1;
        carInfo->RampBrake = 1;
        carInfo->RampSteering = 1;
        sint32 *h = Input::gHandler;
        if (GetDevice(h[0x4f - hoff[i]]) == 1)
            Cars::gHumanRaceCarList[i]->carInfo->RampSteering = 0;
        if (GetDevice(h[0x51 - hoff[i]]) == 1)
            Cars::gHumanRaceCarList[i]->carInfo->RampGas = 0;
        if (GetDevice(h[0x52 - hoff[i]]) == 1)
            Cars::gHumanRaceCarList[i]->carInfo->RampBrake = 0;
    }
}
