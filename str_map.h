#ifndef STR_MAP_H
#define STR_MAP_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cassert>

struct Str
{
    uint32_t hash;
    int32_t len;
    const char *data;

    static Str make(const char *s)
    {
        Str str;
        str.len = strlen(s);
        str.data = s;
        str.hash = compute_hash(str.data, str.len);
        return str;
    }
    static Str make(const char *s, int len)
    {
        Str str;
        str.len = len;
        str.data = s;
        str.hash = compute_hash(str.data, str.len);
        return str;
    }

    static uint64_t compute_hash(const char *s, int n)
    {
        uint64_t h = 0;
        for (int i = 0; i < n; i++)
            h = h * 65599 + s[i];
        return h;
    }

    bool operator == (Str s)
    {
        if (hash != s.hash) return false;
        if (len != s.len) return false;
        return (strncmp(data, s.data, len) == 0);
    }
};


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
};

#undef MAX_PROBE

#endif // STR_MAP_H
