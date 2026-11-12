#include <stddef.h>
#include <stdio.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/globals.h"
#include "frontend/common/feapp.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetracks.h"
#include "frontend/common/fetv.h"
#include "frontend/common/front.h"
#include "frontend/common/frontmovie.h"
#include "frontend/common/screenpinkslips.h"
#include "frontend/psx/drawshp.h"
#include "frontend/psx/mmeffect.h"
#include "frontend/psx/psxfront.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "game/psx/flare.h"

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

static sint32 flareextra;
static char gSwapFileName[20];
static char imageTVOrder[4] = { 1, 2, 0, 3 };
tScreenPinkSlips *screenPinkSlips;

static bool StartPinkSlipsMovie(sint32 &session, const char *movieName, sint32 trackID)
{
    char expectedName[32];
    sprintf(expectedName, "ZZZZTR%02d.DCT", trackID);
    if (!FrontMovieNameMatches(movieName, expectedName))
        return false;
    char logicalPath[80];
    char preparedPath[80];
    sprintf(logicalPath, "DATA/%s", expectedName);
    sprintf(preparedPath, "VIDEO/native/ZZZZTR%02d.STR", trackID);
    return StartFrontMovie(session, logicalPath, preparedPath);
}

void Front::ResetOverlayPinkSlips(void)
{
    flareextra = 0;
}

void tScreenPinkSlips::DrawBackground(void)
{
    FUNCTION_MARKER(0x80038B68, "FRONT.BIN");
    PSX_RECT r = { 347, 143, 144, 14 };
    tTrackInformation trackInfo;
    uint8 *frontBytes = reinterpret_cast<uint8 *>(&frontEnd);
    for (sint16 i = 0; i < (uint8)frontEnd.pinkSlipsNumTracks; ++i)
    {
        tMenuTextState textState = textState_Selected;
        sint32 selected = (sint16)fMenu->fCurrentItem;
        if (i == selected - 1 || (i == 0 && selected == 0))
        {
            flareextra = (flareextra + 1);
            if (flareextra > 60)
                flareextra = 0;
            sint32 phase = flareextra > 30 ? (60 - flareextra) : flareextra;
            sint32 intensity = (((phase / 2) + 20) * (128 - fScreenFadeVal));
            if (intensity > 0)
            {
                sint32 x = r.x;
                sint32 y = r.y;
                Flare::_2DHalo(x - 15, y + 6, intensity / 2, (intensity * 2) / 3, 23);
                DrawShapeExtended(56, 0, x - 18, y, fScreenFadeVal, 1, 0);
            }
            textState = textState_Hilighted;
        }
        trackManager.GetTrack(frontBytes[offsetof(tfrontEnd, track) + i], trackInfo);
        frontEnd.pinkSlipsTrackIndex = (char)i;
        sint16 tv = menuDefs->iteratorTrack.TextValue(kPlayerBoth);
        FETextRender::MenuTextPositionedJustify(tv, (r.x + 10), (r.y + 3), 0, textState, textType_FlybyHelp);
        DrawShape::NFS4RoundRectangle(-1, r, 1);
        r.y = (sint16)(r.y + 14);
    }
    uint16 selected = (uint16)fMenu->fCurrentItem;
    frontEnd.pinkSlipsTrackIndex = (char)selected;
    if ((selected & 255) != 0)
        frontEnd.pinkSlipsTrackIndex = (char)(selected - 1);
    trackManager.GetTrack(frontBytes[offsetof(tfrontEnd, track) + (uint8)frontEnd.pinkSlipsTrackIndex], trackInfo);
    UpdateVideoWall(trackInfo);
    IsShapeFileLoaded(fSwapShapes);
    if (fSwapShapes.fFile != 0 && fTransitionDirection >= 0)
    {
        UploadSwapShapes(4);
        fTransitionDirection = 1;
        fTVTicks = ticks;
    }
    DrawVideoWall();
    sint16 shapeY = (fFrame & 1) == 0 ? 128 : 0;
    if (hVideo == 0)
    {
        if ((uint32)ticks - fTVTicks > 256)
        {
            char moviename[80];
            sprintf(moviename, "%szzzTR%02d.dct", Paths_Paths[41], sint32((sint8)trackInfo.fTrackID));
            if (!StartPinkSlipsMovie(hVideo, moviename, (sint8)trackInfo.fTrackID))
            {
#ifdef _DEBUG
                fprintf(stderr, "pink slips movie: open failed\n");
#endif
            }
        }
    }
    else
    {
        sint32 result = UploadFrontMovieFrameSized(hVideo, fFrame, 160, 128);
        if (result == 1)
        {
            hVideo = 2;
            fFrame = (fFrame + 1);
            shapeY = (fFrame & 1) == 0 ? 128 : 0;
        }
        else if (result < 0)
        {
#ifdef _DEBUG
            fprintf(stderr, "pink slips movie: frame delivery failed\n");
#endif
        }
        for (sint16 i = 0; i < 2; ++i)
        {
            for (sint16 j = 0; j < 4; ++j)
            {
                tTVConfig &tv = fTrackTVs[i * 4 + j];
                tv.x = (sint16)(j * 80 + 160);
                tv.y = (sint16)(i * 64 + 25);
                tv.w = 80;
                tv.h = 64;
                tv.u = (uint8)(j * 40);
                tv.v = (uint8)(i * 64 + shapeY);
                tv.uw = 40;
                tv.vh = i == 1 ? 63 : 64;
                tv.tpage = GetTPage(2, 0, 512, shapeY);
                tv.clut = 0;
                tv.flags = 0x20;
                tv.tint = 0x808080;
                tv.state = tv_StateOn;
            }
        }
    }
    for (sint16 x = 240; x < 480; x = (sint16)(x + 80))
        PSXDrawTransSquare(0x303030, x, 15, 2, 129, 1);
    for (sint16 y = 79; y < 143; y = (sint16)(y + 64))
        PSXDrawTransSquare(0x202020, 160, y, 321, 1, 1);
    FeDraw_SetABRMode(2);
    for (sint16 i = 0; i < 8; ++i)
        DrawTV(fTrackTVs[i]);
}

void tScreenPinkSlips::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x80039118, "FRONT.BIN");
    tTrackInformation trackInfo;
    trackManager.GetTrack((uint8)frontEnd.track[0], trackInfo);
    numPermShapes = 57;
    numSwapShapes = 4;
    *permFileName = const_cast<char *>("zPink");
    sprintf(gSwapFileName, "TR%02dPS", sint32((sint8)trackInfo.fTrackID));
    *swapFileName = gSwapFileName;
    fPreviousTrack = (sint8)trackInfo.fTrackID;
}

void tScreenPinkSlips::Initialize(void)
{
    FUNCTION_MARKER(0x800391D8, "FRONT.BIN");
    PSX_RECT r = { 512, 0, 170, 256 };
    ClearImage(&r, 0, 0, 0);
    DrawSync(0);
    tGlobalMenuDefs *defs = menuDefs;
    frontEnd.pinkSlipsTrackIndex = 0;
    defs->iteratorTrack.Decrement(kPlayerBoth);
    menuDefs->iteratorTrack.Increment(kPlayerBoth);
    fMenu = FEApp->fCurrentMenu[0];
    tScreen::Initialize();
    fTVsInitialized = false;
    tTrackInformation trackInfo;
    trackManager.GetTrack((uint8)frontEnd.track[0], trackInfo);
    char moviename[80];
    sprintf(moviename, "%szzzTR%02d.dct", Paths_Paths[41], sint32((sint8)trackInfo.fTrackID));
    InitializeFrontMovie(hVideo);
    if (!StartPinkSlipsMovie(hVideo, moviename, (sint8)trackInfo.fTrackID))
    {
#ifdef _DEBUG
        fprintf(stderr, "pink slips movie: open failed\n");
#endif
    }
    uint32 tvTicks = ticks;
    fFrame = 0;
    fBrightness = 0;
    fDestBrightness = 0;
    fTVsInitialized = false;
    fTransitionDirection = 1;
    fTVTicks = tvTicks;
}

void tScreenPinkSlips::Cleanup(void)
{
    FUNCTION_MARKER(0x80039320, "FRONT.BIN");
    StopFrontMovie(hVideo);
    frontEnd.pinkSlipsTrackIndex = 0;
    tScreen::Cleanup();
}

void tScreenPinkSlips::UpdateVideoWall(tTrackInformation &trackInfo)
{
    FUNCTION_MARKER(0x80039368, "FRONT.BIN");
    if ((sint8)trackInfo.fTrackID != fPreviousTrack)
    {
        sprintf(gSwapFileName, "TR%02dPS", sint32((sint8)trackInfo.fTrackID));
        AsyncLoadSwapShapeFile(gSwapFileName);
        fTVsInitialized = false;
        fPreviousTrack = (sint8)trackInfo.fTrackID;
        if (fTransitionDirection >= 0)
        {
            uint32 tvTicks = ticks;
            fTransitionDirection = -1;
            fTVTicks = tvTicks;
        }
        StopFrontMovie(hVideo);
    }
}

void tScreenPinkSlips::DrawVideoWall(void)
{
    FUNCTION_MARKER(0x80039414, "FRONT.BIN");
    for (sint16 i = 0; i < 36; ++i)
        DrawShapeExtended(i, 0, 0, 0, 0, 0, 0);
    if ((fSwapShapes.fFlags & 1) != 0 && !fTVsInitialized)
    {
        for (sint16 i = 0; i < 4; ++i)
            InitTV(fImageTVs[i], fSwapShapes.fShapes, i);
        fTVsInitialized = true;
    }
    sint16 i = (sint16)(((uint32)ticks - fTVTicks) >> 2);
    if (fTransitionDirection > 0)
    {
        for (sint16 j = 0; j < i && j < 4; ++j)
        {
            tTVConfig &tv = fImageTVs[(uint8)imageTVOrder[j]];
            if (tv.state == tv_StateOff)
                TurnOnTV(tv);
        }
    }
    else
    {
        for (sint16 j = 0; j < i && j < 4; ++j)
        {
            tTVConfig &tv = fImageTVs[(uint8)imageTVOrder[j]];
            if (tv.state == tv_StateOn)
                TurnOffTV(tv);
        }
        if (i >= 8)
            fTransitionDirection = 0;
    }
    for (sint16 j = 0; j < 4; ++j)
        DrawTV(fImageTVs[j]);
}

void tScreenPinkSlips::ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x80039690, "FRONT.BIN");
    if (keyval == kInput_KeyType_Square)
    {
        tTrackInformation trackInfo;
        uint8 index = (uint8)frontEnd.pinkSlipsTrackIndex;
        uint8 *frontBytes = reinterpret_cast<uint8 *>(&frontEnd);
        trackManager.GetTrack(frontBytes[offsetof(tfrontEnd, track) + index], trackInfo);
        tGlobalMenuDefs *defs = menuDefs;
        uint32 flags = defs->itemTraffic.fFlags & ~1;
        defs->itemTraffic.fFlags = flags;
        if (frontEnd.gameMode != 1 && frontEnd.oppNumber == 2)
            defs->itemTraffic.fFlags = flags | 1;
        if (trackInfo.fTrackDifficulty > 2)
            menuDefs->itemTraffic.fFlags |= 1;
        if (trackInfo.fIsEgg != 0)
            menuDefs->itemTraffic.fFlags |= 1;
        if (frontEnd.gameMode == 1 && frontEnd.raceType == 1)
            menuDefs->itemTraffic.fFlags |= 1;
        menuDefs->itemLocalSpeech.fFlags |= 1;
    }
}
