#ifndef CROSSPRD_H
#define CROSSPRD_H

#include "xport.h"

#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
extern "C" {
#endif

coorddef *crossproduct(coorddef *left, coorddef *right, coorddef *result);

#ifdef __cplusplus
}
#endif

#endif
