#ifndef SIMPLEMEM_H
#define SIMPLEMEM_H

#include "xport.h"

class SimpleMem
{
public:
    void *heap;
    void *freeMem;
    sint32 freeMemSize;

    void *Alloc(sint32 len, sint32 feign);
    void *FeignAlloc(sint32 len);
    void ResizeToFit(void);
};

#endif
