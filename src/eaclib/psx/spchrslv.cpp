#include "xport.h"
#include "eaclib/psx/internal.h"

sint32 iSPCH::BindData(uint8 *data)
{
    FUNCTION_MARKER(0x800EBB84, "NFS4.EXE");
    if (*(const uint16 *)data >= 0x11E)
    {
        for (sint32 i = 0; i < 4; ++i)
        {
            if (gEventDats[i] == 0)
            {
                gEventDats[i] = data;
                return 1;
            }
        }
    }
    return 0;
}
