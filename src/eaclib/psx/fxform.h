#ifndef FXFORM_H
#define FXFORM_H

#include "xport.h"

#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
extern "C" {
#endif

void fixedxformx(matrixtdef *matrix, sint32 angle);
void fixedxformy(matrixtdef *matrix, sint32 angle);
void fixedxformz(matrixtdef *matrix, sint32 angle);

#ifdef __cplusplus
}
#endif

#endif
