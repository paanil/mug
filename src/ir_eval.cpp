#include "ir_eval.h"
#include "ir.h"
#include "list.h"
#include "assert.h"

union Value
{
    int64_t  ivalue;
    uint64_t uvalue;
};

/**
 * Function calls might not return anything.
 */
struct Voidable
{
    Value value;
    bool is_void;
};

/**
 * Very simple evaluator that is used for testing IR generation.
 * The evaluator doesn't call external functions. All external
 * routines evaluate to void.
 */
struct Evaluator
{
    int env_index;
    // TODO: Better environment.
    // NOTE: Temp ids are used as indices to the environment.
    Value env[20][100]; // max 20 nested calls, max 100 temps per call
    List<Routine*> routines; // routine table can be indexed with routine ids

    Voidable lastval; // for tests... which is not very intuitive :S


    Value get(Operand temp)
    {
        assert(temp.temp_id < 100);
        return env[env_index][temp.temp_id];
    }

    void set(Operand temp, uint64_t value)
    {
        assert(temp.temp_id < 100);
        env[env_index][temp.temp_id].uvalue = value;

        lastval.value.uvalue = value;
        lastval.is_void = false;
    }

    Voidable eval(Routine *routine)
    {
        Voidable rv;
        rv.is_void = true;

        int quad_count = routine->quad_count;
        for (int i = 0; i < quad_count; i++)
        {
            Quad quad = (*routine)[i];
            switch (quad.op)
            {
                case IR::MOV_IM:
                    set(quad.target, quad.left.int_value);
                    break;
                case IR::MOV:
                    set(quad.target, get(quad.left).uvalue);
                    break;
                case IR::NOT:
                    set(quad.target, !get(quad.left).uvalue);
                    break;
                case IR::NEG:
                    set(quad.target, -get(quad.left).ivalue);
                    break;
                case IR::MUL:
                    set(quad.target, get(quad.left).uvalue * get(quad.right).uvalue);
                    break;
                case IR::IMUL:
                    set(quad.target, get(quad.left).ivalue * get(quad.right).ivalue);
                    break;
                case IR::DIV:
                    set(quad.target, get(quad.left).uvalue / get(quad.right).uvalue);
                    break;
                case IR::IDIV:
                    set(quad.target, get(quad.left).ivalue / get(quad.right).ivalue);
                    break;
                case IR::ADD:
                    set(quad.target, get(quad.left).uvalue + get(quad.right).uvalue);
                    break;
                case IR::SUB:
                    set(quad.target, get(quad.left).uvalue - get(quad.right).uvalue);
                    break;
                case IR::EQ:
                    set(quad.target, get(quad.left).uvalue == get(quad.right).uvalue);
                    break;
                case IR::NE:
                    set(quad.target, get(quad.left).uvalue != get(quad.right).uvalue);
                    break;
                case IR::LT:
                    set(quad.target, get(quad.left).ivalue < get(quad.right).ivalue);
                    break;
                case IR::BELOW:
                    set(quad.target, get(quad.left).uvalue < get(quad.right).uvalue);
                    break;
                case IR::GT:
                    set(quad.target, get(quad.left).ivalue > get(quad.right).ivalue);
                    break;
                case IR::ABOVE:
                    set(quad.target, get(quad.left).uvalue > get(quad.right).uvalue);
                    break;
                case IR::LE:
                    set(quad.target, get(quad.left).ivalue <= get(quad.right).ivalue);
                    break;
                case IR::BE:
                    set(quad.target, get(quad.left).uvalue <= get(quad.right).uvalue);
                    break;
                case IR::GE:
                    set(quad.target, get(quad.left).ivalue >= get(quad.right).ivalue);
                    break;
                case IR::AE:
                    set(quad.target, get(quad.left).uvalue >= get(quad.right).uvalue);
                    break;
                case IR::JMP:
                    i = quad.target.label;
                    break;
                case IR::JZ:
                    if (get(quad.left).uvalue == 0)
                        i = quad.target.label; // for loop's i++ will skip the label quad
                    break;
                case IR::JNZ:
                    if (get(quad.left).uvalue != 0)
                        i = quad.target.label; // for loop's i++ will skip the label quad
                    break;
                case IR::LABEL:
                    break;
                case IR::CALL:
                {
                    assert(env_index < 20);
                    env_index++;
                    // NOTE: External routines have no quads => evaluation just returns void.
                    Voidable rv = eval(routines[quad.left.func_id]);
                    env_index--;
                    set(quad.target, rv.value.uvalue);
                    lastval.is_void = rv.is_void;
                    break;
                }
                case IR::RET:
                {
                    if (quad.target.returns_something)
                    {
                        rv.value = get(quad.left);
                        rv.is_void = false;
                    }
                    i = quad_count; // break the loop
                    break;
                }
                case IR::ARG:
                {
                    assert(env_index < 20);
                    Value arg = get(quad.left);
                    env_index++;
                    Operand temp;
                    temp.temp_id = quad.target.arg_index;
                    set(temp, arg.uvalue);
                    env_index--;
                    break;
                }
            }
        }

        return rv;
    }

    void eval(IR ir)
    {
        // initialize routine table
        Routine *routine = ir.routines;
        while (routine)
        {
            if (routines.get_size() < routine->id + 1)
                routines.resize(routine->id + 1);
            routines[routine->id] = routine;
            routine = routine->next;
        }

        lastval.is_void = true;

        env_index = 0;
        eval(routines[0]);
    }
};


//
//
//

uint64_t eval(IR &ir)
{
    if (ir.routines == nullptr)
    {
        return 0;
    }

    Evaluator evaluator;
    evaluator.eval(ir);

    if (evaluator.lastval.is_void)
        return VOID_VALUE;
    return evaluator.lastval.value.uvalue;
}
