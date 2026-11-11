#ifndef FIXDSIN_H
#define FIXDSIN_H

#include "xport.h"

#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
extern "C" {
#endif

sint32 fixedsin(sint32 angle);
sint32 fixedcos(sint32 angle);

#ifdef __cplusplus
}
#endif

#endif
