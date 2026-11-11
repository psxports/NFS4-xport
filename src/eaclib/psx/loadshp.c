#include "xport.h"
#include "eaclib/psx/internal.h"

void *loadshapeadr(const char *name, uint32 flags)
{
    FUNCTION_MARKER(0x800F1C3C, "NFS4.EXE");
    char filename[128];
    strcpy(filename, name);
    char *f = filename + strlen(filename) - 1;
    while (f > filename)
    {
        uint8 c = (uint8)*f;
        if (c == '.' || c == ':' || c == '/' || c == '\\')
            break;
        --f;
    }
    if (*f != '.')
        strcat(filename, shapeext);
    return loadpackadrz(filename, flags);
}
