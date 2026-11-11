#include "xport.h"
#include "eaclib/psx/internal.h"

uint32 soundSystemTaskInstalled;

extern "C" sint32 iSNDsystemtaskreal(sint32 context, sint32 lateness)
{
    FUNCTION_MARKER(0x800F2130, "NFS4.EXE");
    (void)context;
    (void)lateness;
    SNDSYS::service();
    return 0;
}

sint32 SNDSYS::vectortoreal(void)
{
    FUNCTION_MARKER(0x800F2150, "NFS4.EXE");
    if (soundSystemTaskInstalled == 0)
    {
        addsystemtask(iSNDsystemtaskreal, 0, 1);
        soundSystemTaskInstalled = 1;
    }
    return 0;
}
