#ifndef BLKFILL_H
#define BLKFILL_H

#include "xport.h"
#ifdef __cplusplus
extern "C" {
#endif
void blockclear(void *destination, sint32 length);
void blockfill(void *destination, sint32 length, sint32 value);
#ifdef __cplusplus
}
#endif

#endif
