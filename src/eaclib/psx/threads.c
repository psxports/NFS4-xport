#include "xport.h"
#include "eaclib/psx/internal.h"

MemMutex mutexbuf[32];

bool iscurrentthread(sint32 thread)
{
    FUNCTION_MARKER(0x800FE408, "NFS4.EXE");
    return g_currentthread == thread;
}

void yieldthread(sint32 thread)
{
    FUNCTION_MARKER(0x800FE41C, "NFS4.EXE");
}

MemMutex *allocmutex(void)
{
    FUNCTION_MARKER(0x800FE424, "NFS4.EXE");
    for (sint32 i = 0; i < 32; ++i) {
        if (mutexbuf[i].allocated == 0) {
            mutexbuf[i].allocated = 1;
            return &mutexbuf[i];
        }
    }
    return 0;
}

void freemutex(MemMutex *mutex)
{
    FUNCTION_MARKER(0x800FE480, "NFS4.EXE");
    memset(mutex, 0, sizeof(*mutex));
}
