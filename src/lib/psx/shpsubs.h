#ifndef SHPSUBS_H
#define SHPSUBS_H

#include "xport.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32 shapecount(const void *shape);
void *shapepointer(void *shape, uint32 index);
sint32 shapename(const void *shape, uint32 index, char *name);

#ifdef __cplusplus
}
#endif

#endif
