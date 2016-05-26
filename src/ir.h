#ifndef IR_H
#define IR_H

#include "ir_gen.h"
#include "str.h"

union Operand
{
    uint32_t temp_id;
    uint32_t func_id;
    uint32_t arg_index;
    uint32_t jump; // TODO: Label.
    uint64_t int_value;
    bool returns_something;
};

struct Quad
{
    IR::Type op;
    Operand target;
    Operand left;
    Operand right;

    Quad()
    {}

    Quad(IR::Type op_)
    : op(op_)
    {}

    Quad(IR::Type op_, Operand target_)
    : op(op_)
    , target(target_)
    {}

    Quad(IR::Type op_, Operand target_, Operand operand)
    : op(op_)
    , target(target_)
    , left(operand)
    {}

    Quad(IR::Type op_, Operand target_, Operand left_, Operand right_)
    : op(op_)
    , target(target_)
    , left(left_)
    , right(right_)
    {}
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

    Operand make_jump_target()
    {
        Operand result;
        result.jump = n;
        return result;
    }

    Quad *add(Quad quad, Alloc &a);

    void set_jump_target_here(Quad *quad)
    {
        quad->target.jump = n;
    }

    Quad &operator [] (uint32_t index);
};

#endif // IR_H
