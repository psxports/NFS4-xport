#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include "xport.h"
#include "eaclib/psx/globals.h"
#include "frontend/common/fefades.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetools.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/fetracks.h"
#include "frontend/common/front.h"
#include "frontend/common/screenpost.h"
#include "frontend/common/stattool.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/nfs3.h"
#include "game/common/textsys.h"
#include "game/psx/textpix.h"

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

tScreenTournamentStandings *screenTournamentStandings;
tScreenTournamentStandings3item *screenTournamentStandings3item;
tScreenPinkSlipStandings *screenPinkSlipStandings;

void DrawMoney(sint32 x, sint32 y, sint32 numplaces, sint32 number, sint32 colfore, sint32 colback)
{
    FUNCTION_MARKER(0x800397F0, "FRONT.BIN");
    char string1[50];
    char string2[50];
    if (numplaces == 6)
        sprintf(string1, TextSys::Word(135), 0, 0);
    else if (numplaces == 3)
        sprintf(string1, TextSys::Word(134), 0);
    else if (numplaces == 9)
        sprintf(string1, TextSys::Word(136), 0, 0, 0);
    FeTools::FormatMoney(string2, number);
    FETextRender::FullTextRGB(string2, (sint16)x, (sint16)y, colfore, 0, 1);
    FETextRender::FullTextRGB(string1, (sint16)x, (sint16)y, colback, 0, 1);
}

tScreenTournamentStandings::tScreenTournamentStandings(void)
{
    FUNCTION_MARKER(0x80039928, "FRONT.BIN");
    fDrawMoney = 0;
}

void tScreenTournamentStandings::Initialize(void)
{
    FUNCTION_MARKER(0x80039960, "FRONT.BIN");
    tScreen::Initialize();
    tAwardInformation tInfo;
    tournamentManager.GetAwardInformation(tInfo);
    gotmoney = 0;
    gotbonus = 0;
    gotbilled = 0;
    if (fDrawMoney == 0)
        moneyFinal = (sint32)(uint32(tournamentManager.fMoney) - uint32(tInfo.fTournMoney));
    else
    {
        if (tInfo.fMoney != 0)
            gotmoney = 1;
        moneyFinal = (sint32)(uint32(tournamentManager.fMoney) - uint32(tInfo.fTournMoney));
        if (tInfo.fCompletedGarageFull != 0)
            moneyFinal = (sint32)(uint32(moneyFinal) - uint32(tInfo.fCompletedBonusMoney));
        moneyAwarded = tInfo.fMoney;
        tournamentManager.CalcTrackFinishDamageBill(false, moneyDamage, moneyBonus);
        if (moneyBonus > 0)
            gotbonus = 1;
        if (moneyDamage > 0)
            gotbilled = 1;
        fCountedDown = 0;
        if (moneyBonus == 0 && moneyDamage == 0 && moneyAwarded == 0)
            fCountedDown = 1;
        fStartCountdownNOW = 0;
        sint32 maximum = moneyBonus;
        if (maximum < tInfo.fMoney)
            maximum = tInfo.fMoney;
        sint32 maximumBill = moneyDamage;
        if (maximumBill < maximum)
            maximumBill = maximum;
        fCountSpeed = maximumBill / 24;
    }
    starttick = ticks;
}

void tScreenTournamentStandings::Cleanup(void)
{
    FUNCTION_MARKER(0x80039AEC, "FRONT.BIN");
    fDrawMoney = 0;
    tScreen::Cleanup();
}

void tScreenTournamentStandings::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x80039B0C, "FRONT.BIN");

    numSwapShapes = 0;
    *swapFileName = 0;
    numPermShapes = 41;
    *permFileName = const_cast<char *>("zTrnSt1");
}

void tScreenTournamentStandings3item::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x80039B30, "FRONT.BIN");

    numSwapShapes = 0;
    *swapFileName = 0;
    numPermShapes = 41;
    *permFileName = const_cast<char *>("zTrnStd");
}

void tScreenTournamentStandings::ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x80039B54, "FRONT.BIN");
    if (keyval == kInput_KeyType_Cross && fCountedDown == 0)
    {
        keyval = kInput_KeyType_AlreadyProcessed;
        fStartCountdownNOW = 1;
    }
}

void tScreenTournamentStandings::DrawBackground(void)
{
    FUNCTION_MARKER(0x80039B84, "FRONT.BIN");
    sint32 fade = fScreenFadeVal;
    sint32 fadeline = fade;
    sint32 row = 766;
    sint32 index = (uint8)tournamentManager.fDefinition->fTiers[tournamentManager.fTier].fTournOffset +
        tournamentManager.fTournament;
    tTourneyInfo *tourneyInfo = &tournamentManager.fDefinition->fTournaments[index];
    sint32 numRacers = (sint16)((sint16)tournamentManager.fNumRacers + ((uint8)tourneyInfo->fKnockout != 0));
    sint32 lastRacer = numRacers - 1;
    for (sint32 i = 0; i < numRacers; ++i, ++row)
    {
        sint16 p = tournamentManager.PlayerRanking((sint16)(i + 1));
        tMenuTextState state = p == 0 ? textState_Hilighted : textState_Selected;
        tMenuTextState statedull = p == 0 ? textState_Hilighted : textState_Unselected;
        char *text = TextSys::Word(i + 599);
        sint16 x = (sint16)TextSys::WordX(759);
        sint16 y = (sint16)TextSys::WordY(row);
        FETextRender::FullTextFade(fade, text, x, y, textType_TrackRecords, statedull, 0);
        text = p == 0 ? PlayerName(0) : Stattool::GetAINameFromPersonality(tournamentManager.fCompetitors[p].fPersonality);
        x = (sint16)TextSys::WordX(760);
        y = (sint16)TextSys::WordY(row);
        FETextRender::FullTextFade(fade, text, x, y, textType_TrackRecords, state, 0);
        char sBuildOutput[80];
        if ((uint8)tourneyInfo->fKnockout == 0)
        {
            sint16 points = tournamentManager.TournPointTotal(&p);
            char *units = TextSys::Word(797);
            sprintf(sBuildOutput, "%d %s", sint32(points), units);
        }
        else
            sprintf(sBuildOutput, TextSys::Word(i == lastRacer ? 796 : 795));
        x = (sint16)TextSys::WordX(763);
        y = (sint16)TextSys::WordY(row);
        FETextRender::FullTextFade(fade, sBuildOutput, x, y, textType_TrackRecords, state, 1);
    }
    tTrackInformation trackInfo;
    trackManager.GetTrack(Front::GetTrackRaced(), trackInfo);
    char *text = TextSys::Word(Front::GetTrackRaced() + 213);
    sint16 x = (sint16)TextSys::WordX(758);
    sint16 y = (sint16)TextSys::WordY(765);
    FETextRender::FullTextFade(fade, text, x, y, textType_TrackRecords, textState_Hilighted, 2);
    tListIteratorTournament *iterator = frontEnd.specialevent != 0 ? &menuDefs->iteratorSpecialEvent : &menuDefs->iteratorTournament;
    sint32 title = iterator->tListIteratorTournament::TextValue(kPlayerBoth) + 19;
    x = (sint16)TextSys::WordX(758);
    y = (sint16)TextSys::WordY(764);
    FETextRender::MenuTextPositionedJustifyFade(fade, (sint16)title, x, y, 2, textState_Hilighted, textType_TrackRecords);
    sint32 wwwww = textpixels(TextSys::Word(title));
    sint32 titleX = TextSys::WordX(758);
    sint32 titleY = TextSys::WordY(764);
    PSXDrawSquare(0, titleX - (wwwww >> 1), titleY - 1, wwwww, 9);
    tTexture::ShapeInfo *shape = &gCurrentShapes[39];
    sint32 width = shape->width;
    sint32 now = ticks;
    if (width == 0 || (width == -1 && now == INT_MIN))
        abort();
    sint32 tt = now % width;
    sint32 lbx = (width >> 1) - 2 - shape->centerx;
    if (width / 2 < tt)
        tt = width - tt;
    sint32 glintY = TextSys::WordY(764);
    DrawShapeExtended(40, 0, lbx + tt, glintY + 1, fScreenFadeVal, 1, 0);
    glintY = TextSys::WordY(764);
    DrawShapeExtended(40, 0, lbx - tt, glintY + 1, fScreenFadeVal, 1, 0);
    tDrawShapeExtended drawflags;
    drawflags.tint[0] = 0x282828;
    DrawShapeExtended(39, 0x400, 0, -1, fScreenFadeVal, 0, &drawflags);
    sint32 lineX = TextSys::WordX(758);
    sint32 lineY = TextSys::WordY(765);
    PSXDrawBrightEndLine(0x232323, lineX - 150, lineY + 10, 300, 1, 3, fadeline, 30);
    sint32 colf = CalcFadeVal(kRGBVals[(uint8)textDefinitions[11][5]], fade);
    sint32 colb = CalcFadeVal(0x232323, fade);
    if ((sint32)(uint32(ticks) - uint32(starttick)) > 1000 || fStartCountdownNOW != 0)
    {
        if (moneyAwarded > 0 || moneyDamage > 0 || moneyBonus > 0)
            AudioCmn::PlayFESFX(21);
        sint32 speed = fCountSpeed;
        moneyAwarded = (sint32)(uint32(moneyAwarded) - uint32(speed));
        if (moneyAwarded <= 0)
        {
            moneyAwarded = 0;
            moneyDamage = (sint32)(uint32(moneyDamage) - uint32(speed));
            if (moneyDamage <= 0)
            {
                sint32 bonus = moneyBonus;
                fCountedDown = 1;
                bonus = (sint32)(uint32(bonus) - uint32(speed));
                moneyDamage = 0;
                if (bonus < 0)
                    bonus = 0;
                moneyBonus = bonus;
            }
        }
    }
    if (fDrawMoney != 0)
    {
        text = TextSys::Word(786);
        x = (sint16)TextSys::WordX(762);
        y = (sint16)TextSys::WordY(786);
        FETextRender::FullTextFade(fade, text, x, y, textType_TrackRecords,
            gotmoney != 0 ? textState_Selected : textState_Unselected, 1);
        sint32 moneyX = TextSys::WordX(763);
        sint32 moneyY = TextSys::WordY(786);
        DrawMoney(moneyX, moneyY, 6, moneyAwarded, colf, colb);
        text = TextSys::Word(787);
        x = (sint16)TextSys::WordX(762);
        y = (sint16)TextSys::WordY(787);
        FETextRender::FullTextFade(fade, text, x, y, textType_TrackRecords,
            gotbilled != 0 ? textState_Selected : textState_Unselected, 1);
        moneyX = TextSys::WordX(763);
        moneyY = TextSys::WordY(787);
        DrawMoney(moneyX, moneyY, 6, moneyDamage, colf, colb);
        text = TextSys::Word(788);
        x = (sint16)TextSys::WordX(762);
        y = (sint16)TextSys::WordY(788);
        FETextRender::FullTextFade(fade, text, x, y, textType_TrackRecords,
            gotbonus != 0 ? textState_Selected : textState_Unselected, 1);
        moneyX = TextSys::WordX(763);
        moneyY = TextSys::WordY(788);
        DrawMoney(moneyX, moneyY, 6, moneyBonus, colf, colb);
    }
    text = TextSys::Word(789);
    x = (sint16)TextSys::WordX(762);
    y = (sint16)TextSys::WordY(789);
    FETextRender::FullTextFade(fade, text, x, y, textType_TrackRecords, textState_Hilighted, 1);
    sint32 moneyX = TextSys::WordX(763);
    sint32 moneyY = TextSys::WordY(789);
    sint32 total = (sint32)(uint32(moneyFinal) - uint32(moneyAwarded) + uint32(moneyDamage) - uint32(moneyBonus));
    DrawMoney(moneyX, moneyY, 9, total, colf, colb);
    DrawBackgroundImage(10, 29, gCurrentShapes, 0);
}

void tScreenPinkSlipStandings::DrawBackground(void)
{
    FUNCTION_MARKER(0x8003A448, "FRONT.BIN");
    sint32 fade = fScreenFadeVal;
    sint32 row = 766;
    for (sint32 i = 0; i < (uint8)frontEnd.pinkSlipsNumTracks; ++i, ++row)
    {
        tMenuTextState state = textState_Hilighted;
        if (i != (uint8)frontEnd.pinkSlipsTrackIndex)
            state = i < (uint8)frontEnd.pinkSlipsTrackIndex ? textState_Selected : textState_Unselected;
        tTrackInformation trackInfo;
        trackManager.GetTrack((uint8)frontEnd.track[i], trackInfo);
        sint16 x = (sint16)TextSys::WordX(759);
        sint16 y = (sint16)TextSys::WordY(row);
        FETextRender::MenuTextPositionedJustifyFade(fScreenFadeVal, (sint16)((sint8)trackInfo.fTrackID + 213),
            x, y, 0, state, textType_TrackRecords);
        sint16 winner = frontEnd.pinkSlipsWinner[i];
        char *text = winner == -1 ? TextSys::Word(781) : PlayerName(winner);
        char string[30];
        sprintf(string, text);
        x = (sint16)TextSys::WordX(763);
        y = (sint16)TextSys::WordY(row);
        FETextRender::FullTextFade(fade, string, x, y, textType_TrackRecords, state, 1);
    }
    for (sint32 i = 0; i < 2; ++i)
    {
        sint32 word = 787 + i;
        char *text = PlayerName(i);
        sint16 x = (sint16)TextSys::WordX(760);
        sint16 y = (sint16)TextSys::WordY(word);
        FETextRender::FullTextFade(fade, text, x, y, textType_TrackRecords, textState_Hilighted, 0);
        char sBuildOutput[50];
        if ((uint8)frontEnd.pinkSlipsWins[i] == 1)
            sprintf(sBuildOutput, TextSys::Word(799));
        else
        {
            text = TextSys::Word(798);
            sprintf(sBuildOutput, text, (uint8)frontEnd.pinkSlipsWins[i]);
        }
        x = (sint16)TextSys::WordX(763);
        y = (sint16)TextSys::WordY(word);
        FETextRender::FullTextFade(fade, sBuildOutput, x, y, textType_TrackRecords, textState_Hilighted, 1);
    }
    sint16 x = (sint16)TextSys::WordX(758);
    sint16 y = (sint16)TextSys::WordY(764);
    FETextRender::MenuTextPositionedJustifyFade(fade, 705, x, y, 2, textState_Hilighted, textType_TrackRecords);
    sint32 wwwww = textpixels(TextSys::Word(705));
    sint32 titleX = TextSys::WordX(758);
    sint32 titleY = TextSys::WordY(764);
    PSXDrawSquare(0, titleX - (wwwww >> 1), titleY - 1, wwwww, 9);
    tTexture::ShapeInfo *shape = &gCurrentShapes[39];
    sint32 width = shape->width;
    sint32 now = ticks;
    if (width == 0 || (width == -1 && now == INT_MIN))
        abort();
    sint32 tt = now % width;
    sint32 lbx = (width >> 1) - 2 - shape->centerx;
    if (width / 2 < tt)
        tt = width - tt;
    tDrawShapeExtended drawflags;
    drawflags.tint[0] = 0x282828;
    sint32 glintY = TextSys::WordY(764);
    DrawShapeExtended(40, 0, lbx + tt, glintY + 1, fScreenFadeVal, 1, 0);
    glintY = TextSys::WordY(764);
    DrawShapeExtended(40, 0, lbx - tt, glintY + 1, fScreenFadeVal, 1, 0);
    DrawShapeExtended(39, 0x400, 0, -1, fScreenFadeVal, 0, &drawflags);
    DrawBackgroundImage(10, 29, gCurrentShapes, 0);
}
