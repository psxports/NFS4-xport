#ifndef NEW_H
#define NEW_H

#include "xport.h"

extern "C" void *__builtin_new(uint32 size);
extern "C" void *__builtin_vec_new(uint32 size);

#endif
