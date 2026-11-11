#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "xport.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/feaudio.h"
#include "frontend/common/fefades.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetools.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/front.h"
#include "frontend/common/screencarselect.h"
#include "frontend/common/screencongrats.h"
#include "frontend/common/screenpost.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/nfs3.h"
#include "game/common/r3dcar.h"
#include "game/common/textsys.h"
#include "game/psx/cario.h"
#include "game/psx/fe3dmenu.h"
#include "lib/psx/blkmov.h"

static uint32 carRotate;
char congratsSwapFileName[16];
char fPermFileNameBuf[16];
tScreenPinkSlipCongrats *screenPinkSlipCongrats;
tScreenTournamentTrophy *screenTournamentTrophy;
tScreenBeTheCopCongrats *screenBeTheCopCongrats;
tScreenTournamentCongrats *screenTournamentCongrats;

void tScreenCongrats::Cleanup(void)
{
    FUNCTION_MARKER(0x80047E60, "FRONT.BIN");
    CleanupSpinningCarsMenu();
    tScreen::Cleanup();
}

void tScreenCongrats::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x80047E8C, "FRONT.BIN");
    tTournamentDefinition *definition = tournamentManager.fDefinition;
    sint32 tournament = definition->fTiers[tournamentManager.fTier].fTournOffset + tournamentManager.fTournament;
    tTourneyInfo *tourneyInfo = &definition->fTournaments[tournament];
    fNumSmallSpinShapes = 0;
    fNumSpinShapes = 0;
    numPermShapes = congratsMessage == kScreenCongrats_Congrats ? 43 : 22;
    const char *name = congratsMessage == kScreenCongrats_Congrats ? "zcong" : "zelim";
    sprintf(fPermFileNameBuf, "%s%d", name, (uint8)frontEnd.language);
    *permFileName = fPermFileNameBuf;
    if ((uint32)trophy - 3 < 2)
    {
        if (smallSpinningThing == kSpinningGold)
        {
            numSwapShapes = 32;
            fNumSmallSpinShapes = 32;
            tournamentManager.GetTrophyName(tourneyInfo, ts_Small, congratsSwapFileName, -1);
            *swapFileName = congratsSwapFileName;
        }
        else if (smallSpinningThing == kSpinningMemCard)
        {
            numSwapShapes = 10;
            fNumSmallSpinShapes = 10;
            *swapFileName = const_cast<char *>("congb");
        }
        else
        {
            *swapFileName = 0;
            numSwapShapes = 0;
        }
    }
    else
    {
        sint32 j = 900;
        sint32 count = (sint16)((sint16)tournamentManager.fNumRacers + (tourneyInfo->fKnockout != 0));
        for (sint32 i = 1; i <= count; ++i)
            if (tournamentManager.PlayerRanking((sint16)i) == 0)
                j = i;
        tournamentManager.GetTrophyName(tourneyInfo, ts_Large, congratsSwapFileName, j);
        *swapFileName = congratsSwapFileName;
        numSwapShapes = 32;
        fNumSpinShapes = 32;
    }
}

bool tScreenCongrats::GetCar(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x800480E4, "FRONT.BIN");
    carManager.GetGarageCar((uint8)frontEnd.garageCar[0], carInfo, 0);
    carInfo.fColor = carInfo.fColorOrder[carInfo.fColor];
    return true;
}

void Front::ResetOverlayCongrats(void)
{
    carRotate = 0;
}

void tScreenCongrats::DrawBackground(void)
{
    FUNCTION_MARKER(0x8004813C, "FRONT.BIN");
    sint32 fJustFadeOff = fTransitionOff ? fScreenFadeVal : 0;
    tDrawShapeExtended drawFlags;
    tDrawShapeExtended drawFlags2;
    drawFlags.custom_shapes = fSwapShapes.fShapes;
    drawFlags2.custom_shapes = fSwapShapes.fShapes;
    carRotate += 3;
    DrawCongratsMessage();
    if (trophy == kTrophyCar &&
        (starttick == -1 || !fGotCar || R3DCar::aSyncLoading >= 0 ||
         (sint32)((uint32)ticks - (uint32)fEnterTick) < 60))
        starttick = ticks;
    else
    {
        if (starttick == -1)
            starttick = ticks;
        if (CashAwarded != -1)
        {
            if ((sint32)((uint32)ticks - (uint32)starttick) > 1000 || fStartCountdownNOW)
            {
                if (CashAwarded != 0)
                    AudioCmn::PlayFESFX(21);
                CashAwarded = (sint32)((uint32)CashAwarded - (uint32)fCountSpeed);
                if (CashAwarded < 0)
                {
                    CashAwarded = 0;
                    fCountedDown = 1;
                }
            }
            sint32 colf = CalcFadeVal(kRGBVals[(uint8)textDefinitions[11][5]], fScreenFadeVal);
            sint32 colb = CalcFadeVal(0x232323, fScreenFadeVal);
            FETextRender::MenuTextFade(fScreenFadeVal, 0x317, textState_Hilighted, textType_TrackRecords);
            sint32 x = TextSys::WordX(0x318);
            sint32 y = TextSys::WordY(0x318);
            DrawMoney(x, y, 6, CashAwarded, colf, colb);
            FETextRender::MenuTextFade(fScreenFadeVal, 0x316, textState_Hilighted, textType_TrackRecords);
            x = TextSys::WordX(0x319);
            y = TextSys::WordY(0x319);
            sint32 countedCash = (sint32)((uint32)TotalCash - (uint32)CashAwarded);
            DrawMoney(x, y, 9, countedCash, colf, colb);
        }
        if (fSpeechToPlay != 0 && (sint32)((uint32)ticks - (uint32)starttick) > 128)
        {
            FeAudio::AsyncPlaySpeech(2, fSpeechToPlay);
            fSpeechToPlay = 0;
        }
        bool scale = false;
        if (congratsMessage == kScreenCongrats_Congrats)
        {
            drawFlags.tint[0] = CalcFadeVal(0xBEBE, fScreenFadeVal);
            drawFlags2.tint[0] = CalcFadeVal(0x808080, fJustFadeOff);
            if (!InExtraSpin)
            {
                framenum = (sint32)((uint32)ticks - (uint32)starttick) / 2;
                if (framenum >= 20)
                {
                    scale = true;
                    sint32 quarterStart = starttick / 4;
                    sint32 now = ticks;
                    framenum = 20;
                    if ((sint32)((uint32)now - (uint32)quarterStart) % 1500 < 45)
                    {
                        sint32 spinTick = ticks;
                        InExtraSpin = true;
                        InExtraSpinTick = spinTick;
                    }
                }
            }
            else
            {
                framenum = (sint32)((uint32)ticks - (uint32)InExtraSpinTick) / 6 + 21;
                scale = true;
                if (framenum >= 42)
                {
                    InExtraSpin = false;
                    framenum = 20;
                }
            }
        }
        else
        {
            drawFlags.tint[0] = CalcFadeVal(0x646464, fJustFadeOff);
            drawFlags2.tint[0] = CalcFadeVal(0x808080, fJustFadeOff);
            framenum = (sint32)((uint32)ticks - (uint32)starttick) / 4;
            if (framenum > 20)
                framenum = 20;
        }
        sint32 y = congratsMessage == kScreenCongrats_Eliminated ? 10 : 0;
        if (scale)
            ScaleShapeExtended(framenum, 0x410, 0, y, 0, 0, &drawFlags);
        else
            DrawShapeExtended(framenum, 0x410, 0, y, 0, 0, &drawFlags);
        if ((uint32)trophy - 3 >= 2)
        {
            sint32 frame = (ticks / 12) % 32;
            ScaleShapeExtended(frame, 0x610, 70, 15, 0, 0, &drawFlags2);
        }
        if ((uint32)trophy >= 3)
        {
            if (smallSpinningThing == kSpinningGold)
            {
                sint32 frame = ticks >> 3;
                sint32 count = fNumSmallSpinShapes;
                if (count == 0)
                    abort();
                ScaleShapeExtended(frame % count, 0x610, 41, 190, 0, 0, &drawFlags2);
            }
            else if (smallSpinningThing == kSpinningMemCard)
            {
                sint32 now = ticks;
                drawFlags.tint[0] = 0x551E00;
                sint32 count = fNumSmallSpinShapes;
                if (count == 0)
                    abort();
                sint32 frame = (now / 20) % count;
                DrawShapeExtended(frame, 0x610, -193, 86, 0, 0, &drawFlags);
            }
        }
    }
    sint32 StripeRGB = 0x30022;
    if (congratsMessage == kScreenCongrats_Congrats)
    {
        switch (trophy)
        {
        case kTrophyGold:
        case kTrophyCar: StripeRGB = 0x3E44; break;
        case kTrophySilver: StripeRGB = 0x212121; break;
        case kTrophyBronze: StripeRGB = 0x3044; break;
        }
    }
    sint32 bannerframe = congratsMessage == kScreenCongrats_Congrats ? 42 : 21;
    tDrawShapeExtended drawFlags3;
    drawFlags3.tint[0] = CalcFadeVal(StripeRGB, fScreenFadeVal);
    if (congratsMessage == kScreenCongrats_Congrats && trophy != kTrophyCar)
    {
        for (sint32 i = 1; i < 30; ++i)
            if (i % 3 != 0)
                DrawShapeExtended(bannerframe, 0x410, i * 2, 0, fScreenFadeVal, 1, &drawFlags3);
        for (sint32 i = 34; i < 63; ++i)
            if (i % 3 != 0)
                DrawShapeExtended(bannerframe, 0x410, i * 2, 0, fScreenFadeVal, 1, &drawFlags3);
    }
    else
    {
        for (sint32 i = 7; i < 40; ++i)
            if (i % 3 != 0)
                DrawShapeExtended(bannerframe, 0x410, i * 2, 0, fScreenFadeVal, 1, &drawFlags3);
    }
    if (trophy == kTrophyCar && fGotCar)
    {
        showRoomFlag = 0;
        DrawCar(fCarInfo, fCarX, fCarY, fCarCX, fCarCY, 128, true, carRotate, (tPlayer)fCarPlayer);
    }
}

void tScreenCongrats::DrawForeground(void)
{
    FUNCTION_MARKER(0x800489B0, "FRONT.BIN");
}

void tScreenCongrats::CalculatePrizes(void)
{
    FUNCTION_MARKER(0x800489B8, "FRONT.BIN");
    trophy = kTrophyNone;
    CashAwarded = -1;
    fCarX = 278;
    congratsMessage = kScreenCongrats_Congrats;
    fCarY = 63;
    fCarCY = -7.4f;
    smallSpinningThing = kSpinningNone;
    fCarPlayer = 0;
    TotalCash = 0;
    fCarCX = 4.0;
    if (congratsMessage == kScreenCongrats_Eliminated)
    {
        fCarX = 288;
        fCarY = 73;
        fCarCY = -8.2f;
    }
}

void tScreenCongrats::Initialize(void)
{
    FUNCTION_MARKER(0x80048A2C, "FRONT.BIN");
    sint32 now = ticks;
    fSpeechToPlay = 0;
    starttick = -1;
    framenum = -1;
    InExtraSpin = false;
    fEnterTick = now;
    SetLicensePlate();
    CalculatePrizes();
    bool gotCar = GetCar(fCarInfo);
    sint32 cash = CashAwarded;
    fGotCar = gotCar;
    if (cash <= 0)
    {
        fCountedDown = 1;
        fStartCountdownNOW = true;
        fCountSpeed = 1000;
    }
    else
    {
        fCountedDown = 0;
        fStartCountdownNOW = false;
        fCountSpeed = cash >> 6;
    }
    tScreen::Initialize();
}

void tScreenCongrats::ProcessInput(tPlayer p, tInputKeyType &keyval, tMenuCommand &c)
{
    FUNCTION_MARKER(0x80048AF0, "FRONT.BIN");
    if (keyval != kInput_KeyType_Circle &&
        (ginfo.areLoading != 0 || ginfo.soundIsPlaying != 0 ||
         (sint32)((uint32)ticks - (uint32)starttick) < 150))
        keyval = kInput_KeyType_AlreadyProcessed;
}

void tScreenPinkSlipCongrats::DrawCongratsMessage(void)
{
    FUNCTION_MARKER(0x80048B48, "FRONT.BIN");
    PSX_RECT r = { 41, 60, 420, 200 };
    char buffer[250];
    bool carBeforeLoser = (uint32)(uint8)frontEnd.language - 2 < 2;
    char *format = TextSys::Word(0x275);
    char *winner = PlayerName(fWinner);
    char *second;
    char *third;
    if (carBeforeLoser)
    {
        second = TextSys::Word((sint8)fCarInfo.fCarID + 0x121);
        third = PlayerName(1 - fWinner);
    }
    else
    {
        second = PlayerName(1 - fWinner);
        third = TextSys::Word((sint8)fCarInfo.fCarID + 0x121);
    }
    sprintf(buffer, format, winner, second, third, fWinner + 1);
    FETextRender::WordWrapText(buffer, r, textState_Selected, textType_PostGame);
}

bool tScreenPinkSlipCongrats::GetCar(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x80048C64, "FRONT.BIN");
    sint32 loser = 1 - fWinner;
    carManager.GetPinkSlipsCar((uint8)frontEnd.pinkSlipsCar[loser], carInfo, (sint16)loser);
    carInfo.fColor = carInfo.fColorOrder[carInfo.fColor];
    return true;
}

void tScreenPinkSlipCongrats::CalculatePrizes(void)
{
    FUNCTION_MARKER(0x80048CDC, "FRONT.BIN");
    sint32 loser = 1 - fWinner;
    trophy = kTrophyCar;
    smallSpinningThing = kSpinningMemCard;
    TotalCash = 0;
    CashAwarded = -1;
    congratsMessage = kScreenCongrats_Congrats;
    fCarPlayer = 1 - fWinner;
    CarIO::CleanUpLicense(loser);
    CarIO::CreateLicense(frontEnd.licensePlate[1 - loser], 0, loser);
    tCarInfo carinfo;
    GetCar(carinfo);
    sint32 speechCar = (sint8)carinfo.fSpeechCarID;
    if (speechCar == -1)
        fSpeechToPlay = fWinner + 23;
    else
        fSpeechToPlay = speechCar * 2 + fWinner + 19;
    fCarX = 278;
    fCarY = 75;
    fCarCX = 4.0;
    fCarCY = -7.4f;
}

void tScreenPinkSlipCongrats::Initialize(void)
{
    FUNCTION_MARKER(0x80048DEC, "FRONT.BIN");
    fWinner = -1;
    if ((uint8)frontEnd.pinkSlipsWins[0] < ((uint8)frontEnd.pinkSlipsNumTracks >> 1) + 1)
        fWinner = 1;
    else
        fWinner = 0;
    tScreenCongrats::Initialize();
}

void tScreenPinkSlipCongrats::Cleanup(void)
{
    FUNCTION_MARKER(0x80048E44, "FRONT.BIN");
    CleanupSpinningCarsMenu();
    tScreen::Cleanup();
}

void tScreenTournamentTrophy::ProcessInput(tPlayer p, tInputKeyType &keyval, tMenuCommand &c)
{
    FUNCTION_MARKER(0x80048E70, "FRONT.BIN");
    if (keyval == kInput_KeyType_Cross && fCountedDown == 0)
    {
        keyval = kInput_KeyType_AlreadyProcessed;
        fStartCountdownNOW = true;
    }
    tScreenCongrats::ProcessInput(p, keyval, c);
}

bool tScreenTournamentTrophy::GetCar(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x80048EB8, "FRONT.BIN");
    if (congratsMessage == kScreenCongrats_Congrats)
    {
        tAwardInformation tInfo;
        tournamentManager.GetAwardInformation(tInfo);
        tCarInfo *car = carManager.GetCarFromID((sint16)tInfo.fAwardCarModel);
        blockmove(car, &carInfo, 204);
        carInfo.fUpgrades = tInfo.fAwardCarUpgrades;
        carInfo.fColor = tInfo.fAwardCarColor;
    }
    else
    {
        carManager.GetGarageCar((uint8)frontEnd.garageCar[0], carInfo, 0);
        carInfo.fColor = carInfo.fColorOrder[carInfo.fColor];
    }
    return true;
}

void tScreenTournamentTrophy::DrawCongratsMessage(void)
{
    FUNCTION_MARKER(0x80048F64, "FRONT.BIN");
    PSX_RECT r = { 41, 60, 200, 400 };
    if (congratsMessage != kScreenCongrats_Congrats)
    {
        sint16 yyy = (sint16)((uint32)TextSys::WordY(800) - 10);
        char *text = TextSys::Word(800);
        FETextRender::FullTextRGB(text, 288, yyy, 0x414141, 3, 2);
        return;
    }
    sint32 firstmessage = 0;
    sint32 secondmessage = 0;
    tTournamentDefinition *definition = tournamentManager.fDefinition;
    tTourneyInfo *tourneyInfo = &definition->fTournaments[definition->fTiers[tournamentManager.fTier].fTournOffset + tournamentManager.fTournament];
    char buffer1[500];
    char buffer2[500];
    if ((sint8)tournamentManager.fPrevBestPlacement >= 4)
    {
        firstmessage = (sint8)tourneyInfo->fTournamentID + 0x3B3;
        sint32 placeoffset = 2;
        if (trophy == kTrophySilver)
            placeoffset = 1;
        else if (trophy == kTrophyGold || trophy == kTrophyCar)
            placeoffset = 0;
        char *format = TextSys::Word((sint8)tourneyInfo->fTournamentID + 0x3B3);
        char *place = TextSys::Word(placeoffset + 0x3E1);
        sprintf(buffer1, format, place);
    }
    if ((trophy == kTrophyGold && (sint8)tournamentManager.fPrevBestPlacement != 1) || trophy == kTrophyCar)
    {
        if (firstmessage == 0)
        {
            firstmessage = (sint8)tourneyInfo->fTournamentID + 0x3C6;
            char *format = TextSys::Word(firstmessage);
            sprintf(buffer1, format);
        }
        else
        {
            secondmessage = (sint8)tourneyInfo->fTournamentID + 0x3C6;
            char *format = TextSys::Word(secondmessage);
            sprintf(buffer2, format);
        }
    }
    if (firstmessage != 0)
    {
        FETextRender::WordWrapTextFade(fScreenFadeVal, buffer1, r, textState_Selected, textType_PostGame);
        sint32 height = FETextRender::WordWrapHeight(r.w, buffer1);
        r.y = (sint16)(r.y + (sint16)height);
    }
    if (secondmessage != 0)
        FETextRender::WordWrapTextFade(fScreenFadeVal, buffer2, r, textState_Selected, textType_PostGame);
    tAwardInformation tInfo;
    tournamentManager.GetAwardInformation(tInfo);
    if (tInfo.fAwardCarGarageFull != 0)
    {
        r.x = 260;
        r.y = 200;
        r.w = 240;
        char buffer[256];
        char money[64];
        FeTools::FormatMoney(money, tInfo.fAwardCarBonusMoney);
        char *format = TextSys::Word(0x40);
        sprintf(buffer, format, money);
        char *text = TextSys::Word(0x40);
        FETextRender::WordWrapTextFade(fScreenFadeVal, text, r, textState_Selected, textType_PostGame);
    }
}

void tScreenTournamentTrophy::CalculatePrizes(void)
{
    FUNCTION_MARKER(0x8004923C, "FRONT.BIN");
    congratsMessage = kScreenCongrats_Eliminated;
    trophy = kTrophyNone;
    smallSpinningThing = kSpinningMemCard;
    tAwardInformation tInfo;
    tournamentManager.GetAwardInformation(tInfo);
    sint32 j = 900;
    tTournamentDefinition *definition = tournamentManager.fDefinition;
    tTourneyInfo *tourneyInfo = &definition->fTournaments[definition->fTiers[tournamentManager.fTier].fTournOffset + tournamentManager.fTournament];
    sint32 count = (sint16)((sint16)tournamentManager.fNumRacers + (tourneyInfo->fKnockout != 0));
    for (sint32 i = 1; i <= count; ++i)
        if (tournamentManager.PlayerRanking((sint16)i) == 0)
            j = i;
    if (j == 1)
    {
        if (tInfo.fAwardCar == 0)
        {
            trophy = kTrophyGold;
            smallSpinningThing = kSpinningNone;
        }
        else
        {
            trophy = kTrophyCar;
            smallSpinningThing = kSpinningGold;
        }
        fSpeechToPlay = 15;
        congratsMessage = kScreenCongrats_Congrats;
    }
    else
    {
        if (j < 1 || j > 3 || tourneyInfo->fKnockout != 0)
        {
            congratsMessage = kScreenCongrats_Eliminated;
            trophy = kTrophyCar;
        }
        else
        {
            fSpeechToPlay = j + 14;
            congratsMessage = kScreenCongrats_Congrats;
            trophy = j == 2 ? kTrophySilver : kTrophyBronze;
        }
        smallSpinningThing = kSpinningNone;
    }
    fCarPlayer = 0;
    TotalCash = tournamentManager.fMoney;
    if (tInfo.fCompletedGarageFull != 0)
        TotalCash = (sint32)((uint32)TotalCash - (uint32)tInfo.fCompletedBonusMoney);
    if (tInfo.fTournMoney == 0)
        tInfo.fTournMoney = -1;
    CashAwarded = tInfo.fTournMoney;
    fCarX = 278;
    fCarY = 63;
    fCarCY = -7.4f;
    fCarCX = 4.0;
    if (congratsMessage == kScreenCongrats_Eliminated)
    {
        fCarX = 288;
        fCarY = 73;
        fCarCY = -8.2f;
    }
}

bool tScreenBeTheCopCongrats::GetCar(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x8004947C, "FRONT.BIN");
    tCarInfo *car = carManager.GetCarFromID((uint8)frontEnd.congratsCopCar);
    blockmove(car, &carInfo, 204);
    carInfo.fCountry = frontEnd.congratsCopCountry;
    return true;
}

void tScreenBeTheCopCongrats::CalculatePrizes(void)
{
    FUNCTION_MARKER(0x800494DC, "FRONT.BIN");
    tScreenCongrats::CalculatePrizes();
    trophy = kTrophyCar;
    fCarX = 278;
    fCarY = 75;
    fCarCX = 4.0;
    fCarCY = -7.4f;
}

void tScreenBeTheCopCongrats::DrawCongratsMessage(void)
{
    FUNCTION_MARKER(0x8004952C, "FRONT.BIN");
    sint16 congrats = 0x4C;
    PSX_RECT r = { 41, 60, 200, 200 };
    char buffer[250];
    if ((uint8)frontEnd.congratsCopCar == 28)
        congrats = 0x4D;
    if (PAD_state(4) != 0)
        (void)TextSys::Word((sint8)fCarInfo.fCarID + 0x121);
    char *format = TextSys::Word(congrats);
    char *car = TextSys::Word((sint8)fCarInfo.fCarID + 0x121);
    sprintf(buffer, format, car);
    FETextRender::WordWrapText(buffer, r, textState_Selected, textType_PostGame);
}

bool tScreenTournamentCongrats::GetCar(tCarInfo &carInfo)
{
    FUNCTION_MARKER(0x800495E8, "FRONT.BIN");
    tAwardInformation tInfo;
    tournamentManager.GetAwardInformation(tInfo);
    tCarInfo *car = carManager.GetCarFromID((sint16)tInfo.fCompletedCar);
    blockmove(car, &carInfo, 204);
    return true;
}

void tScreenTournamentCongrats::CalculatePrizes(void)
{
    FUNCTION_MARKER(0x8004963C, "FRONT.BIN");
    tAwardInformation tInfo;
    tournamentManager.GetAwardInformation(tInfo);
    tScreenCongrats::CalculatePrizes();
    trophy = kTrophyCar;
    TotalCash = tournamentManager.fMoney;
    CashAwarded = tInfo.fCompletedGarageFull != 0 ? tInfo.fCompletedBonusMoney : -1;
    fCarX = 278;
    fCarY = 75;
    fCarCX = 4.0;
    fCarCY = -7.4f;
}

void tScreenTournamentCongrats::DrawCongratsMessage(void)
{
    FUNCTION_MARKER(0x800496D4, "FRONT.BIN");
    PSX_RECT r = { 41, 60, 200, 400 };
    tAwardInformation tInfo;
    tournamentManager.GetAwardInformation(tInfo);
    char *text = TextSys::Word(tInfo.fCompletedText);
    FETextRender::WordWrapText(text, r, textState_Selected, textType_PostGame);
    if (tInfo.fCompletedGarageFull != 0)
    {
        r.y = 180;
        r.w = 430;
        char buffer[256];
        char money[64];
        FeTools::FormatMoney(money, tInfo.fCompletedBonusMoney);
        char *format = TextSys::Word(0x40);
        sprintf(buffer, format, money);
        text = TextSys::Word(0x40);
        FETextRender::WordWrapText(text, r, textState_Selected, textType_PostGame);
    }
}
