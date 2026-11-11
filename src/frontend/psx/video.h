#ifndef VIDEO_H
#define VIDEO_H

#include "xport.h"

enum VIDEOSTATE
{
    VIDEOSTATE_IDLE = 0,
    VIDEOSTATE_SPOOLING = 1,
    VIDEOSTATE_READY = 2,
    VIDEOSTATE_PLAYING = 3
};

class VIDEO
{
public:
    static sint32 create(sint32 width, sint32 height, sint32 fps, sint32 streambuffersize, sint32 memtype);
    static void destroy(sint32 handle);
    static void spoolfile(sint32 handle, char *fname);
    static void startplayback(sint32 handle);
    static void abortplayback(sint32 handle);
    static VIDEOSTATE state(sint32 handle);
    static sint32 updateframexy(sint32 handle, sint32 x, sint32 y);
};

#endif
