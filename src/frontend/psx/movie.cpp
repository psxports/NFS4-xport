#include <stdio.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "frontend/psx/movie.h"
#include "game/common/audiocmn.h"
#include "game/psx/platform.h"

sint16 user_exit;
char skip_all;
char gPlayerNum;
sint16 movieframes[5] = { 1358, 456, 444, 480, 120 };
static char movieName0[] = "NFS4TITL.XA";
static char movieName1[] = "DEMO1AV.XA";
static char movieName2[] = "DEMO2AV.XA";
static char movieName3[] = "DEMO3AV.XA";
static char movieName4[] = "EADOLBY.XA";
char *movienames[5] = { movieName0, movieName1, movieName2, movieName3, movieName4 };

static const char *const preparedMovieNames[5] =
{
    "VIDEO/native/NFS4TITL.STR",
    "VIDEO/native/DEMO1AV.STR",
    "VIDEO/native/DEMO2AV.STR",
    "VIDEO/native/DEMO3AV.STR",
    "VIDEO/native/EADOLBY.STR"
};

sint32 Movie::Play(char movie)
{
    FUNCTION_MARKER(0x8004CCE4, "FRONT.BIN");
    uint32 index = (uint8)movie;
    if (index >= 5)
        return -1;
    char path[80];
    sprintf(path, "DATA/MOVIES/%s", movienames[index]);
    if (!xport_fmv_set_native_source(path, preparedMovieNames[index]))
        return -1;
    SNDcdvol((sint32)(uint32(gMasterMusicLevel) * 127u) >> 7);
    PSX_RECT clear = { 0, 0, 960, 240 };
    ClearImage(&clear, 0, 0, 0);
    clear.y = 256;
    ClearImage(&clear, 0, 0, 0);
    DrawSync(0);
    Platform::ResetDCTBuffer();
    clear.y = 0;
    clear.h = 480;
    ClearImage(&clear, 0, 0, 0);
    DrawSync(0);
    XportFmvPresentation presentation = {0};
    presentation.width = 256;
    presentation.height = 240;
    presentation.screen_height = 240;
    presentation.controllers = 3;
    presentation.skip_buttons = 0xFFFF;
    presentation.skip_held = 1;
    uint32 buttons = 0;
    XportFmvPlayback playback = { &presentation, 256, &buttons, 1, 1 };
    sint32 result = xport_fmv_play_configured(path, movieframes[index] - 2, 0, &playback);
    Platform::ResetDCTBuffer();
    clear.w = 384;
    clear.h = 240;
    ClearImage(&clear, 0, 0, 0);
    clear.y = 256;
    ClearImage(&clear, 0, 0, 0);
    DrawSync(0);
    if (result < 0)
        return -1;
    if (result == 2 && (skip_all != 0 || buttons == 8))
        user_exit = 1;
    return user_exit;
}

sint32 play_movie(char movie)
{
    FUNCTION_MARKER(0x8004CEF4, "FRONT.BIN");
    return Movie::Play(movie);
}
