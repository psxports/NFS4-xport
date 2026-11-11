#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/fescreen.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetools.h"
#include "frontend/common/front.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/movie.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "game/common/r3dcar.h"
#include "game/common/render.h"
#include "game/common/renderview.h"
#include "game/common/textsys.h"
#include "game/psx/audio.h"
#include "game/psx/cario.h"
#include "game/psx/draw.h"
#include "game/psx/drawc.h"
#include "game/psx/fe3dmenu.h"
#include "game/psx/font.h"
#include "game/psx/platform.h"
#include "game/psx/rendercache.h"
#include "game/psx/texture.h"
#include "lib/psx/blkfill.h"
#include "lib/psx/fixdmult.h"

struct shapetbl;

static const char *STR_FRMT = "%sz%s";
static char rendering3DEnvironmentInitialized;
char titleScreenDisplayed;
sint16 gFadeBrightness = 128;
char *creditShapeFile;
tTexture::ShapeInfo *gHelpShapes;
Car::tObj *gCarObj[2];
static uint16 ofs[2];

void Quick_DD(sint32 a, sint32 b, sint32 c)
{
    FUNCTION_MARKER(0x8004D778, "FRONT.BIN");
    Draw::DirectSetEnvironment(0, 0, 512, 240, a, b, c, 0, 0, 0);
}

void InitializeSpinningCars(void)
{
    FUNCTION_MARKER(0x8004D850, "FRONT.BIN");
    if ((uint8)rendering3DEnvironmentInitialized != 0)
        return;
    R3DCar::InMenu = 1;
    Platform::ResetDCTBuffer();
    Texture::InitMenuTexture();
    inFrontEnd = 1;
    CarIO::StartUp();
    R3DCar::StartUp();
    inFrontEnd = 0;
    DrawC::ReadeMapData();
    Fe3D::InitShowroom();
    GameSetup::gData.Weather = 0;
    for (sint32 i = 0; i < 2; ++i)
    {
        Car::tObj *car = static_cast<Car::tObj *>(reservememadr("carObj", 0x8DC, 0));
        gCarObj[i] = car;
        blockclear(car, 0x8DC);
        car->carInfo = &GameSetup::gData.carInfo[i];
        car->carInfo->carType = 1;
        strcpy(car->carName, GameSetup::gCarNames[car->carInfo->carType]);
        car->N.objID = i | 0x100;
        R3DCar::Instantiate3DCar(car, i);
        car->N.active = 1;
    }
    R3DCar::PostStartUp();
    gMenuRotate[1] = 0;
    gMenuRotate[0] = 0;
    blockclear(&gCView, 140);
    DrawC::gMenuColor[1] = -1;
    DrawC::gMenuColor[0] = -1;
    DrawC::gMenuLights = 0;
    DrawC::gMenuLightsDirection = 0;
    rendering3DEnvironmentInitialized = 1;
}

void PSXFront::AllocateDrawMemory(void)
{
    FUNCTION_MARKER(0x8004D7BC, "FRONT.BIN");
    Draw::InitViewOT();
    gEnviro[0].server = static_cast<char *>(reservememadr("ps0", 80000, 16));
    gEnviro[1].server = static_cast<char *>(reservememadr("ps1", 80000, 16));
    Draw::SetViewMemBudget(Draw::gPlayer1View, 80000);
}

void PSXFront::FreeDrawMemory(void)
{
    FUNCTION_MARKER(0x8004D830, "FRONT.BIN");
    ClearPrimitivesBuffer();
}

void CleanupSpinningCars(void)
{
    FUNCTION_MARKER(0x8004D9CC, "FRONT.BIN");
    if ((uint8)rendering3DEnvironmentInitialized == 0)
        return;

    DrawSync(0);
    for (sint32 i = 0; i < 2; ++i)
    {
        R3DCar::DeInstantiate3DCar(gCarObj[i]);
        purgememadr(gCarObj[i]);
    }
    inFrontEnd = 1;
    R3DCar::CleanUp();
    CarIO::CleanUp();
    inFrontEnd = 0;
    Texture::KillMenuTexture();
    PSXFront::FreeDrawMemory();
    R3DCar::InMenu = 0;
    gFlip = -1;
    rendering3DEnvironmentInitialized = 0;
    Platform::ResetDCTBuffer();

    char fname[60];
    sprintf(fname, "%sDCT.BIN", Paths_Paths[32]);
    uint32 handle = asyncloadfileat(fname, Platform::GetDCTBuffer(0, 0));
    while (getasyncreadstatus(handle) == 0)
        systemtask(0);
}

void CleanupSpinningCarsMenu(void)
{
    FUNCTION_MARKER(0x8004DAC4, "FRONT.BIN");
    if ((uint8)rendering3DEnvironmentInitialized != 0)
    {
        DrawSync(0);
        for (sint32 i = 0; i < 2; ++i)
            R3DCar::DeInstantiate3DCarMenu(gCarObj[i]);
        Texture::CleanupMenuTexture();
        CarIO::ReStart();
    }
}

void DoTitleScreen(void)
{
    FUNCTION_MARKER(0x8004DB34, "FRONT.BIN");
    elapsedticks();
    if (creditShapeFile == 0)
    {
        char fileName[48];
        char artfilename[20];
        sprintf(artfilename, "title.psh");
        sprintf(fileName, STR_FRMT, Paths_Paths[32], artfilename);
        creditShapeFile = static_cast<char *>(loadshapeadr(fileName, 0));
        systemtask(0);
        if (creditShapeFile == 0)
            return;
    }
    shapetbl *tempShp2 = static_cast<shapetbl *>(locateshape(creditShapeFile, "back"));
    Quick_DD(1, 0, 1);
    settrans(0);
    movfxya(tempShp2, 0, 0);
    settrans(1);
    Quick_DD(0, 1, 0);
    purgememadr(creditShapeFile);
    creditShapeFile = 0;
    titleScreenDisplayed = 1;
}

void DoLanguageScreen(void)
{
    FUNCTION_MARKER(0x8004DC34, "FRONT.BIN");
    Front::InitialMemCardCheck();
    if ((uint8)frontEnd.language == 0xFF)
        frontEnd.language = 0;
}

void PSXExitFrontend(void)
{
    FUNCTION_MARKER(0x8004DC70, "FRONT.BIN");
    CleanupSpinningCars();
    deltimer(PAD_update);
    Audio::DeInitDriver();
    if (gHelpShapes != 0)
    {
        purgememadr(gHelpShapes);
        gHelpShapes = 0;
    }
    ComingIntoTheFrontEndTheVeryFirstTime = 0;
}

void PSX_AllocShapes(void)
{
    FUNCTION_MARKER(0x8004DCC8, "FRONT.BIN");
    gHelpShapes = static_cast<tTexture::ShapeInfo *>(reservememadr("gHelpShapes", 0x760, 0));
    blockclear(gHelpShapes, 0x760);
}

void Init_RenderingEnvironment(void)
{
    FUNCTION_MARKER(0x8004DD0C, "FRONT.BIN");
    SetDefDispEnv(&gEnviro[0].disp, 0, 256, 512, 240);
    SetDefDispEnv(&gEnviro[1].disp, 0, 0, 512, 240);
    Draw::InitViews();
    Draw::gRearView = -1;
    Draw::gPlayer1View = Draw::SetView(0, 0, 0, 256, 512, 240, 0, 1, 10);
    blockclear(&gCView, 140);
    gCView.id = Draw::gPlayer1View;
    PSXFront::AllocateDrawMemory();
    Draw::gDoVSync = 1;
    FETextRender::SetABR(0, false);
    gFlip = 0;
}

void Init_PSX_FrontEnd(void)
{
    FUNCTION_MARKER(0x8004DDF8, "FRONT.BIN");
    gFlip = -1;
    addtimer(PAD_update);
    InitGeom();
    PSX_AllocShapes();
    Texture::InitMenuClut();
    screenwidth = 512;
    sint32 firstTime = ComingIntoTheFrontEndTheVeryFirstTime;
    screenbpp = 16;
    if (firstTime != 0)
    {
        sint32 movieResult = play_movie(4);
        if (movieResult != 1)
            play_movie(0);
        initlinkmode(0, 1, 1);
        Init_RenderingEnvironment();
        initlinkmode(0, 20, 0);
        DoLanguageScreen();
        TextSys::LoadWords((uint8)frontEnd.language);
        DoTitleScreen();
        initlinkmode(0, 1, 1);
        Front::SecondaryMemCardCheck();
    }
    else
    {
        Init_RenderingEnvironment();
        TextSys::LoadWords((uint8)frontEnd.language);
    }
    FeTools::init();
    Audio::InitDriver(0xD800, 0x18000);
    SetPlayList(frontEnd.GotAPlayList);
    AudioCmn::LoadFESamples();
    LoadAllHelpShapes();
}

static void AdjustShapeDrawing(tTexture::ShapeInfo *tShp, sint32 &x, sint32 &y, sint32 &flags, sint32 bright, sint32 *color, tDrawShapeExtended *extra)
{
    FUNCTION_MARKER(0x8004DF38, "FRONT.BIN");
    if ((flags & 0x400) != 0)
    {
        x = (sint32)((uint32)x - (uint32)(sint32)tShp->centerx);
        y = (sint32)((uint32)y - (uint32)(sint32)tShp->centery);
    }
    else if (y <= 0)
    {
        x = (sint32)((uint32)x - (uint32)(sint32)tShp->centerx);
        y = (sint32)(0U - ((uint32)(sint32)tShp->centery + (uint32)y));
    }
    if ((flags & 0x100) != 0)
    {
        x = (sint32)((uint32)x - (uint32)((sint32)tShp->width >> 1));
        y = (sint32)((uint32)y - (uint32)((sint32)tShp->height >> 1));
    }
    if ((flags & 0x20) != 0)
    {
        uint32 axis = (uint32)(sint32)extra->flip_axis;
        uint32 height = (uint32)(sint32)tShp->height;
        y = (sint32)((axis << 1) - ((uint32)y + height) + 1U);
        flags |= 2;
    }
    sint32 activeFlags = flags;
    if ((activeFlags & 0x10) != 0)
    {
        sint32 b = (sint32)((((uint32)extra->tint[0] >> 16) & 255U) * (uint32)bright) / 128;
        sint32 g = (sint32)((((uint32)extra->tint[0] >> 8) & 255U) * (uint32)bright) / 128;
        sint32 r = (sint32)(((uint32)extra->tint[0] & 255U) * (uint32)bright) / 128;
        color[0] = (sint32)(((uint32)b << 16) | ((uint32)g << 8) | (uint32)r);
        return;
    }
    if ((activeFlags & 0x80) != 0)
    {
        for (sint32 i = 0; i < 4; ++i)
        {
            sint32 b = (sint32)((((uint32)extra->tint[i] >> 16) & 255U) * (uint32)bright) / 128;
            sint32 g = (sint32)((((uint32)extra->tint[i] >> 8) & 255U) * (uint32)bright) / 128;
            sint32 r = (sint32)(((uint32)extra->tint[i] & 255U) * (uint32)bright) / 128;
            color[i] = (sint32)(((uint32)b << 16) | ((uint32)g << 8) | (uint32)r);
        }
        return;
    }
    if ((activeFlags & 0x40) != 0)
    {
        uint32 axis = (uint16)extra->flip_axis;
        uint32 position = (uint16)y;
        uint32 difference = axis - position + 1U;
        if ((sint16)difference < 0)
            difference = 0U - difference;
        uint32 fadetop = difference << 1;
        if ((sint16)fadetop > 128)
            fadetop = 128;
        difference = axis - (position + (uint16)tShp->height) + 1U;
        if ((sint16)difference < 0)
            difference = 0U - difference;
        uint32 fadebottom = difference << 1;
        if ((sint16)fadebottom > 128)
            fadebottom = 128;
        sint32 fbot = 128 - (sint32)(sint16)fadebottom;
        uint32 rgb = ((uint32)fbot << 16) | ((uint32)fbot << 8) | (uint32)fbot;
        sint32 ftop = 128 - (sint32)(sint16)fadetop;
        color[0] = (sint32)rgb;
        color[1] = (sint32)rgb;
        rgb = ((uint32)ftop << 16) | ((uint32)ftop << 8) | (uint32)ftop;
        color[2] = (sint32)rgb;
        color[3] = (sint32)rgb;
        return;
    }
    color[0] = (sint32)(((uint32)bright << 16) | ((uint32)bright << 8) | (uint32)bright);
}

static void DrawGouraudShape(tTexture::ShapeInfo *shp, sint32 flags, sint32 x, sint32 y, sint32 *color, sint32 abr)
{
    FUNCTION_MARKER(0x8004E264, "FRONT.BIN");
    uint32 height = (uint16)shp->height;
    uint32 width = (uint16)shp->width;
    sint32 bpp = (uint8)shp->depth;
    if ((flags & 2) != 0)
    {
        y = (sint32)((uint32)y + (uint32)(sint32)(sint16)height);
        height = 0U - height;
    }
    sint32 v = (uint8)shp->shapey;
    uint32 vh = (uint16)shp->height;
    if ((flags & 2) != 0)
        --v;
    uint8 baseV = (uint8)v;
    uint32 bottom = height + (uint32)y;
    sint32 i = 0;
    for (;;)
    {
        sint32 offset = (sint16)i;
        sint32 currentWidth = shp->width;
        if (offset >= currentWidth)
            break;
        sint32 pixelOffset = offset * bpp / 16;
        uint32 shapeX = (uint16)shp->shapex;
        sint32 baseNumerator = (sint32)(sint16)shapeX * 16;
        if (bpp == 0)
            abort();
        sint32 baseU = baseNumerator / bpp;
        uint32 pageX = shapeX + (uint32)pixelOffset;
        sint32 pageNumerator = (sint32)(sint16)(pageX & ~63U) * 16;
        sint32 pageU = pageNumerator / bpp;
        sint32 u = (sint32)((uint32)i + (uint32)baseU - (uint32)pageU);
        sint32 w = 255 - (sint32)(sint16)u;
        sint32 remaining = currentWidth - offset;
        if (remaining < w)
            w = remaining;
        Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
        POLY_GT4 *prim = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(prim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
        setaddr(ot, prim);
        uint32 rgb = (uint32)color[0];
        memcpy(&prim->p[0].rgbc, &rgb, sizeof(rgb));
        rgb = (uint32)color[1];
        memcpy(&prim->p[1].rgbc, &rgb, sizeof(rgb));
        rgb = (uint32)color[2];
        memcpy(&prim->p[2].rgbc, &rgb, sizeof(rgb));
        rgb = (uint32)color[3];
        setcode(prim, 0x3C + ((flags & 1) << 1));
        setlen(prim, 12);
        memcpy(&prim->p[3].rgbc, &rgb, sizeof(rgb));
        sint32 clutID = (sint32)shp->clutID;
        prim->p[0].attr = GetClut((clutID & 63) << 4, clutID >> 6);
        uint32 pageY = (uint16)shp->shapey;
        prim->p[1].attr = (uint16)(((shp->type & 3U) << 7) |
                             (((uint32)abr & 3U) << 5) | ((pageY & 0x100U) >> 4) |
                             ((pageX & 0x3C0U) >> 6) | ((pageY & 0x200U) << 2));
        sint32 addw = 0;
        if ((flags & 4) != 0 && shp->width < 255)
        {
            u = (sint32)((uint32)u - 1U);
            addw = 1;
        }
        prim->p[0].uv.u = (uint8)u;
        prim->p[0].uv.v = (uint8)v;
        prim->p[1].uv.u = (uint8)((uint32)u + (uint32)w);
        prim->p[1].uv.v = (uint8)v;
        prim->p[2].uv.u = (uint8)u;
        prim->p[3].uv.u = (uint8)((uint32)u + (uint32)w);
        prim->p[2].uv.v = (uint8)(vh + baseV);
        prim->p[3].uv.v = (uint8)(vh + baseV);
        if ((sint16)w <= 0)
            w = 1;
        uint32 adjust = (uint32)addw - 1U;
        if ((flags & 4) != 0)
        {
            prim->p[0].xy.x = (sint16)(width + (uint32)x - (uint32)i + adjust);
            prim->p[0].xy.y = (sint16)y;
            uint32 shapeWidth = (uint16)shp->width;
            uint32 next = (uint32)i + (uint32)w;
            prim->p[1].xy.y = (sint16)y;
            prim->p[1].xy.x = (sint16)(shapeWidth + (uint32)x - next + adjust);
            shapeWidth = (uint16)shp->width;
            prim->p[2].xy.y = (sint16)bottom;
            prim->p[2].xy.x = (sint16)(shapeWidth + (uint32)x - (uint32)i + adjust);
            shapeWidth = (uint16)shp->width;
            prim->p[3].xy.y = (sint16)bottom;
            prim->p[3].xy.x = (sint16)(shapeWidth + (uint32)x - next + adjust);
        }
        else
        {
            uint32 left = (uint32)i + (uint32)x;
            uint32 right = left + (uint32)w;
            prim->p[0].xy.x = (sint16)left;
            prim->p[0].xy.y = (sint16)y;
            prim->p[1].xy.x = (sint16)right;
            prim->p[1].xy.y = (sint16)y;
            prim->p[2].xy.x = (sint16)left;
            prim->p[2].xy.y = (sint16)bottom;
            prim->p[3].xy.x = (sint16)right;
            prim->p[3].xy.y = (sint16)bottom;
        }
        i = (sint32)((uint32)i + (uint32)w);
    }
}

static void DrawFlatShape(tTexture::ShapeInfo *shp, sint32 flags, sint32 x, sint32 y, sint32 *color, sint32 abr)
{
    FUNCTION_MARKER(0x8004E638, "FRONT.BIN");
    sint32 value = color[0];
    color[3] = value;
    color[2] = value;
    color[1] = value;
    DrawGouraudShape(shp, flags, x, y, color, abr);
}

void DrawShapeExtended(sint32 index, sint32 flags, sint32 x, sint32 y, sint32 fade, sint32 abr, tDrawShapeExtended *extra)
{
    FUNCTION_MARKER(0x8004E678, "FRONT.BIN");
    tTexture::ShapeInfo *shapes = (flags & 8) != 0 ? gHelpShapes : gCurrentShapes;
    sint32 bright = (sint16)((uint32)128 - (uint32)fade);
    if ((flags & 0x200) != 0)
        shapes = extra->custom_shapes;
    tTexture::ShapeInfo *tShp = shapes + index;
    sint32 color[4];
    AdjustShapeDrawing(tShp, x, y, flags, bright, color, extra);
    if ((flags & 0xc0) == 0)
        DrawFlatShape(tShp, flags, x, y, color, abr);
    else
        DrawGouraudShape(tShp, flags, x, y, color, abr);
}

static void ScaleGouraudShape(tTexture::ShapeInfo *shp, sint32 flags, sint32 x, sint32 y, sint32 scalex, sint32 scaley, sint32 *color, sint32 abr)
{
    FUNCTION_MARKER(0x8004E77C, "FRONT.BIN");
    sint32 width = (uint16)shp->width;
    sint32 height = (uint16)shp->height;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_GT4 *prim = reinterpret_cast<POLY_GT4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    sint32 bpp = (uint8)shp->depth;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    uint32 rgb = (uint32)color[0];
    memcpy(&prim->p[0].rgbc, &rgb, sizeof(rgb));
    rgb = (uint32)color[1];
    memcpy(&prim->p[1].rgbc, &rgb, sizeof(rgb));
    rgb = (uint32)color[2];
    memcpy(&prim->p[2].rgbc, &rgb, sizeof(rgb));
    rgb = (uint32)color[3];
    memcpy(&prim->p[3].rgbc, &rgb, sizeof(rgb));
    SetPolyGT4(prim);
    SetSemiTrans(prim, flags & 1);
    sint32 clutID = (sint32)shp->clutID;
    prim->p[0].attr = GetClut((clutID & 63) << 4, clutID >> 6);
    uint32 pageY = (uint16)shp->shapey;
    uint32 pageX = (uint16)shp->shapex;
    prim->p[1].attr = (uint16)(((shp->type & 3U) << 7) |
                         (((uint32)abr & 3U) << 5) | ((pageY & 0x100U) >> 4) |
                         ((pageX & 0x3C0U) >> 6) | ((pageY & 0x200U) << 2));
    if ((flags & 4) != 0)
    {
        x = (sint32)((uint32)x + (uint32)fixedmult(scalex, (sint16)width));
        width = -width;
    }
    if ((flags & 2) != 0)
    {
        y = (sint32)((uint32)y + (uint32)fixedmult(scaley, (sint16)height));
        height = -height;
    }
    width = (sint16)width;
    prim->p[0].xy.x = (sint16)x;
    prim->p[0].xy.y = (sint16)y;
    sint32 scaled = fixedmult(scalex, width);
    height = (sint16)height;
    uint32 right = (uint32)x - 1U;
    prim->p[1].xy.x = (sint16)(right + (uint32)scaled);
    prim->p[1].xy.y = (sint16)y;
    prim->p[2].xy.x = (sint16)x;
    scaled = fixedmult(scaley, height);
    prim->p[2].xy.y = (sint16)((uint32)y + (uint32)scaled);
    scaled = fixedmult(scalex, width);
    right += (uint32)scaled;
    prim->p[3].xy.x = (sint16)right;
    scaled = fixedmult(scaley, height);
    prim->p[3].xy.y = (sint16)((uint32)y + (uint32)scaled);
    sint32 numerator = ((uint16)shp->shapex & 63U) << 4;
    if (bpp == 0)
        abort();
    sint32 u = numerator / bpp;
    sint32 uw = (uint8)shp->width;
    sint32 vh = (uint8)shp->height;
    sint32 v = (uint8)shp->shapey;
    if ((flags & 4) != 0)
        --u;
    if ((flags & 2) != 0)
        --v;
    prim->p[0].uv.v = (uint8)v;
    prim->p[1].uv.v = (uint8)v;
    prim->p[0].uv.u = (uint8)u;
    prim->p[1].uv.u = (uint8)(u + uw);
    prim->p[2].uv.u = (uint8)u;
    prim->p[2].uv.v = (uint8)(v + vh);
    prim->p[3].uv.u = (uint8)(u + uw);
    prim->p[3].uv.v = (uint8)(v + vh);
}

static void ScaleFlatShape(tTexture::ShapeInfo *shp, sint32 flags, sint32 x, sint32 y, sint32 scalex, sint32 scaley, sint32 *color, sint32 abr)
{
    FUNCTION_MARKER(0x8004EA38, "FRONT.BIN");
    sint32 value = color[0];
    color[3] = value;
    color[2] = value;
    color[1] = value;
    ScaleGouraudShape(shp, flags, x, y, scalex, scaley, color, abr);
}

void ScaleShapeExtended(sint32 index, sint32 flags, sint32 x, sint32 y, sint32 fade, sint32 abr, tDrawShapeExtended *extra)
{
    FUNCTION_MARKER(0x8004EA88, "FRONT.BIN");
    sint32 scalex = 131072;
    sint32 scaley = 65536;
    tTexture::ShapeInfo *shapes = (flags & 8) != 0 ? gHelpShapes : gCurrentShapes;
    sint32 bright = (sint16)((uint32)128 - (uint32)fade);
    if ((flags & 0x200) != 0)
        shapes = extra->custom_shapes;
    tTexture::ShapeInfo *tShp = shapes + index;
    sint32 color[4];
    AdjustShapeDrawing(tShp, x, y, flags, bright, color, extra);
    if ((flags & 0xc0) == 0)
        ScaleFlatShape(tShp, flags, x, y, scalex, scaley, color, abr);
    else
        ScaleGouraudShape(tShp, flags, x, y, scalex, scaley, color, abr);
}

void LoadAllHelpShapes(void)
{
    FUNCTION_MARKER(0x8004EBB4, "FRONT.BIN");
    char fullName[48];
    sprintf(fullName, "%szperm.psh", Paths_Paths[32]);
    char *permFile = static_cast<char *>(loadshapeadr(fullName, 0));
    if (permFile != 0)
    {
        for (sint32 i = 0; i < 59; ++i)
            FETexture::LoadPmx(permFile, i, &gHelpShapes[i]);
        purgememadr(permFile);
    }
}

void PSXDrawSquare(sint32 col, sint32 x, sint32 y, sint32 w, sint32 h)
{
    FUNCTION_MARKER(0x8004EC44, "FRONT.BIN");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_F4 *prim = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    uint32 color = (uint32)col;
    memcpy(&prim->rgbc, &color, sizeof(color));
    setcode(prim, 0x28);
    prim->p[0].y = (sint16)y;
    prim->p[1].y = (sint16)y;
    uint32 right = (uint32)x + (uint32)w;
    uint32 bottom = (uint32)y + (uint32)h;
    setlen(prim, 5);
    prim->p[0].x = (sint16)x;
    prim->p[1].x = (sint16)right;
    prim->p[2].x = (sint16)x;
    prim->p[2].y = (sint16)bottom;
    prim->p[3].x = (sint16)right;
    prim->p[3].y = (sint16)bottom;
}

void PSXDrawGouraudSquare(sint32 x, sint32 y, sint32 w, sint32 h, sint32 c1, sint32 c2, sint32 c3, sint32 c4)
{
    FUNCTION_MARKER(0x8004ECDC, "FRONT.BIN");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_G4 *prim = reinterpret_cast<POLY_G4 *>(cprim->PrimPtr);
    uint32 *ot = cprim->LastPrim;
    setaddr(prim, getaddr(ot));
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    setaddr(ot, prim);
    uint32 color = (uint32)c1;
    memcpy(&prim->p[0].rgbc, &color, sizeof(color));
    color = (uint32)c2;
    memcpy(&prim->p[1].rgbc, &color, sizeof(color));
    color = (uint32)c3;
    memcpy(&prim->p[2].rgbc, &color, sizeof(color));
    color = (uint32)c4;
    memcpy(&prim->p[3].rgbc, &color, sizeof(color));
    SetPolyG4(prim);
    uint32 right = (uint32)x + (uint32)w;
    uint32 bottom = (uint32)y + (uint32)h;
    prim->p[0].xy.y = (sint16)y;
    prim->p[1].xy.y = (sint16)y;
    prim->p[0].xy.x = (sint16)x;
    prim->p[1].xy.x = (sint16)right;
    prim->p[2].xy.x = (sint16)x;
    prim->p[2].xy.y = (sint16)bottom;
    prim->p[3].xy.x = (sint16)right;
    prim->p[3].xy.y = (sint16)bottom;
}

void PSXDrawTransGouraudSquare(sint32 x, sint32 y, sint32 w, sint32 h, sint32 opacity, sint32 c1, sint32 c2, sint32 c3, sint32 c4)
{
    FUNCTION_MARKER(0x8004EDCC, "FRONT.BIN");
    if (opacity <= 0)
        return;
    uint32 top = (uint32)y << 16;
    uint32 bottom = ((uint32)y + (uint32)h) << 16;
    uint32 right = (uint32)x + (uint32)w;
    uint32 xy0 = top | (uint32)x;
    uint32 xy1 = top | right;
    uint32 xy2 = bottom | (uint32)x;
    uint32 xy3 = bottom | right;
    for (sint32 i = 0; i < opacity; ++i)
    {
        Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
        POLY_G4 *prim = reinterpret_cast<POLY_G4 *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(prim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
        setaddr(ot, prim);
        memcpy(&prim->p[0].xy, &xy0, sizeof(xy0));
        memcpy(&prim->p[2].xy, &xy2, sizeof(xy2));
        uint32 color = (uint32)c1;
        memcpy(&prim->p[0].rgbc, &color, sizeof(color));
        color = (uint32)c2;
        memcpy(&prim->p[1].rgbc, &color, sizeof(color));
        color = (uint32)c3;
        memcpy(&prim->p[2].rgbc, &color, sizeof(color));
        color = (uint32)c4;
        memcpy(&prim->p[3].rgbc, &color, sizeof(color));
        setcode(prim, 0x39);
        setlen(prim, 8);
        memcpy(&prim->p[1].xy, &xy1, sizeof(xy1));
        memcpy(&prim->p[3].xy, &xy3, sizeof(xy3));
    }
}

void PSXDrawTransSquare(sint32 col, sint32 x, sint32 y, sint32 w, sint32 h, sint16 opacity)
{
    FUNCTION_MARKER(0x8004EED0, "FRONT.BIN");
    if (opacity <= 0)
        return;
    uint32 right = (uint32)x + (uint32)w;
    uint32 bottom = (uint32)y + (uint32)h;
    for (sint32 i = 0; i < opacity; ++i)
    {
        Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
        POLY_F4 *prim = reinterpret_cast<POLY_F4 *>(cprim->PrimPtr);
        uint32 *ot = cprim->LastPrim;
        setaddr(prim, getaddr(ot));
        cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
        setaddr(ot, prim);
        uint32 color = (uint32)col;
        memcpy(&prim->rgbc, &color, sizeof(color));
        setcode(prim, 0x2A);
        setlen(prim, 5);
        prim->p[0].x = (sint16)x;
        prim->p[0].y = (sint16)y;
        prim->p[1].x = (sint16)right;
        prim->p[1].y = (sint16)y;
        prim->p[2].x = (sint16)x;
        prim->p[2].y = (sint16)bottom;
        prim->p[3].x = (sint16)right;
        prim->p[3].y = (sint16)bottom;
    }
}

void FontUpsideDownBlit(sint32 x, sint32 y, void *src, sint32 u, sint32 v, charactertbl *ch, sint32)
{
    FUNCTION_MARKER(0x8004EFB4, "FRONT.BIN");
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    POLY_FT4 *prim = reinterpret_cast<POLY_FT4 *>(cprim->PrimPtr);
    sint32 width = ch->width;
    sint32 height = ch->height;
    sint32 yoffset = (sint8)ch->yoffset;
    cprim->PrimPtr = reinterpret_cast<char *>(prim + 1);
    uint32 *ot = cprim->LastPrim;
    uint32 packed;
    memcpy(&packed, static_cast<const char *>(src) + 12, sizeof(packed));
    sint32 dv = (sint32)(packed << 4) >> 20;
    sint32 row = (sint32)(((uint32)dv + (uint32)v) & 255U) - 1;
    setaddr(prim, getaddr(ot));
    uint32 tint = (uint32)font::tint;
    setaddr(ot, prim);
    memcpy(&prim->rgbc, &tint, sizeof(tint));
    setlen(prim, 9);
    setcode(prim, 0x2C);
    uint32 upper = (uint32)y - (uint32)yoffset * 2U + 5U - (uint32)height;
    prim->p[0].attr = gFontClut;
    uint32 type = *static_cast<const uint8 *>(src);
    memcpy(&packed, static_cast<const char *>(src) + 12, sizeof(packed));
    prim->p[0].uv.v = (uint8)row;
    prim->p[1].uv.v = (uint8)row;
    prim->p[0].uv.u = (uint8)u;
    prim->p[1].uv.u = (uint8)((uint32)u + (uint32)width);
    prim->p[2].uv.u = (uint8)u;
    prim->p[2].uv.v = (uint8)(row + height);
    prim->p[3].uv.u = (uint8)((uint32)u + (uint32)width);
    prim->p[3].uv.v = (uint8)(row + height);
    prim->p[0].xy.x = (sint16)x;
    prim->p[1].attr = (uint16)(((type & 3U) << 7) |
                         ((packed >> 20) & 16U) | ((packed & 0x3FFU) >> 6));
    uint32 right = (uint32)x + (uint32)width;
    uint32 lower = upper + (uint32)height;
    prim->p[0].xy.y = (sint16)lower;
    prim->p[1].xy.x = (sint16)right;
    prim->p[1].xy.y = (sint16)lower;
    prim->p[2].xy.x = (sint16)x;
    prim->p[2].xy.y = (sint16)upper;
    prim->p[3].xy.x = (sint16)right;
    prim->p[3].xy.y = (sint16)upper;
}

void PSXFront::ResetOverlayGraphics(void)
{
    rendering3DEnvironmentInitialized = 0;
    gCarObj[0] = gCarObj[1] = 0;
    gHelpShapes = 0;
    gCurrentShapes = 0;
    creditShapeFile = 0;
    titleScreenDisplayed = 0;
    gFadeBrightness = 128;
    ofs[0] = ofs[1] = 0;
    showRoomFlag = 0;
    gMenuRotate[0] = gMenuRotate[1] = 0;
    gShowroomLights = 0;
    font12 = font18 = fontTitle = 0;
    currentSize = 0;
}
