#ifndef GETM_H
#define GETM_H

#include "xport.h"
#ifdef __cplusplus
extern "C" {
#endif

uint32 getm(const void *data, sint32 length);
uint32 geti(const void *data, sint32 length);
void putm(void *data, uint32 value, sint32 length);
void puti(void *data, uint32 value, sint32 length);

#ifdef __cplusplus
}
#endif

#endif
