#include <stddef.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/globals.h"
#include "frontend/common/fefades.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetourn.h"
#include "frontend/common/screentrophyinfo.h"
#include "frontend/common/screentrophyroom.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/psxfront.h"
#include "game/common/nfs3.h"
#include "game/common/textsys.h"

char gSwapFileNameTI[16];
sint32 kBannerColors[4] = { 3948810, 15940, 2171169, 12356 };
tScreenTrophyInfo *screenTrophyInfo;

void tScreenTrophyInfo::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x80041008, "FRONT.BIN");
    tScreenTrophyRoom *room = screenTrophyRoom;
    uint8 currentTier = frontEnd.tier;
    uint8 currentTourn = (uint8)room->fRealCurrentTourn[room->tier];
    tTournamentDefinition *definition = tournamentManager.fDefinition;
    tTourneyInfo *tourn = &definition->fTournaments[definition->fTiers[currentTier].fTournOffset + currentTourn];
    sint32 placement = (sint8)tournamentManager.fBestPlacement[(sint8)tourn->fTournamentID];
    if (placement < 1 || placement > 3)
        placement = 0;
    BannerCol = kBannerColors[placement];
    tournamentManager.GetTrophyName(tourn, ts_Large, gSwapFileNameTI, -1);
    numSwapShapes = 32;
    *swapFileName = gSwapFileNameTI;
    *permFileName = const_cast<char *>("zSTI");
    numPermShapes = 11;
}

void tScreenTrophyInfo::DrawBackground(void)
{
    FUNCTION_MARKER(0x80041138, "FRONT.BIN");
    sint32 screenFade = fScreenFadeVal;
    sint32 FadePartI = screenFade - 64;
    if (FadePartI < 0)
        FadePartI = 0;
    FadePartI *= 2;
    sint32 FadePartIITheRevenge = screenFade * 2;
    if (FadePartIITheRevenge < 0)
        FadePartIITheRevenge = 0;
    if (FadePartIITheRevenge > 128)
        FadePartIITheRevenge = 128;
    tTournamentDefinition *definition = tournamentManager.fDefinition;
    tScreenTrophyRoom *room = screenTrophyRoom;
    uint8 currentTier = frontEnd.tier;
    uint8 currentTourn = (uint8)room->fRealCurrentTourn[room->tier];
    tTourneyInfo *tourn = &definition->fTournaments[definition->fTiers[currentTier].fTournOffset + currentTourn];
    sint32 tournamentID = (sint8)tourn->fTournamentID;
    sint32 col = CalcFadeVal(kRGBVals[(uint8)textDefinitions[textType_ScreenInfo][5]], FadePartI);
    sint32 yyy = 175;
    char *text = TextSys::Word((sint8)tourn->fTournamentID + 0x341);
    FETextRender::FullTextRGB(text, 30, 25, col, 3, 3);
    sint32 word = tournamentID + 0x37A;
    text = TextSys::Word(word);
    if (strlen(text) != 0)
    {
        FETextRender::MenuTextPositionedJustifyFade(FadePartI, 0x3DB, 140, (sint16)yyy, 1, textState_Hilighted, textType_ScreenInfo);
        text = TextSys::Word(word);
        col = CalcFadeVal(0x505050, FadePartI);
        FETextRender::FullTextRGB(text, 145, (sint16)yyy, col, 0, 0);
        yyy = 183;
    }
    PSX_RECT r;
    word = tournamentID + 0x3A0;
    text = TextSys::Word(word);
    if (strlen(text) != 0)
    {
        FETextRender::MenuTextPositionedJustifyFade(FadePartI, 0x3DD, 140, (sint16)yyy, 1, textState_Hilighted, textType_ScreenInfo);
        r.x = 145;
        r.w = 347;
        r.h = 100;
        r.y = (sint16)yyy;
        text = TextSys::Word(word);
        col = CalcFadeVal(0x505050, FadePartI);
        FETextRender::WordWrapTextRGB(text, r, col);
        text = TextSys::Word(word);
        sint32 height = FETextRender::WordWrapHeight(347, text);
        yyy = (sint32)((uint32)yyy + (uint32)height);
    }
    word = tournamentID + 0x38D;
    text = TextSys::Word(word);
    if (strlen(text) != 0)
    {
        FETextRender::MenuTextPositionedJustifyFade(FadePartI, 0x3DC, 140, (sint16)yyy, 1, textState_Hilighted, textType_ScreenInfo);
        r.x = 145;
        r.w = 347;
        r.h = 100;
        r.y = (sint16)yyy;
        text = TextSys::Word(word);
        col = CalcFadeVal(0x505050, FadePartI);
        FETextRender::WordWrapTextRGB(text, r, col);
        text = TextSys::Word(word);
        FETextRender::WordWrapHeight(347, text);
    }
    IsShapeFileLoaded(fSwapShapes);
    if (fSwapShapes.fFile != 0)
        UploadSwapShapes(32);
    r.x = 35;
    r.y = 45;
    r.w = 452;
    r.h = 100;
    text = TextSys::Word(tournamentID + 0x367);
    col = CalcFadeVal(0x505050, FadePartI);
    FETextRender::WordWrapTextRGBJustify(text, r, col, 3, 0, false);
    tDrawShapeExtended drawFlags;
    sint32 tick = ticks;
    drawFlags.custom_shapes = fSwapShapes.fShapes;
    sint32 frame = (tick / 12) % 32;
    ScaleShapeExtended(frame, 0x600, 70, -5, FadePartI, 0, &drawFlags);
    tDrawShapeExtended drawFlags2;
    drawFlags2.tint[0] = BannerCol;
    for (sint32 i = 1; i < 30; ++i)
        if (i % 3 != 0)
            DrawShapeExtended(0, 0x410, i * 2, 0, FadePartIITheRevenge, 0, &drawFlags2);
    for (sint32 i = 34; i < 63; ++i)
        if (i % 3 != 0)
            DrawShapeExtended(0, 0x410, i * 2, 0, FadePartIITheRevenge, 0, &drawFlags2);
}
