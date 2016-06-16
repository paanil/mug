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

/**
 * Routine containes linked list of quad blocks.
 * One block contains 64 quads.
 */
struct Quads
{
    static const uint32_t N = 64;
    Quad quads[N];
    Quads *next;
};

/**
 * Routines form a linked list. The IR struct has
 * a pointer to the first routine (which is the top
 * level routine).
 *
 * All routines have id that can be used as an
 * index to a routine table.
 * E.g. if there are 8 routines total, they will have
 * ids from 0 to 7. Top level routine's id is 0.
 *
 * All temps inside a routine have similarly ids that
 * can be used as indices. Params and vars are temps.
 * Params have the first ids (from 0 to param_count - 1).
 */
struct Routine
{
    uint32_t temp_count;
    uint32_t param_count;

    uint32_t quad_count;
    Quads *head;
    Quads *tail;

    Str name;
    uint32_t id;
    Routine *next;

    Alloc &a;

    Routine(Str name_, uint32_t id_, Alloc &a_)
    : temp_count()
    , param_count()
    , quad_count()
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
        result.temp_id = temp_count++;
        return result;
    }

    Operand make_label()
    {
        Operand result;
        result.label = quad_count;
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
