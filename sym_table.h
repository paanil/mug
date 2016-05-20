#ifndef SYM_TABLE_H
#define SYM_TABLE_H

#include "stack.h"

struct SymTable
{
    struct Entry
    {
        Type type;
        uint32_t scope;
    };

    struct Stashed
    {
        Str symbol;
        Entry entry;
    };

    uint32_t scope_id;
    uint32_t next_scope_id;
    StrMap<Entry> table;
    Stack<Stashed> stash;

    SymTable()
    : scope_id()
    , next_scope_id() // top level has its own scope -> next id can be 0
    , table()
    , stash()
    { }

    bool has(Str symbol, Type *result)
    {
        uint32_t idx = table.find(symbol);
        if (idx == NOT_FOUND) return false;
        *result = table.get(idx).type;
        return true;
    }

    bool in_current_scope(Str symbol)
    {
        uint32_t idx = table.find(symbol);
        if (idx == NOT_FOUND) return false;
        return (table.get(idx).scope == scope_id);
    }

    void put(Str symbol, Type type)
    {
        uint32_t idx = table.find(symbol);
        if (idx != NOT_FOUND)
        {
            Stashed item = { symbol, table.get(idx) };
            stash.push(item);
        }

        Entry entry = { type, scope_id };
        table.set(symbol, entry);
    }

    void enter_scope()
    {
        Stashed scope_marker = {}; // symbol.data = nullptr
        scope_marker.entry.scope = scope_id;
        scope_id = next_scope_id++;
        stash.push(scope_marker);
    }

    void exit_scope()
    {
        while (1)
        {
            Stashed item = stash.pop();

            if (item.symbol.data == nullptr) // scope_marker
            {
                scope_id = item.entry.scope;
                break;
            }

            table.set(item.symbol, item.entry);
        }
    }
};

#endif // SYM_TABLE_H
