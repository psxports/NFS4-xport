#include "xport.h"
#include "eaclib/psx/internal.h"
#include "splysdef.h"

sint32 SNDplaysetdef(struct SNDPLAYOPTS *options)
{
    FUNCTION_MARKER(0x800E6834, "NFS4.EXE");
    options->patnum = -1;
    options->bhandle = (char)0xFF;
    options->keynum = 60;
    options->pitchmult = 0x1000;
    options->timemult = 0x1000;
    options->velocity = 127;
    options->vol = 127;
    options->fxlevel0 = 127;
    options->pan = 64;
    options->bend = 64;
    options->use3dpos = 0;
    options->azimuth = 0;
    options->elevation = 0;
    return 0;
}
