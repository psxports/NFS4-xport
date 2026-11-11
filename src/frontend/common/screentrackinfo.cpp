#include <stddef.h>
#include <stdio.h>
#include "xport.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetracks.h"
#include "frontend/common/screentrackinfo.h"
#include "frontend/psx/mmeffect.h"
#include "frontend/psx/psxfront.h"
#include "game/common/nfs3.h"

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

static char gSwapFileName[64];
static sint16 tvOrder[10] = { 2, 7, 4, 8, 1, 6, 0, 9, 3, 5 };
tScreenTrackInfo *screenTrackInfo;

void tScreenTrackInfo::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x800421F8, "FRONT.BIN");
    numPermShapes = 43;
    numSwapShapes = 10;
    tournamentManager.GetTrackToRace(fTrack);
    *permFileName = const_cast<char *>("zInfo");
    sprintf(gSwapFileName, "TR%02d%c", sint32((sint8)fTrack.fTrackNumber),
        fTrack.fTimeOfDay * 2 + fTrack.fWeather + 'a');
    *swapFileName = gSwapFileName;
}

void tScreenTrackInfo::DrawBackground(void)
{
    FUNCTION_MARKER(0x80042298, "FRONT.BIN");
    sint16 trackConditions[4] = { 204, 205, 206, 207 };
    tTrackInformation *trackInfo = trackManager.GetTrackByID((sint8)fTrack.fTrackNumber);
    uint8 tier = frontEnd.tier;
    uint8 tournament = tier == 0 ? frontEnd.tournament : frontEnd.specialevent;
    sint16 *trackList = tournamentManager.GetTrackList(tier, tournament);
    uint32 i = 0;
    uint16 y = 143;
    while (*trackList != 0)
    {
        tMenuTextState textState = i == (uint32)tournamentManager.fCurrentTrack ? textState_Hilighted : textState_Selected;
        FETextRender::MenuTextPositioned(*trackList, 170, (sint16)y, textState, textType_ScreenInfo);
        y = (uint16)(y + 9);
        ++trackList;
        ++i;
    }
    for (i = 0; i < 4; ++i)
        FETextRender::MenuTextPositioned(trackConditions[i], 340, sint16(143 + i * 18), textState_Selected, textType_ScreenInfo);
    FETextRender::MenuTextPositionedJustify(SelectListTrackDirection[fTrack.fDirection], 480, 152, 1, textState_Hilighted, textType_ScreenInfo);
    FETextRender::MenuTextPositionedJustify(SelectListOffOn[fTrack.fMirrored], 480, 170, 1, textState_Hilighted, textType_ScreenInfo);
    FETextRender::MenuTextPositionedJustify(SelectListOffOn[fTrack.fTimeOfDay], 480, 188, 1, textState_Hilighted, textType_ScreenInfo);
    FETextRender::MenuTextPositionedJustify(SelectListOffOn[fTrack.fWeather], 480, 206, 1, textState_Hilighted, textType_ScreenInfo);
    FETextRender::MenuTextPositionedJustify(trackInfo->fSpeedoCountry + 0x43, 478, 33, 1, textState_Unselected, textType_TrackRecords);
    DrawBackgroundImage(0, 33, fPermShapes.fShapes, 0);
    PSXDrawTransSquare(0, 320, 30, 160, 10, 1);
    FeDraw_SetABRMode(0);
    fVideoWall.UpdateTransition();
    fVideoWall.Draw();
}

void tScreenTrackInfo::Initialize(void)
{
    FUNCTION_MARKER(0x80042520, "FRONT.BIN");
    tScreen::Initialize();
    fVideoWall.Initialize(tvConfigs, fSwapShapes.fShapes, 0, 10, tvOrder, 0);
    fVideoWall.UpdateImages();
    fVideoWall.TurnOn();
}

void tScreenTrackInfo::ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x8004258C, "FRONT.BIN");
    if (keyval == kInput_KeyType_Triangle)
    {
        fVideoWall.TurnOffInstant();
        if (tournamentManager.fCurrentTrack == 0)
        {
            tTournamentDefinition *definition = tournamentManager.fDefinition;
            sint32 tournament = definition->fTiers[tournamentManager.fTier].fTournOffset + tournamentManager.fTournament;
            uint32 entranceFee = definition->fTournaments[tournament].fEntranceFee;
            tournamentManager.fMoney = (sint32)((uint32)tournamentManager.fMoney + entranceFee);
        }
    }
}
