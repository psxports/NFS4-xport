#include "xport.h"
#include "eaclib/psx/internal.h"

void *loadpackadrz(const char *name, uint32 flags)
{
    FUNCTION_MARKER(0x800E5C64, "NFS4.EXE");
    void *loaded = loadfileadr(name, flags);
    if (loaded == 0)
        return 0;
    sint32 size = unpacksizez(loaded);
    if (size == 0)
        return loaded;
    void *packed = reservememadr(name, getblocksize(loaded), flags ^ 0x10);
    blockmove(loaded, packed, getblocksize(loaded));
    purgememadr(loaded);
    void *result = reservememadr(name, size, flags);
    if (result != 0 && unpackz(packed, result) == 0)
    {
        purgememadr(result);
        result = 0;
    }
    purgememadr(packed);
    return result;
}
