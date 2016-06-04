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

    Voidable eval(Routine &r)
    {
        for (uint32_t i = 0; i < r.n; i++)
        {
            Quad q = r[i];
            switch (q.op)
            {
                case IR::MOV_IM:
                    set(q.target, q.left.int_value);
                    break;
                case IR::MOV:
                    set(q.target, get(q.left).uvalue);
                    break;
                case IR::MUL:
                    set(q.target, get(q.left).uvalue * get(q.right).uvalue);
                    break;
                case IR::IMUL:
                    set(q.target, get(q.left).ivalue * get(q.right).ivalue);
                    break;
                case IR::ADD:
                    set(q.target, get(q.left).uvalue + get(q.right).uvalue);
                    break;
                case IR::SUB:
                    set(q.target, get(q.left).uvalue - get(q.right).uvalue);
                    break;
                case IR::EQ:
                    set(q.target, get(q.left).uvalue == get(q.right).uvalue);
                    break;
                case IR::NE:
                    set(q.target, get(q.left).uvalue != get(q.right).uvalue);
                    break;
                case IR::LT:
                    set(q.target, get(q.left).ivalue < get(q.right).ivalue);
                    break;
                case IR::BELOW:
                    set(q.target, get(q.left).uvalue < get(q.right).uvalue);
                    break;
                case IR::GT:
                    set(q.target, get(q.left).ivalue > get(q.right).ivalue);
                    break;
                case IR::ABOVE:
                    set(q.target, get(q.left).uvalue > get(q.right).uvalue);
                    break;
                case IR::LE:
                    set(q.target, get(q.left).ivalue <= get(q.right).ivalue);
                    break;
                case IR::BE:
                    set(q.target, get(q.left).uvalue <= get(q.right).uvalue);
                    break;
                case IR::GE:
                    set(q.target, get(q.left).ivalue >= get(q.right).ivalue);
                    break;
                case IR::AE:
                    set(q.target, get(q.left).uvalue >= get(q.right).uvalue);
                    break;
                case IR::XOR_IM:
                    set(q.target, get(q.left).uvalue ^ q.right.int_value);
                    break;
                case IR::JMP:
                    i = q.target.label;
                    break;
                case IR::JZ:
                    if (get(q.left).uvalue == 0)
                        i = q.target.label;
                    break;
                case IR::JNZ:
                    if (get(q.left).uvalue != 0)
                        i = q.target.label;
                    break;
                case IR::LABEL:
                    break;
                case IR::CALL:
                {
                    assert(env_index < 20);
                    env_index++;
                    Voidable rv = eval(*routines[q.left.func_id]);
                    env_index--;
                    set(q.target, rv.value.uvalue);
                    lastval.is_void = rv.is_void;
                    break;
                }
                case IR::RET:
                {
                    if (q.target.returns_something)
                    {
                        Voidable rv = {};
                        rv.value = get(q.left);
                        return rv;
                    }
                    break;
                }
                case IR::ARG:
                {
                    assert(env_index < 20);
                    Value arg = get(q.left);
                    env_index++;
                    Operand temp;
                    temp.temp_id = q.target.arg_index;
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
        Routine *r = ir.routines;
        while (r)
        {
            assert(r->id < 20);
            routines[r->id] = r;
            r = r->next;
        }

        lastval.is_void = true;

        env_index = 0;
        eval(*routines[0]);
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
