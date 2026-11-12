#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "frontend/common/feaudio.h"
#include "frontend/common/fescreen.h"
#include "frontend/common/fetextrender.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/psxfront.h"
#include "game/common/paths.h"
#include "game/common/textsys.h"
#include "game/psx/draw.h"
#include "game/psx/textpix.h"
#include "game/psx/texture.h"

tTexture::ShapeInfo *gCurrentShapes;
sint32 tScreen::fSuppressLoadingText;

void tScreen::DisplayLoadingText(void)
{
    FUNCTION_MARKER(0x80025AB8, "FRONT.BIN");
    if (fSuppressLoadingText != 0)
    {
        fSuppressLoadingText = 0;
        return;
    }
    sint32 view = Draw::gPlayer1View;
    sint32 oldIsBg = Draw::gView[view].drawenv[0].isbg;
    Draw::gView[view].drawenv[1].isbg = 0;
    Draw::gView[view].drawenv[0].isbg = 0;
    for (sint16 i = 0; i < 2; ++i)
    {
        Draw::StartFrameRender();
        Draw::StartRenderingView(Draw::gPlayer1View);
        FETextRender::MenuTextPositionedJustify(637, 480, 220, 1, textState_Selected, textType_ScreenInfo);
        sint32 width = ((~((uint32)textpixels(TextSys::Word(637))) + 1) - 5);
        PSXDrawSquare(0, 480, 220, width, 7);
        Draw::StopRenderingView(Draw::gPlayer1View);
        Draw::StopFrameRender();
    }
    view = Draw::gPlayer1View;
    Draw::gView[view].drawenv[1].isbg = (uint8)oldIsBg;
    Draw::gView[view].drawenv[0].isbg = (uint8)oldIsBg;
}

void tScreen::GoNonInterlaced(void)
{
    FUNCTION_MARKER(0x80025BF8, "FRONT.BIN");
    screenheight = 240;
    sint16 height = (sint16)screenheight;
    gEnviro[0].disp.isinter = 0;
    gEnviro[1].disp.isinter = 0;
    sint32 view = Draw::gPlayer1View;
    gEnviro[0].disp.disp.y = 256;
    gEnviro[1].disp.disp.y = 0;
    gEnviro[0].disp.disp.h = height;
    gEnviro[0].disp.screen.h = height;
    gEnviro[1].disp.screen.h = height;
    DRAWENV *drenv = &Draw::gView[view].drawenv[0];
    drenv->dfe = 0;
    view = Draw::gPlayer1View;
    drenv->clip.y = 0;
    drenv->clip.h = height;
    drenv->ofs[0] = 0;
    drenv->ofs[1] = 0;
    drenv = &Draw::gView[view].drawenv[1];
    height = (sint16)screenheight;
    drenv->clip.y = 256;
    drenv->ofs[0] = 0;
    drenv->ofs[1] = 256;
    drenv->dfe = 0;
    drenv->clip.h = height;
    DrawSync(0);
    VSync(0);
}

void tScreen::DrawBackgroundImage(sint32 startShape, sint32 numShapes, tTexture::ShapeInfo *shapes, sint32 flip_axis)
{
    FUNCTION_MARKER(0x80025CC8, "FRONT.BIN");
    tDrawShapeExtended drawFlags;
    drawFlags.flip_axis = (sint16)flip_axis;
    drawFlags.custom_shapes = shapes;
    sint32 endShape = (startShape + numShapes);
    for (sint32 i = startShape; i < endShape; ++i)
    {
        DrawShapeExtended(i, 0x200, 0, 0, fScreenFadeVal, 0, &drawFlags);
        if (flip_axis > 0)
            DrawShapeExtended(i, 0x260, 0, 0, fScreenFadeVal, 0, &drawFlags);
    }
}

void tScreen::AsyncLoadPermanentShapeFile(char *fileName)
{
    FUNCTION_MARKER(0x80025D98, "FRONT.BIN");
    AsyncLoadShapeFile(fileName, fPermShapes);
}

void tScreen::AsyncLoadSwapShapeFile(char *fileName)
{
    FUNCTION_MARKER(0x80025DB8, "FRONT.BIN");

    if (fileName != 0)
    {
        char buffer[32];
        sprintf(buffer, "z%s", fileName);
        AsyncLoadShapeFile(buffer, fSwapShapes);
    }
    else
        AsyncLoadShapeFile(0, fSwapShapes);
}

bool tScreen::IsShapeFileLoaded(tShapeInformation &shapes)
{
    FUNCTION_MARKER(0x80025E0C, "FRONT.BIN");

    bool result = true;
    if (shapes.async_handle != 0)
    {
        sint32 async_status = getasyncreadstatus(shapes.async_handle);
        if (async_status > 0)
        {
            char *file = shapes.fDestFile;
            if (file == 0)
                file = getasyncreadadr(shapes.async_handle);
            shapes.fFile = file;
            shapes.async_handle = 0;
            if (shapes.fFile != 0 && shapes.fLoadCancelled)
            {
                if (shapes.fDestFile == 0)
                    purgememadr(shapes.fFile);
                shapes.fFile = 0;
            }
        }
        else
        {
            if (async_status == -1)
            {
                char *bogus = getasyncreadadr(shapes.async_handle);
                if (bogus != 0)
                    purgememadr(bogus);
                AsyncLoadShapeFile(shapes.fFilename, shapes);
            }
            else if (async_status == -2)
                AsyncLoadShapeFile(shapes.fFilename, shapes);
            result = false;
        }
    }
    else if (shapes.fLoadCancelled)
    {
        if (shapes.fFile != 0)
        {
            if (shapes.fDestFile == 0)
                purgememadr(shapes.fFile);
            shapes.fFile = 0;
        }
        shapes.fLoadCancelled = false;
        AsyncLoadShapeFile(shapes.fFilename, shapes);
        IsShapeFileLoaded(shapes);
        result = false;
    }
    return result;
}

void tScreen::UploadPermanentShapes(sint32 numPermanentShapes)
{
    FUNCTION_MARKER(0x80025F84, "FRONT.BIN");
    UploadShapes(fPermShapes, 0, 0, (sint16)numPermanentShapes, 0);
    gCurrentShapes = fPermShapes.fShapes;
}

void tScreen::UploadSwapShapes(sint32 numSwapShapes)
{
    FUNCTION_MARKER(0x80025FD0, "FRONT.BIN");
    UploadShapes(fSwapShapes, 0, 0, (sint16)numSwapShapes, 0);
}

void tScreen::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x80026008, "FRONT.BIN");
    numSwapShapes = 0;
    numPermShapes = 0;
    *swapFileName = 0;
    *permFileName = 0;
}

tScreen::tScreen(void)
{
    FUNCTION_MARKER(0x80026020, "FRONT.BIN");
    fPermShapes.fShapes = 0;
    InitializeShapes(fPermShapes, 0);
    fSwapShapes.fShapes = 0;
    InitializeShapes(fSwapShapes, 0);
    fScreenFadeVal = 0;
}

tScreen::~tScreen(void)
{
    FUNCTION_MARKER(0x80026078, "FRONT.BIN");
}

void tScreen::Initialize(void)
{
    FUNCTION_MARKER(0x800260AC, "FRONT.BIN");

    DisplayLoadingText();
    PreLoad();
    sint16 numPermShapes;
    sint16 numSwapShapes;
    char *permFileName;
    char *swapFileName;
    GetShapeInfo(numPermShapes, numSwapShapes, &permFileName, &swapFileName);
    bool shapesLoaded;
    do
    {
        FeAudio::systemtask(0);
        shapesLoaded = IsShapeFileLoaded(fPermShapes);
        if (fPermShapes.fFile != 0)
            UploadPermanentShapes(numPermShapes);
        shapesLoaded = shapesLoaded && IsShapeFileLoaded(fSwapShapes);
        if (fSwapShapes.fFile != 0)
            UploadSwapShapes(numSwapShapes);
    } while (!shapesLoaded);
    fPermShapes.fNumShapes = numPermShapes;
    fSwapShapes.fNumShapes = numSwapShapes;
    GoNonInterlaced();
}

void tScreen::Cleanup(void)
{
    FUNCTION_MARKER(0x800261A0, "FRONT.BIN");
    AsyncLoadPermanentShapeFile(0);
    AsyncLoadSwapShapeFile(0);
    FreeShapes(fPermShapes);
    FreeShapes(fSwapShapes);
}

void tScreen::Draw(bool drawBackground)
{
    FUNCTION_MARKER(0x800261EC, "FRONT.BIN");
    if (drawBackground)
        DrawBackground();
    else
        DrawForeground();
}

void tScreen::AsyncLoadShapeFile(char *name, tShapeInformation &data)
{
    FUNCTION_MARKER(0x80026240, "FRONT.BIN");

    if (name == 0 && data.async_handle != 0)
    {
        cancelasyncload(data.async_handle);
        data.async_handle = 0;
    }
    CancelAsyncLoad(data);
    if (data.fFile != 0)
    {
        if (data.fDestFile == 0)
            purgememadr(data.fFile);
        data.fFile = 0;
    }
    data.fFlags &= 0xfffe;
    if (name != 0)
    {
        memmove(data.fFilename, name, strlen(name) + 1);
        char buffer[128];
        sprintf(buffer, "%s%s.psh", Paths_Paths[32], name);
        if (!data.fLoadCancelled)
        {
            data.fLoadCancelled = false;
            if (data.fDestFile != 0)
                data.async_handle = asyncloadfileat(buffer, data.fDestFile);
            else
                data.async_handle = asyncloadfile(buffer, 16);
        }
    }
}

void tScreen::CancelAsyncLoad(tShapeInformation &data)
{
    FUNCTION_MARKER(0x80026354, "FRONT.BIN");

    if (data.async_handle != 0)
    {
        if (getasyncreadstatus(data.async_handle) == 0)
            data.fLoadCancelled = true;
        else
        {
            if (data.fDestFile == 0)
                data.fFile = getasyncreadadr(data.async_handle);
            if (data.fFile != 0)
            {
                if (data.fDestFile == 0)
                    purgememadr(data.fFile);
                data.fFile = 0;
            }
            data.async_handle = 0;
        }
    }
}

void tScreen::InitializeShapes(tShapeInformation &data, uint32 numShapes)
{
    FUNCTION_MARKER(0x800263EC, "FRONT.BIN");
    if (data.fShapes != 0)
    {
        purgememadr(data.fShapes);
        data.fShapes = 0;
    }
    data.fFlags = 0;
    data.fNumShapes = (uint16)numShapes;
    data.async_handle = 0;
    data.fFile = 0;
    data.fDestFile = 0;
    data.fLoadCancelled = false;
    if (numShapes != 0)
    {
        data.fShapes = (tTexture::ShapeInfo *)reservememadr("Shapes", (sint32)(numShapes * sizeof(tTexture::ShapeInfo)), 0);
        for (uint16 i = 0; i < numShapes; ++i)
            data.fShapes[i].clutID = 0;
    }
}

void tScreen::FreeShapes(tShapeInformation &data)
{
    FUNCTION_MARKER(0x80026494, "FRONT.BIN");

    CancelAsyncLoad(data);
    while (data.async_handle != 0)
    {
        sint32 async_status = getasyncreadstatus(data.async_handle);
        if (async_status > 0 || async_status == -1)
        {
            if (data.fDestFile == 0)
                data.fFile = getasyncreadadr(data.async_handle);
            else
                data.fFile = 0;
            data.async_handle = 0;
        }
        else if (async_status == -2)
            data.async_handle = 0;
        FeAudio::systemtask(0);
    }
    if (data.fFile != 0)
    {
        if (data.fDestFile == 0)
            purgememadr(data.fFile);
        data.fFile = 0;
    }
    if (data.fShapes != 0)
    {
        for (sint16 i = 0; i < data.fNumShapes; ++i)
            if (data.fShapes[i].clutID != 0)
                Texture::MenuReleaseClutId(data.fShapes[i].clutID);
        purgememadr(data.fShapes);
        data.fShapes = 0;
    }
}

void tScreen::UploadShapes(tShapeInformation &data, sint16 x, sint16 y, sint16 numShapes, sint16 index)
{
    FUNCTION_MARKER(0x800265F4, "FRONT.BIN");

    if (data.fFile != 0)
    {
        if (numShapes == 0)
            numShapes = data.fNumShapes;
        for (sint32 i = 0; i < numShapes; ++i)
            if (data.fShapes[i + index].clutID != 0)
            {
                Texture::MenuReleaseClutId(data.fShapes[i + index].clutID);
                data.fShapes[i + index].clutID = 0;
            }
        data.fFlags &= 0xfffe;
        for (sint32 i = 0; i < numShapes; ++i)
            FETexture::LoadPmxAtOffset(data.fFile, i, data.fShapes + i + index, y, x);
        data.fFlags |= 1;
        if (data.fDestFile == 0)
            purgememadr(data.fFile);
        data.fFile = 0;
    }
}

void tScreen::PreLoad(void)
{
    FUNCTION_MARKER(0x80026780, "FRONT.BIN");
    sint16 numPermShapes;
    sint16 numSwapShapes;
    char *permFileName;
    char *swapFileName;
    GetShapeInfo(numPermShapes, numSwapShapes, &permFileName, &swapFileName);
    InitializeShapes(fPermShapes, numPermShapes);
    InitializeShapes(fSwapShapes, numSwapShapes);
    AsyncLoadPermanentShapeFile(permFileName);
    AsyncLoadSwapShapeFile(swapFileName);
}

void tScreen::TransitionOff(tScreen::TransitionType type, tMenu *)
{
    FUNCTION_MARKER(0x80026804, "FRONT.BIN");
    sint32 startTicks = ticks;
    fScreenFadeVal = 0;
    fInternalScreenFadeVal = 0;
    fTransitionOff = true;
    fTransitionTicks = startTicks;
}

void tScreen::TransitionOn(tScreen::TransitionType type, tMenu *)
{
    FUNCTION_MARKER(0x80026824, "FRONT.BIN");
    fScreenFadeVal = 128;
    sint32 startTicks = ticks;
    fInternalScreenFadeVal = 128;
    fTransitionOff = false;
    fTransitionTicks = startTicks;
}

void tScreen::UpdateTransition(void)
{
    FUNCTION_MARKER(0x80026848, "FRONT.BIN");
    fInternalScreenFadeVal = (fInternalScreenFadeVal + (fTransitionOff ? 12U : (uint32)-12));
    if (fInternalScreenFadeVal < -50)
        fInternalScreenFadeVal = -50;
    if (fInternalScreenFadeVal > 150)
        fInternalScreenFadeVal = 150;
    fScreenFadeVal = (sint16)fInternalScreenFadeVal;
    if (fScreenFadeVal < 0)
        fScreenFadeVal = 0;
    else if (fScreenFadeVal > 128)
        fScreenFadeVal = 128;
}

bool tScreen::TransitionIsFinished(void)
{
    FUNCTION_MARKER(0x800268C0, "FRONT.BIN");
    return fInternalScreenFadeVal + 24U >= 167U;
}

void tScreen::ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command)
{
    FUNCTION_MARKER(0x800268D8, "FRONT.BIN");
}

void tScreen::DrawForeground(void)
{
    FUNCTION_MARKER(0x800268E0, "FRONT.BIN");
}

void tScreen::DrawBackground(void)
{
    FUNCTION_MARKER(0x800268E8, "FRONT.BIN");
}
