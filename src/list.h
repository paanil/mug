#ifndef LIST_H
#define LIST_H

#include "assert.h"

#include <cstdint>
#include <cstdlib>


/**
 * Dynamic array that can be used as a stack or a list.
 */
template <class T>
struct List
{
    uint32_t size;
    uint32_t capacity;
    T *data;

    List()
    : size()
    , capacity()
    , data()
    { }

    ~List()
    {
        if (data)
            free(data);
    }

    void _ensure_capacity(uint32_t capacity_)
    {
        if (capacity >= capacity_)
            return;

        uint32_t cap =
            capacity ? capacity : 32;

        while (cap < capacity_)
            cap *= 2;

        T *data_ = (T *)malloc(sizeof(T) * cap);

        if (data)
        {
            memcpy(data_, data, sizeof(T) * size);
            free(data);
        }

        data = data_;
        capacity = cap;
    }

    void push(const T &value)
    {
        _ensure_capacity(size + 1);

        data[size++] = value;
    }

    const T &pop()
    {
        assert(size > 0);

        return data[--size];
    }

    void resize(uint32_t new_size)
    {
        _ensure_capacity(new_size);

        size = new_size;
    }

    uint32_t get_size()
    {
        return size;
    }

    T &operator [] (uint32_t index)
    {
        assert(index < size);

        return data[index];
    }
};

#endif // LIST_H
