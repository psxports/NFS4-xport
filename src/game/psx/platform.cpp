#include <stdlib.h>
#include "xport.h"
#include "psx.h"
#include "psx_gpu.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "game/common/nfs3.h"
#include "game/common/paths.h"
#include "game/psx/draw.h"
#include "game/psx/platform.h"
#include "game/psx/systemmemory.h"

union DctWorkspace
{
    uint32 alignment;
    char bytes[0x10000];
};

union PrimitiveMemoryOwner
{
    uint32 alignment;
    char bytes[0x44C90];
};

static DctWorkspace dctWorkspace;
static char *gDctXtraMem;

enum { SYSTEM_ALLOCATION_BYTES = NativeSystemMemoryOwner::allocationBytes };

struct CARD_FILE_HEADER
{
    char magic[2];
    char type;
    char numslots;
    char documentname[64];
    char pad[28];
    char clut[32];
    char icon[3][128];
};

struct eacelement
{
    eacelement *link;
};

typedef eacelement EACELEMENT;

static PrimitiveMemoryOwner primitiveMemory;
NativeSystemMemoryOwner gSystemMemoryOwner;
static uint32 (&systemAllocationStorage)[NativeSystemMemoryOwner::allocationWords] = gSystemMemoryOwner.allocation;
char *gLowMemory;
char *gHighMemory;
char *gCurrentMemory;
uint32 gTotalMemory;
sint32 gSysStartUp;
sint32 disablecard;

void Platform::InitMemory(void)
{
    FUNCTION_MARKER(0x800DC2B8, "NFS4.EXE");
    gTotalMemory = sizeof(primitiveMemory.bytes);
    gLowMemory = primitiveMemory.bytes;
    gHighMemory = gLowMemory + gTotalMemory;
    gCurrentMemory = gLowMemory;
}

char * Platform::ReserveMemory(sint32 size, char *string)
{
    FUNCTION_MARKER(0x800DC2E8, "NFS4.EXE");
    (void)string;
    uint32 rounded = (uint32)size + 3;
    if ((sint32)rounded < 0)
        rounded += 3;
    sint32 length = (sint32)((uint32)((sint32)rounded >> 2) << 2);
    char *current = gCurrentMemory;
    char *next = reinterpret_cast<char *>(reinterpret_cast<uintptr_t>(current) + (uintptr_t)length);
    sint32 used = (sint32)(uint32)(reinterpret_cast<uintptr_t>(next) - reinterpret_cast<uintptr_t>(gLowMemory));
    if ((sint32)gTotalMemory < used)
        return 0;
    gCurrentMemory = next;
    return current;
}

char * Platform::TempReserveMemory(sint32 size, char *string)
{
    FUNCTION_MARKER(0x800DC334, "NFS4.EXE");
    (void)string;
    uint32 rounded = (uint32)size + 3;
    if ((sint32)rounded < 0)
        rounded += 3;
    sint32 length = (sint32)((uint32)((sint32)rounded >> 2) << 2);
    char *current = gCurrentMemory;
    char *next = reinterpret_cast<char *>(reinterpret_cast<uintptr_t>(current) + (uintptr_t)length);
    sint32 used = (sint32)(uint32)(reinterpret_cast<uintptr_t>(next) - reinterpret_cast<uintptr_t>(gLowMemory));
    if ((sint32)gTotalMemory < used)
        return 0;
    return current;
}

void Platform::SysStartUp(void)
{
    FUNCTION_MARKER(0x800DC378, "NFS4.EXE");
    if (!gpu_register_packet_range(primitiveMemory.bytes, sizeof(primitiveMemory.bytes)) ||
        !gpu_register_packet_range(systemAllocationStorage, sizeof(systemAllocationStorage)))
        abort();
    uintptr_t aligned = (reinterpret_cast<uintptr_t>(systemAllocationStorage) + 31) & ~(uintptr_t)31;
    char *endofcode = reinterpret_cast<char *>(aligned + 12);
    disablecd = 0;
    nfs_sysInfo.userRam = SYSTEM_ALLOCATION_BYTES;
    initmemadr(endofcode, SYSTEM_ALLOCATION_BYTES);
    nfs2eacinit();
    Draw::SetEnvironment(512, 240, 1, 0, 1, 0, 0, 0);
    initlinkmode(0, 1, 1);
    initlinkmode(0, 1000, 1);
    initlinkmode(0, 1000, 1);
    gSysStartUp = 1;
    inittimer(128);
    Paths::StartUp();
    initasync(30, 0x2000, 0);
}

void nfs2eacinit(void)
{
    FUNCTION_MARKER(0x800DC458, "NFS4.EXE");
    FlushCache();
    ResetCallback();
    ResetGraph(3);
    initjoy(2);
    XPORT_UNPLACED::FILE::init(10, 800, 48);
}

void Platform::DebuggerPollHost(void)
{
    FUNCTION_MARKER(0x800DC450, "NFS4.EXE");
}

void Platform::ResetDCTBuffer(void)
{
    FUNCTION_MARKER(0x800DC4A0, "NFS4.EXE");
    gDctXtraMem = dctWorkspace.bytes;
}

char * Platform::GetDCTBuffer(sint32 size, char *string)
{
    FUNCTION_MARKER(0x800DC4B4, "NFS4.EXE");
    (void)string;
    char *p = gDctXtraMem;
    if (p == 0 || size < 0 || size > dctWorkspace.bytes + sizeof(dctWorkspace.bytes) - p)
        abort();
    gDctXtraMem = p + size;
    return p;
}

char * Platform::GetDCTBufferBase(void)
{
    return dctWorkspace.bytes;
}
