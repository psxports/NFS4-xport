#ifndef GLOBALS_H
#define GLOBALS_H

#include "xport.h"
#include "eaclib/psx/callback.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32 soundSystemTaskInstalled;
extern uint32 mb_default;
extern uint32 abortflag;
extern GameFileLoadCallback loadfilecallback;
extern uint32 disablecd;
extern uint32 currentfilesystem;
extern uint32 availablefilesystems;
extern uint32 numjoy;
extern uint32 joy_inited;
extern volatile sint32 ticks;
extern GameTimerCallback tmrsub[8];
extern GameTimerCallback gameExitCallbacks[32];
extern volatile sint32 libticks;
extern uint32 tickset;
extern uint32 tickval;
extern uint32 drawpending;
extern void *otbuf;
extern void *primbuf;
extern uint32 linkmodeflag;
extern sint32 oti;
extern sint32 otp;
extern uint32 *otbl;
extern uint32 *alternateOT;
extern uint32 *nextprim;
extern sint32 maxot;
extern char *primptr;
extern char *primbase;
extern char *alternatePrimitives;
extern sint32 maxprim;
extern uint32 semitrans;
extern char *shapeext;
extern sint32 biglen;
extern uint8 isqrttbl[256];
extern sint32 timerhz;
extern uint32 timerflag;
extern sint32 timerevent;
extern uint32 reentryflag;
extern sint32 screenheight;
extern sint32 screenwidth;
extern sint32 screenbpp;
extern sint32 g_currentthread;
extern matrixtdef identitymatrix;
extern sint32 reorthogonalizeCoefficients[4];

#ifdef __cplusplus
}
#endif

#endif
