#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "frontend/common/fefades.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/front.h"
#include "frontend/common/screentrackrecords.h"
#include "frontend/common/statchk.h"
#include "frontend/common/stattool.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/psxfront.h"
#include "game/common/textsys.h"
#include "game/psx/flare.h"
#include "game/psx/textpix.h"

struct tRecordBuffer
{
    char sName[8];
    sint32 nCar;
    sint32 nTime;
    sint32 nBestLap;
};

tScreenTrackRecords *screenTrackRecords;

void tScreenTrackRecords::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x80042648, "FRONT.BIN");
    numPermShapes = 40;
    numSwapShapes = 0;
    *permFileName = const_cast<char *>("zRec");
    *swapFileName = 0;
}

void tScreenTrackRecords::Initialize(void)
{
    FUNCTION_MARKER(0x8004266C, "FRONT.BIN");
    flareextra = 0;
    tScreen::Initialize();
    TrackRecords = static_cast<tRecordBuffer *>(reservememadr("trkrcrds", 360, 16));
    sint16 nShowTrack = Front::GetTrackRaced();
    Stattool::GetRecords(nShowTrack, TrackRecords);
    fReadNewData = false;
}

void tScreenTrackRecords::Cleanup(void)
{
    FUNCTION_MARKER(0x800426C4, "FRONT.BIN");
    purgememadr(TrackRecords);
    TrackRecords = 0;
    StatChk::ClearNewRecords();
    tScreen::Cleanup();
}

void tScreenTrackRecords::DrawOneRecord(sint32 index, bool newrecord, sint32 y)
{
    FUNCTION_MARKER(0x80042700, "FRONT.BIN");
    tMenuTextState textState = newrecord ? textState_Hilighted : textState_Unselected;
    if (TrackRecords[index].nBestLap < 0)
        return;
    if (newrecord)
    {
        sint32 x = TextSys::WordX(0x247);
        sint32 intensity = flare_intensity;
        sint32 doubleIntensity = (sint32)((uint32)intensity << 1);
        Flare::_2DHalo(x + 3, (sint32)((uint32)y + 4), intensity / 2, doubleIntensity / 3, 0x17);
        x = TextSys::WordX(0x247);
        DrawShapeExtended(0, 0, x, y, 0, 0, 0);
    }
    sint16 x = TextSys::WordX(0x24B);
    FETextRender::FullText(TrackRecords[index].sName, x, (sint16)y, textType_TrackRecords,
        newrecord ? textState_Hilighted : textState_Selected, 0);
    char *sMenuText = TextSys::Word((sint32)((uint32)TrackRecords[index].nCar + 0x153));
    x = TextSys::WordX(0x24C);
    FETextRender::FullText(sMenuText, x, (sint16)y, textType_TrackRecords, textState, 0);
    char sBuildOutput[80];
    sint32 time = TrackRecords[index].nTime;
    if (time != 0)
        Stattool::ParseTime(time, sBuildOutput);
    else
        sprintf(sBuildOutput, TextSys::Word(0x261));
    x = TextSys::WordX(0x24D);
    FETextRender::FullText(sBuildOutput, x, (sint16)y, textType_TrackRecords,
        newrecord ? textState_Hilighted : (index != 0 ? textState_Selected : textState_Unselected), 0);
    Stattool::ParseTime(TrackRecords[index].nBestLap, sBuildOutput);
    x = TextSys::WordX(0x24E);
    if (index == 0 && textState == textState_Unselected)
        textState = textState_Selected;
    FETextRender::FullText(sBuildOutput, x, (sint16)y, textType_TrackRecords, textState, 0);
}

void tScreenTrackRecords::DrawRecords(sint16 maxitem)
{
    FUNCTION_MARKER(0x80042968, "FRONT.BIN");
    flareextra = (sint32)((uint32)flareextra + 1);
    if (flareextra > 60)
        flareextra = 0;
    sint32 intensity = flareextra > 30 ? 60 - flareextra : flareextra;
    flare_intensity = intensity / 2;
    flare_intensity = (sint32)(((uint32)flare_intensity + 20) << 7);
    sint16 nLapIndicator = Front::GetLapsForType() == 2 ? 1 : 9;
    for (sint16 k = 0; k < 8 && k < maxitem; ++k)
    {
        char *sMenuText = TextSys::Word(k + 0x257);
        sint16 x = TextSys::WordX(0x249);
        sint16 y = TextSys::WordY(k + 0x257);
        tMenuTextState textState = NewRecords[k] != 0 ? textState_Hilighted : textState_Unselected;
        FETextRender::FullText(sMenuText, x, y, textType_TrackRecords, textState, 0);
        sint32 recordY = TextSys::WordY(k + 0x257);
        DrawOneRecord(k + nLapIndicator, NewRecords[k] != 0, recordY);
    }
    if (maxitem > 8)
    {
        sint32 y = TextSys::WordY(0x260);
        DrawOneRecord(0, NewBestLap != 0, y);
    }
}

void tScreenTrackRecords::DrawBackground(void)
{
    FUNCTION_MARKER(0x80042B48, "FRONT.BIN");
    sint32 lbx = fScreenFadeVal * 308 / 128;
    sint32 fade = lbx - 180;
    if (fade < 0)
        fade = 0;
    if (fade > 128)
        fade = 128;
    sint32 progress = lbx;
    if (progress < 0)
        progress = 0;
    if (progress > 180)
        progress = 180;
    sint32 lineFade = lbx * 128 / 180;
    if (lineFade < 0)
        lineFade = 0;
    if (lineFade > 128)
        lineFade = 128;
    sint16 linefadeval = lineFade;
    sint16 maxitem = (180 - progress) / 20;
    sint16 boxx = TextSys::WordX(0x248);
    sint16 boxy = TextSys::WordY(0x256);
    sint16 boxw = (sint16)(TextSys::WordX(0x24F) - boxx);
    sint16 midy = TextSys::WordY(0x25F);
    sint32 ColTextSel = CalcFadeVal(kRGBVals[(uint8)textDefinitions[textType_TrackRecords][4]], fade);
    sint32 ColTextBright = CalcFadeVal(kRGBVals[(uint8)textDefinitions[textType_TrackRecords][5]], fade);
    DrawRecords(maxitem);
    char string[50];
    char string2[50];
    char *format = TextSys::Word(0x251);
    sint32 laps = Front::GetLapsForType();
    sprintf(string2, format, laps);
    sint16 j = Front::GetTrackRaced();
    char *trackName = TextSys::Word(j + 0xD5);
    sprintf(string, "%s %s", trackName, string2);
    sint16 titleY = TextSys::WordY(0x255);
    FETextRender::FullTextRGB(string, 260, titleY, ColTextBright, 0, 2);
    sint32 titleWidth = textpixels(string);
    sint32 titleX = 260 - (titleWidth >> 1);
    sint32 squareY = TextSys::WordY(0x255);
    titleWidth = textpixels(string);
    PSXDrawSquare(0, titleX, squareY, titleWidth, 9);
    tTexture::ShapeInfo *shape = &gCurrentShapes[38];
    sint32 width = shape->width;
    sint32 tick = ticks;
    if (width == 0 || (width == -1 && tick == (-2147483647 - 1)))
        abort();
    sint32 tt = tick % width;
    sint32 xx = (width >> 1) - 2 - shape->centerx;
    if (width / 2 < tt)
        tt = width - tt;
    sint32 y = TextSys::WordY(0x255);
    DrawShapeExtended(39, 0, xx + tt, y + 1, fScreenFadeVal, 1, 0);
    y = TextSys::WordY(0x255);
    DrawShapeExtended(39, 0, xx - tt, y + 1, fScreenFadeVal, 1, 0);
    tDrawShapeExtended drawflags;
    drawflags.tint[0] = 0x505050;
    DrawShapeExtended(38, 0x410, -2, 0, fScreenFadeVal, 0, &drawflags);
    sint32 Col = 0x232323;
    for (j = 0; j < 3; ++j)
    {
        sint32 x = TextSys::WordX(j + 0x24C);
        char *label = TextSys::Word(j + 0x252);
        FETextRender::FullTextRGB(label, (sint16)x, (sint16)(boxy + 4), ColTextSel, 0, 0);
        if (maxitem > 0)
            PSXDrawSquare(Col, x - 6, boxy + 2, 2, (128 - linefadeval) / 16);
    }
    char *label = TextSys::Word(0x262);
    sint16 x = TextSys::WordX(0x249);
    FETextRender::FullTextRGB(label, x, (sint16)(midy + 3), ColTextSel, 0, 0);
    PSXDrawBrightEndLine(Col, boxx, boxy + 3, boxw, -1, 2, linefadeval, 0x23);
    sint32 columnX = TextSys::WordX(0x24C);
    PSXDrawBrightEndLine(Col, columnX - 6, boxy + 4, 2, midy - 12 - boxy, 1, linefadeval, 0);
    if (maxitem > 8)
    {
        columnX = TextSys::WordX(0x24C);
        sint32 recordY = TextSys::WordY(0x260);
        PSXDrawSquare(Col, columnX - 6, recordY - 1, 2, 8);
    }
    DrawBackgroundImage(11, 27, gCurrentShapes, 0);
}
