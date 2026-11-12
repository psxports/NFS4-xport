#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "psx_mdec.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "frontend/common/feapp.h"
#include "frontend/common/feaudio.h"
#include "frontend/common/fecredits.h"
#include "frontend/common/fefades.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetv.h"
#include "frontend/common/frontmovie.h"
#include "frontend/common/screenmain.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/mmeffect.h"
#include "frontend/psx/psxfront.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "game/common/textsys.h"
#include "game/psx/drawc.h"

struct tVideo
{
    char x;
    char y;
    char width;
    char height;
    char tileWidth;
    char tileHeight;
    char deltaX;
    char deltaY;
};

struct tVertex
{
    sint16 x;
    sint16 y;
};

struct tVideoWallConfig
{
    sint16 numVideos;
    sint16 flags;
    tVideo *videos[4];
};

static tVideo video11a = { 0, 0, 1, 1, 1, 1, 0, 0 };
static tVideo video11b = { 1, 0, 1, 1, 1, 1, 0, 0 };
static tVideo video11c = { 2, 0, 1, 1, 1, 1, 0, 0 };
static tVideo video11d = { 3, 0, 1, 1, 1, 1, 0, 0 };
static tVideo video11e = { 0, 1, 1, 1, 1, 1, 0, 0 };
static tVideo video11h = { 3, 1, 1, 1, 1, 1, 0, 0 };
static tVideo video11i = { 0, 2, 1, 1, 1, 1, 0, 0 };
static tVideo video11l = { 3, 2, 1, 1, 1, 1, 0, 0 };
static tVideo video11m = { 0, 3, 1, 1, 1, 1, 0, 0 };
static tVideo video11n = { 1, 3, 1, 1, 1, 1, 0, 0 };
static tVideo video11o = { 2, 3, 1, 1, 1, 1, 0, 0 };
static tVideo video11p = { 3, 3, 1, 1, 1, 1, 0, 0 };
static tVideo video21a = { 0, 0, 2, 1, 1, 1, 0, 0 };
static tVideo video21c = { 2, 0, 2, 1, 1, 1, 0, 0 };
static tVideo video21j = { 0, 3, 2, 1, 1, 1, 0, 0 };
static tVideo video21l = { 2, 3, 2, 1, 1, 1, 0, 0 };
static tVideo video31a = { 0, 0, 3, 1, 1, 1, 0, 0 };
static tVideo video31b = { 1, 0, 3, 1, 1, 1, 0, 0 };
static tVideo video31g = { 0, 3, 3, 1, 1, 1, 0, 0 };
static tVideo video31h = { 1, 3, 3, 1, 1, 1, 0, 0 };
static tVideo video22a = { 0, 0, 2, 2, 1, 1, 0, 0 };
static tVideo video22b = { 1, 0, 2, 2, 1, 1, 0, 0 };
static tVideo video22c = { 2, 0, 2, 2, 1, 1, 0, 0 };
static tVideo video22d = { 0, 1, 2, 2, 1, 1, 0, 0 };
static tVideo video22f = { 2, 1, 2, 2, 1, 1, 0, 0 };
static tVideo video22g = { 0, 2, 2, 2, 1, 1, 0, 0 };
static tVideo video22h = { 1, 2, 2, 2, 1, 1, 0, 0 };
static tVideo video22i = { 2, 2, 2, 2, 1, 1, 0, 0 };
static tVideo video32a = { 0, 0, 3, 2, 1, 1, 0, 0 };
static tVideo video32b = { 1, 0, 3, 2, 1, 1, 0, 0 };
static tVideo video32e = { 0, 2, 3, 2, 1, 1, 0, 0 };
static tVideo video32f = { 1, 2, 3, 2, 1, 1, 0, 0 };

static tVertex dropShadow[4][4] =
{
    { { 163, 42 }, { 163, 210 }, { 183, 52 }, { 183, 200 } },
    { { 163, 42 }, { 483, 42 }, { 183, 52 }, { 463, 52 } },
    { { 483, 42 }, { 483, 210 }, { 463, 52 }, { 463, 200 } },
    { { 163, 210 }, { 483, 210 }, { 183, 200 }, { 463, 200 } },
};

static tVertex animLocations[4] =
{
    { 163, 42 }, { 403, 42 }, { 163, 168 }, { 403, 168 }
};

static tVideoWallConfig videoWallConfigs[24] =
{
    { 2, 0, { &video32a, &video21l, 0, 0 } },
    { 2, 0, { &video32b, &video21j, 0, 0 } },
    { 2, 0, { &video32e, &video21c, 0, 0 } },
    { 2, 0, { &video32f, &video21a, 0, 0 } },
    { 3, 0, { &video22a, &video11l, &video11p, 0 } },
    { 3, 0, { &video22c, &video11i, &video11m, 0 } },
    { 3, 0, { &video22g, &video11d, &video11h, 0 } },
    { 3, 0, { &video22i, &video11a, &video11e, 0 } },
    { 3, 0, { &video11b, &video21c, &video22h, 0 } },
    { 3, 0, { &video11c, &video21a, &video22h, 0 } },
    { 3, 0, { &video11n, &video21l, &video22b, 0 } },
    { 3, 0, { &video11o, &video21j, &video22b, 0 } },
    { 3, 0, { &video11d, &video22d, &video21j, 0 } },
    { 3, 0, { &video11a, &video22f, &video21l, 0 } },
    { 3, 0, { &video11p, &video22d, &video21a, 0 } },
    { 3, 0, { &video11m, &video22f, &video21c, 0 } },
    { 3, 0, { &video22d, &video11d, &video11p, 0 } },
    { 3, 0, { &video22f, &video11a, &video11m, 0 } },
    { 3, 0, { &video22b, &video11m, &video11p, 0 } },
    { 3, 0, { &video22h, &video11a, &video11d, 0 } },
    { 3, 0, { &video22a, &video31h, &video11m, 0 } },
    { 3, 0, { &video22c, &video31g, &video11p, 0 } },
    { 3, 0, { &video22i, &video31a, &video11d, 0 } },
    { 3, 0, { &video22g, &video31b, &video11a, 0 } },
};

static char tvOrder[16] = { 5, 14, 3, 8, 6, 0, 11, 13, 2, 15, 4, 10, 1, 12, 7, 9 };
static sint32 tintColors[28] = { 2190222, 2190222, 2701056, 2701056, 6040088, 6040088, 6040088, 6040088, 2190222, 2190222, 6040088, 6040088, 789581, 789581, 789581, 789581, 2701056, 2701056, 6901054, 6901054, 6040088, 6040088, 789581, 789581, 6901054, 6901054, 6243377, 6243377 };
static char numberValues[25];
static char gNameBuffer[32];
static char gPermBuffer[32];

tScreenMain *screenMain;

static void StopMainMovie(tScreenMain &screen)
{
    StopFrontMovie(screen.hVideo);
}

static bool StartMainMovie(tScreenMain &screen, const char *moviename)
{
    if (screen.hVideo != 0 || screen.fCurrentMovie < 0 || screen.fCurrentMovie >= 20)
        return false;
    char logical[32];
    char prepared[40];
    char expected[16];
    sprintf(expected, "ZZZZVD%02d.DCT", sint32(screen.fCurrentMovie));
    if (!FrontMovieNameMatches(moviename, expected))
        return false;
    sprintf(logical, "DATA/%s", expected);
    sprintf(prepared, "VIDEO/native/ZZZZVD%02d.STR", sint32(screen.fCurrentMovie));
    return StartFrontMovie(screen.hVideo, logical, prepared);
}

static sint32 UploadMainMovieFrame(tScreenMain &screen)
{
    return UploadFrontMovieFrame(screen.hVideo, screen.fFrame);
}

void tScreenMain::SwapBackground(sint32 num)
{
    FUNCTION_MARKER(0x80036F20, "FRONT.BIN");
    StopMainMovie(*this);
    sint32 slot = fCurrentSlot;
    bVideoAborted = true;
    if (fVideoShapes[slot].async_handle != 0 || fVideoShapes[slot].fFile != 0 || fVideoShapes[1 - slot].async_handle != 0 || fVideoShapes[1 - slot].fFile != 0)
        return;
    if (num == -1)
    {
        do
        {
            sint32 random = randPSX();
            fCurrentBG[fCurrentSlot] = random % 28;
        } while (fCurrentBG[fCurrentSlot] == fCurrentBG[1 - fCurrentSlot]);
    }
    else
        fCurrentBG[slot] = num;
    char buffer[8];
    sprintf(buffer, "zyVid%02d", fCurrentBG[fCurrentSlot]);
    AsyncLoadShapeFile(buffer, fVideoShapes[fCurrentSlot]);
    tScreenMainState oldState = fState;
    fState = kScreenMain_Off;
    fCurrentSlot = 1 - fCurrentSlot;
    SetState(oldState == kScreenMain_WarningImage ? kScreenMain_WarningImage : kScreenMain_StaticImage);
    fNumTVsInTransition = 16;
}

bool tScreenMain::DoneLoadingBackground(void)
{
    FUNCTION_MARKER(0x800370DC, "FRONT.BIN");

    sint32 slot = fCurrentSlot;
    return fVideoShapes[slot].async_handle == 0 && fVideoShapes[slot].fFile == 0 && fVideoShapes[1 - slot].async_handle == 0 && fVideoShapes[1 - slot].fFile == 0;
}

void tScreenMain::SetState(tScreenMainState state)
{
    FUNCTION_MARKER(0x80037158, "FRONT.BIN");
    if (state == fState)
        return;
    StopMainMovie(*this);
    uint32 movieTicks = ticks;
    bVideoAborted = true;
    fMovieTicks = movieTicks;
    if (state != kScreenMain_Credits && CreditManager.fCreditsInitialized == 1)
        CreditManager.DeInit();
    uint32 startTicks = ticks;
    fState = state;
    fStartTicks = startTicks;
    for (sint16 i = 0; i < 16; ++i)
    {
        tTexture::ShapeInfo *shape = &fVideoShapes[fCurrentSlot].fShapes[i];
        tvTransitions[i].state = kScreenMain_StaticImage;
        sint32 offset = (shape->shapex - (sint16)((uint16)shape->shapex & 0xFFC0)) * 16;
        uint8 depth = (uint8)shape->depth;
        if (depth == 0)
            abort();
        tvTransitions[i].u = (uint8)(offset / depth);
        tvTransitions[i].v = (uint8)shape->shapey;
        tvTransitions[i].uw = (uint8)shape->width;
        tvTransitions[i].vh = (uint8)shape->height;
        tvTransitions[i].tpage = (uint16)getTPage(shape->type & 3, 0, (uint16)shape->shapex, (uint16)shape->shapey);
        tvTransitions[i].clut = GetClut((shape->clutID & 63) << 4, shape->clutID >> 6);
        tvTransitions[i].flags = 0;
        tvTransitions[i].tint = 0x808080;
        tvTransitions[i].bright = 128;
    }
    if (fState == kScreenMain_WarningImage)
    {
        for (sint16 i = 4; i < 12; ++i)
        {
            tvTransitions[i].bright = 128;
            tvTransitions[i].state = kScreenMain_WarningImage;
        }
        tvTransitions[6].bright = 128;
        tvTransitions[5].bright = 128;
    }
    else if (fState == kScreenMain_DynamicImage)
        InitDynamicImages();
    else if (fState == kScreenMain_Credits)
    {
        for (sint16 i = 0; i < 16; ++i)
        {
            tvTransitions[i].bright = 128;
            tvTransitions[i].state = kScreenMain_Credits;
        }
        CreditManager.Init((sint32)fStartTicks);
    }
}

void tScreenMain::InitDynamicImages(void)
{
    FUNCTION_MARKER(0x8003742C, "FRONT.BIN");
    sint32 videoWallConfig = randPSX() % 24;
    for (sint32 i = 0; i < videoWallConfigs[videoWallConfig].numVideos; ++i)
    {
        tVideo *video = videoWallConfigs[videoWallConfig].videos[i];
        sint32 startX = (uint8)video->x;
        sint32 startY = (uint8)video->y;
        for (sint32 j = 0; j < (uint8)video->tileWidth; ++j)
        {
            for (sint32 k = 0; k < (uint8)video->tileHeight; ++k)
            {
                for (sint32 y = 0; y < (uint8)video->height; ++y)
                {
                    for (sint32 x = 0; x < (uint8)video->width; ++x)
                    {
                        sint32 index = (y + startY + k * (uint8)video->height) * 4 + x + startX + j * (uint8)video->width;
                        tvTransitions[index].u = (uint8)(x * (80 / (uint8)video->width));
                        tvTransitions[index].v = (uint8)(y * (80 / (uint8)video->height));
                        tvTransitions[index].uw = (uint8)(80 / (uint8)video->width - 1);
                        tvTransitions[index].vh = (uint8)(80 / (uint8)video->height);
                        tvTransitions[index].tpage = GetTPage(2, 0, 512, 0);
                        tvTransitions[index].clut = 0;
                        tvTransitions[index].flags = 0x22;
                        uint32 tint = tintColors[fCurrentBG[fCurrentSlot]];
                        tvTransitions[index].state = kScreenMain_DynamicImage;
                        tvTransitions[index].bright = 128;
                        tvTransitions[index].tint = tint;
                    }
                }
            }
        }
    }
}

void tScreenMain::ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &)
{
    FUNCTION_MARKER(0x80037798, "FRONT.BIN");

    if (keyval == kInput_KeyType_Triangle && FEApp->backDepth[0] > 0)
    {
        SwapBackground(-1);
        if (FEApp->fCurrentMenu[0] == &menuDefs->menuPinkSlipSelect)
            frontEnd.raceType = 0;
    }
}

void tScreenMain::DrawDropShadow(void)
{
    FUNCTION_MARKER(0x80037804, "FRONT.BIN");
    for (sint32 i = 0; i < 4; ++i)
    {
        Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
        POLY_G4 *prim = reinterpret_cast<POLY_G4 *>(cprim->PrimPtr);
        setaddr(prim, getaddr(cprim->LastPrim));
        cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
        setaddr(cprim->LastPrim, prim);
        uint32 color = 0x808080;
        memcpy(&prim->p[0].rgbc, &color, sizeof(color));
        setcode(prim, 0x3A);
        memcpy(&prim->p[1].rgbc, &color, sizeof(color));
        color = 0;
        memcpy(&prim->p[3].rgbc, &color, sizeof(color));
        memcpy(&prim->p[2].rgbc, &color, sizeof(color));
        setlen(prim, 8);
        for (sint32 j = 0; j < 4; ++j)
        {
            prim->p[j].xy.x = dropShadow[i][j].x;
            prim->p[j].xy.y = dropShadow[i][j].y;
        }
    }
    FeDraw_SetABRMode(2);
}

void tScreenMain::DrawVideoLines(void)
{
    FUNCTION_MARKER(0x80037918, "FRONT.BIN");
    for (sint32 i = 243; i < 483; i += 80)
        PSXDrawTransSquare(0x303030, i, 42, 2, 169, 1);
    for (sint32 i = 84; i < 210; i += 42)
        PSXDrawTransSquare(0x202020, 163, i, 321, 1, 1);
    FeDraw_SetABRMode(2);
}

void tScreenMain::DrawBackground(void)
{
    FUNCTION_MARKER(0x800379C0, "FRONT.BIN");
    sint16 animFade = 0;
    tDrawShapeExtended drawFlags;
    for (sint16 i = 0; i < 2; ++i)
    {
        IsShapeFileLoaded(fVideoShapes[i]);
        if (fVideoShapes[i].fFile == 0)
            continue;
        bool bAllTVsOn = true;
        for (sint16 j = 0; j < 16; ++j)
        {
            tScreenMainState state = tvStates[j];
            if ((state != kScreenMain_StaticImage && state != kScreenMain_Credits) || tvTransitions[j].state != state || tvConfigs[j].clut != tvTransitions[j].clut)
                bAllTVsOn = false;
        }
        if (bAllTVsOn)
            UploadShapes(fVideoShapes[i], (sint16)(i * 166), 0, 16, 0);
    }
    if (fState == kScreenMain_Credits && FEApp->fCurrentMenu[0] != &menuDefs->menuCredits)
        SetState(kScreenMain_StaticImage);
    else if (FEApp->fCurrentMenu[0] == &menuDefs->menuCredits)
        SetState(kScreenMain_Credits);
    CreditManager.Draw(fState == kScreenMain_Credits);
    if (fState == kScreenMain_WarningImage)
    {
        if (frontEnd.raceType != 6 && FEApp->fCurrentMenu[0]->fItemList[FEApp->fCurrentMenu[0]->fCurrentItem] != &menuDefs->itemTwoPlayerPinkSlips)
            SetState(kScreenMain_StaticImage);
    }
    else if (frontEnd.raceType == 6 || FEApp->fCurrentMenu[0]->fItemList[FEApp->fCurrentMenu[0]->fCurrentItem] == &menuDefs->itemTwoPlayerPinkSlips)
        SetState(kScreenMain_WarningImage);
    bool drawWarning = false;
    if (fState == kScreenMain_WarningImage)
    {
        if (fWarningFade < 96)
            fWarningFade = (sint16)((uint16)fWarningFade + 4);
        drawWarning = true;
    }
    else if (fWarningFade > 0)
    {
        fWarningFade = (sint16)((uint16)fWarningFade - 4);
        drawWarning = true;
    }
    if (drawWarning)
    {
        for (sint16 i = 4; i < 12; ++i)
        {
            tvConfigs[i].flags |= 2;
            uint32 tint = uint32(128 - fWarningFade);
            tvConfigs[i].tint = (tint << 16) | (tint << 8) | tint;
        }
        uint32 tint = uint32(128 - (fWarningFade * 64) / 96);
        tvConfigs[6].tint = (tint << 16) | (tint << 8) | tint;
        tvConfigs[5].tint = (tint << 16) | (tint << 8) | tint;
        drawFlags.tint[0] = 0xBEBE;
        DrawShapeExtended((gettick() / 15) % 10 + 257, 0x411, 163, -15, 96 - fWarningFade, 1, &drawFlags);
        DrawShapeExtended((gettick() / 15) % 10 + 257, 0x415, 403, -15, 96 - fWarningFade, 1, &drawFlags);
        DrawShapeExtended(256, 0x11, 0, 0, 96 - fWarningFade, 1, &drawFlags);
        ScaleShapeExtended((gettick() / 21) % 6 + 250, 1, 0, 0, 96 - fWarningFade, 3, 0);
        FETextRender::SetABR(1, true);
        sint32 TextCol = CalcFadeVal(0xBEBE, 96 - fWarningFade);
        PSX_RECT r = { 181, 129, 284, 42 };
        FETextRender::WordWrapTextRGB(TextSys::Word(626), r, TextCol);
        FETextRender::SetABR(0, false);
        FeDraw_SetABRMode(0);
    }
    if (hVideo != 0)
    {
        if (hVideo == 2)
        {
            uint32 movieTicks = ticks;
            bVideoAborted = false;
            fMovieTicks = movieTicks;
        }
        sint32 result = UploadMainMovieFrame(*this);
        if (result == 1)
        {
            if (hVideo == 1)
            {
                bVideoAborted = false;
                fMovieTicks = ticks;
            }
            hVideo = 2;
            fFrame = (fFrame + 1);
        }
        else if (result < 0)
        {
#ifdef _DEBUG
            fprintf(stderr, "menu movie %d: frame delivery failed\n", sint32(fCurrentMovie));
#endif
        }
    }
    else if (uint32(ticks) - fStartTicks > 640U && fState == kScreenMain_StaticImage)
    {
        PSX_RECT r = { 512, 0, 80, 160 };
        ClearImage(&r, 0, 0, 0);
        DrawSync(0);
        do
        {
            fCurrentMovie = (sint16)(randPSX() % 20);
        } while (fCurrentMovie == fPreviousMovie);
        fPreviousMovie = fCurrentMovie;
        SetState(kScreenMain_DynamicImage);
        char moviename[80];
        sprintf(moviename, "%szzzVD%02d.dct", Paths_Paths[41], sint32(fCurrentMovie));
        if (!StartMainMovie(*this, moviename))
        {
#ifdef _DEBUG
            fprintf(stderr, "menu movie %d: source open failed\n", sint32(fCurrentMovie));
#endif
        }
    }
    if (fState == kScreenMain_DynamicImage && hVideo == 0)
        SetState(kScreenMain_StaticImage);
    DrawBackgroundImage(2, 28, fPermShapes.fShapes, 0);
    DrawDropShadow();
    DrawVideoLines();
    uint32 deltaTicks = uint32(ticks) - fAnimTicks;
    if (deltaTicks > 1500 && fState != kScreenMain_WarningImage)
    {
        sint32 async = fSwapShapes.async_handle;
        deltaTicks = 0;
        fAnimationUploaded = 0;
        if (async == 0)
        {
            sint32 nextAnim;
            do
            {
                nextAnim = randPSX() % 25;
            } while ((sint16)nextAnim == fPreviousAnim);
            fPreviousAnim = (sint16)nextAnim;
            char buffer[32];
            sprintf(buffer, "yVda%02d", sint32(fPreviousAnim));
            AsyncLoadSwapShapeFile(buffer);
        }
    }
    IsShapeFileLoaded(fSwapShapes);
    if (fSwapShapes.fFile != 0)
    {
        fSwapShapes.fNumShapes = 10;
        UploadSwapShapes(10);
        fAnimTicks = ticks;
        fAnimationUploaded = 1;
        fAnimLocation = (sint16)(fAnimTicks & 3);
    }
    if (deltaTicks < 128)
        animFade = (sint16)(128 - deltaTicks);
    else if ((deltaTicks - 672) < 128)
        animFade = (sint16)(deltaTicks - 672);
    if (deltaTicks < 800 && fAnimationUploaded != 0)
    {
        drawFlags.tint[0] = tintColors[fCurrentBG[fCurrentSlot]];
        drawFlags.custom_shapes = fSwapShapes.fShapes;
        sint32 drawAnimFade = animFade;
        if (CreditManager.fCreditsInitialized != 0)
        {
            drawAnimFade += 64;
            if (drawAnimFade > 128)
                drawAnimFade = 128;
        }
        DrawShapeExtended((deltaTicks / 15) % 10, 0x611, animLocations[fAnimLocation].x, animLocations[fAnimLocation].y, drawAnimFade, 1, &drawFlags);
    }
    sint16 y = 50;
    for (sint16 i = 0; i < 25 && y < 210; ++i, y = (sint16)(y + 8))
    {
        sint16 x = 418;
        for (sint16 j = 4; j >= 0; --j, x = (sint16)(x + 13))
        {
            sint32 index = (i + ticks / 20) % 25;
            DrawShapeExtended(((uint8)numberValues[index] >> j) & 1, 1, x, y, 64, 3, 0);
        }
    }
    sint32 transitionLimit = (sint32)((uint32(ticks) - fStartTicks) << 12) >> 16;
    for (sint16 i = 0; i < 16 && i <= transitionLimit; ++i)
    {
        uint8 index = (uint8)tvOrder[i];
        if ((tvStates[index] != tvTransitions[index].state || tvConfigs[index].clut != tvTransitions[index].clut) && tvConfigs[index].state == tv_StateOn)
            TurnOffTV(tvConfigs[index]);
    }
    for (sint16 i = 0; i < 16; ++i)
    {
        if (tvConfigs[i].state != tv_StateOff)
            continue;
        tvConfigs[i].u = tvTransitions[i].u;
        tvConfigs[i].v = tvTransitions[i].v;
        tvConfigs[i].uw = tvTransitions[i].uw;
        tvConfigs[i].vh = tvTransitions[i].vh;
        tvConfigs[i].tpage = tvTransitions[i].tpage;
        tvConfigs[i].clut = tvTransitions[i].clut;
        tvConfigs[i].flags = tvTransitions[i].flags;
        tvConfigs[i].tint = tvTransitions[i].tint;
        tvStates[i] = tvTransitions[i].state;
        TurnOnTV(tvConfigs[i]);
        tvConfigs[i].destBrightness = tvTransitions[i].bright;
        fNumTVsInTransition = (fNumTVsInTransition - 1);
    }
    for (sint16 i = 0; i < 16; ++i)
        DrawTV(tvConfigs[i]);
    for (sint16 x = 243; x < 483; x = (sint16)(x + 80))
        PSXDrawSquare(0x404040, x, 42, 2, 169);
    for (sint16 y = 84; y < 210; y = (sint16)(y + 42))
        PSXDrawSquare(0x303030, 163, y, 321, 1);
}

void tScreenMain::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x80038698, "FRONT.BIN");
    numPermShapes = 267;
    *permFileName = gPermBuffer;
    numSwapShapes = 10;
    *swapFileName = gNameBuffer;
}

void tScreenMain::PreLoad(void)
{
    FUNCTION_MARKER(0x800386C8, "FRONT.BIN");
    sprintf(gPermBuffer, "zMain%d", (uint8)frontEnd.language);
    fPreviousAnim = (sint16)(randPSX() % 25);
    sprintf(gNameBuffer, "yVda%02d", sint32(fPreviousAnim));
    tScreen::PreLoad();
    for (sint16 i = 0; i < 2; ++i)
    {
        fVideoShapes[i].fShapes = 0;
        InitializeShapes(fVideoShapes[i], 16);
    }
    fCurrentBG[0] = randPSX() % 28;
    sint32 random = randPSX();
    fCurrentBG[1] = (fCurrentBG[0] + random % 27 + 1) % 28;
    for (sint16 i = 0; i < 2; ++i)
    {
        char buffer[32];
        sprintf(buffer, "zyVid%02d", fCurrentBG[i]);
        AsyncLoadShapeFile(buffer, fVideoShapes[i]);
    }
}

void tScreenMain::Initialize(void)
{
    FUNCTION_MARKER(0x800388D4, "FRONT.BIN");
    tScreen::Initialize();
    bool shapesLoaded;
    do
    {
        FeAudio::systemtask(0);
        bool firstLoaded = IsShapeFileLoaded(fVideoShapes[0]);
        if (fVideoShapes[0].fFile != 0)
            UploadShapes(fVideoShapes[0], 0, 0, 16, 0);
        shapesLoaded = firstLoaded && IsShapeFileLoaded(fVideoShapes[1]);
        if (fVideoShapes[1].fFile != 0)
            UploadShapes(fVideoShapes[1], 166, 0, 16, 0);
    } while (!shapesLoaded);
    fPreviousMovie = -1;
    fFrame = 0;
    InitializeFrontMovie(hVideo);
    uint32 startTicks = ticks;
    uint32 animTicks = ticks;
    bVideoAborted = false;
    fTransitionDirection = 1;
    fAnimationUploaded = 0;
    fWarningFade = 0;
    fNumTVsInTransition = 0;
    fCurrentSlot = 0;
    fStartTicks = startTicks;
    fAnimTicks = animTicks - 800;
    for (sint16 i = 0; i < 16; ++i)
        InitTV(tvConfigs[i], fVideoShapes[fCurrentSlot].fShapes, i);
    fState = kScreenMain_Off;
    SetState(kScreenMain_StaticImage);
    for (sint16 i = 0; i < 16; ++i)
        tvStates[i] = kScreenMain_Off;
    for (sint16 i = 0; i < 25; ++i)
    {
        sint32 random = randPSX();
        numberValues[i] = (char)random;
    }
}

void tScreenMain::Cleanup(void)
{
    FUNCTION_MARKER(0x80038AD8, "FRONT.BIN");
    StopMainMovie(*this);
    for (sint32 i = 0; i < 2; ++i)
        FreeShapes(fVideoShapes[i]);
    tScreen::Cleanup();
}
