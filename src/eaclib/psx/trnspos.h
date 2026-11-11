#ifndef TRNSPOS_H
#define TRNSPOS_H

#include "xport.h"

#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
extern "C" {
#endif

void transpose(matrixtdef *source, matrixtdef *destination);

#ifdef __cplusplus
}
#endif

#endif
