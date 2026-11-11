#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 shapetoclutid(shapetbl *shape)
{
    FUNCTION_MARKER(0x800F6C94, "NFS4.EXE");
    while (shape != 0)
    {
        if ((shape->type & 0xF7) == 0x23)
            return (sint16)GetClut(shape->shapex, shape->shapey);
        sint32 next = shape->next;
        shape = next == 0 ? 0 : ((shapetbl *)(((char *)(shape)) + next));
    }
    return 0;
}
