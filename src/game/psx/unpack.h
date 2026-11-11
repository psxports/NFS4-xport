#ifndef UNPACK_H
#define UNPACK_H

#include "xport.h"
#ifdef __cplusplus
extern "C" {
#endif

sint32 unpackz(void *src, void *dst);
sint32 unpacksizez(void *src);
sint32 unpacksize(void *src);

#ifdef __cplusplus
}
#endif

#endif
