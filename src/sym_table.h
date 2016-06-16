#ifndef SYM_TABLE_H
#define SYM_TABLE_H

#include "str_map.h"
#include "list.h"
#include "type.h"

/**
 * Value type templated symbol table.
 */
template <class ValueT>
struct SymTable
{
    struct Entry
    {
        ValueT value;
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
    List<Stashed> stash;

    SymTable()
    : scope_id()
    , next_scope_id() // top level has its own scope -> next id can be 0
    , table()
    , stash()
    { }

    bool has(Str symbol)
    {
        uint32_t idx = table.find(symbol);
        return (idx != NOT_FOUND);
    }

    bool has(Str symbol, ValueT *result)
    {
        uint32_t idx = table.find(symbol);
        if (idx == NOT_FOUND) return false;
        *result = table.get(idx).value;
        return true;
    }

    bool in_current_scope(Str symbol)
    {
        uint32_t idx = table.find(symbol);
        if (idx == NOT_FOUND) return false;
        return (table.get(idx).scope == scope_id);
    }

    void put(Str symbol, const ValueT &value)
    {
        uint32_t idx = table.find(symbol);
        if (idx == NOT_FOUND)
        {
            Stashed item = { symbol, {} };
            item.entry.scope = ~0u;
            stash.push(item);
        }
        else
        {
            Stashed item = { symbol, table.get(idx) };
            stash.push(item);
        }

        Entry entry = { value, scope_id };
        table.set(symbol, entry);
    }

    ValueT get(Str symbol)
    {
        uint32_t idx = table.find(symbol);
        return table.get(idx).value;
    }

    void enter_scope()
    {
        Stashed scope_marker;
        scope_marker.symbol.data = "@scope_marker";
        scope_marker.entry.scope = scope_id;
        scope_id = next_scope_id++;
        stash.push(scope_marker);
    }

    void exit_scope()
    {
        while (1)
        {
            Stashed item = stash.pop();

            const char *scope_marker = "@scope_marker";
            if (item.symbol.data == scope_marker)
            {
                scope_id = item.entry.scope;
                break;
            }

            if (item.entry.scope == ~0u)
                table.remove(table.find(item.symbol));
            else
                table.set(item.symbol, item.entry);
        }
    }
};

#endif // SYM_TABLE_H
