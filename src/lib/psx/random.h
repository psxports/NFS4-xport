#ifndef RANDOM_H
#define RANDOM_H

#ifdef __EMSCRIPTEN__
#include <stdlib.h>

#include "xport.h"
#define random nfs4_random
#endif

#ifdef __cplusplus
extern "C" {
#endif

uint32 random(void);
void seedrandom(uint32 value);

#ifdef __cplusplus
}
#endif

#endif
