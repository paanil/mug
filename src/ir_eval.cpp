#include "ir_eval.h"
#include "ir.h"
#include "assert.h"

union Value
{
    int64_t  ivalue;
    uint64_t uvalue;
};

struct Voidable
{
    Value value;
    bool is_void;
};

struct Evaluator
{
    int env_index;
    // TODO: Better environment and routine table.
    Value env[20][100]; // max 20 nested calls, max 100 temps per call
    Routine *routines[20]; // max 20 routines

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
                case IR::XOR_IM:
                    set(quad.target, get(quad.left).uvalue ^ quad.right.int_value);
                    break;
                case IR::JMP:
                    i = quad.target.label;
                    break;
                case IR::JZ:
                    if (get(quad.left).uvalue == 0)
                        i = quad.target.label;
                    break;
                case IR::JNZ:
                    if (get(quad.left).uvalue != 0)
                        i = quad.target.label;
                    break;
                case IR::LABEL:
                    break;
                case IR::CALL:
                {
                    assert(env_index < 20);
                    env_index++;
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
                        Voidable rv = {};
                        rv.value = get(quad.left);
                        return rv;
                    }
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

        Voidable rv;
        rv.is_void = true;
        return rv;
    }

    void eval(IR ir)
    {
        Routine *routine = ir.routines;
        while (routine)
        {
            assert(routine->id < 20);
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
