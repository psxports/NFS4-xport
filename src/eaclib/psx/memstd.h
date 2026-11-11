#ifndef MEMSTD_H
#define MEMSTD_H

#include <stddef.h>
#include <stdint.h>
#include "xport.h"

typedef struct MemBlock MemBlock;
typedef struct MemFreeBlock MemFreeBlock;
typedef struct MemMutex MemMutex;
typedef struct MemClass MemClass;

struct MemBlock
{
    uint16 tag;
    uint16 flags;
    sint32 length;
    MemBlock *nextPhysical;
    MemBlock *previousPhysical;
};

struct MemFreeBlock
{
    MemBlock block;
    MemFreeBlock *nextFree;
    MemFreeBlock *previousFree;
};

struct MemMutex
{
    sint16 allocated;
    sint16 reserved;
};

#ifdef __cplusplus
typedef sint32 (*MemEventFuncPtr)(...);
#else
typedef sint32 (*MemEventFuncPtr)();
#endif

struct MemClass
{
    char name[8];
    MemBlock *lowBlock;
    MemBlock *highBlock;
    MemFreeBlock freeList;
    sint32 alignment;
    sint32 boundaryAlignment;
    sint32 tailSize;
    uint32 flags;
    MemMutex *mutex;
    MemEventFuncPtr event;
};

#ifdef __cplusplus
class FREE
{
public:
    static MemFreeBlock *find(MemClass *heap, sint32 length, sint32 fromEnd);
    static MemFreeBlock *findlargest(MemClass *heap, sint32 length, sint32 fromEnd);
    static void add(MemClass *heap, MemFreeBlock *block);
    static void remove(MemClass *heap, MemFreeBlock *block);
};

class MEM
{
public:
    static sint32 infosize(uint32 flags);
    static sint32 tailsize(const char *name, uint32 flags);
    static sint32 defaultevent(...);
};

#endif

#ifdef __cplusplus
extern "C" {
#endif
extern MemClass *memclass[16];
extern MemMutex mutexbuf[32];
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C"
#endif
MemMutex *allocmutex(void);
#ifdef __cplusplus
extern "C"
#endif
void freemutex(MemMutex *mutex);
#ifdef __cplusplus
extern "C"
#endif
sint32 initmemadr(void *memory, sint32 size);
#ifdef __cplusplus
extern "C"
#endif
sint32 creatememclass(uint32 classFlags, const char *name, void *memory, sint32 size, sint32 alignment, sint32 boundaryAlignment, sint32 tailSize, sint32 checkTail, uint32 unused, sint32 storeNames, sint32 useMutex, MemEventFuncPtr event);

#ifdef __cplusplus
extern "C"
#endif
uint32 initmemblock(MemBlock *block, const char *name, sint32 length, sint32 tailSize, uint32 flags, MemBlock *previous, MemBlock *next);

#endif
