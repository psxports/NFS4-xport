#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/new.h"

class New
{
public:
    static char *cplusplus_heap;
};

char *New::cplusplus_heap;

void *__builtin_new(uint32 size)
{
    FUNCTION_MARKER(0x800A3D5C, "NFS4.EXE");
    return reservememadr(0, (sint32)size, 0);
}

void *__builtin_vec_new(uint32 size)
{
    FUNCTION_MARKER(0x800A3D84, "NFS4.EXE");
    return reservememadr(0, (sint32)size, 0);
}
