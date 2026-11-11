#ifndef ISINCOS_H
#define ISINCOS_H

#include "xport.h"

#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
extern "C" {
#endif

void intsincos(sint32 angle, sint32 *sine, sint32 *cosine);

#ifdef __cplusplus
}
#endif

#endif
