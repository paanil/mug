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

    void alloc_block(unsigned size);
    void free_all();

    void *allocate(unsigned size)
    {
        if (current + size > end)
        {
            alloc_block(4*1024);
        }

        void *ptr = current;
        current += size;
        return ptr;
    }

    template <class T>
    T *allocate()
    {
        return (T *)allocate(sizeof(T));
    }

    template <class T>
    T *allocate_array(int count)
    {
        return (T *)allocate(sizeof(T) * count);
    }
};

#endif // ALLOC_H
