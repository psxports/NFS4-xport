#include <string.h>
#include "xport.h"
#include "psx.h"
#include "psx_mdec.h"
#include "frontend/common/frontmovie.h"
#include "game/psx/platform.h"

void InitializeFrontMovie(sint32 &session)
{
    Platform::ResetDCTBuffer();
    session = 0;
}

void StopFrontMovie(sint32 &session)
{
    if (session != 0)
    {
        xport_fmv_close();
        session = 0;
    }
}

bool FrontMovieNameMatches(const char *movieName, const char *expectedName)
{
    size_t length = strlen(movieName);
    if (length != strlen(expectedName))
        return false;
    for (size_t i = 0; i < length; ++i)
    {
        char letter = movieName[i];
        if (letter >= 'a' && letter <= 'z')
            letter = char(letter - 'a' + 'A');
        if (letter != expectedName[i])
            return false;
    }
    return true;
}

bool StartFrontMovie(sint32 &session, const char *logicalPath, const char *preparedPath)
{
    if (session != 0)
        return false;
    if (!xport_fmv_set_native_source(logicalPath, preparedPath) || xport_fmv_open(logicalPath) <= 0)
        return false;
    session = 1;
    return true;
}

sint32 UploadFrontMovieFrame(sint32 &session, const sint32 &frameCounter)
{
    return UploadFrontMovieFrameSized(session, frameCounter, 80, 80);
}

sint32 UploadFrontMovieFrameSized(sint32 &session, const sint32 &frameCounter, sint32 width, sint32 height)
{
    if ((width != 80 || height != 80) && (width != 160 || height != 128))
    {
        StopFrontMovie(session);
        return -1;
    }

    static uint8 rgb[160 * 128 * 3];
    static uint32 pixels[160 * 128 / 2];
    XportFmvFrame frame;
    sint32 result = xport_fmv_next(rgb, sizeof(rgb), &frame);
    if (result == 0)
        return 0;
    if (result != 1 || frame.width != width || frame.height != height)
    {
        StopFrontMovie(session);
        return result == 2 ? 2 : -1;
    }
    for (sint32 i = 0; i < width * height; i += 2)
    {
        uint32 first = uint32(rgb[i * 3]) | (uint32(rgb[i * 3 + 1]) << 8) | (uint32(rgb[i * 3 + 2]) << 16);
        uint32 second = uint32(rgb[i * 3 + 3]) | (uint32(rgb[i * 3 + 4]) << 8) | (uint32(rgb[i * 3 + 5]) << 16);
        pixels[i / 2] = mdec_rgb_fifteen(first, 1) | (mdec_rgb_fifteen(second, 1) << 16);
    }
    PSX_RECT r = { 512, (sint16)((frameCounter & 1) == 0 ? height : 0), (sint16)width, (sint16)height };
    if (LoadImagePSX(&r, pixels) < 0 || DrawSync(0) < 0)
    {
        StopFrontMovie(session);
        return -1;
    }
    return 1;
}
