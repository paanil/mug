#ifndef ALLOC_H
#define ALLOC_H

#define BLOCK_SIZE 64*1024

/**
 * With Alloc the user can allocate memory without worrying about
 * freeing it. All the allocated memory is freed by calling free_all()
 * or by letting the destructor do it.
 * Internally allocates new blocks of memory when needed and
 * gives the user small pieces of that memory. The blocks are
 * kept in a linked list for future freeing.
 */
struct Alloc
{
    struct Block
    {
        void *memory;
        Block *next;
    };

    Block *blocks;
    char *current;
    char *end;

    Alloc()
    : blocks()
    , current()
    , end()
    {}

    ~Alloc()
    {
        free_all();
    }

    char *align(char *ptr, unsigned alignment);

    void alloc_block(unsigned size);
    void free_all();

    void *allocate(unsigned size, unsigned alignment)
    {
        char *ptr = align(current, alignment);

        if (ptr + size > end)
        {
            if (size + alignment - 1 > BLOCK_SIZE)
                alloc_block(size + alignment); // some extra space for aligning
            else
                alloc_block(BLOCK_SIZE);
            ptr = align(current, alignment);
        }

        current = ptr + size;
        return ptr;
    }

    template <class T>
    T *allocate()
    {
        return (T *)allocate(sizeof(T), alignof(T));
    }

    template <class T>
    T *allocate_array(unsigned count)
    {
        return (T *)allocate(sizeof(T) * count, alignof(T));
    }
};

#endif // ALLOC_H
