#include "xport.h"
#include "eaclib/psx/textcrnt.h"


    void fortext4(sint32 x, sint32 y, void *source, sint32 u, sint32 v, sint32 width, sint32 height, sint32 rowbytes)
    {
        FUNCTION_MARKER(0x800E3E40, "NFS4.EXE");
    }

    void inittextdraw(void)
    {
        FUNCTION_MARKER(0x800E3E48, "NFS4.EXE");
    currentfont.draw = fortext4;
    }

    void restoretextdraw(void)
    {
        FUNCTION_MARKER(0x800E3E5C, "NFS4.EXE");
    currentfont.draw = 0;
    }

