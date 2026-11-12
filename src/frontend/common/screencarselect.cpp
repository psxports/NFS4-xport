#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/feapp.h"
#include "frontend/common/feaudio.h"
#include "frontend/common/fecars.h"
#include "frontend/common/fecheats.h"
#include "frontend/common/fefades.h"
#include "frontend/common/fememcard.h"
#include "frontend/common/femenu.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetracks.h"
#include "frontend/common/front.h"
#include "frontend/common/screencarselect.h"
#include "frontend/common/screencongrats.h"
#include "frontend/common/screenpost.h"
#include "frontend/common/screentrackinfo.h"
#include "frontend/psx/drawshp.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiomus.h"
#include "game/common/carobj.h"
#include "game/common/nfs3.h"
#include "game/common/render.h"
#include "game/common/renderview.h"
#include "game/common/textsys.h"
#include "game/psx/drawc.h"
#include "game/psx/fe3dmenu.h"
#include "game/psx/platform.h"
#include "lib/psx/fixddiv.h"
#include "lib/psx/fixdmult.h"

enum tCarStatType
{
    cst_Acceleration = 0,
    cst_Brake = 1,
    cst_Speed = 2,
    cst_Handling = 3,
    cst_Overall = 4,
};

struct tTrackInformation

{
    char fTrackID;

    uint8 fSimNumber;

    uint8 fTrackDifficulty;

    uint8 fAvailable;

    uint8 fIsEgg;

    uint8 fLengthKM;

    uint8 fLengthMiles;

    uint8 fNumMoments;

    char fShapeName[8];

    char fSplineName[8];

    char fCountry;

    char fDispatch;

    char fReverseCall;

    char fLanguage;

    char fTrafficCars[6];

    sint16 fTX;

    sint16 fTY;

    sint16 fSX;

    sint16 fSY;

    uint8 fSpeedoCountry;

    uint8 fPad;

    sint32 fRotate;
};

enum PinkSlipsCarSelectState
{
    NoCardInserted = 0,
    CardFailed = 1,
    CardFailedNotFound = 2,
    CardFailedUnformatted = 3,
    NotEnoughCars = 4,
    TooManyCars = 5,
    CardLoadedFine = 6,
    WhoCaresWeBeExiting = 7,
    CardCurrentlyLoading = 8,
};

static sint16 text2PVals[5] = { 283, 284, 285, 286, 287 };
static char WaitingString[50];
static sint16 textVals[5] = { 278, 279, 280, 281, 282 };
static tCarStatType remap[5] = { cst_Acceleration, cst_Speed, cst_Brake, cst_Handling, cst_Overall };
static char gSwapFileName[2][20];
static sint16 tvOrder[10] = { 2, 7, 4, 8, 1, 6, 0, 9, 3, 5 };
static sint16 tvSplitOrder[5] = { 2, 1, 3, 0, 4 };

static PSX_RECT gOverlayPositions[7][2] =
{
    { { 290, 256, 200, 12 }, { 290, 200, 200, 12 } },
    { { 213, 256, 288, 41 }, { 213, 185, 288, 41 } },
    { { 213, 256, 288, 41 }, { 213, 185, 288, 41 } },
    { { 213, 256, 288, 41 }, { 213, 185, 288, 41 } },
    { { 325, 5, 162, 24 }, { 325, 29, 162, 24 } },
    { { 495, 21, 3, 3 }, { 213, 21, 285, 105 } },
    { { -120, 136, 130, 77 }, { 0, 136, 130, 77 } },
};

PinkSlipsCarSelectState PinkSlipsScreenState[2];
static sint32 gCatmullRom[4][4] = { { -65536, 196608, -196608, 65536 }, { 131072, -327680, 262144, -65536 }, { -65536, 0, 65536, 0 }, { 0, 131072, 0, 0 } };
static sint32 gKnots[8][5] = { { 491520, -524288, 17039360, 131072, 0 }, { 491520, -524288, 17039360, 131072, 0 }, { 196608, -524288, 17039360, 6684672, 11206656 }, { 196608, -524288, 17039360, 6684672, 22347776 }, { 688128, -524288, 17039360, -4718592, 33554432 }, { 196608, -524288, 17039360, 6684672, 44761088 }, { 196608, -524288, 17039360, 6684672, 55902208 }, { 196608, -524288, 17039360, 6684672, 55902208 } };
static char gStateOverlays[8][4] = { { 255, 6, 0, 255 }, { 4, 6, 0, 255 }, { 255, 6, 2, 255 }, { 4, 6, 1, 255 }, { 255, 6, 3, 5 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 } };
static sint32 gRotateOffset[4];
tScreenCarSelect *screenCarSelect;
tScreenCarSelectDuel *screenCarSelectDuel;
tScreenCarSelectTwoPlayer *screenCarSelectTwoPlayer;
tScreenCarSelectTwoPlayer *screenCarSelectPlayerTwo;
tScreenPinkSlipsCarSelect *screenPinkSlipsCarSelectTwoPlayer;
tScreenPinkSlipsCarSelect *screenPinkSlipsCarSelectPlayerTwo;

static void TransformVector(sint32 (&vect)[4], sint32 (&transform)[4][4], sint32 (&result)[4])
{
    FUNCTION_MARKER(0x8003A8F0, "FRONT.BIN");
    for (sint16 I = 0; I < 4; ++I) {
        result[I] = 0;
        for (sint16 J = 0; J < 4; ++J) {
            sint32 product = fixedmult(vect[J], transform[J][I]);
            result[I] = (result[I] + product);
        }
    }
}

void DrawCar(tCarInfo &carInfo, sint16 x, sint16 y, float camerax, float cameray, char brightness, bool reflection, uint32 rotate, tPlayer player)
{
    FUNCTION_MARKER(0x8003A9D4, "FRONT.BIN");
    sint32 ticks = ::ticks;
    DrawC::gMenuLights = 0;
    DrawC::gMenuLightsDirection = (uint32)(ticks % 2048) >= 1024;
    if ((sint8)carInfo.fCarID < 0)
        return;
    sint32 the_simcarcolor = carInfo.fColor;
    gCarObj[player]->carInfo->carType = carInfo.fSimNumber;
    gCarObj[player]->carInfo->Country = carInfo.fCountry;
    gMenuRotate[player] = (gMenuRotate[player] + 3);
    gCarObj[player]->carInfo->EngineMods = (carInfo.fUpgrades >> 2) & 1;
    gCarObj[player]->carInfo->WeightTransfer = (carInfo.fUpgrades >> 1) & 1;
    gCarObj[player]->carInfo->GroundEffects = carInfo.fUpgrades & 1;
    DrawC::MenuColorData(the_simcarcolor, gCarObj[player], player);
    Draw::MenuRenderingView(gCarObj[player], &gCView, x, y, player, 0, rotate, camerax, cameray, (uint8)brightness, reflection);
}

void Front::ResetOverlayCarSelect(void)
{
    gKnots[1][4] = 0;
    memcpy(gKnots[0], gKnots[1], sizeof(gKnots[0]));
    memset(gRotateOffset, 0, sizeof(gRotateOffset));
    PinkSlipsScreenState[0] = PinkSlipsScreenState[1] = NoCardInserted;
}

tScreenCarSelect::tScreenCarSelect(void)
{
    FUNCTION_MARKER(0x8003AB80, "FRONT.BIN");
    fPreviousCar = 0;
    fPreviousCarID = -1;
    fPreviousCountry = 0;
    for (sint16 i = 0; i < 7; ++i) {
        tOverlay *overlay = &fOverlays[i];
        memcpy(&overlay->location[0], &gOverlayPositions[i][0], 8);
        memcpy(&overlay->location[1], &gOverlayPositions[i][1], 8);
        overlay->ID = i;
        overlay->direction = 0;
        overlay->transition = 0;
        overlay->delta = 6;
    }
    for (sint16 i = 0; i < 4; ++i)
        fCurrentOverlays[i] = 0;
}

tScreenCarSelect::~tScreenCarSelect(void)
{
    FUNCTION_MARKER(0x8003AC90, "FRONT.BIN");
}

void tScreenCarSelect::Cleanup(void)
{
    FUNCTION_MARKER(0x8003ACB8, "FRONT.BIN");
    CleanupSpinningCarsMenu();
    tScreen::Cleanup();
    FreeAsyncBuffer();
}

void tScreenCarSelect::DrawOverlay(tOverlay *overlay)
{
    FUNCTION_MARKER(0x8003AD00, "FRONT.BIN");

    if (!overlay)
        return;

    const sint16 upgradeTranslate[3] = { 2, 3, 1 };
    const sint16 upgradeIcons[3] = { 2, 4, 1 };
    tCarInfo carInfo;
    bool validCar = GetCar(carInfo);
    if (!validCar) {
        carInfo.fCarID = (char)-1;
        carInfo.fUpgrades = 0;
    }

    if (overlay->direction != 0) {
        overlay->transition = (sint16)(overlay->transition + overlay->delta * overlay->direction);
        if (overlay->transition <= 0) {
            overlay->transition = 0;
            overlay->direction = 0;
        } else if (overlay->transition >= 128) {
            overlay->transition = 128;
            overlay->direction = 0;
        }
    }

    const PSX_RECT &from = overlay->location[0];
    const PSX_RECT &to = overlay->location[1];
    sint32 transition = overlay->transition;
    PSX_RECT pos = {
        (sint16)(from.x + transition * (to.x - from.x) / 128),
        (sint16)(from.y + transition * (to.y - from.y) / 128),
        (sint16)(from.w + transition * (to.w - from.w) / 128),
        (sint16)(from.h + transition * (to.h - from.h) / 128)
    };

    switch (overlay->ID) {
    case 0:
        if (validCar)
            DrawShape::NFS4RoundRectangle((sint8)carInfo.fCarID + 289, pos, 0);
        break;
    case 1:
    case 2:
    case 3: {
        PSX_RECT temp = { (sint16)(pos.x + 15), (sint16)(pos.y + pos.h - 30), (sint16)(pos.w - 30), 23 };
        sint32 edge = temp.x + temp.w - 12;
        if (validCar) {
            FETextRender::MenuTextPositionedJustify((sint8)carInfo.fCarID + 289, edge, (pos.y + 2), 1, textState_Selected, textType_FramedInfo);
        }

        sint16 text = (sint16)(139 + overlay->ID);
        sint32 value = 0;
        sint32 moneyColor = 0x232323;
        if (validCar) {
            moneyColor = 0xBEBE;
            if (overlay->ID == 2)
                value = carInfo.fPrices[0];
            else if (overlay->ID == 3)
                value = carInfo.fPrices[upgradeTranslate[(sint16)menuDefs->menuCarUpgrades.fCurrentItem]];
            else
                value = carManager.CalcUsedPrice((uint8)frontEnd.sellerCar);
        }

        sint16 center = (sint16)(temp.x + (temp.w >> 1));
        DrawMoney(edge, temp.y + 3, 6, value, moneyColor, 0x232323);
        FETextRender::MenuTextPositionedJustify(text, center, (temp.y + 3), 1, textState_Selected, textType_FramedInfo);
        DrawMoney(edge, temp.y + 13, 9, tournamentManager.fMoney, 0xBEBE, 0x232323);
        FETextRender::MenuTextPositionedJustify(123, center, (temp.y + 13), 1, textState_Selected, textType_FramedInfo);
        DrawShape::NFS4Rectangle(temp);
        PSXDrawSquare(0, pos.x, pos.y, pos.w, 10);
        break;
    }
    case 4:
    case 5: {
        bool description = overlay->ID == 5;
        if (!description || transition == 128) {
            for (sint32 i = 0; i < 3; ++i) {
                bool owned = (carInfo.fUpgrades & upgradeIcons[i]) != 0;
                tDrawShapeExtended drawFlags;
                drawFlags.tint[0] = 0xBEBE;
                sint32 frame = ((sint32)ticks >> 4) % 10;
                DrawShapeExtended(i * 10 + frame + 98, owned ? 0x410 : 0x411, pos.x + i * 40 + (description ? 133 : 33), pos.y + 6, owned ? 0 : 96, 1, &drawFlags);
            }
            if (description) {
                PSX_RECT temp = { (sint16)(pos.x + 30), (sint16)(pos.y + 35),
                    (sint16)(pos.w - 60), (sint16)(pos.h - 75) };
                FETextRender::MenuTextPositionedJustify((menuDefs->menuCarUpgrades.fCurrentItem + 150), (pos.x + (pos.w >> 1)), (pos.y + 24), 2, textState_Hilighted, textType_FramedInfo);
                sint32 descrItem = (sint16)menuDefs->menuCarUpgrades.fCurrentItem + 175;
                if (descrItem == 176 && (carInfo.fCarID == 12 || carInfo.fCarID == 10))
                    descrItem = 65;
                FETextRender::WordWrap(descrItem, temp, textState_Hilighted, textType_PopUpText);

                sint32 item = (sint16)menuDefs->menuCarUpgrades.fCurrentItem;
                sint16 text = 160;
                if (!(carInfo.fUpgrades & upgradeIcons[item]))
                    text = gPadinfo.buf[0].ID == 0x23 ? 159 : 158;
                FETextRender::MenuTextPositionedJustify(text, (pos.x + pos.w - 15), (pos.y + pos.h - 20), 1, textState_Hilighted, textType_FramedInfo);
            }
        }
        if (description && pos.w >= 67 && pos.h >= 51) {
            PSX_RECT temp = { (sint16)(pos.x + 15), (sint16)(pos.y + 20),
                (sint16)(pos.w - 30), (sint16)(pos.h - 25) };
            DrawShape::NFS4Rectangle(temp);
        }
        break;
    }
    case 6:
        if (!validCar)
            memset(carInfo.fStats, 0, sizeof(carInfo.fStats));
        DrawSliders(carInfo, (sint16)(pos.x + 13), (sint16)(pos.y + 4));
        break;
    }

    if (overlay->ID != 0)
        DrawShape::NFS4TransRectangle(pos, 1);
}

void tScreenCarSelect::SetState(sint32 state)
{
    FUNCTION_MARKER(0x8003B59C, "FRONT.BIN");
    sint16 fPreviousState = fState;
    if (state != fPreviousState) {
        fState = (sint16)state;
        if ((uint16)(fPreviousState - 2) < 2) {
            fVideoWall[0].TurnOff();
            SetBrightness(0, 0);
            fPreviousCar = -1;
            fPreviousCarID = -1;
        }
        for (sint16 i = 0; i < 4; ++i) {
            tOverlay *overlay = fCurrentOverlays[i];
            sint32 nextOverlay = (sint8)gStateOverlays[state][i];
            if (overlay != 0) {
                if (overlay->ID != nextOverlay)
                    overlay->direction = -1;
            } else if (nextOverlay >= 0) {
                fCurrentOverlays[i] = &fOverlays[nextOverlay];
                fOverlays[nextOverlay].transition = 0;
                fCurrentOverlays[i]->direction = 1;
            }
        }
    }
    if (state != 0 && state != 2 && state != 5 && state != 6)
        return;
    fInShowroom = state - 5U < 2;
    uint32 speechTicks = ticks;
    uint32 showroomTicks = ticks;
    gStopCommentaryNow = 1;
    fSpeechPlayed = false;
    fSpeechTicks = speechTicks;
    fShowroomTicks = showroomTicks;
    if (!fInShowroom) {
        for (sint16 i = 0; i < 10; ++i) {
            tvConfigs[i].state = tv_StateOff;
            tvConfigs[i].transition = 0;
        }
        if (fPreviousState != 1)
            tScreen::TransitionOn(tScreen::kScreen_TransitionTypeScreen, 0);
        fVideoWall[0].TurnOn();
    } else {
        AudioMus::StopSong(1000);
        fSplineInterval = 0;
        gKnots[1][4] = fCameraRotation & 0x3FF;
        for (sint16 i = 0; i < 5; ++i)
            gKnots[0][i] = ((gKnots[1][i] * 2) - gKnots[2][i]);
        gRotateOffset[3] = 0x10000;
        gRotateOffset[2] = 0x10000;
        gRotateOffset[1] = 0x10000;
        gRotateOffset[0] = 0x10000;
        tScreen::TransitionOff(tScreen::kScreen_TransitionTypeScreen, 0);
    }
}

void tScreenCarSelect::CalcSplinePosition(sint32 knot1, sint32 knot2, sint32 knot3, sint32 knot4, uint32 elapsed, sint32 &camY, sint32 &camZ, sint32 &screenX, sint32 &screenY, sint32 &camRot)
{
    FUNCTION_MARKER(0x8003B820, "FRONT.BIN");
    sint32 T[4];
    sint32 G[4][4];
    sint32 Result1[4];
    sint32 Result2[4];
    T[2] = fixeddiv((elapsed << 16), 0x2580000);
    T[1] = fixedmult(T[2], T[2]);
    T[0] = fixedmult(T[1], T[2]);
    T[3] = 0x10000;
    for (sint16 i = 0; i < 4; ++i) {
        G[0][i] = gKnots[knot1][i];
        G[1][i] = gKnots[knot2][i];
        G[2][i] = gKnots[knot3][i];
        G[3][i] = gKnots[knot4][i];
    }
    TransformVector(T, gCatmullRom, Result1);
    TransformVector(Result1, G, Result2);
    camY = Result2[0] >> 1;
    camZ = Result2[1] >> 1;
    screenX = (Result2[2] >> 1) / 65536;
    screenY = (Result2[3] >> 1) / 65536;
    G[0][0] = (gKnots[knot1][4] + gRotateOffset[0]);
    G[1][0] = (gKnots[knot2][4] + gRotateOffset[1]);
    G[2][0] = (gKnots[knot3][4] + gRotateOffset[2]);
    G[3][0] = (gKnots[knot4][4] + gRotateOffset[3]);
    TransformVector(T, gCatmullRom, Result1);
    TransformVector(Result1, G, Result2);
    camRot = (Result2[0] >> 1) / 65536;
}

void tScreenCarSelect::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x8003BAE0, "FRONT.BIN");
    numPermShapes = 142;
    numSwapShapes = 11;
    *permFileName = const_cast<char *>("zcars");
    tCarInfo carInfo;
    if (!GetCar(carInfo))
        carManager.GetStockCar(0, carInfo);
    fPreviousCar = carInfo.fCarIndex;
    fPreviousCarID = (sint8)carInfo.fCarID;
    fPreviousCountry = carInfo.fCountry;
    sprintf(gSwapFileName[0], "%s", carInfo.fShapeName);
    *swapFileName = gSwapFileName[0];
}

void tScreenCarSelect::UpdateVideoWall(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x8003BBA4, "FRONT.BIN");
    if (carInfo.fCarIndex != fPreviousCar || (sint8)carInfo.fCarID != fPreviousCarID || (carInfo.fCarClass == 7 && fPreviousCountry != carInfo.fCountry)) {
        if ((sint8)carInfo.fCarID >= 0)
            AsyncLoadSwapShapeFile(carInfo.fShapeName);
        fPreviousCar = carInfo.fCarIndex;
        fPreviousCarID = (sint8)carInfo.fCarID;
        uint8 country = carInfo.fCountry;
        fTVsInitialized = 0;
        fPreviousCountry = country;
        SetBrightness(0, 0);
        fVideoWall[0].TurnOff();
    }
}

void tScreenCarSelect::AllocateAsyncBuffer(void)
{
    FUNCTION_MARKER(0x8003BC74, "FRONT.BIN");
    fSwapShapes.fDestFile = Platform::GetDCTBuffer(40000, const_cast<char *>("VideoWall"));
}

void tScreenCarSelect::FreeAsyncBuffer(void)
{
    FUNCTION_MARKER(0x8003BCA8, "FRONT.BIN");
    Platform::ResetDCTBuffer();
    fSwapShapes.fDestFile = 0;
}

void tScreenCarSelect::InitializeVideoWall(void)
{
    FUNCTION_MARKER(0x8003BCD0, "FRONT.BIN");
    fVideoWall[0].Initialize(tvConfigs, fSwapShapes.fShapes, 0, 10, tvOrder, 150);
    fVideoWall[0].SetAvailableText(248, 320, 80);
    fVideoWall[0].SetAvailableIcon(28, 10, 310, 60, fPermShapes.fShapes);
    if ((fSwapShapes.fFlags & 1) != 0) {
        fVideoWall[0].UpdateImages();
        fTVsInitialized = 1;
    }
}

void tScreenCarSelect::Initialize(void)
{
    FUNCTION_MARKER(0x8003BD88, "FRONT.BIN");
    tTrackInformation trackInfo;
    uint8 simNumber;
    if (frontEnd.raceType == 2) {
        tTrackInfo tourneyTrack;
        tournamentManager.GetTrackToRace(tourneyTrack);
        simNumber = trackManager.GetTrackByID((sint8)tourneyTrack.fTrackNumber)->fSimNumber;
    } else {
        trackManager.GetTrack((uint8)frontEnd.track[(uint8)frontEnd.pinkSlipsTrackIndex], trackInfo);
        simNumber = trackInfo.fSimNumber;
    }
    GameSetup::gData.track = simNumber;
    tGlobalMenuDefs *menus = menuDefs;
    gShowroomLights = 1;
    menus->itemDamage.fFlags &= ~1U;
    if (frontEnd.raceType == 2)
        menus->itemDamage.fFlags |= 1;
    tScreen::Initialize();
    AllocateAsyncBuffer();
    SetLicensePlate();
    fTVsInitialized = 0;
    fCameraRotation = 0;
    fInShowroom = false;
    tCarInfo carInfo;
    if (GetCar(carInfo)) {
        fPreviousCar = carInfo.fCarIndex;
        fPreviousCarID = (sint8)carInfo.fCarID;
        fPreviousCountry = carInfo.fCountry;
    } else {
        fPreviousCar = -1;
        fPreviousCountry = -1;
        fPreviousCarID = -1;
    }
    uint32 showroomTicks = ticks;
    sint32 fadeTicks = (sint32)((uint32)ticks - 256);
    fBrightness[1] = 0;
    fBrightness[0] = 0;
    fDestBrightness[1] = 0;
    fDestBrightness[0] = 0;
    fShowroomTicks = showroomTicks;
    fFadeTicks[1] = fadeTicks;
    fFadeTicks[0] = fadeTicks;
    InitializeVideoWall();
    for (sint16 i = 0; i < 7; ++i) {
        fOverlays[i].transition = 0;
        fOverlays[i].direction = 0;
    }
    for (sint16 i = 0; i < 4; ++i)
        fCurrentOverlays[i] = 0;
}

void tScreenCarSelect::ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x8003BF90, "FRONT.BIN");
    if (keyval == kInput_KeyType_Square) {
        tCarInfo carInfo;
        bool validCar = GetCar(carInfo);
        tMenuItem *item = (uint8)FEApp->fPlayer == 0 ? &menuDefs->itemABS : &menuDefs->itemABS2;
        item->fTextDescription = 267;
        if (validCar) {
            if ((sint8)carInfo.fCarID == 8)
                item->fTextDescription = 268;
            if ((sint8)carInfo.fCarID == 1)
                item->fTextDescription = 269;
        }
        if ((uint8)frontEnd.oppNumber == 1 || (uint8)frontEnd.gameMode == 1)
            menuDefs->itemOpponentUpgrades.fFlags |= 1;
    }
    if (keyval != kInput_KeyType_Triangle)
        return;
    sint16 state = fState;
    if (state == 5)
        SetState(0);
    else if (state == 6)
        SetState(2);
    else if (state >= 2 && state < 5 && (uint8)frontEnd.gameMode != 1)
        SetState(1);
}

void tScreenCarSelect::DrawVideoWall(sint16 y)
{
    FUNCTION_MARKER(0x8003C118, "FRONT.BIN");
    tCarInfo carInfo;
    bool validCar = GetCar(carInfo);
    DrawBackgroundImage(0, 28, fPermShapes.fShapes, 150);
    DrawOverlay(fCurrentOverlays[0]);
    if ((fSwapShapes.fFlags & 1) != 0 && fTVsInitialized == 0)
    {
        fVideoWall[0].UpdateImages();
        fTVsInitialized = 1;
    }
    if (fScreenFadeVal > 0 && fTransitionOff)
    {
        fVideoWall[0].TurnOffInstant();
        SetBrightness(0, 0);
    }
    tVideoWall *wall = &fVideoWall[0];
    wall->UpdateTransition();
    wall->SetValid(validCar);
    if (validCar)
        wall->SetAvailable(carInfo.fAvailable);
    wall->Draw();
}

bool tScreenCarSelect::GetCar(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x8003C224, "FRONT.BIN");
    switch (fState) {
    case 0:
    case 5:
        carManager.GetStockCar((uint8)frontEnd.playerCar[0], carInfo);
        if ((uint8)frontEnd.playerCar[0] < (sint32)carManager.fNumCars)
            carInfo.fColor = frontEnd.carColors[0][(sint8)carInfo.fCarID];
        carInfo.fCountry = frontEnd.carCountry[0][(sint8)carInfo.fCarID];
        break;
    case 2:
    case 6:
    {
        carManager.GetStockCar((uint8)frontEnd.dealerCar, carInfo);
        uint8 color = frontEnd.carColors[0][(sint8)carInfo.fCarID];
        carInfo.fAvailable = 1;
        carInfo.fColor = color;
        break;
    }
    case 7:
    {
        if (frontEnd.carListType == 1 && carManager.GetNumOwnedCars(0) <= 0)
            return false;
        uint8 carNumber = frontEnd.carListType == 0 ? frontEnd.playerCar[0] : frontEnd.garageCar[0];
        carManager.GetStockCar(carNumber, carInfo);
        if (frontEnd.carListType == 0)
            carInfo.fColor = frontEnd.carColors[0][(sint8)carInfo.fCarID];
        carInfo.fCountry = frontEnd.carCountry[0][(sint8)carInfo.fCarID];
        break;
    }
    default:
        if (carManager.GetNumOwnedCars(0) <= 0 && frontEnd.raceType != 1)
            return false;
        if (frontEnd.raceType == 2 && fState != 3 && carManager.GetNumTourneyCars(0) <= 0)
            return false;
        uint8 carNumber = fState == 3 ? frontEnd.sellerCar : frontEnd.garageCar[0];
        carManager.GetStockCar(carNumber, carInfo);
        carInfo.fCountry = frontEnd.carCountry[0][(sint8)carInfo.fCarID];
        break;
    }
    carInfo.fColor = carInfo.fColorOrder[carInfo.fColor];
    return true;
}

void tScreenCarSelect::SetBrightness(sint16 bright, sint16 i)
{
    FUNCTION_MARKER(0x8003C4A4, "FRONT.BIN");
    fDestBrightness[i] = bright;
}

void tScreenCarSelect::UpdateBrightness(sint16 i)
{
    FUNCTION_MARKER(0x8003C4B8, "FRONT.BIN");
    sint16 target = fDestBrightness[i];
    sint16 current = fBrightness[i];
    if (current < target) {
        fBrightness[i] = (sint16)(current + 8);
        if (target < fBrightness[i])
            fBrightness[i] = fDestBrightness[i];
    } else if (target < current) {
        fBrightness[i] = (sint16)(current - 8);
        if (fBrightness[i] < fDestBrightness[i])
            fBrightness[i] = fDestBrightness[i];
    }
}

void tScreenCarSelect::DrawBackground(void)
{
    FUNCTION_MARKER(0x8003C53C, "FRONT.BIN");
    tCarInfo carInfo;
    if (GetCar(carInfo)) {
        IsShapeFileLoaded(fSwapShapes);
        if (fSwapShapes.fFile != 0 && fVideoWall[0].fTransitionDirection != -1 && gCarObj[(uint8)FEApp->fPlayer]->async_handle == 0 && (sint32)((uint32)ticks - fFadeTicks[0]) > 128) {
            UploadSwapShapes(11);
            fVideoWall[0].TurnOn();
            if (fBrightness[0] == fDestBrightness[0])
                SetBrightness(carInfo.fAvailable != 0 ? 128 : 32, 0);
        }
    }
    if (fScreenFadeVal < 128)
        DrawVideoWall(0);
}

void tScreenCarSelect::DrawSliders(tCarInfo &carInfo, sint16 x, sint16 y)
{
    FUNCTION_MARKER(0x8003C66C, "FRONT.BIN");
    for (sint16 j = 0; j < 5; ++j) {
        FETextRender::MenuTextPositioned(textVals[j], x, (y + 4), textState_Unselected, textType_Default);
        tCarStatType carStat = remap[j];
        uint8 upgrades = carInfo.fUpgrades;
        sint16 result = carInfo.fStats[0][carStat];
        if ((upgrades & 1) != 0)
            result += carInfo.fStats[1][carStat];
        if ((upgrades & 2) != 0)
            result += carInfo.fStats[2][carStat];
        if ((upgrades & 4) != 0)
            result += carInfo.fStats[3][carStat];
        DrawSlider(result, 0, 11, x, y, 104, 3, 7, 3, false, false, 128, 0);
        y = (sint16)(y + 15);
    }
}

void tScreenCarSelect::DrawForeground(void)
{
    FUNCTION_MARKER(0x8003C7D8, "FRONT.BIN");

    tMenu *menu = FEApp->fCurrentMenu[0];
    tMenuItem *currentItem = menu->fItemList[menu->fCurrentItem];
    tCarInfo carInfo;
    bool validCar = GetCar(carInfo);
    bool bShowStats = false;
    menuDefs->itemOpponentUpgrades.fFlags |= 1U;
    switch (fState) {
    case 0:
        bShowStats = currentItem == &menuDefs->itemCar;
        menuDefs->itemColor.fFlags &= ~1U;
        menuDefs->itemShowcase.fFlags &= ~1U;
        if (validCar && carInfo.fCarClass == 7) {
            menuDefs->itemColor.fFlags |= 1U;
            menuDefs->itemShowcase.fFlags |= 1U;
        }
        break;
    case 1:
        bShowStats = currentItem == &menuDefs->itemGarageCar;
        menuDefs->itemUpgradeCar.fFlags &= ~1U;
        if (frontEnd.raceType == 2 && frontEnd.tier == 0 && FECheat::IsCheatEnabled(cheat_FinishedTournament))
            menuDefs->itemOpponentUpgrades.fFlags &= ~1U;
        if (!validCar || carInfo.fCarClass >= 5) {
            menuDefs->itemUpgradeCar.fFlags |= 1U;
            fOverlays[4].direction = -1;
        } else {
            fOverlays[4].direction = 1;
        }
        break;
    case 2:
        bShowStats = currentItem == &menuDefs->itemDealerCar;
        break;
    case 3:
        bShowStats = currentItem == &menuDefs->itemSellerCar;
        menuDefs->itemSellCar.fFlags &= ~1U;
        fOverlays[4].direction = validCar ? 1 : -1;
        if (!validCar)
            menuDefs->itemSellCar.fFlags |= 1U;
        break;
    case 4:
        bShowStats = true;
        break;
    }

    fOverlays[6].direction = validCar && bShowStats ? 1 : -1;
    for (sint16 i = 0; i < 4; ++i) {
        sint32 next = (sint8)gStateOverlays[fState][i];
        tOverlay *&overlay = fCurrentOverlays[i];
        if (overlay && overlay->ID != next) {
            overlay->direction = -1;
            if (overlay->transition <= 0)
                overlay = 0;
        }
        if (!overlay && next >= 0) {
            overlay = &fOverlays[next];
            overlay->transition = 0;
            overlay->direction = 1;
        }
    }
    for (sint16 i = 1; i < 4; ++i)
        DrawOverlay(fCurrentOverlays[i]);

    if (!validCar) {
        carInfo.fCarID = (char)-1;
        carInfo.fCarIndex = 0xFF;
        carInfo.fCarClass = 0;
        carInfo.fCountry = 0xFF;
        carInfo.fAvailable = 0;
        carInfo.fSpeechCarID = (char)-1;
    }
    UpdateVideoWall(carInfo);
    if (gCarObj[(uint8)FEApp->fPlayer]->async_handle != 0) {
        SetBrightness(0, 0);
        fVideoWall[0].TurnOff();
        fFadeTicks[0] = ticks;
    }
    UpdateBrightness(0);

    if ((uint16)fState - 5U >= 2U) {
        showRoomFlag = 0;
        fCameraRotation = fCameraRotation + 3U;
        DrawCar(carInfo, 314, 84, 4.0f, -7.5f, (char)fBrightness[0], true, fCameraRotation, kPlayerOne);
    } else {
        sint32 elapsedticks = (sint32)((uint32)ticks - fSpeechTicks - 256U);
        sint32 speechCar = (sint8)carInfo.fSpeechCarID;
        if (elapsedticks >= 0 && speechCar >= 0) {
            sint32 textID = (elapsedticks >> 9) % 19 + 996 + speechCar * 19;
            uint32 textTicks = elapsedticks & 511U;
            sint32 textColor = kRGBVals[(uint8)textDefinitions[TextSys::WordFlags((sint16)textID)][4]];
            if (textTicks < 128)
                textColor = CalcFadeVal(textColor, (sint32)(128 - textTicks));
            else if (512 - textTicks < 128)
                textColor = CalcFadeVal(textColor, (sint32)(textTicks - 384));
            char *word = TextSys::Word(textID);
            sint16 x = (sint16)TextSys::WordX(textID);
            sint16 y = (sint16)TextSys::WordY(textID);
            FETextRender::FullTextRGB(word, x, y, textColor, 0, 0);
        }

        tDrawShapeExtended drawFlags;
        drawFlags.tint[0] = 0x551E00;
        drawFlags.custom_shapes = fSwapShapes.fShapes;
        uint32 speechTicks = fSpeechTicks;
        sint32 fadeVal = 128;
        if (speechTicks > 256)
            fadeVal = speechTicks <= 384 ? (sint32)(384 - speechTicks) : 0;
        DrawShapeExtended(10, 0x200, 0, 0, fadeVal, 0, &drawFlags);

        uint32 elapsed = (uint32)ticks - fShowroomTicks;
        while ((sint32)elapsed > 600) {
            fShowroomTicks = fShowroomTicks + 600U;
            fSplineInterval = (fSplineInterval + 1);
            elapsed -= 600U;
            if (fSplineInterval > 6)
                fSplineInterval -= 5;
        }
        sint16 knots[4];
        knots[0] = (sint16)fSplineInterval;
        gRotateOffset[0] = 0;
        if (knots[0] > 6) {
            knots[0] -= 5;
            gRotateOffset[0] = 0x4000000;
        }
        for (sint16 i = 1; i < 4; ++i) {
            knots[i] = (sint16)(knots[i - 1] + 1);
            gRotateOffset[i] = gRotateOffset[i - 1];
            if (knots[i] > 6) {
                knots[i] -= 5;
                gRotateOffset[i] += 0x4000000;
            }
        }

        sint32 cameraY = 0;
        sint32 cameraZ = 0;
        sint32 screenX = 0;
        sint32 screenY = 0;
        sint32 camRot = 0;
        CalcSplinePosition(knots[0], knots[1], knots[2], knots[3], elapsed, cameraY, cameraZ, screenX, screenY, camRot);
        showRoomFlag = 1;
        DrawCar(carInfo, (sint16)screenX, (sint16)screenY, (float)cameraY * (1.0f / 65536.0f), (float)cameraZ * (1.0f / 65536.0f), (char)fBrightness[0], true, camRot, kPlayerOne);
        if (validCar && gettick() - fSpeechTicks > 640U && !fSpeechPlayed && (sint8)carInfo.fSpeechCarID >= 0) {
            fSpeechPlayed = true;
            FeAudio::AsyncPlaySpeech(0, (sint8)carInfo.fSpeechCarID);
        }
    }

    if (gCarObj[0]->async_handle == 0 && fBrightness[0] == fDestBrightness[0] && fBrightness[0] == 0 && (sint32)((uint32)ticks - fFadeTicks[0]) > 128) {
        SetBrightness(carInfo.fAvailable != 0 ? 128 : 32, 0);
        fVideoWall[0].TurnOn();
    }
}

void tScreenCarSelectDuel::PreLoad(void)
{
    FUNCTION_MARKER(0x8003D08C, "FRONT.BIN");
    fOpponentShapes.fShapes = 0;
    tScreen::InitializeShapes(fOpponentShapes, 5);
    tScreen::PreLoad();
    fSwapShapes.fDestFile = Platform::GetDCTBuffer(16000, const_cast<char *>("VideoWall"));
    fOpponentShapes.fDestFile = Platform::GetDCTBuffer(16000, const_cast<char *>("OpponentVid"));
    tCarInfo carInfo;
    if (!GetCar(carInfo))
        carManager.GetStockCar(0, carInfo);
    char buffer[32];
    sprintf(buffer, "z%s", carInfo.fSmallName);
    AsyncLoadShapeFile(buffer, fSwapShapes);
    carManager.GetStockCar((uint8)frontEnd.oppCar, carInfo);
    sprintf(buffer, "z%s", carInfo.fSmallName);
    AsyncLoadShapeFile(buffer, fOpponentShapes);
    fOpponentTVsInitialized = 0;
}

void tScreenCarSelectDuel::AllocateAsyncBuffer(void)
{
    FUNCTION_MARKER(0x8003D1B4, "FRONT.BIN");
}

void tScreenCarSelectDuel::FreeAsyncBuffer(void)
{
    FUNCTION_MARKER(0x8003D1BC, "FRONT.BIN");
    Platform::ResetDCTBuffer();
    fSwapShapes.fDestFile = 0;
    fOpponentShapes.fDestFile = 0;
}

void tScreenCarSelectDuel::InitializeVideoWall(void)
{
    FUNCTION_MARKER(0x8003D1E8, "FRONT.BIN");
    fVideoWall[0].Initialize(tvConfigs, fSwapShapes.fShapes, 0, 5, tvSplitOrder, 0);
    fVideoWall[0].SetAvailableText(248, 270, 45);
    if ((fSwapShapes.fFlags & 1) != 0) {
        fVideoWall[0].SetOffset(6, 0);
        fVideoWall[0].UpdateImages();
        fTVsInitialized = 1;
    }
    fVideoWall[1].Initialize(tvConfigs + 5, fOpponentShapes.fShapes, 0, 5, tvSplitOrder, 0);
    fVideoWall[0].SetAvailableText(248, 270, 150);
    if ((fOpponentShapes.fFlags & 1) != 0) {
        fVideoWall[1].SetOffset(6, 105);
        fVideoWall[1].UpdateImages();
        fTVsInitialized = 1;
    }
}

void tScreenCarSelectDuel::Initialize(void)
{
    FUNCTION_MARKER(0x8003D304, "FRONT.BIN");
    tScreenCarSelect::Initialize();
    fState = 0;
    gShowroomLights = 0;
}

void tScreenCarSelectDuel::Cleanup(void)
{
    FUNCTION_MARKER(0x8003D334, "FRONT.BIN");
    tScreen::FreeShapes(fOpponentShapes);
    tScreenCarSelect::Cleanup();
}

void tScreenCarSelectDuel::DrawVideoWall(sint16 y)
{
    FUNCTION_MARKER(0x8003D364, "FRONT.BIN");
    tCarInfo carInfo;
    bool validCar = GetCar(carInfo);
    for (sint16 i = 0; i < 12; ++i)
        DrawShapeExtended(i, 0, 0, -y, fScreenFadeVal, 0, 0);
    if (fScreenFadeVal > 0 && fTransitionOff)
    {
        fVideoWall[0].TurnOffInstant();
        SetBrightness(0, 0);
    }
    if ((fSwapShapes.fFlags & 1) != 0 && fTVsInitialized == 0)
    {
        tVideoWall *wall = &fVideoWall[0];
        wall->SetOffset(6, 0);
        wall->UpdateImages();
        fTVsInitialized = 1;
    }
    tVideoWall *wall = &fVideoWall[0];
    wall->UpdateTransition();
    wall->SetValid(validCar);
    if (validCar)
        wall->SetAvailable(carInfo.fAvailable);
    wall->Draw();
}

void tScreenCarSelectDuel::DrawOpponentVideoWall(sint16 y)
{
    FUNCTION_MARKER(0x8003D4BC, "FRONT.BIN");
    for (sint16 i = 0; i < 12; ++i)
        DrawShapeExtended(i, 0, 0, -y, fScreenFadeVal, 0, 0);
    if (fScreenFadeVal > 0 && fTransitionOff) {
        fVideoWall[1].TurnOffInstant();
        SetBrightness(0, 1);
    }
    if ((fSwapShapes.fFlags & 1) != 0 && fOpponentTVsInitialized == 0) {
        fVideoWall[1].SetOffset(6, 105);
        fVideoWall[1].UpdateImages();
        fOpponentTVsInitialized = 1;
    }
    fVideoWall[1].UpdateTransition();
    fVideoWall[1].SetValid(1);
    fVideoWall[1].SetAvailable(1);
    fVideoWall[1].Draw();
}

void tScreenCarSelectDuel::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x8003D5E0, "FRONT.BIN");
    numPermShapes = 52;
    numSwapShapes = 5;
    tCarInfo carInfo;
    if (!GetCar(carInfo))
        carManager.GetStockCar(0, carInfo);
    fPreviousCar = carInfo.fCarIndex;
    fPreviousCarID = (sint8)carInfo.fCarID;
    uint8 country = carInfo.fCountry;
    fPreviousOpponent = -1;
    fPreviousCountry = country;
    *permFileName = const_cast<char *>("zDuel");
    *swapFileName = 0;
}

void tScreenCarSelectDuel::UpdateVideoWall(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x8003D69C, "FRONT.BIN");
    if (carInfo.fCarIndex != fPreviousCar || (sint8)carInfo.fCarID != fPreviousCarID || (carInfo.fCarClass == 7 && fPreviousCountry != carInfo.fCountry)) {
        if ((sint8)carInfo.fCarID >= 0)
            AsyncLoadSwapShapeFile(carInfo.fSmallName);
        fPreviousCar = carInfo.fCarIndex;
        fPreviousCarID = (sint8)carInfo.fCarID;
        uint8 country = carInfo.fCountry;
        fTVsInitialized = 0;
        fPreviousCountry = country;
        SetBrightness(0, 0);
        fVideoWall[0].TurnOff();
    }
}

void tScreenCarSelectDuel::UpdateOpponentVideoWall(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x8003D76C, "FRONT.BIN");
    if (carInfo.fCarIndex != fPreviousOpponent) {
        char buffer[32];
        sprintf(buffer, "z%s", carInfo.fSmallName);
        AsyncLoadShapeFile(buffer, fOpponentShapes);
        fOpponentTVsInitialized = 0;
        fPreviousOpponent = carInfo.fCarIndex;
        SetBrightness(0, 1);
        fVideoWall[1].TurnOff();
    }
}

void tScreenCarSelectDuel::DrawBackground(void)
{
    FUNCTION_MARKER(0x8003D7F0, "FRONT.BIN");

    DRAWENV *drenv = Draw::GetDRAWENV(Draw::gPlayer1View, gFlip);
    fState = 7;
    PSX_RECT temp;
    temp.x = 0;
    temp.y = drenv->clip.y;
    temp.w = 512;
    temp.h = (sint16)screenheight;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    DR_AREA *daprim = reinterpret_cast<DR_AREA *>(cprim->PrimPtr);
    setaddr(daprim, getaddr(cprim->LastPrim));
    cprim->PrimPtr = reinterpret_cast<char *>(daprim + 1);
    setaddr(cprim->LastPrim, daprim);
    SetDrawArea(daprim, &temp);

    PSX_RECT r = { 290, 130, 200, 12 };
    sint16 text = menuDefs->iteratorOpponentCar.TextValue(kPlayerBoth);
    DrawShape::NFS4RoundRectangle(text, r, 0);
    tCarInfo carInfo;
    carManager.GetStockCar((uint8)frontEnd.oppCar, carInfo);
    carInfo.fColor = carInfo.fColorOrder[carInfo.fDefaultColor];
    UpdateOpponentVideoWall(carInfo);
    IsShapeFileLoaded(fOpponentShapes);
    if (fOpponentShapes.fFile != 0 && fVideoWall[1].fTransitionDirection != -1 && gCarObj[1]->async_handle == 0 && (sint32)((uint32)ticks - fFadeTicks[1]) > 128)
    {
        UploadShapes(fOpponentShapes, 0, 65, 5, 0);
        fOpponentTVsInitialized = 0;
        fVideoWall[1].TurnOn();
        if (fBrightness[1] == fDestBrightness[1])
            SetBrightness(128, 1);
    }
    fCameraRotation += 3;
    carInfo.fUpgrades = 0;
    if (gCarObj[1]->async_handle != 0 && (sint32)((uint32)ticks - fFadeTicks[1]) > 128)
    {
        SetBrightness(0, 1);
        fVideoWall[1].TurnOff();
        fFadeTicks[1] = ticks;
    }
    UpdateBrightness(1);
    showRoomFlag = 0;
    DrawCar(carInfo, 278, 184, 1.7f, -9.9f, (char)fBrightness[1], false, fCameraRotation, kPlayerTwo);
    if (gCarObj[1]->async_handle == 0 && fBrightness[1] == fDestBrightness[1] && fBrightness[1] == 0 && (sint32)((uint32)ticks - fFadeTicks[1]) > 128)
    {
        SetBrightness(128, 1);
        fVideoWall[1].TurnOn();
    }
    DrawOpponentVideoWall(105);

    temp.x = 0;
    temp.y = (sint16)((uint16)drenv->clip.y + 128);
    temp.w = 512;
    temp.h = (sint16)((uint16)screenheight - 128);
    daprim = reinterpret_cast<DR_AREA *>(cprim->PrimPtr);
    setaddr(daprim, getaddr(cprim->LastPrim));
    cprim->PrimPtr = reinterpret_cast<char *>(daprim + 1);
    setaddr(cprim->LastPrim, daprim);
    SetDrawArea(daprim, &temp);
    sint32 halfHeight = screenheight / 2;
    PSXDrawSquare(0, 0, halfHeight, 512, halfHeight);

    bool validCar = GetCar(carInfo);
    if (validCar)
    {
        r.y = 25;
        if (frontEnd.carListType == 0)
            text = menuDefs->iteratorCar1.TextValue(kPlayerBoth);
        else
            text = menuDefs->iteratorGarageCar.TextValue(kPlayerBoth);
        DrawShape::NFS4RoundRectangle(text, r, 0);
    }
    else
        carInfo.fCarID = (char)0xFF;
    UpdateVideoWall(carInfo);
    if (gCarObj[0]->async_handle != 0 && (sint32)((uint32)ticks - fFadeTicks[0]) > 128)
    {
        SetBrightness(0, 0);
        fVideoWall[0].TurnOff();
        fFadeTicks[0] = ticks;
    }
    UpdateBrightness(0);
    showRoomFlag = 0;
    DrawCar(carInfo, 278, 79, 1.7f, -9.9f, (char)fBrightness[0], false, fCameraRotation, kPlayerOne);
    if (gCarObj[0]->async_handle == 0 && fBrightness[0] == fDestBrightness[0] && fBrightness[0] == 0 && (sint32)((uint32)ticks - fFadeTicks[0]) > 128)
    {
        SetBrightness(carInfo.fAvailable != 0 ? 128 : 32, 0);
        fVideoWall[0].TurnOn();
    }
    if (validCar)
    {
        IsShapeFileLoaded(fSwapShapes);
        if (fSwapShapes.fFile != 0 && fVideoWall[0].fTransitionDirection != -1 && gCarObj[0]->async_handle == 0 && (sint32)((uint32)ticks - fFadeTicks[0]) > 128)
        {
            UploadSwapShapes(5);
            fVideoWall[0].TurnOn();
            if (fDestBrightness[0] == fBrightness[0])
                SetBrightness(carInfo.fAvailable != 0 ? 128 : 32, 0);
        }
    }
    DrawVideoWall(0);
}

void tScreenCarSelectDuel::DrawForeground(void)
{
    FUNCTION_MARKER(0x8003DE68, "FRONT.BIN");
    sint16 y = 45;
    tCarInfo carInfo;
    bool validCar = GetCar(carInfo);
    for (sint16 i = 0; i < 2; ++i) {
        for (sint16 j = 0; j < 5; ++j) {
            FETextRender::MenuTextPositionedJustify(text2PVals[j], 500, (y + 4), 1, textState_Unselected, textType_ScreenInfo);
            sint16 result = 0;
            if (validCar) {
                tCarStatType carStat = remap[j];
                uint8 upgrades = carInfo.fUpgrades;
                result = carInfo.fStats[0][carStat];
                if ((upgrades & 1) != 0)
                    result += carInfo.fStats[1][carStat];
                if ((upgrades & 2) != 0)
                    result += carInfo.fStats[2][carStat];
                if ((upgrades & 4) != 0)
                    result += carInfo.fStats[3][carStat];
            }
            DrawSlider(result, 0, 11, 417, y, 73, 3, 4, 3, true, false, 128, 0);
            y = (sint16)(y + 15);
        }
        y = 150;
        carManager.GetStockCar((uint8)frontEnd.oppCar, carInfo);
    }
}

tScreenCarSelectTwoPlayer::tScreenCarSelectTwoPlayer(void)
{
    CarDialog.MaxW = 288;
    CarDialog.currentlyOn = 0;
    CarDialog.reservedheight = 0;
    CarDialog.MaxH = 0;
    CarDialog.OffsetY = 0;
    CarDialog.OffsetX = 0;
    CarDialog.height = 0;
    CarDialog.width = 0;
    CarDialog.top = 0;
    CarDialog.left = 0;
    CarDialog.specificPlayer = -1;
    CarDialog.fDefault = 0;
    CarDialog.timeOutTicks = 0;
    CarDialog.Centerit = 0;
    CarDialog.fFullyOpen = 0;
    CarDialog.timeOutTicks = 0;
    CarDialog.fFadeText = 128;
}

bool tScreenCarSelectTwoPlayer::GetCar(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x8003E040, "FRONT.BIN");
    sint32 currentplayer = (uint8)FEApp->fPlayer;
    uint8 color;
    if (frontEnd.carListType == 0) {
        carManager.GetStockCar((uint8)frontEnd.playerCar[currentplayer], carInfo);
        color = frontEnd.carColors[currentplayer][(sint8)carInfo.fCarID];
    } else {
        sint32 garageNumber = currentplayer;
        if (carManager.GetNumOwnedCars((sint16)currentplayer) <= 0)
            garageNumber = 0;
        if (carManager.GetNumOwnedCars((sint16)garageNumber) <= 0)
            return false;
        carManager.GetGarageCar((uint8)frontEnd.garageCar[currentplayer], carInfo, (sint16)garageNumber);
        color = carInfo.fColor;
    }
    carInfo.fColor = carInfo.fColorOrder[color];
    carInfo.fCountry = frontEnd.carCountry[(uint8)FEApp->fPlayer][(sint8)carInfo.fCarID];
    return true;
}

void tScreenCarSelectTwoPlayer::DrawVideoWall(sint16 y)
{
    FUNCTION_MARKER(0x8003E190, "FRONT.BIN");
    tCarInfo carInfo;
    bool validCar = GetCar(carInfo);
    for (sint16 i = 0; i < 12; ++i)
        DrawShapeExtended(i, 0, 0, -y, fScreenFadeVal, 0, 0);
    if ((fSwapShapes.fFlags & 1) != 0 && fTVsInitialized == 0)
    {
        sint16 offset = FEApp->fPlayer != 0 ? 105 : 0;
        tVideoWall *wall = &fVideoWall[0];
        wall->SetOffset(6, offset);
        sint16 textY = FEApp->fPlayer != 0 ? 150 : 45;
        wall->SetAvailableText(248, 270, textY);
        wall->UpdateImages();
        fTVsInitialized = 1;
    }
    if (fScreenFadeVal > 0 && fTransitionOff)
    {
        fVideoWall[0].TurnOffInstant();
        SetBrightness(0, 0);
    }
    tVideoWall *wall = &fVideoWall[0];
    wall->UpdateTransition();
    wall->SetValid(validCar);
    if (validCar)
        wall->SetAvailable(carInfo.fAvailable);
    wall->Draw();
}

void tScreenCarSelectTwoPlayer::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x8003E330, "FRONT.BIN");
    numPermShapes = 52;
    numSwapShapes = 5;
    fPreviousCar = -1;
    fPreviousCarID = -1;
    fPreviousCountry = -1;
    tCarInfo carInfo;
    carManager.GetStockCar(0, carInfo);
    *permFileName = const_cast<char *>("zcarsb");
    sprintf(gSwapFileName[0], "%s", carInfo.fSmallName);
    *swapFileName = gSwapFileName[0];
}

void tScreenCarSelectTwoPlayer::UpdateVideoWall(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x8003E3C8, "FRONT.BIN");
    if (carInfo.fCarIndex != fPreviousCar || (sint8)carInfo.fCarID != fPreviousCarID || (carInfo.fCarClass == 7 && fPreviousCountry != carInfo.fCountry)) {
        if ((sint8)carInfo.fCarID >= 0)
            AsyncLoadSwapShapeFile(carInfo.fSmallName);
        fPreviousCar = carInfo.fCarIndex;
        fPreviousCarID = (sint8)carInfo.fCarID;
        uint8 country = carInfo.fCountry;
        fTVsInitialized = 0;
        fPreviousCountry = country;
        fVideoWall[0].TurnOff();
        SetBrightness(0, 0);
    }
}

void tScreenCarSelectTwoPlayer::TurnOffVideoWall(void)
{
    FUNCTION_MARKER(0x8003E49C, "FRONT.BIN");
    fVideoWall[0].TurnOffInstant();
}

void tScreenCarSelectTwoPlayer::DrawBackground(void)
{
    FUNCTION_MARKER(0x8003E4BC, "FRONT.BIN");
    DRAWENV *drenv = Draw::GetDRAWENV(Draw::gPlayer1View, gFlip);
    PSX_RECT temp = { 0, drenv->clip.y, 512, (sint16)screenheight };
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    DR_AREA *daprim = reinterpret_cast<DR_AREA *>(cprim->PrimPtr);
    setaddr(daprim, getaddr(cprim->LastPrim));
    cprim->PrimPtr = reinterpret_cast<char *>(daprim + 1);
    setaddr(cprim->LastPrim, daprim);
    SetDrawArea(daprim, &temp);

    tCarInfo carInfo;
    bool validCar = GetCar(carInfo);
    sint16 carY = 79;
    if (validCar)
    {
        PSX_RECT r = { 290, 20, 200, 12 };
        if ((uint8)FEApp->fPlayer == 1)
        {
            carY = 184;
            r.y = 128;
        }
        fCameraRotation += 3;
        DrawShape::NFS4RoundRectangle((sint8)carInfo.fCarID + 289, r, 0);
        UpdateVideoWall(carInfo);
        if (gCarObj[(uint8)FEApp->fPlayer]->async_handle != 0)
        {
            SetBrightness(0, 0);
            fFadeTicks[0] = ticks;
        }
        if (gCarObj[(uint8)FEApp->fPlayer]->async_handle == 0 && fBrightness[0] == fDestBrightness[0] && fBrightness[0] == 0 && (sint32)((uint32)ticks - fFadeTicks[0]) > 128)
        {
            SetBrightness(carInfo.fAvailable != 0 ? 128 : 32, 0);
            fVideoWall[0].TurnOn();
        }
        UpdateBrightness(0);
    }
    else
    {
        carInfo.fCarID = (char)0xFF;
        fPreviousCarID = -1;
    }
    showRoomFlag = 0;
    DrawCar(carInfo, 278, carY, 1.7f, -9.9f, (char)fBrightness[0], false, fCameraRotation, (tPlayer)(uint8)FEApp->fPlayer);
    if (!validCar)
        TurnOffVideoWall();

    IsShapeFileLoaded(fSwapShapes);
    if (fSwapShapes.fFile != 0 && fVideoWall[0].fTransitionDirection != -1 && gCarObj[(uint8)FEApp->fPlayer]->async_handle == 0 && (sint32)((uint32)ticks - fFadeTicks[0]) > 128)
    {
        sint16 offset = (uint8)FEApp->fPlayer == 1 ? 65 : 0;
        UploadShapes(fSwapShapes, 0, offset, 5, 0);
        fVideoWall[0].TurnOn();
        if (fDestBrightness[0] == fBrightness[0])
            SetBrightness(validCar && carInfo.fAvailable != 0 ? 128 : 32, 0);
    }
    DrawVideoWall((uint8)FEApp->fPlayer == 1 ? 105 : 0);
    SetDialog();

    sint32 halfHeight = screenheight / 2;
    temp.x = 0;
    temp.y = drenv->clip.y;
    temp.w = 512;
    temp.h = (sint16)halfHeight;
    if ((uint8)FEApp->fPlayer == 1)
        temp.y = (sint16)(temp.y + halfHeight);
    daprim = reinterpret_cast<DR_AREA *>(cprim->PrimPtr);
    setaddr(daprim, getaddr(cprim->LastPrim));
    cprim->PrimPtr = reinterpret_cast<char *>(daprim + 1);
    setaddr(cprim->LastPrim, daprim);
    SetDrawArea(daprim, &temp);
}

void tScreenCarSelectTwoPlayer::DrawForeground(void)
{
    FUNCTION_MARKER(0x8003EA14, "FRONT.BIN");
    sint16 yOffset = (uint8)FEApp->fPlayer == 1 ? 150 : 45;
    tCarInfo carInfo;
    bool gotcar = GetCar(carInfo);
    tGlobalMenuDefs *menus = menuDefs;
    tMenuItem *item = (uint8)FEApp->fPlayer == 0 ? &menus->itemColorP1 : &menus->itemColorP2;
    item->fFlags &= ~1U;
    if (!gotcar || carInfo.fCarClass == 7)
        item->fFlags |= 1;
    for (sint16 j = 0; j < 5; ++j) {
        FETextRender::MenuTextPositionedJustify(text2PVals[j], 500, (yOffset + 4), 1, textState_Unselected, textType_Default);
        sint16 result = 0;
        if (gotcar) {
            tCarStatType carStat = remap[j];
            uint8 upgrades = carInfo.fUpgrades;
            result = carInfo.fStats[0][carStat];
            if ((upgrades & 1) != 0)
                result += carInfo.fStats[1][carStat];
            if ((upgrades & 2) != 0)
                result += carInfo.fStats[2][carStat];
            if ((upgrades & 4) != 0)
                result += carInfo.fStats[3][carStat];
        }
        DrawSlider(result, 0, 11, 417, yOffset, 73, 3, 4, 3, true, false, 128, 0);
        yOffset = (sint16)(yOffset + 15);
    }
}

void tScreenCarSelectTwoPlayer::SetDialog(void)
{
    FUNCTION_MARKER(0x8003EC50, "FRONT.BIN");
    sint32 player = (uint8)FEApp->fPlayer;
    if (FEApp->waitingForOtherPlayer[player] != 0) {
        CarDialog.OffsetX = 0;
        CarDialog.OffsetY = player == 0 ? -60 : 60;
        CarDialog.specificPlayer = (sint16)player;
        char *format = TextSys::Word(680);
        char *name = PlayerName(1 - player);
        sprintf(WaitingString, format, name);
        CarDialog.string = WaitingString;
        CarDialog.Display();
    } else {
        CarDialog.Hide();
    }
}

void tScreenCarSelectTwoPlayer::AllocateAsyncBuffer(void)
{
    FUNCTION_MARKER(0x8003ED10, "FRONT.BIN");
    fSwapShapes.fDestFile = Platform::GetDCTBuffer(16000, const_cast<char *>("VideoWall"));
}

void tScreenCarSelectTwoPlayer::InitializeVideoWall(void)
{
    FUNCTION_MARKER(0x8003ED44, "FRONT.BIN");
    fVideoWall[0].Initialize(tvConfigs, fSwapShapes.fShapes, 0, 5, tvSplitOrder, 0);
    if ((fSwapShapes.fFlags & 1) != 0) {
        fVideoWall[0].UpdateImages();
        fTVsInitialized = 1;
    }
}

void tScreenCarSelectTwoPlayer::Initialize(void)
{
    FUNCTION_MARKER(0x8003EDC0, "FRONT.BIN");
    tScreenCarSelect::Initialize();
    fState = 0;
    fPreviousCar = -1;
    fPreviousCarID = -1;
    fPreviousCountry = -1;
    fDestBrightness[1] = 0;
    fDestBrightness[0] = 0;
    fBrightness[1] = 0;
    fBrightness[0] = 0;
    gShowroomLights = 0;
}

void tScreenCarSelectTwoPlayer::Cleanup(void)
{
    FUNCTION_MARKER(0x8003EE10, "FRONT.BIN");
    CarDialog.Hide();
    tScreenCarSelect::Cleanup();
}

bool tScreenPinkSlipsCarSelect::GetCar(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x8003EE40, "FRONT.BIN");
    if (PinkSlipsScreenState[0] != CardLoadedFine || PinkSlipsScreenState[1] != CardLoadedFine)
        return false;
    sint32 player = (uint8)FEApp->fPlayer;
    carManager.GetPinkSlipsCar((uint8)frontEnd.pinkSlipsCar[player], carInfo, (sint16)player);
    carInfo.fColor = carInfo.fColorOrder[carInfo.fColor];
    return true;
}

void tScreenPinkSlipsCarSelect::DrawBackground(void)
{
    FUNCTION_MARKER(0x8003EED0, "FRONT.BIN");
    DoMemCardStuff();
    tScreenCarSelectTwoPlayer::DrawBackground();
    if (fExitingScreen)
        CarDialog.Hide();
}

void tScreenPinkSlipsCarSelect::DoMemCardStuff(void)
{
    FUNCTION_MARKER(0x8003EF14, "FRONT.BIN");
    sint32 player = (uint8)FEApp->fPlayer;
    if (fExitingScreen || (player == 1 && PinkSlipsScreenState[0] != CardLoadedFine))
        return;
    if (PinkSlipsScreenState[0] == CardCurrentlyLoading || PinkSlipsScreenState[1] == CardCurrentlyLoading)
        return;
    if (PinkSlipsScreenState[player] != NoCardInserted || CURRENTLYUSINGMEMCARD != 0)
        return;
    PinkSlipsScreenState[player] = CardCurrentlyLoading;
    sint16 result = LoadGame((sint16)player, true, false);
    if (result == 0)
    {
        carManager.GetNumPinkSlipsCars((sint16)player);
        carManager.CheapestCarStockPrice();
        if (carManager.GetNumPinkSlipsCars((sint16)player) == 32)
            PinkSlipsScreenState[player] = TooManyCars;
        else if (carManager.GetNumPinkSlipsCars((sint16)player) > 1 || (carManager.GetNumPinkSlipsCars((sint16)player) == 1 && carManager.CheapestCarStockPrice() <= frontEnd.pinkSlipsCash[player]))
            PinkSlipsScreenState[player] = CardLoadedFine;
        else
            PinkSlipsScreenState[player] = NotEnoughCars;
    }
    else
        PinkSlipsScreenState[player] = result == 3 ? CardFailedNotFound : CardFailed;
    if (PinkSlipsScreenState[player] == CardLoadedFine)
    {
        menuDefs->iteratorPinkSlipsCar.Decrement((tPlayer)player);
        menuDefs->iteratorPinkSlipsCar.Increment((tPlayer)player);
    }
    if (PinkSlipsScreenState[0] == CardLoadedFine && PinkSlipsScreenState[1] == CardLoadedFine)
        SetLicensePlate();
}

void tScreenPinkSlipsCarSelect::Initialize(void)
{
    FUNCTION_MARKER(0x8003F24C, "FRONT.BIN");
    waitfordialog = 0;
    fStartCheckTick = 0;
    fCardFailed = 0;
    PinkSlipsScreenState[0] = NoCardInserted;
    PinkSlipsScreenState[1] = NoCardInserted;
    pCI = 0;
    fExitingScreen = false;
    tScreenCarSelectTwoPlayer::Initialize();
}

void tScreenPinkSlipsCarSelect::Cleanup(void)
{
    FUNCTION_MARKER(0x8003F2AC, "FRONT.BIN");
    fExitingScreen = true;
    PinkSlipsScreenState[0] = WhoCaresWeBeExiting;
    PinkSlipsScreenState[1] = WhoCaresWeBeExiting;
    CarDialog.Hide();
    tScreenCarSelectTwoPlayer::Cleanup();
}

void tScreenPinkSlipsCarSelect::SetDialog(void)
{
    FUNCTION_MARKER(0x8003F300, "FRONT.BIN");
    sint32 player = (uint8)FEApp->fPlayer;
    CarDialog.OffsetX = 0;
    CarDialog.OffsetY = player == 0 ? -60 : 60;
    CarDialog.specificPlayer = (sint16)player;
    if (fExitingScreen || (player == 1 && PinkSlipsScreenState[0] != CardLoadedFine))
    {
        CarDialog.Hide();
        return;
    }
    switch (PinkSlipsScreenState[player])
    {
    case CardLoadedFine:
        if (FEApp->waitingForOtherPlayer[player] == 0 && PinkSlipsScreenState[1 - player] == CardLoadedFine)
        {
            CarDialog.Hide();
            return;
        }
        {
            char *format = TextSys::Word(680);
            char *name = PlayerName(1 - player);
            sprintf(WaitingString, format, name);
        }
        CarDialog.string = WaitingString;
        break;
    case NotEnoughCars:
        CarDialog.string = TextSys::Word(player + 813);
        break;
    case TooManyCars:
        CarDialog.string = TextSys::Word(player + 815);
        break;
    case CardFailed:
    case CardFailedNotFound:
    case CardFailedUnformatted:
        CarDialog.string = const_cast<char *>("Saved garage could not be loaded");
        break;
    default:
        CarDialog.Hide();
        return;
    }
    CarDialog.Display();
}

void tScreenPinkSlipsCarSelect::ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x8003F590, "FRONT.BIN");
    if (keyval != kInput_KeyType_Triangle)
    {
        if (keyval != kInput_KeyType_Circle && (PinkSlipsScreenState[0] != CardLoadedFine || PinkSlipsScreenState[1] != CardLoadedFine))
            keyval = kInput_KeyType_AlreadyProcessed;
        if (keyval != kInput_KeyType_Triangle)
            return;
    }
    if (PinkSlipsScreenState[0] != CardLoadedFine || PinkSlipsScreenState[1] != CardLoadedFine)
    {
        fExitingScreen = true;
        PinkSlipsScreenState[0] = WhoCaresWeBeExiting;
        PinkSlipsScreenState[1] = WhoCaresWeBeExiting;
        CarDialog.Hide();
    }
}

void tScreenPinkSlipsCarSelect::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x8003F638, "FRONT.BIN");
    numPermShapes = 52;
    numSwapShapes = 5;
    fPreviousCar = -1;
    fPreviousCarID = -1;
    fPreviousCountry = -1;
    tCarInfo carInfo;
    carManager.GetStockCar(0, carInfo);
    *permFileName = const_cast<char *>("zcarsb");
    sprintf(gSwapFileName[0], "%s", carInfo.fSmallName);
    *swapFileName = gSwapFileName[0];
}
