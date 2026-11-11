#ifndef TRNSMULT_H
#define TRNSMULT_H

#include "xport.h"

#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
extern "C" {
#endif

void transmult(matrixtdef *left, matrixtdef *right, matrixtdef *result);

#ifdef __cplusplus
}
#endif

#endif
