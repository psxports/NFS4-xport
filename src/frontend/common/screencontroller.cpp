#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/feapp.h"
#include "frontend/common/fefades.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/front.h"
#include "frontend/common/screencontroller.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/mmeffect.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/nfs3.h"
#include "game/common/textsys.h"
#include "game/psx/flare.h"
#include "game/psx/psxcontroller.h"
#include "game/psx/textpix.h"

static const char fileNames_shared[] = "aCtrl";
static const char *fileNames[7] = { "None", "Neg", "Whl", "Ctrl", fileNames_shared, fileNames_shared, fileNames_shared };
static sint32 flare_intensity;
static sint32 max_fi = 3750;
static sint32 max_fidir = 250;
static uint8 Offset[7][2] = { { 0, 0 }, { 133, 117 }, { 228, 117 }, { 130, 117 }, { 130, 117 }, { 130, 117 }, { 130, 117 } };
tScreenControllerConfig *screenControllerConfig;

sint16 ArrowLocations[55][6] =
{
    {0, 0, 290, 37, 290, 47},
    {0, 0, 290, 85, 290, 58},
    {0, 0, 413, 37, 413, 47},
    {0, 0, 413, 85, 413, 58},
    {153, 143, 193, 143, 193, 160},
    {153, 163, 181, 163, 181, 167},
    {153, 183, 193, 183, 193, 175},
    {153, 203, 205, 203, 205, 168},
    {237, 213, 242, 213, 242, 175},
    {272, 213, 265, 213, 265, 175},
    {351, 143, 314, 143, 314, 158},
    {351, 163, 329, 163, 329, 167},
    {351, 183, 314, 183, 314, 175},
    {351, 203, 298, 203, 298, 168},
    {0, 0, 290, 37, 290, 47},
    {0, 0, 290, 85, 290, 58},
    {0, 0, 415, 37, 415, 47},
    {0, 0, 415, 85, 415, 58},
    {153, 143, 193, 143, 193, 161},
    {153, 163, 182, 163, 182, 167},
    {153, 183, 193, 183, 193, 174},
    {153, 203, 205, 203, 205, 167},
    {231, 215, 238, 215, 238, 165},
    {280, 215, 271, 215, 271, 165},
    {351, 143, 317, 143, 317, 158},
    {351, 163, 332, 163, 332, 167},
    {351, 183, 318, 183, 318, 178},
    {351, 203, 302, 203, 302, 168},
    {351, 193, 302, 193, 302, 168},
    {233, 130, 239, 130, 239, 165},
    {280, 130, 271, 130, 271, 165},
    {215, 215, 225, 215, 225, 180},
    {215, 209, 225, 209, 225, 180},
    {296, 215, 285, 215, 285, 180},
    {0, 0, 288, 40, 288, 60},
    {0, 0, 428, 40, 428, 60},
    {150, 146, 189, 146, 189, 168},
    {150, 166, 179, 166, 179, 173},
    {150, 186, 189, 186, 189, 180},
    {150, 206, 200, 206, 200, 173},
    {352, 146, 310, 146, 310, 166},
    {352, 166, 327, 166, 327, 175},
    {352, 186, 310, 186, 310, 183},
    {352, 206, 290, 206, 290, 175},
    {195, 220, 214, 220, 214, 183},
    {292, 160, 292, 170, 172, 170},
    {167, 150, 172, 145, 172, 170},
    {167, 200, 172, 200, 172, 171},
    {0, 0, 292, 100, 292, 136},
    {0, 0, 300, 65, 300, 138},
    {0, 0, 308, 30, 308, 143},
    {0, 0, 378, 30, 378, 143},
    {0, 0, 386, 65, 386, 138},
    {0, 0, 394, 100, 394, 136},
    {167, 210, 341, 210, 341, 170}
};

sint16 TextLocations[80][5] =
{
    {335, 29, 570, 1, 0},
    {335, 85, 543, 1, 0},
    {365, 29, 569, 0, 0},
    {365, 85, 568, 0, 0},
    {335, 29, 569, 1, 0},
    {365, 29, 568, 0, 0},
    {365, 85, 570, 0, 0},
    {150, 140, 558, 1, 0},
    {150, 148, 559, 1, 0},
    {150, 160, 555, 1, 0},
    {150, 200, 556, 1, 0},
    {232, 210, 554, 1, 0},
    {277, 210, 552, 0, 0},
    {354, 140, 547, 0, 0},
    {354, 148, 548, 0, 0},
    {354, 160, 550, 0, 0},
    {354, 200, 564, 0, 0},
    {354, 180, 557, 0, 0},
    {354, 140, 557, 0, 0},
    {354, 208, 548, 0, 0},
    {354, 160, 564, 0, 0},
    {354, 200, 547, 0, 0},
    {354, 180, 550, 0, 0},
    {354, 180, 564, 0, 0},
    {354, 200, 557, 0, 0},
    {335, 85, 567, 1, 0},
    {375, 29, 543, 0, 0},
    {226, 212, 554, 1, 0},
    {283, 212, 552, 0, 0},
    {228, 127, 554, 1, 0},
    {281, 127, 552, 0, 0},
    {210, 212, 572, 1, 0},
    {210, 204, 574, 1, 0},
    {210, 212, 573, 1, 0},
    {210, 220, 564, 1, 0},
    {210, 206, 576, 1, 0},
    {301, 212, 572, 0, 0},
    {301, 212, 575, 0, 0},
    {301, 220, 567, 0, 0},
    {147, 140, 557, 1, 0},
    {147, 180, 565, 1, 0},
    {147, 188, 545, 1, 0},
    {357, 190, 558, 0, 0},
    {357, 198, 559, 0, 0},
    {330, 33, 543, 1, 0},
    {370, 33, 571, 0, 0},
    {370, 33, 553, 0, 0},
    {148, 143, 566, 1, 0},
    {148, 183, 567, 1, 0},
    {148, 163, 558, 1, 0},
    {148, 171, 559, 1, 0},
    {148, 203, 554, 1, 0},
    {148, 203, 570, 1, 0},
    {353, 183, 557, 0, 0},
    {353, 203, 564, 0, 0},
    {353, 203, 557, 0, 0},
    {353, 183, 564, 0, 0},
    {353, 143, 547, 0, 0},
    {353, 151, 548, 0, 0},
    {353, 163, 550, 0, 0},
    {195, 217, 552, 1, 0},
    {352, 20, 570, 0, 0},
    {352, 20, 554, 0, 0},
    {326, 20, 543, 1, 0},
    {326, 54, 565, 1, 0},
    {326, 54, 557, 1, 0},
    {326, 68, 545, 1, 0},
    {352, 54, 547, 0, 0},
    {352, 68, 548, 0, 0},
    {326, 90, 557, 1, 0},
    {326, 90, 544, 1, 0},
    {326, 104, 545, 1, 0},
    {352, 90, 550, 0, 0},
    {156, 146, 566, 1, 0},
    {156, 159, 567, 1, 0},
    {156, 170, 558, 1, 0},
    {156, 178, 559, 1, 0},
    {156, 189, 554, 1, 0},
    {156, 189, 570, 1, 0},
    {162, 207, 552, 1, 0}
};

sint16 NumTexts[7][3] =
{
    {13, 13, 13},
    {14, 14, 14},
    {15, 15, 15},
    {15, 15, 15},
    {14, 14, 14},
    {0, 0, 0},
    {0, 0, 0}
};

char ControllerItemIndex[5][3][16][2] =
{
    {
        {
            {44, 34},
            {45, 35},
            {47, 36},
            {48, 38},
            {49, 37},
            {50, 0xff},
            {51, 39},
            {53, 42},
            {54, 43},
            {57, 41},
            {58, 0xff},
            {59, 40},
            {60, 44},
            {0, 0},
            {0, 0},
            {0, 0}
        },
        {
            {44, 34},
            {46, 35},
            {47, 36},
            {48, 38},
            {49, 37},
            {50, 0xff},
            {52, 39},
            {53, 42},
            {54, 43},
            {57, 41},
            {58, 0xff},
            {59, 40},
            {60, 44},
            {0, 0},
            {0, 0},
            {0, 0}
        },
        {
            {44, 34},
            {45, 35},
            {47, 36},
            {48, 38},
            {49, 37},
            {50, 0xff},
            {51, 39},
            {55, 42},
            {56, 43},
            {57, 41},
            {58, 0xff},
            {59, 40},
            {60, 44},
            {0, 0},
            {0, 0},
            {0, 0}
        }
    },
    {
        {
            {63, 50},
            {61, 51},
            {73, 45},
            {74, 46},
            {75, 47},
            {76, 0xff},
            {77, 0xff},
            {69, 48},
            {64, 49},
            {66, 0xff},
            {67, 52},
            {68, 0xff},
            {72, 53},
            {79, 54},
            {0, 0},
            {0, 0}
        },
        {
            {63, 50},
            {62, 51},
            {73, 45},
            {74, 46},
            {75, 47},
            {76, 0xff},
            {78, 0xff},
            {69, 48},
            {64, 49},
            {66, 0xff},
            {67, 52},
            {68, 0xff},
            {72, 53},
            {79, 54},
            {0, 0},
            {0, 0}
        },
        {
            {63, 50},
            {61, 51},
            {73, 45},
            {74, 46},
            {75, 47},
            {76, 0xff},
            {77, 0xff},
            {65, 48},
            {70, 49},
            {71, 0xff},
            {67, 52},
            {68, 0xff},
            {72, 53},
            {79, 54},
            {0, 0},
            {0, 0}
        }
    },
    {
        {
            {0, 1},
            {1, 0},
            {2, 3},
            {3, 2},
            {7, 4},
            {8, 0xff},
            {9, 5},
            {10, 7},
            {11, 8},
            {12, 9},
            {13, 10},
            {14, 0xff},
            {15, 11},
            {16, 12},
            {17, 13},
            {0, 0}
        },
        {
            {0, 1},
            {1, 0},
            {2, 17},
            {3, 16},
            {7, 4},
            {8, 0xff},
            {9, 5},
            {10, 7},
            {11, 8},
            {12, 9},
            {13, 10},
            {14, 0xff},
            {15, 11},
            {23, 12},
            {24, 13},
            {0, 0}
        },
        {
            {4, 1},
            {1, 0},
            {5, 3},
            {6, 2},
            {7, 4},
            {8, 0xff},
            {9, 5},
            {10, 7},
            {11, 8},
            {12, 9},
            {13, 10},
            {14, 0xff},
            {15, 11},
            {16, 12},
            {17, 13},
            {0, 0}
        }
    },
    {
        {
            {0, 15},
            {1, 14},
            {2, 17},
            {3, 16},
            {7, 18},
            {8, 0xff},
            {9, 19},
            {10, 21},
            {27, 22},
            {28, 23},
            {13, 24},
            {14, 0xff},
            {15, 25},
            {16, 26},
            {17, 27},
            {0, 0}
        },
        {
            {0, 15},
            {1, 14},
            {2, 17},
            {3, 16},
            {7, 18},
            {8, 0xff},
            {9, 19},
            {10, 21},
            {27, 22},
            {28, 23},
            {13, 24},
            {14, 0xff},
            {15, 25},
            {23, 26},
            {24, 27},
            {0, 0}
        },
        {
            {4, 15},
            {1, 14},
            {5, 17},
            {6, 16},
            {7, 18},
            {8, 0xff},
            {9, 19},
            {10, 21},
            {27, 22},
            {28, 23},
            {13, 24},
            {14, 0xff},
            {15, 25},
            {16, 26},
            {17, 27},
            {0, 0}
        }
    },
    {
        {
            {29, 29},
            {30, 30},
            {31, 31},
            {0, 15},
            {1, 14},
            {2, 17},
            {3, 16},
            {7, 18},
            {8, 0xff},
            {13, 24},
            {14, 0xff},
            {15, 25},
            {16, 26},
            {17, 27},
            {0, 0},
            {0, 0}
        },
        {
            {29, 29},
            {30, 30},
            {32, 31},
            {33, 0xff},
            {34, 0xff},
            {37, 33},
            {38, 0xff},
            {0, 15},
            {1, 14},
            {7, 18},
            {8, 0xff},
            {13, 24},
            {14, 0xff},
            {15, 25},
            {0, 0},
            {0, 0}
        },
        {
            {29, 29},
            {30, 30},
            {31, 31},
            {4, 15},
            {1, 14},
            {5, 17},
            {6, 16},
            {7, 18},
            {8, 0xff},
            {13, 24},
            {14, 0xff},
            {15, 25},
            {16, 26},
            {17, 27},
            {0, 0},
            {0, 0}
        }
    }
};

static sint16 ControllerTextCount(uint8 controller, uint8 config)
{
    if (controller == 0)
        return TextLocations[79][config + 2];
    return NumTexts[controller - 1][config];
}

void Controller_SetRamp(void)
{
    FUNCTION_MARKER(0x80043250, "FRONT.BIN");
    for (sint16 i = 0; i < 2; ++i)
    {
        sint32 type = frontEnd.controlType[i] == 0x23 ? 0 :
            (frontEnd.controlType[i] == 0x53 || frontEnd.controlType[i] == 0x73 ? 1 : 2);
        sint32 config = (uint8)frontEnd.controlConfig[i];
        frontEnd.rampGas[i] = 1;
        frontEnd.rampBrake[i] = 1;
        frontEnd.rampSteer[i] = 1;
        sint32 control = GetPSXPadValue(mappings[config][0][type], 0);
        if (InGame::GetDevice(control) == 1)
            frontEnd.rampSteer[i] = 0;
        control = GetPSXPadValue(mappings[config][2][type], 0);
        if (InGame::GetDevice(control) == 1)
            frontEnd.rampGas[i] = 0;
        control = GetPSXPadValue(mappings[config][3][type], 0);
        if (InGame::GetDevice(control) == 1)
            frontEnd.rampBrake[i] = 0;
    }
}

void tScreenControllerConfig::ClearActuators(void)
{
    FUNCTION_MARKER(0x80043118, "FRONT.BIN");
    fShaker.actuator[0] = 0;
    fShaker.actuator[1] = 0;
}

void tScreenControllerConfig::SetActuators(sint32 max)
{
    FUNCTION_MARKER(0x80043124, "FRONT.BIN");
    if (fResetShakeTimeOut != 0)
    {
        fResetShakeTimeOut = 0;
        fShaker.time = 64;
    }
    else
    {
        if (fShaker.time == 0)
        {
            ClearActuators();
            return;
        }
        --fShaker.time;
    }
    if (fShaker.time != 0)
    {
        uint8 random = Force::rand_256[(ticks >> 2) & 255];
        fShaker.actuator[1] = (uint8)max;
        fShaker.actuator[0] = random < max;
    }
    else
        ClearActuators();
}

void tScreenControllerConfig::TurnOffShakers(void)
{
    FUNCTION_MARKER(0x800431AC, "FRONT.BIN");
    fShaker.active = 0;
}

void tScreenControllerConfig::ShakeIt(void)
{
    FUNCTION_MARKER(0x800431B4, "FRONT.BIN");
    sint32 padnum = (sint32)(uint32(player) << 4);
    sint32 padState = PadGetState(padnum);
    if (padState == 6)
    {
        if (fShaker.active == 0)
        {
            fShaker.active = 1;
            PadSetAct(padnum, fShaker.actuator, 2);
            PadSetActAlign(padnum, Force::gActAlign);
        }
    }
    else
    {
        if (padState < 4)
            fShaker.active = 0;
        ClearActuators();
    }
}

sint16 tScreenControllerConfig::AnimKeyPoints(bool forward, bool pt)
{
    FUNCTION_MARKER(0x8004339C, "FRONT.BIN");
    if ((uint8)fCurrentController == 2)
    {
        if (!pt)
            return 20;
        return forward ? 0 : 40;
    }
    if (pt & forward)
        return 0;
    return pt || forward ? 24 : 0;
}

void tScreenControllerConfig::CheckConfigs(void)
{
    FUNCTION_MARKER(0x80043400, "FRONT.BIN");
    if ((uint8)fCurrentController == (uint8)fPrevController)
    {
        if ((uint8)frontEnd.controlConfig[player] == (uint8)fPrevConfig)
            return;
        Front::ResetPSXController(player, (uint8)frontEnd.controlConfig[player]);
        if (fFade[0] == 0 && fFade[1] == 0 && fAnim == 0)
        {
            fAnim = 1;
            fAnimController = (uint8)fCurrentController;
            uint8 config = (uint8)frontEnd.controlConfig[player];
            uint8 previous = (uint8)fPrevConfig;
            if ((previous < config && (config != 2 || previous != 0)) || (config == 0 && previous == 2))
            {
                fAnimStart = AnimKeyPoints(true, true);
                fAnimStop = AnimKeyPoints(true, false);
                fAnimStep = 1;
            }
            else
            {
                fAnimStart = AnimKeyPoints(false, true);
                fAnimStop = AnimKeyPoints(false, false);
                fAnimStep = -1;
            }
            fAnimFrame = fAnimStart;
        }
        fPrevConfig = frontEnd.controlConfig[player];
        return;
    }
    TurnOffShakers();
    Front::ResetPSXController(player, (uint8)frontEnd.controlConfig[player]);
    if (fAnim != 0)
        return;
    if ((uint8)fCurrentController == 0)
    {
        if (fAnimFade != 0)
            return;
        uint8 previous = (uint8)fPrevController;
        uint8 current = (uint8)fCurrentController;
        fFade[0] = 1;
        fFadeController[0] = previous;
        fPrevController = (char)current;
        return;
    }
    if ((uint8)fPrevController == 0)
    {
        if (fAnimFade != 0)
            return;
        uint8 current = (uint8)fCurrentController;
        sint16 arrowFade = fArrowFade;
        fSwap = 1;
        fFade[1] = 1;
        fFadeController[1] = current;
        if (arrowFade >= 128)
            fTextController = fCurrentController;
        if ((uint8)fTextController == 6)
            fTextController = 5;
    }
    else if (CurrentlyLoadedArt != -1 &&
        strcmp(fileNames[(uint8)fCurrentController], fileNames[CurrentlyLoadedArt]) == 0)
    {
        fFadeTextOut = 1;
        if (((uint8)fPrevController == 5 || (uint8)fPrevController == 3) &&
            ((uint8)fCurrentController == 4 || (uint8)fCurrentController == 6))
            SwapInController();
    }
    else
    {
        if (fAnimFade != 0)
            return;
        uint8 previous = (uint8)fPrevController;
        uint8 current = (uint8)fCurrentController;
        sint16 arrowFade = fArrowFade;
        fFade[0] = 1;
        fSwap = 1;
        fFade[1] = 1;
        fFadeController[0] = previous;
        fFadeController[1] = current;
        if (arrowFade >= 128)
            fTextController = fCurrentController;
        if ((uint8)fTextController == 6)
            fTextController = 5;
    }
    fPrevController = fCurrentController;
}

void tScreenControllerConfig::SwapInController(void)
{
    FUNCTION_MARKER(0x800436EC, "FRONT.BIN");
    if (CurrentlyLoadedArt == -1 ||
        strcmp(fileNames[(uint8)fCurrentController], fileNames[CurrentlyLoadedArt]) != 0)
    {
        AsyncLoadSwapShapeFile(const_cast<char *>(fileNames[(uint8)fCurrentController]));
        CurrentlyLoadedArt = (uint8)fCurrentController;
    }
}

void tScreenControllerConfig::SetCurrentController(bool firsttime)
{
    FUNCTION_MARKER(0x80043778, "FRONT.BIN");
    tInsideBoxMenu *fSetMenu = 0;
    negconPopUp.string = TextSys::Word(523);
    negconPopUp.yesnowords[0] = 524;
    negconPopUp.yesnowords[1] = 525;
    negconPopUp.fDefault = 1;
    bool setmenutonull = false;
    PAD_COMMON *pad = &gPadinfo.buf[player != 0 ? 4 : 0];
    if (pad->nopad != 0)
    {
        sint16 arrowFade = fArrowFade;
        fCurrentController = 0;
        if (arrowFade >= 128)
            CurrentlyLoadedArt = 0;
        setmenutonull = true;
    }
    else if (pad->ID == 0x23)
    {
        fTimeOutStartTick = 0;
        if (!firsttime && (uint8)fCurrentController != 2 && (uint8)fCurrentController != 1)
        {
            bool freeDialog = false;
            if (negconPopUp.currentlyOn == 0)
                freeDialog = negconPopUp.fCurrentlyRunning == 0;
            if (freeDialog && negconChoice == -1)
            {
                fCurrentController = 0;
                fArrowFade = 128;
                negconChoice = negconPopUp.tDialogInteractive::Run();
                fCurrentController = negconChoice == 0 ? 1 : 2;
                negconChoice = -1;
                fSetMenu = &menuDefs->menuControllerNegcon;
            }
            else
            {
                setmenutonull = true;
                firsttime = true;
                fArrowFade = 128;
                fCurrentController = 0;
            }
        }
        else
            fSetMenu = &menuDefs->menuControllerNegcon;
    }
    else if (pad->ID == 0x53 || pad->ID == 0x73)
    {
        fTimeOutStartTick = 0;
        if (PadGetState((player != 0 ? 1 : 0) << 4) == 6)
        {
            fCurrentController = 6;
            fSetMenu = &menuDefs->menuControllerDualShockAnalog;
        }
        else if (PadGetState((player != 0 ? 1 : 0) << 4) == 2)
        {
            fCurrentController = 5;
            fSetMenu = &menuDefs->menuControllerAnalog;
        }
    }
    else
    {
        bool chooseDigital = true;
        if (pad->ID == 0x41)
        {
            if (PadGetState((player != 0 ? 1 : 0) << 4) == 6)
            {
                fCurrentController = 4;
                fTimeOutStartTick = 0;
                fSetMenu = &menuDefs->menuControllerDualShock;
                chooseDigital = false;
            }
            else
            {
                bool ready = false;
                if (PadGetState((player != 0 ? 1 : 0) << 4) == 2)
                    ready = true;
                else if (fTimeOutStartTick != 0 && PadGetState((player != 0 ? 1 : 0) << 4) == 1 &&
                    (sint32)(uint32(ticks) - uint32(fTimeOutStartTick)) > 96)
                    ready = true;
                if (!ready && (uint8)fCurrentController != 3)
                {
                    if (fTimeOutStartTick == 0)
                        fTimeOutStartTick = ticks;
                    chooseDigital = false;
                }
            }
        }
        if (chooseDigital)
        {
            setmenutonull = true;
            fTimeOutStartTick = 0;
            fCurrentController = 3;
        }
    }
    if (fSetMenu != 0 || setmenutonull)
        menuDefs->itemControllerSettings.SetMenu(firsttime, fSetMenu);
    menuDefs->menuControllerConfig.VertHelp =
        ((uint8)fCurrentController == 5 || (uint8)fCurrentController == 6) &&
        (uint8)fTextConfig < 2;
}

sint32 tScreenControllerConfig::CalcAnimFrame(sint32 frame)
{
    FUNCTION_MARKER(0x80043AF0, "FRONT.BIN");
    if ((uint8)fCurrentController != 2)
        return frame;
    if (frame == 0 || frame == 20 || frame == 40)
        return 1;
    if (frame < 11)
    {
        if (frame > 9)
            frame = 9;
        return frame + 2;
    }
    if (frame < 20)
        return 22 - frame;
    if (frame > 30)
        return 56 - frame;
    frame -= 20;
    if (frame > 9)
        frame = 9;
    return frame + 16;
}

void tScreenControllerConfig::ActualDrawController(sint32 frame, sint32 fadelevelmain, sint32 fadeleveltop, sint32 shakex, sint32 shakey)
{
    FUNCTION_MARKER(0x80043B7C, "FRONT.BIN");
    if (CurrentlyLoadedArt == 0)
        return;
    tDrawShapeExtended drawFlags;
    drawFlags.custom_shapes = fSwapShapes.fShapes;
    if (frame == 0 && CurrentlyLoadedArt != 1)
    {
        DrawShapeExtended(1, fadelevelmain != 0 ? 0x601 : 0x600,
            (sint32)(uint32(Offset[CurrentlyLoadedArt][0]) + uint32(shakex)),
            (sint32)(uint32(Offset[CurrentlyLoadedArt][1]) + uint32(shakey)), fadelevelmain, 0, &drawFlags);
    }
    else
    {
        ScaleShapeExtended((sint32)(uint32(frame) + 1U), fadelevelmain != 0 ? 0x601 : 0x600,
            Offset[CurrentlyLoadedArt][0], Offset[CurrentlyLoadedArt][1], fadelevelmain, 0, &drawFlags);
        if (CurrentlyLoadedArt != 2)
            goto drawTop;
        DrawShapeExtended(0, fadelevelmain != 0 ? 0x601 : 0x600,
            (sint32)(uint32(Offset[2][0]) + uint32(shakex)),
            (sint32)(uint32(Offset[2][1]) + uint32(shakey)), fadelevelmain, 0, &drawFlags);
    }
    if (CurrentlyLoadedArt == 2)
    {
        tDrawShapeExtended drawFlags;
        drawFlags.tint[0] = CalcFadeVal(0xC8C8C8, fArrowFade);
        DrawShapeExtended(26, 0x18, 158, 146, 0, 1, &drawFlags);
        DrawShapeExtended(27, 0x18, 158, 161, 0, 1, &drawFlags);
        DrawShapeExtended(28, 0x18, 158, 172, 0, 1, &drawFlags);
        DrawShapeExtended(29, 0x18, 158, 187, 0, 1, &drawFlags);
        for (sint32 i = 48; i < 54; ++i)
            PSXDrawSquare(drawFlags.tint[0], ArrowLocations[i][2] - 2, ArrowLocations[i][3] - 2, 5, 3);
        return;
    }
drawTop:
    DrawShapeExtended(0, fadeleveltop != 0 ? 0x201 : 0x200, 0, 0, fadeleveltop, 0, &drawFlags);
}

void Front::ResetOverlayController(void)
{
    flare_intensity = 0;
    max_fi = 3750;
    max_fidir = 250;
}

void tScreenControllerConfig::DrawController(void)
{
    FUNCTION_MARKER(0x80043DE4, "FRONT.BIN");
    sint16 shakex = 0;
    sint16 shakey = 0;
    sint16 maxshakex = 0;
    sint16 maxshakey = 0;
    tDrawShapeExtended drawFlags;
    drawFlags.custom_shapes = fSwapShapes.fShapes;
    bool onShockMode = (((uint8)fCurrentController == 4 && (sint16)menuDefs->menuControllerDualShock.fCurrentItem == 0) ||
        ((uint8)fCurrentController == 6 && (sint16)menuDefs->menuControllerDualShockAnalog.fCurrentItem == 0)) &&
        (sint16)menuDefs->menuControllerConfig.fCurrentItem == 1;
    if (onShockMode && (uint8)frontEnd.shockMode[player] != 0)
    {
        if (fShakingItem != 0)
        {
            fShakingItem = 0;
            fResetShakeTimeOut = 1;
        }
        SetActuators((uint8)frontEnd.shockMode[player] * 2);
        uint8 strength = (uint8)frontEnd.shockMode[player];
        maxshakex = (sint16)((strength >> 5) + 2);
        maxshakey = (sint16)((strength >> 6) + 2);
    }
    else
    {
        bool onShockImpact = (((uint8)fCurrentController == 4 && (sint16)menuDefs->menuControllerDualShock.fCurrentItem == 1) ||
            ((uint8)fCurrentController == 6 && (sint16)menuDefs->menuControllerDualShockAnalog.fCurrentItem == 1)) &&
            (sint16)menuDefs->menuControllerConfig.fCurrentItem == 1;
        if (onShockImpact && (uint8)frontEnd.shockImpact[player] != 0)
        {
            SetActuators((uint8)frontEnd.shockImpact[player] * 2);
            uint8 strength = (uint8)frontEnd.shockImpact[player];
            maxshakex = (sint16)((strength >> 5) + 2);
            maxshakey = (sint16)((strength >> 6) + 2);
            if (fShakingItem != 1)
            {
                fShakingItem = 1;
                fResetShakeTimeOut = 1;
            }
        }
        else
        {
            fShakingItem = -1;
            ClearActuators();
        }
    }
    ShakeIt();
    if (maxshakex != 0 || maxshakey != 0)
    {
        sint32 random = randPSX();
        if (maxshakex == 0)
            abort();
        shakex = (sint16)(random % maxshakex - (maxshakex >> 1));
        random = randPSX();
        if (maxshakey == 0)
            abort();
        shakey = (sint16)(random % maxshakey - (maxshakey >> 1));
    }
    IsShapeFileLoaded(fSwapShapes);
    if (fSwapShapes.fFile != 0)
    {
        UploadSwapShapes(66);
        return;
    }
    if ((fSwapShapes.fFlags & 1) == 0)
    {
        fGotTick = 0;
        return;
    }
    if (((uint8)fCurrentController == 5 || (uint8)fCurrentController == 6) &&
        fAnim == 0 && fFade[0] == 0 && fFade[1] == 0 && fAnimFade == 0)
    {
        flare_intensity = (sint32)(uint32(flare_intensity) + 250U);
        if (flare_intensity >= max_fi)
        {
            max_fi = (sint32)(uint32(max_fi) + uint32(max_fidir));
            if (max_fi > 4750)
            {
                max_fi = 4750;
                max_fidir = -55;
            }
            if (max_fi < 3750)
            {
                max_fi = 3750;
                max_fidir = 55;
            }
            flare_intensity = max_fi;
        }
    }
    else
    {
        max_fi = 3750;
        flare_intensity = (sint32)(uint32(flare_intensity) - 250U);
        if (flare_intensity < 0)
            flare_intensity = 0;
    }
    if (flare_intensity > 0)
    {
        for (sint32 ii = 0; ii < 2; ++ii)
            Flare::_2DHalo(Offset[(uint8)fCurrentController][0] + shakex + 126,
                Offset[(uint8)fCurrentController][1] + ii + 63 + shakey,
                flare_intensity, flare_intensity / 4, 21);
    }
    if (fFade[0] != 0 && fAnimFade == 0)
    {
        sint16 controller = fFadeController[0];
        fFade[0] = 0;
        fAnimFade = -1;
        fAnimFadeController = controller;
        fAnimFadeStart = AnimKeyPoints(false, true);
        fAnimFadeStop = AnimKeyPoints(false, false);
        fAnimFadeFrame = fAnimFadeStart;
    }
    else
    {
        bool swapPending = false;
        if (fFade[1] != 0 && (fAnimFade == 0 || (fAnimFade == 1 && fFade[0] == 0)))
        {
            bool restartClock = fAnimFade == 1;
            sint16 controller = fFadeController[1];
            fFade[1] = 0;
            fAnimFade = 1;
            fAnimFadeController = controller;
            fAnimFadeStart = AnimKeyPoints(true, true);
            fAnimFadeStop = AnimKeyPoints(true, false);
            fAnimFadeFrame = fAnimFadeStart;
            if (fSwap != 0)
            {
                SwapInController();
                if (restartClock)
                    fGotTick = 0;
                fSwap = 0;
            }
        }
        else if (fFade[0] != 0)
        {
            fFade[0] = 0;
            swapPending = true;
        }
        else if (fFade[1] != 0 && fAnimFade == 1)
        {
            fFade[1] = 0;
            swapPending = true;
        }
        if (swapPending && fSwap != 0)
        {
            SwapInController();
            fSwap = 0;
        }
    }
    if (fAnimFade != 0)
    {
        if (fGotTick == 0)
        {
            sint32 now = ticks;
            fGotTick = 1;
            fPlayedInSound = 0;
            fStartTick = now;
        }
        if ((sint32)(uint32(ticks) - uint32(fStartTick)) > 10 && fPlayedInSound == 0 && fAnimFade == 1)
        {
            AudioCmn::PlayFESFX(15);
            fPlayedInSound = 1;
        }
        sint32 step = (sint32)(uint32(ticks) - uint32(fStartTick)) / 6;
        fAnimFadeFrame = (sint16)(uint32((uint16)fAnimFadeStart) + uint32(step) * uint32(sint32(fAnimFade)));
        if ((fAnimFadeFrame > fAnimFadeStop && fAnimFade == 1) ||
            (fAnimFadeFrame < fAnimFadeStop && fAnimFade == -1))
        {
            fGotTick = 0;
            fAnimFadeFrame = fAnimFadeStop;
            if (fAnimFade == 1)
            {
                fTextTypeOn = 0;
                fArrowFade = 128;
            }
        }
        sint32 numerator = (sint32(fAnimFadeFrame) - fAnimFadeStart) * 128;
        sint32 denominator = sint32(fAnimFadeStop) - fAnimFadeStart;
        if (denominator == 0)
            abort();
        sint32 fadelevel = numerator / denominator;
        if (fadelevel < 0)
            fadelevel = -fadelevel;
        fadelevel *= SuperFastFadeOut != 0 ? 4 : 2;
        if (fadelevel > 128)
            fadelevel = 128;
        if (fAnimFade == 1)
            fadelevel = 128 - fadelevel;
        if (fGotTick == 0)
            fAnimFade = 0;
        sint32 frame = CalcAnimFrame(fAnimFadeFrame);
        ActualDrawController(frame, fadelevel, fadelevel, 0, 0);
        return;
    }
    if (fAnim != 0)
    {
        if (fGotTick == 0)
        {
            AudioCmn::PlayFESFX(15);
            sint32 now = ticks;
            fGotTick = 1;
            fStartTick = now;
        }
        sint32 step = (sint32)(uint32(ticks) - uint32(fStartTick)) / 6;
        fAnimFrame = (sint16)(uint32((uint16)fAnimStart) + uint32(step) * uint32(sint32(fAnimStep)));
        if ((fAnimFrame > fAnimStop && fAnimStep == 1) || (fAnimFrame < fAnimStop && fAnimStep == -1))
        {
            fAnim = 0;
            fGotTick = 0;
            fAnimFrame = fAnimStop;
        }
        float elapsed = float((sint32)(uint32(ticks) - uint32(fStartTick)));
        sint32 duration = (sint32)(uint32(sint32(fAnimStop) - fAnimStart) * uint32(sint32(fAnimStep)) * 6U);
        float fraction = elapsed / float(duration);
        sint32 fadelevel = sint32(double(fraction) * 256.0);
        if (fadelevel > 256)
            fadelevel = 256;
        if (fadelevel > 128)
            fadelevel = 256 - fadelevel;
        fadelevel = (sint32)(uint32(fadelevel) + 10U);
        if (fadelevel < 0)
            fadelevel = 0;
        if (fadelevel > 128)
            fadelevel = 128;
        sint32 frame = CalcAnimFrame(fAnimFrame);
        ActualDrawController(frame, 0, fadelevel, 0, 0);
        return;
    }
    if ((uint8)fCurrentController == 5 || (uint8)fCurrentController == 6)
    {
        sint32 frame = 0;
        if (menuDefs->itemControllerSteeringRange2.fActive != 0 || menuDefs->itemControllerDeadSpot2.fActive != 0)
        {
            PAD_COMMON *pad = &gPadinfo.buf[player * 4];
            sint32 range2 = 128 - pad->data.analog.lefty;
            sint32 range1 = 128 - pad->data.analog.leftx;
            if (abs(range2) < abs(range1))
                range2 = range1;
            sint16 ret = (sint16)range2;
            frame = ret < 0 ? -sint32(ret) / 13 + 36 : sint32(ret) / 13 + 26;
        }
        if (menuDefs->itemControllerSteeringRange1.fActive != 0 || menuDefs->itemControllerDeadSpot1.fActive != 0)
        {
            PAD_COMMON *pad = &gPadinfo.buf[player * 4];
            sint32 range2 = 128 - pad->data.analog.righty;
            sint32 range1 = 128 - pad->data.analog.rightx;
            if (abs(range2) < abs(range1))
                range2 = range1;
            sint16 ret = (sint16)range2;
            frame = ret < 0 ? -sint32(ret) / 13 + 56 : sint32(ret) / 13 + 46;
        }
        uint8 index = (uint8)frame;
        if (index != 0)
            DrawShapeExtended(index, 0x600, Offset[CurrentlyLoadedArt][0], Offset[CurrentlyLoadedArt][1], 0, 0, &drawFlags);
    }
    uint8 controller = (uint8)fCurrentController;
    if ((controller == 1 || controller == 2) &&
        (menuDefs->itemControllerJoyRange.fActive != 0 || menuDefs->itemControllerCenterPoint.fActive != 0))
    {
        sint32 range = sint32(gPadinfo.buf[player * 4].data.analog.rightx) - 128;
        sint32 frame;
        if (range > 10)
            frame = controller == 2 ? 2 + range * 13 / 129 : 26 + range * 8 / 129;
        else if (range < -10)
        {
            range = -range;
            frame = controller == 2 ? 16 + range * 13 / 129 : 35 + range * 8 / 129;
        }
        else
            frame = controller == 2 ? 1 : 0;
        ActualDrawController(frame, 0, 0, 0, 0);
    }
    else if ((uint8)fCurrentController != 0)
        ActualDrawController(0, 0, 0, shakex, shakey);
    else
    {
        fTextController = 0;
        fArrowFade = 128;
        fTextTypeOn = 0;
    }
}

void tScreenControllerConfig::HorzVertLine(sint16 *ArrowLoc, bool type)
{
    FUNCTION_MARKER(0x80044AF4, "FRONT.BIN");
    sint32 startx = ArrowLoc[2];
    sint32 starty = ArrowLoc[3];
    sint32 endx = ArrowLoc[4];
    sint32 endy = ArrowLoc[5];
    if (type)
    {
        endx = startx;
        endy = starty;
        startx = ArrowLoc[0];
        starty = ArrowLoc[1];
    }
    sint32 LineCol = 0x1E1E1E;
    if (fArrowFadeDir > 0)
        LineCol = CalcFadeVal(0x1E1E1E, fArrowFade);
    if (startx == endx)
    {
        sint32 extrabitoff = !type && endy >= starty;
        sint32 extrabit = type && endy >= starty;
        sint16 length = (sint16)(endy - starty);
        if (mult != 64)
        {
            length = (sint16)((sint32(length) * mult) >> 6);
            sint32 brightlength = sint32(length) >> 3;
            PSXDrawTransSquare(0xFFFFFF, endx,
                endy - (sint32(length) - brightlength - extrabitoff + extrabit), 2, -brightlength, 2);
        }
        PSXDrawTransSquare(LineCol, endx, endy + extrabit, 2, -(sint32(length) - extrabitoff + extrabit), 2);
    }
    else
    {
        sint32 extrabit = endx >= startx ? 2 : 0;
        sint16 length = (sint16)(endx - startx);
        if (mult != 64)
        {
            length = (sint16)((sint32(length) * mult) >> 6);
            sint32 absolute = length < 0 ? -sint32(length) : sint32(length);
            if (absolute <= extrabit)
                return;
            sint32 brightlength = sint32(length) >> 3;
            PSXDrawTransSquare(0xFFFFFF, endx - (sint32(length) - brightlength) + extrabit,
                starty, -(brightlength + extrabit), 1, 2);
        }
        sint32 absolute = length < 0 ? -sint32(length) : sint32(length);
        if (absolute <= extrabit)
            return;
        PSXDrawTransSquare(LineCol, endx + extrabit, endy, -(sint32(length) + extrabit), 1, 2);
    }
}

void tScreenControllerConfig::DrawArrow(sint16 *ArrowLoc)
{
    FUNCTION_MARKER(0x80044D40, "FRONT.BIN");
    mult = 0;
    settrans(1);
    FeDraw_SetABRMode(0);
    if (ArrowLoc[0] != 0)
    {
        if (fArrowFadeDir < 0)
        {
            mult = (sint16)(64U - (uint16)fArrowFade);
            if (mult < 0)
                mult = 0;
        }
        else
            mult = 64;
        HorzVertLine(ArrowLoc, true);
    }
    sint16 next = 64;
    if (fArrowFadeDir < 0)
    {
        mult = (sint16)(128U - (uint16)fArrowFade);
        next = mult;
        if (next > 64)
            next = 64;
        else if (next < 0)
            next = 0;
    }
    mult = next;
    HorzVertLine(ArrowLoc, false);
    FeDraw_SetABRMode(1);
    settrans(0);
}

void tScreenControllerConfig::DrawBackground(void)
{
    FUNCTION_MARKER(0x80044E40, "FRONT.BIN");
    sint16 fade = (sint16)(menuDefs->menuControllerConfig.fScreenFade >> 1);
    if (fade > 128)
        fade = 128;
    if (fTransitionedIn == 0 && menuDefs->menuControllerConfig.tOptionsMenu::TransitionIsFinished())
        fTransitionedIn = 1;
    if (fade < 129 && fTransitioningIn == 0 && (uint8)fCurrentController != 0)
    {
        fAnimFade = 1;
        fAnimFadeStart = AnimKeyPoints(true, true);
        fAnimFadeStop = AnimKeyPoints(true, false);
        sint16 start = fAnimFadeStart;
        uint8 controller = (uint8)fCurrentController;
        fTransitioningIn = 1;
        fAnimFadeFrame = start;
        fAnimFadeController = controller;
    }
    if (!menuDefs->menuControllerConfig.tOptionsMenu::TransitionIsFinished() && fTransitionedIn != 0)
    {
        if (fTransitioningOut != 0)
            goto drawBackground;
        if ((uint8)fCurrentController != 0)
        {
            fAnimFade = -1;
            fAnimFadeStart = AnimKeyPoints(false, true);
            fAnimFadeStop = AnimKeyPoints(false, false);
            sint16 start = fAnimFadeStart;
            uint8 controller = (uint8)fCurrentController;
            fTransitioningOut = 1;
            SuperFastFadeOut = 1;
            fAnimFadeFrame = start;
            fAnimFadeController = controller;
        }
    }
    if (fTransitioningOut == 0)
    {
        SetCurrentController(false);
        CheckConfigs();
    }
drawBackground:
    if ((fAnimFade != 0 || fTransitioningOut == 0) && fTransitioningIn != 0)
        DrawController();
    for (sint32 i = 0; i < 12; ++i)
        ScaleShapeExtended(i + 10, 0, 0, 0, fade, 0, 0);
}

void tScreenControllerConfig::DrawForeground(void)
{
    FUNCTION_MARKER(0x80045064, "FRONT.BIN");
    SetCurrentController(false);
    CheckConfigs();
    sint16 TL[5];
    char string1[40];
    char string2[40];
    char brightstring[2];
    brightstring[1] = 0;
    sint16 direction = 8;
    if (fFadeTextOut == 0 && fAnim == 0 && fAnimFade == 0 && fFade[0] == 0 && fFade[1] == 0 &&
        menuDefs->menuControllerConfig.tOptionsMenu::TransitionIsFinished() && fTransitioningOut == 0)
    {
        direction = -8;
        if (fArrowFadeDir > 0)
            fTextTypeOn = 0;
    }
    fArrowFadeDir = direction;
    fArrowFade = (sint16)((uint16)fArrowFade + (uint16)fArrowFadeDir);
    if (fArrowFade < 0)
        fArrowFade = 0;
    if (fArrowFade >= 128)
    {
        fArrowFade = 128;
        fTextController = fCurrentController;
        if ((uint8)fCurrentController == 6)
            fTextController = 5;
        char config = frontEnd.controlConfig[player];
        fFadeTextOut = 0;
        fTextConfig = config;
        return;
    }
    if (fArrowFade == 0)
        fTextTypeOn = (sint16)((uint16)fTextTypeOn + 1);
    if (fTextTypeOn > 100)
        fTextTypeOn = 100;
    if (fArrowFade != 0)
        FETextRender::SetABR(1, true);
    if ((uint8)fTextController != 0)
    {
        sint32 ColText = kRGBVals[(uint8)textDefinitions[11][3]];
        sint32 ColText2 = CalcFadeVal(ColText, 0xFFFFFF, 100, fArrowFade);
        ColText = CalcFadeVal(ColText, fArrowFade);
        for (sint16 i = 0; i < ControllerTextCount((uint8)fTextController, (uint8)fTextConfig); ++i)
        {
            sint32 TextIndex = (sint8)ControllerItemIndex[(uint8)fTextController - 1][(uint8)fTextConfig][i][0];
            if (TextIndex == -1)
                continue;
            for (sint16 j = 0; j < 5; ++j)
                TL[j] = TextLocations[TextIndex][j];
            char *original = TextSys::Word(TL[2]);
            sint32 length = (sint32)strlen(original);
            sint16 sl = (sint16)length;
            sint16 k = fTextTypeOn;
            if (sl < fTextTypeOn)
                k = (sint16)length;
            char *astringpointer;
            if (fTextTypeOn < sl)
            {
                sprintf(string2, TextSys::Word(TL[2]));
                if (TL[3] == 0)
                {
                    sint16 j = 0;
                    if (fTextTypeOn > 0)
                    {
                        do
                        {
                            if (j >= (sint16)strlen(string2))
                                break;
                            string1[j] = string2[j];
                            j = (sint16)((uint16)j + 1);
                        } while (j < fTextTypeOn);
                    }
                    string1[j] = 0;
                    brightstring[0] = string1[j - 1];
                }
                else
                {
                    sint16 j = (sint16)(length - k);
                    while (j < sl)
                    {
                        string1[sint32(j) - (sint32(sl) - k)] = string2[j];
                        j = (sint16)((uint16)j + 1);
                    }
                    string1[sint32(j) - (sint32(sl) - k)] = 0;
                    brightstring[0] = string1[0];
                }
                FETextRender::SetFont(0);
                sint32 brightX = TL[0];
                sint32 advance = textpixels(string1);
                advance = (sint32)(uint32(advance) - uint32(strlen(string1)));
                brightX = TL[3] == 1 ? (sint32)(uint32(brightX) - uint32(advance)) :
                    (sint32)(uint32(brightX) + uint32(advance));
                FETextRender::FullTextRGB(brightstring, (sint16)brightX, TL[1], ColText2, 0, TL[3] == 0);
                astringpointer = string1;
            }
            else
                astringpointer = TextSys::Word(TL[2]);
            FETextRender::FullTextRGB(astringpointer, TL[0], TL[1], ColText, 0, TL[3]);
            sint32 width = textpixels(astringpointer);
            width = (sint32)(uint32(width) - uint32(strlen(astringpointer)));
            if (TL[3] != 0)
                width = (sint32)(0U - uint32(width));
            PSXDrawSquare(0, TL[0], TL[1], width, 7);
        }
    }
    for (sint16 i = 0; i < ControllerTextCount((uint8)fTextController, (uint8)fTextConfig); ++i)
    {
        sint32 index = (uint8)fTextController - 1;
        bool freeDialog = false;
        if (negconPopUp.currentlyOn == 0)
            freeDialog = negconPopUp.fCurrentlyRunning == 0;
        if (freeDialog && index >= 0)
        {
            sint32 arrow = (sint8)ControllerItemIndex[index][(uint8)fTextConfig][i][1];
            if (arrow != -1)
                DrawArrow(ArrowLocations[arrow]);
        }
    }
    FETextRender::SetABR(0, false);
    sint16 fade = (sint16)(menuDefs->menuControllerConfig.fScreenFade >> 1);
    if (fade > 128)
        fade = 128;
    for (sint16 i = 0; i < 2; ++i)
        ScaleShapeExtended(i + 22, 0, 0, 0, fade, 0, 0);
}

void tScreenControllerConfig::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x800456CC, "FRONT.BIN");
    numPermShapes = 24;
    *permFileName = const_cast<char *>("zpCtrl");
    numSwapShapes = 66;
    *swapFileName = const_cast<char *>("aCtrl");
}

void tScreenControllerConfig::Initialize(void)
{
    FUNCTION_MARKER(0x800456FC, "FRONT.BIN");
    fShaker.active = 0;
    TurnOffShakers();
    ClearActuators();
    fResetShakeTimeOut = 1;
    tFEApplication *application = FEApp;
    fShakingItem = -1;
    uint8 inputPlayer = (uint8)application->fInputPlayer;
    fTimeOutStartTick = 0;
    CurrentlyLoadedArt = -1;
    negconChoice = -1;
    player = inputPlayer;
    tScreen::Initialize();
    fCurrentController = 0;
    menuDefs->itemControllerSettings.SetMenu(true, 0);
    SetCurrentController(true);
    SwapInController();
    sint16 arrowFade = fArrowFade;
    uint8 controller = (uint8)fCurrentController;
    fPrevController = (char)controller;
    if (arrowFade >= 128)
        fTextController = (char)controller;
    if ((uint8)fTextController == 6)
        fTextController = 5;
    char config = frontEnd.controlConfig[player];
    fPrevConfig = config;
    fTextConfig = config;
    CheckConfigs();
    fGotTick = 0;
    fAnim = 0;
    fAnimFade = 0;
    fFade[0] = 0;
    fFade[1] = 0;
    fArrowFade = 128;
    fArrowFadeDir = 0;
    fTextTypeOn = 0;
    fPlayedInSound = 0;
    fTransitionedIn = 0;
    fTransitioningIn = 0;
    fTransitioningOut = 0;
    SuperFastFadeOut = 0;
}

void tScreenControllerConfig::Cleanup(void)
{
    FUNCTION_MARKER(0x80045820, "FRONT.BIN");
    ClearActuators();
    TurnOffShakers();
    PadStartCom();
    tScreen::Cleanup();
}

tScreenControllerConfig::tScreenControllerConfig(void)
{
    FUNCTION_MARKER(0x8004585C, "FRONT.BIN");
    fGotTick = 0;
    fAnim = 0;
    fFade[1] = 0;
    fFade[0] = 0;
    player = 0;
}

sint32 tScreenControllerConfig::GetHelpText(void)
{
    FUNCTION_MARKER(0x800458AC, "FRONT.BIN");
    uint8 controller = (uint8)fCurrentController;
    if (controller == 1)
        return 0x219;
    if (controller == 2)
        return 0x21A;
    if (controller == 5 || controller == 6)
        return 0x218;
    return -1;
}
