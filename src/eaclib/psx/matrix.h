#ifndef MATRIX_H
#define MATRIX_H

#include "xport.h"

#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
extern "C" {
#endif

matrixtdef *addmatrix(matrixtdef *left, matrixtdef *right, matrixtdef *result);
matrixtdef *submatrix(matrixtdef *left, matrixtdef *right, matrixtdef *result);
matrixtdef *scalematrix(matrixtdef *matrix, sint32 scale, matrixtdef *result);
void reorthogonalize(matrixtdef *matrix);

#ifdef __cplusplus
}
#endif

#endif
