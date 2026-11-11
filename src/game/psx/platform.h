#ifndef PLATFORM_H
#define PLATFORM_H

#include "xport.h"
#include "game/common/car.h"

class Platform
{
public:
    static void InitMemory(void);
    static char * ReserveMemory(sint32 size, char *string);
    static char * TempReserveMemory(sint32 size, char *string);
    static void SysStartUp(void);
    static void DebuggerPollHost(void);
    static void ResetDCTBuffer(void);
    static char * GetDCTBuffer(sint32 size, char *string);
    static char * GetDCTBufferBase(void);
};

extern char *gLowMemory;
extern char *gHighMemory;
extern char *gCurrentMemory;
extern uint32 gTotalMemory;
extern sint32 gSysStartUp;

void nfs2eacinit(void);

#endif
