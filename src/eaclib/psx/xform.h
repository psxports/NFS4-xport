#ifndef XFORM_H
#define XFORM_H

#include "xport.h"

#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
extern "C" {
#endif

void xformy(matrixtdef *matrix, sint32 angle);

#ifdef __cplusplus
}
#endif

#endif
