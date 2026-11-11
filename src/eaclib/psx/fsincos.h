#ifndef FSINCOS_H
#define FSINCOS_H

#include "xport.h"

#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
extern "C" {
#endif

void fixedsincos(sint32 angle, sint32 *sine, sint32 *cosine);

#ifdef __cplusplus
}
#endif

#endif
