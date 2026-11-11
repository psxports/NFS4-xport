#include <stddef.h>
#include <stdio.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/globals.h"
#include "frontend/common/fefades.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetv.h"
#include "frontend/common/frontmovie.h"
#include "frontend/common/screenpost.h"
#include "frontend/common/screentournselect.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/mmeffect.h"
#include "frontend/psx/psxfront.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "game/common/textsys.h"

tScreenTournSelect *screenTournSelect;
static char gSwapFileName[16];
static char trophyTVOrder[4] = { 1, 2, 0, 3 };

tScreenTournSelect::tScreenTournSelect(void)
{
    FUNCTION_MARKER(0x8003F778, "FRONT.BIN");
}

tScreenTournSelect::~tScreenTournSelect(void)
{
    FUNCTION_MARKER(0x8003F7AC, "FRONT.BIN");
}

void tScreenTournSelect::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x8003F7D4, "FRONT.BIN");
    uint8 currentTourn = frontEnd.tier == 0 ? frontEnd.tournament : frontEnd.specialevent;
    uint8 currentTier = frontEnd.tier;
    tTournamentDefinition *definition = tournamentManager.fDefinition;
    sint32 index = definition->fTiers[currentTier].fTournOffset + currentTourn;
    numPermShapes = 64;
    numSwapShapes = 32;
    *permFileName = const_cast<char *>("ztourn");
    tournamentManager.GetTrophyName(&definition->fTournaments[index], ts_Medium, gSwapFileName, -1);
    *swapFileName = gSwapFileName;
}

void tScreenTournSelect::Initialize(void)
{
    FUNCTION_MARKER(0x8003F8A0, "FRONT.BIN");
    PreCalculatedTournamentY = -1;
    fPrevi = -1;
    PSX_RECT r = { 512, 0, 80, 160 };
    ClearImage(&r, 0, 0, 0);
    DrawSync(0);
    tScreen::Initialize();
    fCurrentMovie = 0;
    fPreviousMovie = 0;
    char moviename[80];
    sprintf(moviename, "%szzzTRN.dct", Paths_Paths[41]);
    InitializeFrontMovie(hVideo);
    if (!FrontMovieNameMatches(moviename, "ZZZZTRN.DCT") || !StartFrontMovie(hVideo, "DATA/ZZZZTRN.DCT", "VIDEO/native/ZZZZTRN.STR"))
    {
#ifdef _DEBUG
        fprintf(stderr, "tournament movie: open failed\n");
#endif
    }
    fFrame = 0;
    for (sint16 i = 0; i < 2; ++i)
    {
        for (sint16 j = 0; j < 2; ++j)
        {
            tTVConfig &tv = trophyTV[i * 2 + j];
            InitTV(tv, fPermShapes.fShapes, 0);
            tv.y = (sint16)(j * 37 + 142);
            tv.w = 76;
            uint16 flags = tv.flags;
            tv.x = (sint16)(i * 76 + 165);
            tv.h = 37;
            tv.flags = flags | 0x30;
        }
    }
    uint8 currentTourn = frontEnd.tier == 0 ? frontEnd.tournament : frontEnd.specialevent;
    uint8 currentTier = frontEnd.tier;
    tTournamentDefinition *definition = tournamentManager.fDefinition;
    fPreviousTrophy = definition->fTournaments[definition->fTiers[currentTier].fTournOffset + currentTourn].fTrophyID;
    uint32 tick = ticks;
    fTransitionDirection = 1;
    fTVTicks = tick;
}

void tScreenTournSelect::Cleanup(void)
{
    FUNCTION_MARKER(0x8003FAE8, "FRONT.BIN");
    StopFrontMovie(hVideo);
    tScreen::Cleanup();
}

void tScreenTournSelect::UpdateVideoWall(tTourneyInfo *tourn)
{
    FUNCTION_MARKER(0x8003FB28, "FRONT.BIN");
    if (tourn->fTrophyID != (sint8)fPreviousTrophy)
    {
        tournamentManager.GetTrophyName(tourn, ts_Medium, gSwapFileName, -1);
        AsyncLoadSwapShapeFile(gSwapFileName);
        fTVsInitialized = 0;
        fPreviousTrophy = tourn->fTrophyID;
        if (fTransitionDirection >= 0)
        {
            uint32 tick = ticks;
            fTransitionDirection = -1;
            fTVTicks = tick;
        }
    }
}

void tScreenTournSelect::DrawVideoWall(void)
{
    FUNCTION_MARKER(0x8003FBCC, "FRONT.BIN");
    tDrawShapeExtended drawFlags;
    drawFlags.custom_shapes = fSwapShapes.fShapes;
    DrawBackgroundImage(0, 34, fPermShapes.fShapes, 0);
    for (sint32 j = 244; j < 485; j += 80)
        PSXDrawTransSquare(0x202020, j, 41, 2, 97, 1);
    for (sint32 j = 89; j < 137; j += 48)
        PSXDrawTransSquare(0x141414, 165, j, 321, 1, 1);
    FeDraw_SetABRMode(2);
    sint32 i = ((uint32)ticks - (uint32)fTVTicks) >> 2;
    if (fTransitionDirection > 0)
    {
        for (sint32 j = 0; j < 4 && j < i; ++j)
            if (trophyTV[trophyTVOrder[j]].state == tv_StateOff)
                TurnOnTV(trophyTV[trophyTVOrder[j]]);
    }
    else
    {
        for (sint32 j = 0; j < 4 && j < i; ++j)
            if (trophyTV[trophyTVOrder[j]].state == tv_StateOn)
                TurnOffTV(trophyTV[trophyTVOrder[j]]);
        if (i >= 4)
            fTransitionDirection = 0;
    }
    PSXDrawTransSquare(0x303030, 241, 142, 2, 74, 1);
    PSXDrawTransSquare(0x202020, 165, 179, 152, 1, 1);
    FeDraw_SetABRMode(2);
    for (sint32 j = 0; j < 4; ++j)
        DrawTV(trophyTV[j]);
    sint32 frame = (ticks >> 4) % 32;
    ScaleShapeExtended(frame, 0x600, 182, 147, 0, 0, &drawFlags);
}

void tScreenTournSelect::DrawBackground(void)
{
    FUNCTION_MARKER(0x8003FE5C, "FRONT.BIN");
    uint8 currentTourn = frontEnd.tier == 0 ? frontEnd.tournament : frontEnd.specialevent;
    uint8 currentTier = frontEnd.tier;
    tTournamentDefinition *definition = tournamentManager.fDefinition;
    tTourneyInfo *tourn = &definition->fTournaments[definition->fTiers[currentTier].fTournOffset + currentTourn];
    sint32 YellowCol = CalcFadeVal(0xBEBE, fScreenFadeVal);
    sint32 DarkGreyCol = CalcFadeVal(0x232323, fScreenFadeVal);
    sint32 GreyCol = CalcFadeVal(0x505050, fScreenFadeVal);
    sint32 number = tournamentManager.fMoney;
    FETextRender::MenuTextFade(fScreenFadeVal, 0x7B, textState_Selected, textType_ScreenInfo);
    sint32 x = TextSys::WordX(0x7B) + 140;
    sint32 moneyY = TextSys::WordY(0x7B) + 9;
    DrawMoney(x, moneyY, 6, number, YellowCol, DarkGreyCol);
    FETextRender::MenuTextFade(fScreenFadeVal, 0x99, textState_Selected, textType_Default);
    x = TextSys::WordX(0x99) + 140;
    moneyY = TextSys::WordY(0x99) + 9;
    DrawMoney(x, moneyY, 6, tourn->fEntranceFee, YellowCol, DarkGreyCol);
    FETextRender::MenuTextFade(fScreenFadeVal, 0x9A, textState_Selected, textType_Default);
    sint32 nextY = TextSys::WordY(0x9A) + 9;
    for (sint16 i = 0; i < 3; ++i)
    {
        sint16 y = (sint16)nextY;
        char *label = TextSys::Word(i + 0x2D4);
        sint16 labelX = TextSys::WordX(0x9A);
        FETextRender::FullTextRGB(label, labelX, y, GreyCol, 0, 0);
        x = TextSys::WordX(0x99) + 140;
        DrawMoney(x, y, 6, tourn->fPrize[i], YellowCol, DarkGreyCol);
        nextY += 9;
    }
    UpdateVideoWall(tourn);
    IsShapeFileLoaded(fSwapShapes);
    if (fSwapShapes.fFile != 0 && fTransitionDirection >= 0)
    {
        UploadSwapShapes(32);
        uint32 tick = ticks;
        fTransitionDirection = 1;
        fTVTicks = tick;
    }
    DrawVideoWall();
    sint32 moviePageFrame = fFrame;
    sint16 shapeY = (moviePageFrame & 1) == 0 ? 80 : 0;
    uint32 movieRGB = 0x2C1E1E;
    for (sint16 i = 0; i < 2; ++i)
    {
        for (sint16 j = 0; j < 4; ++j)
        {
            tTVConfig &tv = tvConfigs[i * 4 + j];
            tv.x = (sint16)(j * 80 + 165);
            tv.w = 80;
            tv.h = 48;
            tv.uw = 20;
            tv.y = (sint16)(i * 48 + 41);
            tv.u = (uint8)(j * 20);
            tv.v = (uint8)(i * 40);
            tv.vh = 40;
            tv.tpage = GetTPage(2, 0, 512, shapeY);
            tv.state = tv_StateOn;
            tv.clut = 0;
            tv.flags = 0x22;
            tv.tint = movieRGB;
            tv.destBrightness = 128;
            tv.transition = 128;
        }
    }
    if (hVideo == 0)
    {
        fCurrentMovie = 0;
        char moviename[80];
        sprintf(moviename, "%szzzTRN.dct", Paths_Paths[41]);
        if (!FrontMovieNameMatches(moviename, "ZZZZTRN.DCT") || !StartFrontMovie(hVideo, "DATA/ZZZZTRN.DCT", "VIDEO/native/ZZZZTRN.STR"))
        {
#ifdef _DEBUG
            fprintf(stderr, "tournament movie: open failed\n");
#endif
        }
    }
    else
    {
        sint32 result = UploadFrontMovieFrame(hVideo, moviePageFrame);
        if (result == 1)
        {
            hVideo = 2;
            fFrame = (sint32)((uint32)fFrame + 1);
        }
        else if (result < 0)
        {
#ifdef _DEBUG
            fprintf(stderr, "tournament movie: frame delivery failed\n");
#endif
        }
    }
    for (sint16 i = 0; i < 8; ++i)
        DrawTVLines(tvConfigs[i]);
    PSX_RECT r = { 325, 43, 314, 10 };
    tListIteratorTournament *iterator = frontEnd.tier == 0 ? &menuDefs->iteratorTournament : &menuDefs->iteratorSpecialEvent;
    sint16 title = iterator->TextValue(kPlayerBoth);
    FETextRender::WordWrapFade(fScreenFadeVal, title, r, textState_Hilighted, textType_VideoWall);
    r.x = 170;
    r.w = (sint16)(r.w - 10);
    iterator = frontEnd.tier == 0 ? &menuDefs->iteratorTournament : &menuDefs->iteratorSpecialEvent;
    sint32 textValue = iterator->TextValue(kPlayerBoth);
    sint16 descriptionWord = (sint16)(textValue + 0x26);
    if (descriptionWord != fPrevi || PreCalculatedTournamentY == -1)
    {
        fPrevi = descriptionWord;
        char *description = TextSys::Word(descriptionWord);
        sint32 height = FETextRender::WordWrapHeight(r.w, description);
        PreCalculatedTournamentY = (sint32)((uint32)117 - (uint32)height);
    }
    r.y = (sint16)PreCalculatedTournamentY;
    char *description = TextSys::Word(descriptionWord);
    sint32 color = CalcFadeVal(0x505050, fScreenFadeVal);
    FETextRender::WordWrapTextRGB(description, r, color);
    FETextRender::MenuTextPositionedJustifyFade(fScreenFadeVal, 0x3DB, 170, 117, 0, textState_Selected, textType_ScreenInfo);
    FETextRender::MenuTextPositionedJustifyFade(fScreenFadeVal, (sint16)(textValue + 0x39), 170, 125, 0, textState_Hilighted, textType_ScreenInfo);
    for (sint16 i = 0; i < 8; ++i)
        DrawTV(tvConfigs[i]);
}

void tScreenTournSelect::DrawForeground(void)
{
    FUNCTION_MARKER(0x800404D8, "FRONT.BIN");
    PSXDrawBrightEndLine(0x232323, 167, 41, 316, 1, 3, fScreenFadeVal, 20);
    PSXDrawBrightEndLine(0x232323, 167, 74, 316, 1, 2, fScreenFadeVal, 20);
}
