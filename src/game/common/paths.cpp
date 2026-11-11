#include "xport.h"
#include "game/common/paths.h"

char burnPath[2] = { 122, 0 };
char fePath[1];
char *Paths_Paths[50];
char *Paths::File;

void Paths::StartUp(void)
{
    FUNCTION_MARKER(0x800A6E14, "NFS4.EXE");
    for (sint32 dirCounter = 49; dirCounter >= 0; --dirCounter)
        Paths_Paths[dirCounter] = burnPath;
    Paths_Paths[36] = fePath;
    Paths_Paths[32] = fePath;
}
