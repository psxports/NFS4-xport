#include "xport.h"
#include "eaclib/psx/internal.h"

extern "C" void *resizememadr(void *memory, sint32 length)
{
    FUNCTION_MARKER(0x800F1950, "NFS4.EXE");
    uint8 *payload = static_cast<uint8 *>(memory);
    MemBlock *block = reinterpret_cast<MemBlock *>(payload - sizeof(MemBlock));
    uint16 flags = block->flags;
    MemBlock *next = block->nextPhysical;
    MemClass *heap = memclass[flags & 15];
    if ((next->flags & 0x4000) != 0) {
        FREE::remove(heap, reinterpret_cast<MemFreeBlock *>(next));
        next = next->nextPhysical;
        block->nextPhysical = next;
        next->previousPhysical = block;
    }
    sint32 needed = length;
    sint32 minimumPayload = (sint32)(sizeof(MemFreeBlock) - sizeof(MemBlock));
    if (needed < minimumPayload) {
        if (needed == -1)
            needed = 0x40000000;
        else if (needed >= 0)
            needed = minimumPayload;
    }
    char *name = getblockname(memory);
    sint32 tailSize = MEM::tailsize(name, flags);
    uint32 aligned = (needed + (uint32)tailSize + heap->alignment + (uint32)sizeof(MemBlock) - 1U) & (~(((uint32)heap->alignment)) + 1);
    needed = aligned - (uint32)sizeof(MemBlock);
    sint32 available = reinterpret_cast<uintptr_t>(next) - reinterpret_cast<uintptr_t>(block) - sizeof(MemBlock);
    if (available < needed) {
        needed = available;
        length = available - tailSize;
    }
    blockmove(payload + block->length, payload + length, tailSize);
    block->length = length;
    sint32 remaining = available - needed;
    if (remaining > 64) {
        MemFreeBlock *fragment = ::new (payload + needed + sizeof(MemBlock)) MemFreeBlock;
        initmemblock(&fragment->block, 0, 0, 0, 0, block, next);
        FREE::add(heap, fragment);
        next->previousPhysical = &fragment->block;
        block->nextPhysical = &fragment->block;
    }
    return memory;
}
