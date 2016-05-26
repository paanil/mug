#ifndef STR_H
#define STR_H

#include <cstdint>
#include <cstring>

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

#endif // STR_H

