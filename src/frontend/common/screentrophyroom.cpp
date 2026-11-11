#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/fefades.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/screentrophyroom.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/nfs3.h"
#include "game/common/textsys.h"

tScreenTrophyRoom *screenTrophyRoom;
char gSwapFileName[16];

tScreenTrophyRoom::tScreenTrophyRoom(void)
{
    FUNCTION_MARKER(0x80040570, "FRONT.BIN");
    fPreviousTrophy = 0;
    fRealCurrentTourn[0] = 0;
    fRealCurrentTourn[1] = 0;
}

tScreenTrophyRoom::~tScreenTrophyRoom(void)
{
    FUNCTION_MARKER(0x800405B0, "FRONT.BIN");
}

void tScreenTrophyRoom::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x800405D8, "FRONT.BIN");
    numPermShapes = 38;
    numSwapShapes = 32;
    uint8 currentTier = frontEnd.tier;
    uint8 currentTourn = (uint8)fRealCurrentTourn[currentTier];
    tTournamentDefinition *definition = tournamentManager.fDefinition;
    tTourneyInfo *tourn = &definition->fTournaments[definition->fTiers[currentTier].fTournOffset + currentTourn];
    tournamentManager.GetTrophyName(tourn, ts_Small, gSwapFileName, -1);
    *permFileName = const_cast<char *>("zTrophy");
    *swapFileName = gSwapFileName;
}

void tScreenTrophyRoom::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x80040698, "FRONT.BIN");
    tInputKeyType key = keyval;
    if (key == kInput_KeyType_Cross)
    {
        tGlobalMenuDefs *defs = menuDefs;
        command.type = kMenu_Command_GoToMenu;
        command.nextMenu = &defs->menuTrophyInfo;
        key = keyval;
    }
    if (key == kInput_KeyType_Up || key == kInput_KeyType_Down)
    {
        sint32 half = fNumTrophies / 2;
        sint16 &selection = fRealCurrentTourn[tier];
        if (selection < half)
        {
            selection = (sint16)(selection + half);
            AudioCmn::PlayFESFX(3);
        }
        else
        {
            selection = (sint16)(selection - half);
            AudioCmn::PlayFESFX(4);
        }
        sint16 &current = fRealCurrentTourn[tier];
        sint32 selected = current;
        if (selected < 0)
            selected = 0;
        if (selected >= fNumTrophies)
            selected = fNumTrophies;
        current = (sint16)selected;
        key = keyval;
    }
    if (key == kInput_KeyType_Left)
    {
        fRealCurrentTourn[tier] = (sint16)(fRealCurrentTourn[tier] - 1);
        if (fRealCurrentTourn[tier] < 0)
            fRealCurrentTourn[tier] = (sint16)(fNumTrophies - 1);
    }
    if (keyval == kInput_KeyType_Right)
    {
        fRealCurrentTourn[tier] = (sint16)(fRealCurrentTourn[tier] + 1);
        if (fRealCurrentTourn[tier] >= fNumTrophies)
            fRealCurrentTourn[tier] = 0;
    }
    tScreen::ProcessInput(fromPlayer, keyval, command);
}

void tScreenTrophyRoom::PreLoad(void)
{
    FUNCTION_MARKER(0x800408A8, "FRONT.BIN");
    tScreen::PreLoad();
    fTrophyShapes.fShapes = 0;
    InitializeShapes(fTrophyShapes, 8);
    char *name = frontEnd.tier == 0 ? const_cast<char *>("zCase") : const_cast<char *>("zCase2");
    AsyncLoadShapeFile(name, fTrophyShapes);
}

void tScreenTrophyRoom::Initialize(void)
{
    FUNCTION_MARKER(0x80040910, "FRONT.BIN");
    tScreen::Initialize();
    bool loaded;
    do
    {
        systemtask(0);
        loaded = IsShapeFileLoaded(fTrophyShapes);
    }
    while (!loaded);
    tier = (uint8)frontEnd.tier;
    fNumTrophies = frontEnd.tier == 0 ? 6 : 8;
    fClearScreen = 1;
    for (sint16 i = 0; i < fNumTrophies; ++i)
    {
        fTrophyList[i] = 1;
        uint8 currentTier = frontEnd.tier;
        tTournamentDefinition *definition = tournamentManager.fDefinition;
        tTourneyInfo *tourn = &definition->fTournaments[definition->fTiers[currentTier].fTournOffset + (uint8)i];
        sint32 placement = (sint8)tournamentManager.fBestPlacement[(sint8)tourn->fTournamentID];
        if (placement < 1 || placement > 3)
            placement = 0;
        FETexture::LoadPmxAtOffset(fTrophyShapes.fFile, i * 4 + placement, fTrophyShapes.fShapes + i, 0, 0);
    }
    purgememadr(fTrophyShapes.fFile);
    fTrophyShapes.fFile = 0;
    fTrophyList[fNumTrophies] = 0;
    sint32 selected = fNumTrophies - 1;
    if (fRealCurrentTourn[tier] < selected)
        selected = fRealCurrentTourn[tier];
    fRealCurrentTourn[tier] = (sint16)selected;
    fPreviousTrophy = (char)fRealCurrentTourn[tier];
    startTicks = ticks;
}

void tScreenTrophyRoom::Cleanup(void)
{
    FUNCTION_MARKER(0x80040AE8, "FRONT.BIN");
    FreeShapes(fTrophyShapes);
    tScreen::Cleanup();
}

void tScreenTrophyRoom::DrawBackground(void)
{
    FUNCTION_MARKER(0x80040B18, "FRONT.BIN");
    tDrawShapeExtended drawFlags3;
    drawFlags3.tint[0] = 0xCEC844;
    sint32 frame = (ticks >> 4) % 10 + 28;
    DrawShapeExtended(frame, 0x410, 16, 16, 0, 0, &drawFlags3);
    sint32 fModNumber = frontEnd.tier == 0 ? 3 : 4;
    sint32 TROPHY_LEFTOFFSET = 276 - ((fModNumber * 95) >> 1);
    DrawBackgroundImage(0, 24, gCurrentShapes, 0);
    PSXDrawBrightEndLine(0x232323, 106, 57, 300, 1, 3, fScreenFadeVal, 30);
    LoadTrophy();
    IsShapeFileLoaded(fSwapShapes);
    if (fSwapShapes.fFile != 0)
    {
        UploadSwapShapes(32);
        startTicks = ticks;
    }
    uint8 currentTier = frontEnd.tier;
    uint8 currentTourn = (uint8)fRealCurrentTourn[tier];
    tTournamentDefinition *definition = tournamentManager.fDefinition;
    tTourneyInfo *tourn = &definition->fTournaments[definition->fTiers[currentTier].fTournOffset + currentTourn];
    sint32 trophyText = (sint8)tourn->fTournamentID + 0x354;
    FETextRender::MenuTextPositionedJustifyFade(fScreenFadeVal, (sint16)trophyText, 256, 47, 2,
        textState_Hilighted, textType_ScreenInfo);
    sint32 texttoshow = 0x3DE;
    if ((gPadinfo.buf[0].ID == 0x23 && (gPadinfo.buf[4].ID == 0x23 || gPadinfo.buf[4].nopad != 0)) ||
        (gPadinfo.buf[4].ID == 0x23 && (gPadinfo.buf[0].ID == 0x23 || gPadinfo.buf[0].nopad != 0)))
        texttoshow = 0x3DF;
    else if (gPadinfo.buf[0].ID == 0x23 || gPadinfo.buf[4].ID == 0x23)
        texttoshow = 0x3E0;
    char *help = TextSys::Word(texttoshow);
    sint32 color = CalcFadeVal(0x505050, fScreenFadeVal);
    FETextRender::FullTextRGB(help, 256, 200, color, 0, 2);
    tDrawShapeExtended drawFlags;
    for (sint16 i = 0; i < fNumTrophies; ++i)
    {
        sint32 x = TROPHY_LEFTOFFSET + (i % fModNumber) * 95;
        sint16 y = (sint16)((i / fModNumber) * 45 + 70);
        if (i == fRealCurrentTourn[tier] && (fSwapShapes.fFlags & 1) != 0)
        {
            drawFlags.custom_shapes = fSwapShapes.fShapes;
            sint32 elapsed = (sint32)((uint32)ticks - (uint32)startTicks);
            frame = (elapsed / 12) % 32;
        }
        else
        {
            frame = i;
            x = (sint16)x;
            drawFlags.custom_shapes = fTrophyShapes.fShapes;
        }
        ScaleShapeExtended(frame, 0x600, x, y, fScreenFadeVal, 0, &drawFlags);
    }
}

void tScreenTrophyRoom::LoadTrophy(void)
{
    FUNCTION_MARKER(0x80040F2C, "FRONT.BIN");
    if (fRealCurrentTourn[tier] != (uint8)fPreviousTrophy)
    {
        uint8 currentTier = frontEnd.tier;
        uint8 currentTourn = (uint8)fRealCurrentTourn[tier];
        tTournamentDefinition *definition = tournamentManager.fDefinition;
        tTourneyInfo *tourn = &definition->fTournaments[definition->fTiers[currentTier].fTournOffset + currentTourn];
        tournamentManager.GetTrophyName(tourn, ts_Small, gSwapFileName, -1);
        AsyncLoadSwapShapeFile(gSwapFileName);
        fPreviousTrophy = (char)fRealCurrentTourn[tier];
    }
}
