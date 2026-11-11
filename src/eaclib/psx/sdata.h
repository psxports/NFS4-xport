#ifndef SDATA_H
#define SDATA_H

#include "xport.h"
#include "eaclib/psx/slimits.h"
#include "eaclib/psx/ssysinit.h"

#ifdef __cplusplus
typedef void (*SoundServerFuncPtr)(...);
#else
typedef void (*SoundServerFuncPtr)();
#endif

#ifdef __cplusplus
typedef sint32 (*SoundStreamDestroyCallback)(...);
#else
typedef sint32 (*SoundStreamDestroyCallback)();
#endif

typedef void (*SoundRestoreHook)(void);

typedef sint32 (*SoundBankRemoveHook)(sint32 bank, sint32 unused);

typedef struct SoundGlobalState SoundGlobalState;
typedef struct NativeCdLevels NativeCdLevels;

struct SoundGlobalState
{
    SNDSYSOPTS options;
    sint8 initialized;
    uint8 masterVolume;
    uint8 serverRunning;
    uint8 audioLockDepth;
    sint8 client100hzCount;
    sint8 serviceClientCount;
    uint8 unknown66[2];
    uint32 clock;
    SoundServerFuncPtr server100hz;
    void (*clients100hz[6])(void);
    SoundServerFuncPtr serviceClients[6];
    SoundBankRemoveHook removeBanks;
    SoundRestoreHook restoreHook128;
    SoundRestoreHook restoreHook132;
    SoundStreamDestroyCallback streamService;
    SoundRestoreHook restoreHook140;
    SoundRestoreHook restoreHook144;
    void *voiceMemory;
    void *bankMemory;
    uint8 unknown156[4];
    sint32 unknown160;
    sint32 unknown164;
    sint32 unknown168;
    sint32 unknown172;
    sint16 serverTimerHz;
    uint16 pendingServerTicks;
};

struct NativeCdLevels
{
    sint16 pan;
    sint16 volume;
};

#ifdef __cplusplus
extern "C" {
#endif
extern NativeCdLevels sndcdvs;
extern SoundGlobalState sndgs;
extern SNDLIMITS sndpsxlimits;
extern uint32 soundCapsInitialized;
extern const uint16 sndpe[10][33];
#ifdef __cplusplus
}
#endif

#endif
