#include "xport.h"

enum { NATIVE_GP_CONTEXT = 0 };

const uint8 original_savegp_data[] =
{
    0x00, 0x00, 0x00, 0x00,
};

void initgp(void)
{
    FUNCTION_MARKER(0x800EB080, "NFS4.EXE");
}

void savegp(uint32 *savedGp)
{
    FUNCTION_MARKER(0x800EB090, "NFS4.EXE");
    *savedGp = NATIVE_GP_CONTEXT;
}

void restoregp(uint32 savedGp)
{
    FUNCTION_MARKER(0x800EB0A4, "NFS4.EXE");
}
