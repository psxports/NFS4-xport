#ifndef SSYSINIT_H
#define SSYSINIT_H

#include "xport.h"

typedef struct SNDSYSCAP SNDSYSCAP;
typedef struct SNDSYSSET SNDSYSSET;
typedef struct SNDSYSVEC SNDSYSVEC;
typedef struct SNDSYSOPTS SNDSYSOPTS;

struct SNDSYSCAP
{
    uint16 outputratemin;
    uint16 outputratemax;
    uint8 outputchannelsmin;
    uint8 outputchannelsmax;
    uint8 inputvoicesmax;
    uint8 input3dvoicesmax;
    uint8 eax;
    uint8 voicemanager;
    char pad[2];
};

struct SNDSYSSET
{
    uint16 maxbanks;
    uint16 outputrate;
    uint8 outputchannels;
    uint8 inputvoices;
    uint8 useeax;
    uint8 use3dacceleration;
    uint8 use3dmixing;
    char pad;
    uint16 emulationsubtype;
    uint16 spkrcfg3d[4][4];
};

struct SNDSYSVEC
{

#ifdef __cplusplus
    sint32 (*issurfacelocked)(...);
#else
    sint32 (*issurfacelocked)();
#endif
};

struct SNDSYSOPTS
{
    SNDSYSCAP cap;
    SNDSYSSET set;
    SNDSYSVEC vec;
};

#ifdef __cplusplus
class SNDSYS
{
public:
    static sint32 getopts(SNDSYSOPTS *options);
    static sint32 setopts(SNDSYSOPTS *options);
    static sint32 init(void *heap, sint32 size);
    static sint32 restore(void);
    static sint32 vectortoreal(void);
    static void service(void);
};
#endif

#endif
