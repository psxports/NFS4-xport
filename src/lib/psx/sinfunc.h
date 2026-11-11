#ifndef SINFUNC_H
#define SINFUNC_H

#include "xport.h"
#ifdef __cplusplus
extern "C" {
#endif

extern const sint32 sintbl[257];

sint32 fastintcos(sint32 angle);
sint32 fastintsin(sint32 angle);

#ifdef __cplusplus
}
#endif

#endif
