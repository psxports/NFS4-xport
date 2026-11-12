#include "xport.h"
#include "eaclib/psx/internal.h"
#include "memstd.h"

MemClass *memclass[16];
uint32 mb_default;

extern "C" uint32 initmemblock(MemBlock *block, const char *name, sint32 length, sint32 tailSize, uint32 flags, MemBlock *previous, MemBlock *next)
{
    FUNCTION_MARKER(0x800E4F2C, "NFS4.EXE");
    uint8 *tail = reinterpret_cast<uint8 *>(block) + sizeof(MemBlock) + length;
    ::new (block) MemBlock;
    block->tag = 0x424D;
    block->length = length;
    block->flags = flags;
    block->nextPhysical = next;
    block->previousPhysical = previous;
    if ((flags & 0x200) != 0)
        putm(tail, 0x42454E44, 4);
    if ((flags & 0x800) != 0)
        puti(tail + 12, 0, 4);
    tail += tailSize;
    if (name && (flags & 0x100) != 0) {
        strcpy(reinterpret_cast<char *>(tail), name);
        tail += strlen(reinterpret_cast<char *>(tail)) + 1;
    }
    return reinterpret_cast<uintptr_t>(tail) - reinterpret_cast<uintptr_t>(block);
}

MemFreeBlock *FREE::find(MemClass *heap, sint32 length, sint32 fromEnd)
{
    FUNCTION_MARKER(0x800E4D4C, "NFS4.EXE");
    MemFreeBlock *block = &heap->freeList;
    do {
        block = fromEnd ? block->previousFree : block->nextFree;
    } while (block->block.length < length);
    return block->block.tag == 0x4253 ? 0 : block;
}

MemFreeBlock *FREE::findlargest(MemClass *heap, sint32 length, sint32 fromEnd)
{
    FUNCTION_MARKER(0x800E4DB0, "NFS4.EXE");
    sint32 minimum = (length - 1);
    if (minimum < 0)
        minimum = 0;
    MemFreeBlock *best = 0;
    MemFreeBlock *block = &heap->freeList;
    for (;;) {
        do {
            block = fromEnd ? block->previousFree : block->nextFree;
        } while (block->block.length <= minimum);
        if (block->block.tag == 0x4253)
            return best;
        best = block;
        minimum = block->block.length;
    }
}

void FREE::add(MemClass *heap, MemFreeBlock *block)
{
    FUNCTION_MARKER(0x800E4E70, "NFS4.EXE");
    MemFreeBlock *next = &heap->freeList;
    MemFreeBlock *previous = next;
    uintptr_t address = reinterpret_cast<uintptr_t>(block);
    uintptr_t first = reinterpret_cast<uintptr_t>(next->nextFree);
    uintptr_t last = reinterpret_cast<uintptr_t>(next->previousFree);
    uintptr_t middle = first + ((last - first) >> 1);
    sint32 length = ((uint32)(reinterpret_cast<uintptr_t>(block->block.nextPhysical) - address) - (uint32)sizeof(MemBlock));
    if (middle < address) {
        do {
            previous = previous->previousFree;
        } while (address < reinterpret_cast<uintptr_t>(previous));
        next = previous->nextFree;
    } else {
        do {
            next = next->nextFree;
        } while (reinterpret_cast<uintptr_t>(next) < address);
        previous = next->previousFree;
    }
    block->nextFree = next;
    block->previousFree = previous;
    block->block.length = length;
    previous->nextFree = block;
    next->previousFree = block;
    uint16 flags = block->block.flags;
    block->block.tag = 0x4246;
    block->block.flags = flags | 0x4000;
}

extern "C" sint32 creatememclass(uint32 classFlags, const char *name, void *memory, sint32 size, sint32 alignment, sint32 boundaryAlignment, sint32 tailSize, sint32 checkTail, uint32, sint32 storeNames, sint32 useMutex, MemEventFuncPtr event)
{
    FUNCTION_MARKER(0x800E5094, "NFS4.EXE");
    uint32 flags = classFlags;
    if (checkTail)
        flags |= 0x200;
    if (storeNames)
        flags |= 0x100;
    if (alignment < (sint32)sizeof(void *))
        alignment = sizeof(void *);
    if (boundaryAlignment < (sint32)sizeof(void *))
        boundaryAlignment = sizeof(void *);
    uint8 *arena = static_cast<uint8 *>(memory);
    uintptr_t firstAddress = (reinterpret_cast<uintptr_t>(arena) + (uint32)tailSize + sizeof(MemBlock) + sizeof(MemClass) + (uint32)boundaryAlignment + 31) & (~((uintptr_t)boundaryAlignment - 1));
    MemFreeBlock *first = ::new (reinterpret_cast<void *>(firstAddress - sizeof(MemBlock))) MemFreeBlock;
    MemBlock *low = reinterpret_cast<MemBlock *>(arena);
    uintptr_t highAddress = reinterpret_cast<uintptr_t>(arena + size - ((uint32)tailSize + sizeof(MemBlock) + 16));
    highAddress &= ~((uintptr_t)sizeof(void *) - 1);
    MemBlock *high = reinterpret_cast<MemBlock *>(highAddress);
    char label[256];
    sprintf(label, "%s LOW", name);
    initmemblock(low, label, sizeof(MemClass), tailSize, flags | 0x8000, 0, &first->block);
    sint32 available = reinterpret_cast<uintptr_t>(high) - reinterpret_cast<uintptr_t>(first) - sizeof(MemBlock);
    initmemblock(&first->block, 0, available, tailSize, flags, low, high);
    sprintf(label, "%s HIGH", name);
    initmemblock(high, label, 0, tailSize, flags | 0x8010, &first->block, 0);
    MemClass *heap = ::new (arena + sizeof(MemBlock)) MemClass;
    memclass[classFlags & 15] = heap;
    blockclear(heap, sizeof(*heap));
    strcpy(heap->name, name);
    heap->freeList.block.tag = 0x4253;
    heap->lowBlock = low;
    heap->highBlock = high;
    heap->freeList.nextFree = &heap->freeList;
    heap->freeList.previousFree = &heap->freeList;
    heap->freeList.block.length = 0x7FFFFFFF;
    heap->boundaryAlignment = boundaryAlignment;
    heap->tailSize = tailSize;
    heap->flags = flags;
    heap->mutex = 0;
    heap->alignment = alignment;
    heap->event = event;
    FREE::add(heap, first);
    if (useMutex)
        heap->mutex = allocmutex();
    return first->block.length;
}

extern "C" sint32 largestunused(void)
{
    FUNCTION_MARKER(0x800E5284, "NFS4.EXE");
    MemFreeBlock *block = FREE::findlargest(memclass[mb_default & 15], 0, 0);
    return block ? block->block.length : 0;
}

extern "C" sint32 getblocksize(const void *memory)
{
    FUNCTION_MARKER(0x800E52D4, "NFS4.EXE");
    const MemBlock *block = reinterpret_cast<const MemBlock *>(static_cast<const uint8 *>(memory) - sizeof(MemBlock));
    return block->length;
}

void FREE::remove(MemClass *heap, MemFreeBlock *block)
{
    FUNCTION_MARKER(0x800E4F04, "NFS4.EXE");
    MemFreeBlock *previous = block->previousFree;
    MemFreeBlock *next = block->nextFree;
    previous->nextFree = next;
    next->previousFree = previous;
    uint16 flags = block->block.flags;
    block->block.tag = 0;
    block->block.flags = flags & 0xBFFF;
}

sint32 MEM::infosize(uint32 flags)
{
    FUNCTION_MARKER(0x800E5008, "NFS4.EXE");
    return memclass[flags & 15]->tailSize;
}

extern "C" char *getblockname(void *memory)
{
    FUNCTION_MARKER(0x800E52E0, "NFS4.EXE");
    MemBlock *block = reinterpret_cast<MemBlock *>(static_cast<uint8 *>(memory) - sizeof(MemBlock));
    uint16 flags = block->flags;
    if ((flags & 0x100) == 0)
        return 0;
    sint32 tailSize = MEM::infosize(flags);
    return static_cast<char *>(memory) + block->length + tailSize;
}

sint32 MEM::tailsize(const char *name, uint32 flags)
{
    FUNCTION_MARKER(0x800E5030, "NFS4.EXE");
    MemClass *heap = memclass[flags & 15];
    uint32 nameSize = 0;
    if (name && (heap->flags & 0x100) != 0)
        nameSize = (uint32)strlen(name) + 1;
    return heap->tailSize + nameSize;
}

extern "C" void *reservememadr(const char *name, sint32 length, uint32 flags)
{
    FUNCTION_MARKER(0x800E533C, "NFS4.EXE");
    MemClass *heap = memclass[flags & 15];
    sint32 needed = length;
    sint32 minimumPayload = (sint32)(sizeof(MemFreeBlock) - sizeof(MemBlock));
    if (length < minimumPayload) {
        if (length < 0)
            return 0;
        needed = minimumPayload;
    }
    sint32 tailSize = MEM::tailsize(name, flags);
    uint32 mask = (uint32)heap->alignment - 1;
    uint32 total = (needed + (uint32)tailSize + heap->alignment + (uint32)sizeof(MemBlock) - 1U) & ~mask;
    needed = total - (uint32)sizeof(MemBlock);
    MemFreeBlock *freeBlock;
    if ((flags & 0x20) != 0)
        freeBlock = FREE::findlargest(heap, needed, flags & 0x10);
    else
        freeBlock = FREE::find(heap, needed, flags & 0x10);
    if (!freeBlock)
        return 0;
    FREE::remove(heap, freeBlock);
    MemBlock *block = &freeBlock->block;
    sint32 remaining = block->length - needed;
    if (remaining > 64) {
        if ((flags & 0x10) != 0) {
            MemBlock *original = block;
            uint32 offset = remaining & ~mask;
            block = ::new (reinterpret_cast<uint8 *>(original) + offset) MemBlock;
            original->nextPhysical->previousPhysical = block;
            MemBlock *next = original->nextPhysical;
            block->previousPhysical = original;
            block->nextPhysical = next;
            initmemblock(original, 0, 0, 0, 0, original->previousPhysical, block);
            FREE::add(heap, freeBlock);
        } else {
            MemFreeBlock *fragment = ::new (reinterpret_cast<uint8 *>(block) + total) MemFreeBlock;
            block->nextPhysical->previousPhysical = &fragment->block;
            initmemblock(&fragment->block, 0, 0, 0, 0, block, block->nextPhysical);
            FREE::add(heap, fragment);
            block->nextPhysical = &fragment->block;
        }
    }
    flags |= heap->flags & 0x700;
    if (!name)
        flags &= ~0x100;
    uint8 *result = reinterpret_cast<uint8 *>(block) + sizeof(MemBlock);
    initmemblock(block, name, length, heap->tailSize, flags, block->previousPhysical, block->nextPhysical);
    return result;
}

extern "C" uint32 purgememadr(void *memory)
{
    FUNCTION_MARKER(0x800E5540, "NFS4.EXE");
    if (memory) {
        MemBlock *block = reinterpret_cast<MemBlock *>(static_cast<uint8 *>(memory) - sizeof(MemBlock));
        MemBlock *next = block->nextPhysical;
        MemBlock *previous = block->previousPhysical;
        MemClass *heap = memclass[block->flags & 15];
        if ((previous->flags & 0x4000) != 0) {
            FREE::remove(heap, reinterpret_cast<MemFreeBlock *>(previous));
            block = previous;
            previous = block->previousPhysical;
            block->nextPhysical = next;
            next->previousPhysical = block;
            previous->nextPhysical = block;
        }
        if ((next->flags & 0x4000) != 0) {
            FREE::remove(heap, reinterpret_cast<MemFreeBlock *>(next));
            next = next->nextPhysical;
            block->previousPhysical = previous;
            block->nextPhysical = next;
            next->previousPhysical = block;
        }
        MemBlock header = *block;
        MemFreeBlock *freeBlock = ::new (block) MemFreeBlock;
        freeBlock->block = header;
        FREE::add(heap, freeBlock);
    }
    return 1;
}
