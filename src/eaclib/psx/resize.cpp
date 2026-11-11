#include "xport.h"
#include "eaclib/psx/internal.h"

extern "C" void *resizememadr(void *memory, sint32 length)
{
    FUNCTION_MARKER(0x800F1950, "NFS4.EXE");
    uint8 *payload = static_cast<uint8 *>(memory);
    MemBlock *block = reinterpret_cast<MemBlock *>(payload - 16);
    uint16 flags = block->flags;
    MemBlock *next = block->nextPhysical;
    MemClass *heap = memclass[flags & 15];
    if ((next->flags & 0x4000) != 0) {
        FREE::remove(heap, reinterpret_cast<MemFreeBlock *>(next));
        next = next->nextPhysical;
        block->nextPhysical = next;
    }
    sint32 needed = length;
    if (needed < 8) {
        if (needed == -1)
            needed = 0x40000000;
        else if (needed >= 0)
            needed = 8;
    }
    char *name = getblockname(memory);
    sint32 tailSize = MEM::tailsize(name, flags);
    uint32 aligned = ((uint32)needed + (uint32)tailSize + (uint32)heap->alignment + 15) & (0 - (uint32)heap->alignment);
    needed = aligned - 16;
    sint32 available = reinterpret_cast<uintptr_t>(next) - reinterpret_cast<uintptr_t>(block) - 16;
    if (available < needed) {
        needed = available;
        length = (uint32)available - (uint32)tailSize;
    }
    blockmove(payload + block->length, payload + length, tailSize);
    block->length = length;
    sint32 remaining = (uint32)available - (uint32)needed;
    if (remaining > 64) {
        MemFreeBlock *fragment = ::new (payload + needed + 16) MemFreeBlock;
        initmemblock(&fragment->block, 0, 0, 0, 0, block, next);
        FREE::add(heap, fragment);
        next->previousPhysical = &fragment->block;
        block->nextPhysical = &fragment->block;
    }
    return memory;
}
