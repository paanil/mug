#include "code_gen.h"
#include "ir.h"
#include "list.h"
#include "assert.h"

#define PASTE_REGS  \
    PASTE_REG(rax)  \
    PASTE_REG(rcx)  \
    PASTE_REG(rdx)  \
    PASTE_REG(r8)   \
    PASTE_REG(r9)   \
    PASTE_REG(r10)  \
    PASTE_REG(r11)

#define PASTE_REG(r) Reg_##r,

enum RegID
{
    Reg_NONE = -1,

    PASTE_REGS

    Reg_COUNT
};

#undef PASTE_REG

struct Register
{
    Str name; // We might want to put registers in a StrMap (?) => not just c-string.
    RegID id; // Actually just an index.
    int32_t temp_id; // Temp currently in the register or -1 if none.
};

#define PARAM_REG_COUNT 4

struct RegisterAlloc
{
    RegID param_registers[PARAM_REG_COUNT];
    RegID register_queue[Reg_COUNT]; // Used for least recently used allocation.
    Register registers[Reg_COUNT];

#define PASTE_REG(r) registers[Reg_##r] = (Register){ Str::make(#r), Reg_##r, -1 };

    void reset()
    {
        param_registers[0] = Reg_rcx;
        param_registers[1] = Reg_rdx;
        param_registers[2] = Reg_r8;
        param_registers[3] = Reg_r9;

        for (int i = 0; i < Reg_COUNT; i++)
            register_queue[i] = (RegID)i;

        PASTE_REGS
    }

#undef PASTE_REG

    void move_back_in_queue(int queue_index)
    {
        RegID reg_id = register_queue[queue_index];
        for (int i = queue_index + 1; i < Reg_COUNT; i++)
            register_queue[i - 1] = register_queue[i];
        register_queue[Reg_COUNT - 1] = reg_id;
    }

    Register alloc_register(RegID reg_id, int32_t temp_id)
    {
        assert(reg_id != Reg_NONE);
        assert(reg_id < Reg_COUNT);

        for (int i = 0; i < Reg_COUNT; i++)
        {
            if (register_queue[i] == reg_id)
            {
                move_back_in_queue(i);
                break;
            }
        }

        Register reg = registers[reg_id];
        registers[reg_id].temp_id = temp_id;
        return reg;
    }

    Register alloc_any_register(int32_t temp_id)
    {
        RegID reg_id = register_queue[0];

        move_back_in_queue(0);

        Register reg = registers[reg_id];
        registers[reg_id].temp_id = temp_id;
        return reg;
    }

    bool alloc_param_register(int param_index, Register *reg)
    {
        if(param_index >= PARAM_REG_COUNT)
            return false;

        RegID reg_id = param_registers[param_index];

        for (int i = 0; i < Reg_COUNT; i++)
        {
            if (register_queue[i] == reg_id)
            {
                move_back_in_queue(i);
                break;
            }
        }

        *reg = registers[reg_id];
        registers[reg_id].temp_id = param_index;
        return true;
    }

    Register get_register_by_id(RegID reg_id)
    {
        assert(reg_id != Reg_NONE);
        assert(reg_id < Reg_COUNT);

        return registers[reg_id];
    }
};

struct Code
{
    FILE *f;

    Code(FILE *out)
    : f(out)
    {}

    void global(Str name)
    {
        fprintf(f, "\t" "global %s\n", name.data);
    }

    void section_text()
    {
        fprintf(f, "\t" "section .text\n");
    }

    void prolog(Str func_name, uint32_t stack_bytes)
    {

        fprintf(f, "%s:\n", func_name.data);
        fprintf(f, "\t" "push rbp\n");
        fprintf(f, "\t" "mov rbp, rsp\n");
        fprintf(f, "\t" "sub rsp, %u\n", stack_bytes);
        fprintf(f, "\n");
    }

    void epilog()
    {
        fprintf(f, ".epi:\n");
        fprintf(f, "\t" "mov rsp, rbp\n");
        fprintf(f, "\t" "pop rbp\n");
        fprintf(f, "\t" "ret\n\n");
    }

    void store(int32_t dest, Register source)
    {
        fprintf(f, "\t" "mov [rbp%+d], %s\n", dest, source.name.data);
    }

    void load(Register dest, int32_t source)
    {
        fprintf(f, "\t" "mov %s, [rbp%+d]\n", dest.name.data, source);
    }

    void mov(Register dest, uint64_t value)
    {
        fprintf(f, "\t" "mov %s, %llu\n", dest.name.data, value);
    }

    void xor_(Register reg, uint64_t value)
    {
        fprintf(f, "\t" "xor %s, %llu\n", reg.name.data, value);
    }

    void cmp(Register reg, uint64_t value)
    {
        fprintf(f, "\t" "cmp %s, %llu\n", reg.name.data, value);
    }

    void neg(Register reg)
    {
        fprintf(f, "\t" "neg %s\n", reg.name.data);
    }

    void zero(Register reg)
    {
        fprintf(f, "\t" "xor %s, %s\n", reg.name.data, reg.name.data);
    }

    void sign_extend_rax_to_rdx()
    {
        fprintf(f, "\t" "cqo\n");
    }

    void label(uint32_t label)
    {
        fprintf(f, ".l%u:\n", label);
    }

    void jmp_epi()
    {
        fprintf(f, "\t" "jmp .epi\n");
    }

    void jmp(uint32_t label)
    {
        fprintf(f, "\t" "jmp .l%u\n", label);
    }

    void je(uint32_t label)
    {
        fprintf(f, "\t" "je .l%u\n", label);
    }

    void jne(uint32_t label)
    {
        fprintf(f, "\t" "jne .l%u\n", label);
    }

#define INSTRUCTION(instr_name) \
    void instr_name(Register r) { \
        fprintf(f, "\t" #instr_name " %s\n", r.name.data); \
    }

    INSTRUCTION(mul)
    INSTRUCTION(imul)
    INSTRUCTION(div)
    INSTRUCTION(idiv)

#define INSTRUCTION2(instr_name) \
    void instr_name(Register a, Register b) { \
        fprintf(f, "\t" #instr_name " %s, %s\n", a.name.data, b.name.data); \
    }

    INSTRUCTION2(mov)
    INSTRUCTION2(cmove)
    INSTRUCTION2(cmovne)
    INSTRUCTION2(cmovl)
    INSTRUCTION2(cmovb)
    INSTRUCTION2(cmovg)
    INSTRUCTION2(cmova)
    INSTRUCTION2(cmovle)
    INSTRUCTION2(cmovbe)
    INSTRUCTION2(cmovge)
    INSTRUCTION2(cmovae)
    INSTRUCTION2(add)
    INSTRUCTION2(sub)
    INSTRUCTION2(cmp)
};

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
    Code &code;

    CodeGen(Code &code_)
    : code(code_)
    {}

    void alloc_param(int param_index)
    {
        Register reg;
        if (regs.alloc_param_register(param_index, &reg))
        {
            temps[param_index].reg_id = reg.id;
        }
        else
        {
//            temps[param_index].stack_offset = 8 * param_index;
//            temps[param_index].spilled = true;
        }
    }

    void spill(Register reg)
    {
        if (reg.temp_id < 0)
            return;

        Temp &temp = temps[reg.temp_id];
        if (!temp.spilled)
        {
            assert(spilled_count < 16);
            temp.base_offset = -8 - 8 * spilled_count++;
            temp.spilled = true;
        }
        code.store(temp.base_offset, reg);
        temp.reg_id = Reg_NONE;
    }

    Register get_register(RegID reg_id)
    {
        Register reg = regs.alloc_register(reg_id, -1);
        spill(reg);
        return reg;
    }

    Register get_register_for_no_unspill(RegID reg_id, int32_t temp_id)
    {
        Register reg = regs.alloc_register(reg_id, temp_id);
        spill(reg);
        temps[temp_id].reg_id = reg.id;
        return reg;
    }

    Register get_any_register()
    {
        Register reg = regs.alloc_any_register(-1);
        spill(reg);
        return reg;
    }

    Register get_any_register_for(int32_t temp_id)
    {
        Temp temp = temps[temp_id];
        if (temp.reg_id >= 0)
            return regs.get_register_by_id(temp.reg_id);

        Register reg = regs.alloc_any_register(temp_id);
        spill(reg);
        if (temp.spilled)
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
                Register reg = get_any_register_for/*_no_unspill*/(q.target.temp_id);
                code.mov(reg, q.left.int_value);
                break;
            }
            case IR::MOV:
            {
                Register target = get_any_register_for/*_no_unspill*/(q.target.temp_id);
                Register left = get_any_register_for(q.left.temp_id);
                code.mov(target, left);
                break;
            }
            case IR::NEG:
            {
                Register target = get_any_register_for/*_no_unspill*/(q.target.temp_id);
                Register left = get_any_register_for(q.left.temp_id);
                code.mov(target, left);
                code.neg(target);
                break;
            }
            case IR::MUL: case IR::IMUL:
            case IR::DIV: case IR::IDIV:
            {
                Register rax = get_register_for_no_unspill(Reg_rax, q.target.temp_id);
                Register rdx = get_register(Reg_rdx);
                Register left = get_any_register_for(q.left.temp_id);
                Register right = get_any_register_for(q.right.temp_id);
                code.mov(rax, left);
                switch (q.op)
                {
                    case IR::MUL:
                        code.zero(rdx);
                        code.mul(right);
                        break;
                    case IR::IMUL:
                        code.sign_extend_rax_to_rdx();
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
                Register target = get_any_register_for/*_no_unspill*/(q.target.temp_id);
                Register left = get_any_register_for(q.left.temp_id);
                Register right = get_any_register_for(q.right.temp_id);
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
                Register target = get_any_register_for/*_no_unspill*/(q.target.temp_id);
                Register left = get_any_register_for(q.left.temp_id);
                Register right = get_any_register_for(q.right.temp_id);
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
            case IR::XOR_IM:
            {
                Register target = get_any_register_for/*_no_unspill*/(q.target.temp_id);
                Register left = get_any_register_for(q.left.temp_id);
                code.mov(target, left);
                code.xor_(target, q.right.int_value);
                break;
            }
            case IR::JMP:
            {
                code.jmp(q.target.label);
                break;
            }
            case IR::JZ:
            case IR::JNZ:
            {
                Register left = get_any_register_for(q.left.temp_id);
                code.cmp(left, 0);
                switch (q.op)
                {
                    case IR::JZ:  code.je(q.target.label);  break;
                    case IR::JNZ: code.jne(q.target.label); break;
                    InvalidDefaultCase;
                }
                break;
            }
            case IR::LABEL:
            {
                code.label(q.target.label);
                break;
            }
            case IR::RET:
            {
                if (q.target.returns_something)
                {
                    Register rax = regs.get_register_by_id(Reg_rax);
                    Register reg = get_any_register_for(q.left.temp_id);
                    if (rax.id != reg.id)
                        code.mov(rax, reg);
                }
                code.jmp_epi();
                break;
            }
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

        code.prolog(routine->name, 16*8);

        int param_count = routine->param_count;
        for (int i = 0; i < param_count; i++)
        {
            alloc_param(i);
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

    Routine *routine = ir.routines->next;
    while (routine)
    {
        code.global(routine->name);
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
