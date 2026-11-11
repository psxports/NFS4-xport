#ifndef CRC_H
#define CRC_H

#include "xport.h"
#ifdef __cplusplus
extern "C" {
#endif

uint32 crc16(const uint8 *data, sint32 length);

#ifdef __cplusplus
}
#endif

#endif
