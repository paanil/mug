#ifndef STACK_H
#define STACK_H

template <class T>
struct Stack
{
    uint32_t n;
    uint32_t size;
    T *data;

    Stack()
    : n()
    , size()
    , data()
    { }

    ~Stack()
    {
        if (data)
            free(data);
    }

    void _grow()
    {
        uint32_t new_size = size * 2;
        T *new_data = (T *)malloc(sizeof(T) * new_size);

        if (data)
        {
            memcpy(new_data, data, sizeof(T) * n);
            free(data);
        }

        data = new_data;
        size = new_size;
    }

    void push(const T &value)
    {
        if (n == size)
        {
            if (size == 0)
                size = 32;
            _grow();
        }

        data[n++] = value;
    }

    const T &pop()
    {
        assert(n > 0);

        return data[--n];
    }
};

#endif // STACK_H
