#ifndef STR_MAP_H
#define STR_MAP_H

#include "str.h"

#include <cstdlib>
#include <cassert>

#define MAX_PROBE 8
#define NOT_FOUND (~0u)

template <class ValueT>
struct StrMap
{
    struct Entry
    {
        Str key;
        ValueT value;
    };

    uint32_t table_size;
    Entry *table;

    StrMap()
    : table_size()
    , table()
    {
        _alloc_table(61);
    }

    ~StrMap()
    {
        if (table)
            free(table);
    }

    void _alloc_table(uint32_t size)
    {
        table_size = size;
        table = (Entry *)malloc(sizeof(Entry) * size);
        memset(table, 0, sizeof(Entry) * size);
    }

    void _grow()
    {
        uint32_t old_size = table_size;
        Entry *old_table = table;

        _alloc_table(table_size * 2);

        for (uint32_t i = 0; i < old_size; i++)
        {
            Entry &e = old_table[i];
            if (e.key.len != 0)
                _add(e.key, e.value);
        }

        free(old_table);
    }

    void _add(Str key, const ValueT &value)
    {
        for (int i = 0, probe = 0; probe < MAX_PROBE; i++)
        {
            uint32_t index = (key.hash + i) % table_size;

            Entry &entry = table[index];

            if (entry.key.len == 0)
            {
                entry.key = key;
                entry.value = value;
                return;
            }

            probe++;
        }

        // probed too far -> grow table
        _grow();
        _add(key, value);
    }

    uint32_t _find(Str key)
    {
        for (int i = 0, probe = 0; probe < MAX_PROBE; i++)
        {
            uint32_t index = (key.hash + i) % table_size;

            Entry &entry = table[index];

            if (entry.key.hash != key.hash)
                probe++;
            else if (entry.key == key)
                return index;
        }

        return NOT_FOUND;
    }

    uint32_t find(Str key)
    {
        return _find(key);
    }

    void set(Str key, const ValueT &value)
    {
        uint32_t index = _find(key);
        if (index == NOT_FOUND) _add(key, value);
        else table[index].value = value;
    }

    const ValueT &get(uint32_t index)
    {
        assert(index < table_size);
        return table[index].value;
    }

    void remove(uint32_t index)
    {
        assert(index < table_size);
        memset(table + index, 0, sizeof(Entry));
    }
};

#undef MAX_PROBE

#endif // STR_MAP_H
