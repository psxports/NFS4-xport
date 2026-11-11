#include "eaclib/psx/internal.h"

void SNDstopall(void)
{
    FUNCTION_MARKER(0x800E81A8, "NFS4.EXE");
    StopSoundVoicesExcept(-1);
}
