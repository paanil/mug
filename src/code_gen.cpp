#include "code_gen.h"
#include "ir.h"
#include "register_alloc.h"
#include "code.h"

struct CodeGen
{
    struct Temp
    {
        RegID reg_id; // Register that has the temp in it or none.
        int32_t base_offset;
        bool spilled;
    };

    int32_t spilled_count;
    RegisterAlloc regs;
    List<Temp> temps;
    List<int32_t> args;
    Code &code;

    CodeGen(Code &code_)
    : code(code_)
    {}

    void spill(Register reg, bool no_reset_to_none = false)
    {
        if (reg.temp_id == Reg_NONE)
            return;

        Temp &temp = temps[reg.temp_id];
        if (!temp.spilled)
        {
            if (temp.base_offset == 0)
                temp.base_offset = -8 - 8 * spilled_count++;
            temp.spilled = true;
        }
        code.store(temp.base_offset, reg);
        if (!no_reset_to_none)
            temp.reg_id = Reg_NONE;
    }

    Register get_register(RegID reg_id)
    {
        Register reg = regs.alloc_register(reg_id, -1);
        spill(reg);
        return reg;
    }

    Register get_register_for(RegID reg_id, int32_t temp_id, bool load_spilled)
    {
        Register reg = regs.alloc_register(reg_id, temp_id);

        if (reg.temp_id != temp_id)
        {
            spill(reg);

            if (load_spilled)
            {
                Temp temp = temps[temp_id];
                if (temp.reg_id != Reg_NONE)
                {
                    Register old = regs.get_register_by_id(temp.reg_id);
                    code.mov(reg, old);
                    regs.dealloc_register(old.id);
                }
                else if (temp.spilled)
                {
                    code.load(reg, temp.base_offset);
                }
            }

            temps[temp_id].reg_id = reg.id;
        }
        else
        {
            assert(temps[temp_id].reg_id == reg.id);
        }

        return reg;
    }

    Register get_any_register()
    {
        Register reg = regs.alloc_any_register(-1);
        spill(reg);
        return reg;
    }

    Register get_any_register_for(int32_t temp_id, bool load_spilled)
    {
        Temp temp = temps[temp_id];
        if (temp.reg_id != Reg_NONE)
            return regs.alloc_register(temp.reg_id, temp_id);

        Register reg = regs.alloc_any_register(temp_id);
        spill(reg);
        if (temp.spilled && load_spilled)
            code.load(reg, temp.base_offset);
        temps[temp_id].reg_id = reg.id;
        return reg;
    }

    void gen_code(Quad q)
    {
        switch (q.op)
        {
            case IR::MOV_IM:
            {
                Register reg = get_any_register_for(q.target.temp_id, false);
                code.mov(reg, q.left.int_value);
                break;
            }
            case IR::MOV:
            {
                Register target = get_any_register_for(q.target.temp_id, false);
                Register left = get_any_register_for(q.left.temp_id, true);
                code.mov(target, left);
                break;
            }
            case IR::NOT:
            {
                Register target = get_any_register_for(q.target.temp_id, false);
                Register left = get_any_register_for(q.left.temp_id, true);
                code.mov(target, left);
                code.xor_(target, 1);
                break;
            }
            case IR::NEG:
            {
                Register target = get_any_register_for(q.target.temp_id, false);
                Register left = get_any_register_for(q.left.temp_id, true);
                code.mov(target, left);
                code.neg(target);
                break;
            }
            case IR::MUL: case IR::IMUL:
            case IR::DIV: case IR::IDIV:
            {
                Register rax = get_register_for(Reg_rax, q.target.temp_id, false);
                Register rdx = get_register(Reg_rdx);
                Register left = get_any_register_for(q.left.temp_id, true);
                Register right = get_any_register_for(q.right.temp_id, true);
                code.mov(rax, left);
                switch (q.op)
                {
                    case IR::MUL:
                        code.mul(right);
                        break;
                    case IR::IMUL:
                        code.imul(right);
                        break;
                    case IR::DIV:
                        code.zero(rdx);
                        code.div(right);
                        break;
                    case IR::IDIV:
                        code.sign_extend_rax_to_rdx();
                        code.idiv(right);
                        break;
                    InvalidDefaultCase;
                }
                break;
            }
            case IR::ADD:
            case IR::SUB:
            {
                Register target = get_any_register_for(q.target.temp_id, false);
                Register left = get_any_register_for(q.left.temp_id, true);
                Register right = get_any_register_for(q.right.temp_id, true);
                code.mov(target, left);
                switch (q.op)
                {
                    case IR::ADD: code.add(target, right); break;
                    case IR::SUB: code.sub(target, right); break;
                    InvalidDefaultCase;
                }
                break;
            }
            case IR::EQ: case IR::NE:
            case IR::LT: case IR::BELOW:
            case IR::GT: case IR::ABOVE:
            case IR::LE: case IR::BE:
            case IR::GE: case IR::AE:
            {
                Register left = get_any_register_for(q.left.temp_id, true);
                Register right = get_any_register_for(q.right.temp_id, true);
                Register target = get_any_register_for(q.target.temp_id, false);
                Register temp = get_any_register();
                code.zero(target);
                code.mov(temp, 1);
                code.cmp(left, right);
                switch (q.op)
                {
                    case IR::EQ:    code.cmove(target, temp);   break;
                    case IR::NE:    code.cmovne(target, temp);  break;
                    case IR::LT:    code.cmovl(target, temp);   break;
                    case IR::BELOW: code.cmovb(target, temp);   break;
                    case IR::GT:    code.cmovg(target, temp);   break;
                    case IR::ABOVE: code.cmova(target, temp);   break;
                    case IR::LE:    code.cmovle(target, temp);  break;
                    case IR::BE:    code.cmovbe(target, temp);  break;
                    case IR::GE:    code.cmovge(target, temp);  break;
                    case IR::AE:    code.cmovae(target, temp);  break;
                    InvalidDefaultCase;
                }
                break;
            }
            case IR::JMP:
            {
                end_basic_block();
                code.jmp(q.target.label);
                begin_basic_block();
                break;
            }
            case IR::JZ:
            case IR::JNZ:
            {
                Register left = get_any_register_for(q.left.temp_id, true);
                code.cmp(left, 0);
                end_basic_block();
                switch (q.op)
                {
                    case IR::JZ:  code.je(q.target.label);  break;
                    case IR::JNZ: code.jne(q.target.label); break;
                    InvalidDefaultCase;
                }
                begin_basic_block();
                break;
            }
            case IR::LABEL:
            {
                end_basic_block();
                code.label(q.target.label);
                begin_basic_block();
                break;
            }
            case IR::RET:
            {
                if (q.target.returns_something)
                {
                    Register rax = regs.get_register_by_id(Reg_rax);
                    Register reg = get_any_register_for(q.left.temp_id, true);
                    if (rax.id != reg.id)
                        code.mov(rax, reg);
                }
                code.jmp_epi();
                break;
            }
            case IR::ARG:
            {
                uint32_t arg_index = q.target.arg_index;
                if (args.get_size() < arg_index + 1)
                    args.resize(arg_index + 1);
                args[arg_index] = q.left.temp_id;
                break;
            }
            case IR::CALL:
            {
                end_basic_block();

                while (args.get_size())
                {
                    int32_t temp_id = args.pop();

                    Register reg;
                    if (regs.get_param_register(args.get_size(), &reg))
                    {
                        get_register_for(reg.id, temp_id, true);
                    }
                    else
                    {
                        reg = get_any_register_for(temp_id, true);
                    }
                    code.push(reg);
                }

                code.call(q.left.func_id);
                begin_basic_block();
                get_register_for(Reg_rax, q.target.temp_id, false);
                break;
            }
        }
    }

    void begin_basic_block()
    {
        regs.reset();
        int temp_count = temps.get_size();
        for (int i = 0; i < temp_count; i++)
            temps[i].reg_id = Reg_NONE;
    }

    void end_basic_block()
    {
        for (int i = Reg_NONE + 1; i < Reg_COUNT; i++)
        {
            Register reg = regs.get_register_by_id((RegID)i);
            spill(reg, true);
        }
    }

    void gen_code(Routine *routine)
    {
        spilled_count = 0;
        regs.reset();
        temps.resize(routine->temp_count);

        int temp_count = temps.get_size();
        for (int i = 0; i < temp_count; i++)
        {
            temps[i].reg_id = Reg_NONE;
            temps[i].base_offset = 0;
            temps[i].spilled = false;
        }

        int param_count = routine->param_count;
        // TODO: Use spilled_count in prolog.
        code.prolog(routine->name, (((uint32_t)(temp_count - param_count) + 1u) & ~1u)*8u);

        for (int i = 0; i < param_count; i++)
        {
            Register reg;
            if (regs.get_param_register(i, &reg))
            {
                regs.alloc_register(reg.id, i);
                temps[i].reg_id = reg.id;
            }
            else
            {
                temps[i].spilled = true;
            }
            temps[i].base_offset = 16 + 8 * i;
        }

        int quad_count = routine->quad_count;
        for (int i = 0; i < quad_count; i++)
        {
            gen_code((*routine)[i]);
        }

        code.epilog();
    }
};

void gen_code(IR ir, FILE *f)
{
    if (ir.routines == nullptr || ir.routines->next == nullptr)
    {
        fprintf(stderr, "ir is null!\n");
        return;
    }

    print_ir(ir);
    fprintf(stdout, "\n\n");

    Code code(f);

    // TODO: How to handle top level?
    code.global_routine(Str::make("@top_level"));

    Routine *routine = ir.routines->next;
    while (routine)
    {
        code.global_routine(routine->name);
        routine = routine->next;
    }

    code.section_text();

    CodeGen gen(code);
    routine = ir.routines->next;
    while (routine)
    {
        gen.gen_code(routine);
        routine = routine->next;
    }
}
