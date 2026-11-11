#ifndef PAD_H
#define PAD_H

#include "xport.h"
#include "psx.h"

typedef struct PAD_PSX { uint16 state; uint16 unused[2]; } PAD_PSX;

typedef struct PAD_ANALOG { uint16 state; uint8 rightx, righty, leftx, lefty; } PAD_ANALOG;

typedef struct PAD_COMMON
{
    uint8 nopad, ID;

    union { PAD_PSX standard; PAD_ANALOG analog; } data;
} PAD_COMMON;

typedef struct PAD_INFO PAD_INFO;

struct PAD_INFO
{
    sint32 initialized;
    PAD_COMMON buf[8];

    struct { uint8 bActive, time; } state[8];
};

#ifdef __cplusplus
extern "C" {
#endif
extern struct PAD_INFO gPadinfo;
extern PAD_COMMON Padglobal[2];
void padinit(void);
void PAD_restore(void);
uint16 PAD_state(sint32 padID);
void PAD_update(void);
#ifdef __cplusplus
}
#endif

#endif
