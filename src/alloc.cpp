#include "alloc.h"

#include <cstdlib>
#include <cassert>

char *Alloc::align(char *ptr, unsigned alignment)
{
    uintptr_t uptr = (uintptr_t)ptr;
    uintptr_t mask = alignment - 1;
    assert(alignment && (alignment & mask) == 0);
    uptr = (uptr + mask) & ~mask;
    return (char *)uptr;
}

void Alloc::alloc_block(unsigned size)
{
    void *memory = malloc(size);

    Block *block = (Block *)memory;
    block->memory = memory;
    block->next = blocks;
    blocks = block;

    current = (char *)(block + 1);
    end = ((char *)memory) + size;
}

void Alloc::free_all()
{
    Block *block = blocks;
    while (block)
    {
        Block *next = block->next;
        free(block->memory);
        block = next;
    }

    blocks = nullptr;
    current = nullptr;
    end = nullptr;
}
