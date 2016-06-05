#ifndef IR_H
#define IR_H

#include "ir_gen.h"
#include "str.h"

union Operand
{
    uint32_t temp_id;
    uint32_t func_id;
    uint32_t arg_index;
    uint32_t label;
    uint64_t int_value;
    bool returns_something;
};

struct Quad
{
    IR::Type op;
    Operand target;
    Operand left;
    Operand right;
};

struct Quads
{
    static const uint32_t N = 64;
    Quad quads[N];
    Quads *next;
};

struct Routine
{
    uint32_t next_temp_id;

    uint32_t param_count;

    uint32_t n;
    Quads *head;
    Quads *tail;

    Str name;
    uint32_t id;
    Routine *next;

    Routine(Str name_, uint32_t id_)
    : next_temp_id()
    , n()
    , head()
    , tail()
    , name(name_)
    , id(id_)
    , next()
    {}

    Operand make_temp()
    {
        Operand result;
        result.temp_id = next_temp_id++;
        return result;
    }

    Operand make_label(Alloc &a)
    {
        Operand result;
        result.label = n;
        add(a, IR::LABEL, result);
        return result;
    }

    Quad *add(Alloc &a, Quad quad);

    Quad *add(Alloc &a, IR::Type op)
    { return add(a, (Quad){op, {}, {}, {}}); }
    Quad *add(Alloc &a, IR::Type op, Operand target)
    { return add(a, (Quad){op, target, {}, {}}); }
    Quad *add(Alloc &a, IR::Type op, Operand target, Operand operand)
    { return add(a, (Quad){op, target, operand, {}}); }
    Quad *add(Alloc &a, IR::Type op, Operand target, Operand left, Operand right)
    { return add(a, (Quad){op, target, left, right}); }

    Quad &operator [] (uint32_t index);
};

#endif // IR_H
