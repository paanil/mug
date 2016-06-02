#ifndef ALLOC_H
#define ALLOC_H

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
            alloc_block(16*1024);
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
