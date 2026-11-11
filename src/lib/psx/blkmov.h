#ifndef BLKMOV_H
#define BLKMOV_H

#include "xport.h"
#ifdef __cplusplus
extern "C" {
#endif
void blockmove(const void *source, void *destination, sint32 length);
#ifdef __cplusplus
}
#endif

#endif
