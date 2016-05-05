#include "alloc.h"

#include <cstdlib>


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

    blocks = 0;
    current = 0;
    end = 0;
}
