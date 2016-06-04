#include "ir.h"
#include "check.h"
#include "parser.h"
#include "alloc.h"
#include "error_context.h"
#include "assert.h"

#include <cstdio>

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

    RegisterAlloc()
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

    void prolog(Str func_name)
    {

        fprintf(f, "%s:\n", func_name.data);
        fprintf(f, "\t" "push rbp\n");
        fprintf(f, "\t" "mov rbp, rsp\n");
        fprintf(f, "\n");
    }

    void epilog()
    {
        fprintf(f, ".epi:\n");
        fprintf(f, "\t" "mov rsp, rbp\n");
        fprintf(f, "\t" "pop rbp\n");
        fprintf(f, "\t" "ret\n\n");
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

    void je(uint32_t label)
    {
        fprintf(f, "\t" "je .l%u\n", label);
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

struct AsmGen
{
    struct Temp
    {
        RegID reg_id; // Register that has the temp in it or none.
//        int32_t stack_offset;
//        bool spilled;
    };

    RegisterAlloc regs;
    Temp temps[100];
    Code &code;

    AsmGen(Code &code_)
    : code(code_)
    {
        for (int i = 0; i < 100; i++)
        {
            temps[i].reg_id = Reg_NONE;
        }
    }

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

    void spill(int32_t temp_id)
    {
        if (temp_id < 0)
            return;

//        temps[temp_id].reg_id = Reg_NONE;
        // TODO: spill

        // TODO: Remove this hack!
        Register src = regs.get_register_by_id(temps[temp_id].reg_id);
        Register reg = regs.alloc_any_register(temp_id);
        code.mov(reg, src); // spill
        temps[temp_id].reg_id = reg.id;
    }

    Register get_register(RegID reg_id)
    {
        Register reg = regs.alloc_register(reg_id, -1);
        spill(reg.temp_id);
        return reg;
    }

    Register get_register_for_no_unspill(RegID reg_id, int32_t temp_id)
    {
        Register reg = regs.alloc_register(reg_id, temp_id);
        spill(reg.temp_id);
        temps[temp_id].reg_id = reg.id;
        return reg;
    }

    Register get_any_register()
    {
        Register reg = regs.alloc_any_register(-1);
        spill(reg.temp_id);
        return reg;
    }

    Register get_any_register_for(int32_t temp_id)
    {
        Temp temp = temps[temp_id];
        if (temp.reg_id >= 0)
            return regs.get_register_by_id(temp.reg_id);

        Register reg = regs.alloc_any_register(temp_id);
        spill(reg.temp_id);
//        if (temp.spilled)
//        {
//            // load
//        }
        temps[temp_id].reg_id = reg.id;
        return reg;
    }

    void gen_asm(Quad q)
    {
        switch (q.op)
        {
            case IR::MOV_IM:
            {
                Register reg = get_any_register_for(q.target.temp_id);
                code.mov(reg, q.left.int_value);
                break;
            }
            //case IR::MOV:
            case IR::NEG:
            {
                Register target = get_any_register_for(q.target.temp_id);
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
                Register target = get_any_register_for(q.target.temp_id);
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
                Register target = get_any_register_for(q.target.temp_id);
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
                Register target = get_any_register_for(q.target.temp_id);
                Register left = get_any_register_for(q.left.temp_id);
                code.mov(target, left);
                code.xor_(target, q.right.int_value);
                break;
            }
            //case IR::JMP:
            case IR::JZ:
            //case IR::JNZ:
            {
                Register left = get_any_register_for(q.left.temp_id);
                code.cmp(left, 0);
                switch (q.op)
                {
                    case IR::JZ:
                        code.je(q.target.label);
                        break;
                    //case IR::JNZ:
                    //    fprintf(f, "\tjne .l%u\n", q.target.label);
                    //    break;
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

    void gen_asm(Routine &r)
    {
        code.prolog(r.name);

        int param_count = r.param_count;
        for (int i = 0; i < param_count; i++)
        {
            alloc_param(i);
        }

        for (uint32_t i = 0; i < r.n; i++)
        {
            gen_asm(r[i]);
        }

        code.epilog();
    }
};

void gen_asm(IR ir, FILE *f)
{
    if (ir.routines == nullptr || ir.routines->next == nullptr)
    {
        fprintf(stderr, "ir is null!\n");
        return;
    }

    print_ir(ir);
    fprintf(stdout, "\n\n");

    Code code(f);

    Routine *r = ir.routines->next;
    while (r)
    {
        code.global(r->name);
        r = r->next;
    }

    code.section_text();

    r = ir.routines->next;
    while (r)
    {
        AsmGen gen(code);
        gen.gen_asm(*r);
        r = r->next;
    }
}

int compile(const char *source_file, const char *output_file)
{
    Alloc a;
    char *buf;

    {
        FILE *f = fopen(source_file, "rb");
        if (f == nullptr)
        {
            fprintf(stderr, "error: couldn't open file '%s'\n", source_file);
            return 1;
        }

        fseek(f, 0, SEEK_END);
        unsigned size = ftell(f);
        fseek(f, 0, SEEK_SET);

        buf = a.allocate_array<char>(size + 1);
        buf[size] = 0;

        if (fread(buf, 1, size, f) != size)
        {
            fprintf(stderr, "error: couldn't read file '%s'\n", source_file);
            fclose(f);
            return 1;
        }

        fclose(f);
    }

    ErrorContext ec;
    Ast ast = parse(buf, a, ec);
    if (!check(ast, ec))
    {
        return 0;
    }

    IR ir = gen_ir(ast, a);

    if (output_file)
    {
        FILE *f = fopen(output_file, "w");
        if (f == nullptr)
        {
            fprintf(stderr, "error: couldn't open file '%s' for writing\n", output_file);
            return 1;
        }

        gen_asm(ir, f);

        fclose(f);
    }
    else
    {
        gen_asm(ir, stdout);
    }

    return 0;
}
