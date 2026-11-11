#ifndef MATH64_H
#define MATH64_H

#include "xport.h"
#ifdef __cplusplus
extern "C" {
#endif

void make64(sint64 *result, sint32 value, sint32 shift);
uint32 divu64(uint64 dividend, uint32 divisor);

#ifdef __cplusplus
}
#endif

#endif
