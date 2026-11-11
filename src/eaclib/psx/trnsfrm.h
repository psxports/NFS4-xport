#ifndef TRNSFRM_H
#define TRNSFRM_H

#include "xport.h"

#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
extern "C" {
#endif

void transform(coorddef *vector, matrixtdef *matrix, coorddef *result);

#ifdef __cplusplus
}
#endif

#endif
