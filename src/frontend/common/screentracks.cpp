#include <stddef.h>
#include <stdio.h>
#include <string.h>
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
#include "frontend/common/screentracks.h"
#include "frontend/psx/drawshp.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/mmeffect.h"
#include "frontend/psx/psxfront.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "game/psx/drawc.h"

enum VIDEOSTATE
{
    VIDEOSTATE_IDLE = 0,
    VIDEOSTATE_SPOOLING = 1,
    VIDEOSTATE_READY = 2,
    VIDEOSTATE_PLAYING = 3
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

static char gSwapFileName[20];
static sint16 tvOrder[10] = { 2, 7, 4, 8, 1, 6, 0, 9, 3, 5 };
tScreenTrackSelect *screenTrackSelect;

static bool StartTrackMovie(sint32 &session, const char *movieName, sint32 trackID)
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

void tScreenTrackSelect::DrawBackground(void)
{
    FUNCTION_MARKER(0x80041600, "FRONT.BIN");

    PSX_RECT r = { 320, 200, 170, 12 };
    sint16 trackText = menuDefs->iteratorTrack.TextValue(kPlayerBoth);
    DrawShape::NFS4RoundRectangle(trackText, r, 0);
    tTrackInformation trackInfo;
    trackManager.GetTrack((uint8)frontEnd.track[0], trackInfo);
    UpdateBrightness(trackInfo);
    UpdateVideoWall(trackInfo);
    IsShapeFileLoaded(fSwapShapes);
    if (fSwapShapes.fFile && fVideoWall.fTransitionDirection != -1 && fBrightness == 0)
    {
        UploadSwapShapes(10);
        fVideoWall.TurnOn();
    }
    sint32 frameCounter = fFrame;
    sint16 shapeY = (frameCounter & 1) == 0 ? 128 : 0;
    if (hVideo != 0)
    {
        sint32 result = UploadFrontMovieFrameSized(hVideo, frameCounter, 160, 128);
        if (result == 1)
        {
            hVideo = 2;
            fFrame = (sint32)((uint32)fFrame + 1);
            shapeY = (fFrame & 1) == 0 ? 128 : 0;
        }
        else if (result == 0 && hVideo == 1)
        {
            PSX_RECT clearRect = { 512, 0, 170, 256 };
            ClearImage(&clearRect, 0, 0, 0);
            DrawSync(0);
            uint32 startTicks = ticks;
            fBrightness = 0;
            fStartTicks = startTicks - 20;
        }
        else if (result < 0)
        {
#ifdef _DEBUG
            fprintf(stderr, "track movie: frame failed\n");
#endif
        }
    }
    else if ((fTicksSet || fDestBrightness < fBrightness) &&
        (uint32)ticks - fVideoTicks > 256)
    {
        if (fBrightness <= fDestBrightness)
            SetBrightness(trackInfo.fAvailable ? 128 : 32);
        char moviename[80];
        sprintf(moviename, "%szzzTR%02d.dct", Paths_Paths[41], sint32(fMovieTrack));
        if (!StartTrackMovie(hVideo, moviename, fMovieTrack))
        {
#ifdef _DEBUG
            fprintf(stderr, "track movie: open failed\n");
#endif
        }
    }
    if (fBrightness > 0)
    {
        Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
        POLY_FT4 *prim = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
        setaddr(prim, getaddr(cprim->LastPrim));
        cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
        setaddr(cprim->LastPrim, prim);
        uint32 brightness = fBrightness;
        uint32 color = (brightness << 16) | (brightness << 8) | brightness;
        memcpy(&prim->rgbc, &color, sizeof(color));
        setlen(prim, 9);
        setcode(prim, 0x2E);
        prim->p[0].xy.x = 153;
        prim->p[1].xy.x = 313;
        prim->p[2].xy.x = 153;
        prim->p[3].xy.x = 313;
        prim->p[0].xy.y = prim->p[1].xy.y = 105;
        prim->p[2].xy.y = prim->p[3].xy.y = 232;
        prim->p[0].uv.u = prim->p[2].uv.u = 0;
        prim->p[1].uv.u = prim->p[3].uv.u = 80;
        prim->p[0].uv.v = prim->p[1].uv.v = (uint8)shapeY;
        prim->p[2].uv.v = prim->p[3].uv.v = (uint8)(shapeY | 127);
        prim->p[1].attr = (uint16)GetTPage(2, 1, 512, 0);
        prim->p[0].attr = 0;

        cprim = &DrawC::carCache.head.cprim;
        prim = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
        setaddr(prim, getaddr(cprim->LastPrim));
        setaddr(cprim->LastPrim, prim);
        brightness = fBrightness;
        cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
        color = (brightness << 16) | (brightness << 8) | brightness;
        memcpy(&prim->rgbc, &color, sizeof(color));
        setlen(prim, 9);
        setcode(prim, 0x2E);
        prim->p[0].xy.x = 313;
        prim->p[1].xy.x = 473;
        prim->p[2].xy.x = 313;
        prim->p[3].xy.x = 473;
        prim->p[0].xy.y = prim->p[1].xy.y = 105;
        prim->p[2].xy.y = prim->p[3].xy.y = 232;
        prim->p[0].uv.u = prim->p[2].uv.u = 16;
        prim->p[1].uv.u = prim->p[3].uv.u = 96;
        prim->p[0].uv.v = prim->p[1].uv.v = (uint8)shapeY;
        prim->p[2].uv.v = prim->p[3].uv.v = (uint8)(shapeY | 127);
        prim->p[1].attr = (uint16)GetTPage(2, 1, 576, 0);
        prim->p[0].attr = 0;
    }
    DrawVideoWall();
}

void tScreenTrackSelect::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x80041AAC, "FRONT.BIN");
    tTrackInformation trackInfo;
    trackManager.GetTrack((uint8)frontEnd.track[0], trackInfo);
    numPermShapes = 68;
    numSwapShapes = 10;
    *permFileName = const_cast<char *>("ztrack");
    sprintf(gSwapFileName, "%s", trackInfo.fShapeName);
    *swapFileName = gSwapFileName;
    fPreviousTrack = (sint8)trackInfo.fTrackID;
}

void tScreenTrackSelect::Initialize(void)
{
    FUNCTION_MARKER(0x80041B6C, "FRONT.BIN");

    tGlobalMenuDefs *defs = menuDefs;
    frontEnd.pinkSlipsTrackIndex = 0;
    defs->iteratorTrack.Decrement(kPlayerBoth);
    menuDefs->iteratorTrack.Increment(kPlayerBoth);
    tTrackInformation trackInfo;
    trackManager.GetTrack((uint8)frontEnd.track[0], trackInfo);
    PSX_RECT r = { 512, 0, 170, 256 };
    ClearImage(&r, 0, 0, 0);
    DrawSync(0);
    tScreen::Initialize();
    char moviename[80];
    sprintf(moviename, "%szzzTR%02d.dct", Paths_Paths[41], sint32((sint8)trackInfo.fTrackID));
    InitializeFrontMovie(hVideo);
    if (!StartTrackMovie(hVideo, moviename, (sint8)trackInfo.fTrackID))
    {
#ifdef _DEBUG
        fprintf(stderr, "track movie: open failed\n");
#endif
    }
    fFrame = 0;
    fVideoWall.Initialize(tvConfigs, fSwapShapes.fShapes, 0, 10, tvOrder, 150);
    fVideoWall.SetAvailableText(248, 320, 80);
    fVideoWall.SetAvailableIcon(38, 10, 310, 60, fPermShapes.fShapes);
    fBrightness = 0;
    fDestBrightness = 0;
    fTVsInitialized = false;
    fVideoWall.TurnOn();
    uint32 videoTicks = ticks;
    fTicksSet = true;
    fVideoTicks = videoTicks - 256;
    fMovieTrack = (sint8)trackInfo.fTrackID;
}

void tScreenTrackSelect::Cleanup(void)
{
    FUNCTION_MARKER(0x80041D28, "FRONT.BIN");

    StopFrontMovie(hVideo);
    tScreen::Cleanup();
}

void tScreenTrackSelect::SetBrightness(sint16 bright)
{
    FUNCTION_MARKER(0x80041D68, "FRONT.BIN");
    if (bright != fDestBrightness)
    {
        sint16 brightness = fBrightness;
        uint32 startTicks = ticks;
        fDestBrightness = bright;
        fStartBrightness = brightness;
        fStartTicks = startTicks;
    }
}

void tScreenTrackSelect::UpdateBrightness(tTrackInformation &trackInfo)
{
    FUNCTION_MARKER(0x80041D98, "FRONT.BIN");

    uint32 currentTicks = ticks;
    sint32 elapsed = (sint32)(currentTicks - fStartTicks);
    if (fDestBrightness != fBrightness)
    {
        if (elapsed >= 128)
            fBrightness = fDestBrightness;
        else if (elapsed < 0)
            fBrightness = 0;
        else
            fBrightness = sint16(fStartBrightness +
                (fDestBrightness - fStartBrightness) * elapsed / 128);
    }
    if (fBrightness == 0 && fDestBrightness == 0)
    {
        StopFrontMovie(hVideo);
        if (!fTicksSet)
        {
            uint32 videoTicks = ticks;
            fTicksSet = true;
            fVideoTicks = videoTicks;
            fMovieTrack = (sint8)trackInfo.fTrackID;
        }
    }
}

void tScreenTrackSelect::UpdateVideoWall(tTrackInformation &trackInfo)
{
    FUNCTION_MARKER(0x80041E88, "FRONT.BIN");
    if ((sint8)trackInfo.fTrackID != fPreviousTrack)
    {
        AsyncLoadSwapShapeFile(trackInfo.fShapeName);
        fTVsInitialized = false;
        fPreviousTrack = (sint8)trackInfo.fTrackID;
        fVideoWall.TurnOff();
        SetBrightness(0);
        fTicksSet = false;
    }
}

void tScreenTrackSelect::DrawVideoWall(void)
{
    FUNCTION_MARKER(0x80041EFC, "FRONT.BIN");
    tTrackInformation trackInfo;
    trackManager.GetTrack((uint8)frontEnd.track[(uint8)frontEnd.pinkSlipsTrackIndex], trackInfo);
    FETextRender::MenuTextPositionedJustify(trackInfo.fSpeedoCountry + 0x43, 478, 33, 1, textState_Unselected, textType_TrackRecords);
    DrawBackgroundImage(0, 28, fPermShapes.fShapes, 150);
    PSXDrawTransSquare(0, 320, 30, 160, 10, 1);
    FeDraw_SetABRMode(0);
    if ((fSwapShapes.fFlags & 1) != 0 && !fTVsInitialized)
    {
        fVideoWall.SetAvailable(trackInfo.fAvailable);
        fVideoWall.UpdateImages();
        fTVsInitialized = true;
    }
    fVideoWall.UpdateTransition();
    fVideoWall.Draw();
}

void tScreenTrackSelect::ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x8004200C, "FRONT.BIN");
    tInputKeyType key = keyval;
    if (key == kInput_KeyType_Square)
    {
        tTrackInformation trackInfo;
        trackManager.GetTrack((uint8)frontEnd.track[(uint8)frontEnd.pinkSlipsTrackIndex], trackInfo);
        tGlobalMenuDefs *defs = menuDefs;
        uint32 flags = defs->itemTraffic.fFlags & ~1;
        defs->itemTraffic.fFlags = flags;
        if (frontEnd.gameMode != 1 && frontEnd.oppNumber == 2)
            defs->itemTraffic.fFlags = flags | 1;
        if (trackInfo.fTrackDifficulty > 2)
            menuDefs->itemTraffic.fFlags |= 1;
        if (trackInfo.fIsEgg != 0)
            menuDefs->itemTraffic.fFlags |= 1;
        if (frontEnd.gameMode == 1)
        {
            if (frontEnd.raceType != 1)
            {
                menuDefs->itemLocalSpeech.fFlags |= 1;
                return;
            }
            menuDefs->itemTraffic.fFlags |= 1;
        }
        if (frontEnd.raceType == 1 && Front::EnableLocalSpeech())
            menuDefs->itemLocalSpeech.fFlags &= ~1;
        else
            menuDefs->itemLocalSpeech.fFlags |= 1;
    }
    else if (key == kInput_KeyType_Triangle && !FEApp->fCurrentMenu[0]->IsSubMenu())
        fVideoWall.TurnOffInstant();
}
