#include <stdio.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/gmesetup.h"
#include "game/common/paths.h"
#include "game/psx/loading.h"
#include "game/psx/texture.h"
#include "lib/psx/shpsubs.h"

struct shapetbl;

sint32 totalAvailMem;
char *smallShapeFile;
static const char *loading_languageNames[6] = { "engl", "germ", "fren", "span", "itly", "swed" };

void Loading::DrawLoadingScreen(void)
{
    FUNCTION_MARKER(0x800DB0B8, "NFS4.EXE");
    char name[255];
    sint32 language = GameSetup::gData.userSetting.language;
    sprintf(name, "%sLoad%d%c.qps", Paths_Paths[25], GameSetup::gData.track,
        (sint32)((uint32)GameSetup::gData.mirrorTrack + 97U));
    void *f = loadshapeadr(name, 0);
    sprintf(name, "%sLoada.psh", Paths_Paths[25]);
    void *g = loadshapeadr(name, 0);
    Draw::SetEnvironment(512, 240, 1, 0, 1, 0, 0, 0);
    shapetbl *s = static_cast<shapetbl *>(shapepointer(f, 0));
    Draw::DrawDirectScreen(s, 0, 0);
    s = static_cast<shapetbl *>(locateshape(g, loading_languageNames[language]));
    Draw::DrawDirectScreen(s, s->shapex, s->shapey);
    purgememadr(f);
    purgememadr(g);
    Draw::SetEnvironment(512, 240, 0, 1, 0, 0, 0, 0);
}

void Loading::UpdateLoadingScreen(sint32 checkpoint)
{
    FUNCTION_MARKER(0x800DB200, "NFS4.EXE");
    if (checkpoint == 1 && smallShapeFile == 0)
    {
        char name[255];
        sprintf(name, "%sLoadb.psh", Paths_Paths[25]);
        smallShapeFile = static_cast<char *>(loadshapeadr(name, 16));
    }
    const sint32 max = 23;
    uint32 start = ((uint32)checkpoint - 1U) * 23U;
    for (sint32 i = 0; i < max; ++i)
    {
        shapetbl *s = static_cast<shapetbl *>(locateshape(smallShapeFile, "back"));
        Draw::DrawDirectScreen(s, (sint32)(start + (uint32)i + 142U), 192);
    }
    if (checkpoint == 10 && smallShapeFile != 0)
    {
        purgememadr(smallShapeFile);
        smallShapeFile = 0;
    }
}

void Loading::GetInitialMemory(void)
{
    FUNCTION_MARKER(0x800DB2F8, "NFS4.EXE");
    totalAvailMem = largestunused();
}
