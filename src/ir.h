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

    Alloc &a;

    Routine(Str name_, uint32_t id_, Alloc &a_)
    : next_temp_id()
    , n()
    , head()
    , tail()
    , name(name_)
    , id(id_)
    , next()
    , a(a_)
    {}

    Operand make_temp()
    {
        Operand result;
        result.temp_id = next_temp_id++;
        return result;
    }

    Operand make_label()
    {
        Operand result;
        result.label = n;
        add(IR::LABEL, result);
        return result;
    }

    Quad *add(Quad quad);

    Quad *add(IR::Type op)
    { return add((Quad){op, {}, {}, {}}); }
    Quad *add(IR::Type op, Operand target)
    { return add((Quad){op, target, {}, {}}); }
    Quad *add(IR::Type op, Operand target, Operand operand)
    { return add((Quad){op, target, operand, {}}); }
    Quad *add(IR::Type op, Operand target, Operand left, Operand right)
    { return add((Quad){op, target, left, right}); }

    Quad &operator [] (uint32_t index);
};

#endif // IR_H
