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
    FILE *f;

    AsmGen()
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

        temps[temp_id].reg_id = Reg_NONE;
        // TODO: spill

//        Register src = regs.get_register_by_id(temps[temp_id].reg_id);
//        Register reg = regs.alloc_any_register(temp_id);
//        fprintf(f, "\tmov %s, %s \t; spill\n", reg.name.data, src.name.data);
//        temps[temp_id].reg_id = reg.id;
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
                fprintf(f, "\tmov %s, %llu\n", reg.name.data, q.left.int_value);
                break;
            }
            //case IR::MOV:
            case IR::MUL:
            {
                Register rax = get_register_for_no_unspill(Reg_rax, q.target.temp_id);
//                /*Register rdx = */get_register(Reg_rdx);
                Register left = get_any_register_for(q.left.temp_id);
                Register right = get_any_register_for(q.right.temp_id);
                fprintf(f, "\tmov %s, %s\n", rax.name.data, left.name.data);
                fprintf(f, "\tmul %s\n", right.name.data);
                break;
            }
            case IR::IMUL:
            case IR::ADD:
             case IR::SUB:
            {
                Register target = get_any_register_for(q.target.temp_id);
                Register left = get_any_register_for(q.left.temp_id);
                Register right = get_any_register_for(q.right.temp_id);
                fprintf(f, "\tmov %s, %s\n", target.name.data, left.name.data);
                switch (q.op)
                {
                    case IR::IMUL:
                        fprintf(f, "\timul %s, %s\n", target.name.data, right.name.data);
                        break;
                    case IR::ADD:
                        fprintf(f, "\tadd %s, %s\n", target.name.data, right.name.data);
                        break;
                    case IR::SUB:
                        fprintf(f, "\tsub %s, %s\n", target.name.data, right.name.data);
                        break;
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
                fprintf(f, "\txor %s, %s\n", target.name.data, target.name.data);
                fprintf(f, "\tmov %s, 1\n", temp.name.data);
                fprintf(f, "\tcmp %s, %s\n", left.name.data, right.name.data);
                switch (q.op)
                {
                    case IR::EQ:
                        fprintf(f, "\tcmove %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::NE:
                        fprintf(f, "\tcmovne %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::LT:
                        fprintf(f, "\tcmovl %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::BELOW:
                        fprintf(f, "\tcmovb %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::GT:
                        fprintf(f, "\tcmovg %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::ABOVE:
                        fprintf(f, "\tcmova %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::LE:
                        fprintf(f, "\tcmovle %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::BE:
                        fprintf(f, "\tcmovbe %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::GE:
                        fprintf(f, "\tcmovge %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::AE:
                        fprintf(f, "\tcmovae %s, %s\n", target.name.data, temp.name.data);
                        break;
                    InvalidDefaultCase;
                }
                break;
            }
            case IR::XOR_IM:
            {
                Register target = get_any_register_for(q.target.temp_id);
                Register left = get_any_register_for(q.left.temp_id);
                fprintf(f, "\tmov %s, %s\n", target.name.data, left.name.data);
                fprintf(f, "\txor %s, %llu\n", target.name.data, q.right.int_value);
                break;
            }
            //case IR::JMP:
            case IR::JZ:
            //case IR::JNZ:
            {
                Register left = get_any_register_for(q.left.temp_id);
                fprintf(f, "\tcmp %s, 0\n", left.name.data);
                switch (q.op)
                {
                    case IR::JZ:
                        fprintf(f, "\tje .l%u\n", q.target.label);
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
                fprintf(f, ".l%u:\n", q.target.label);
                break;
            }
            case IR::RET:
            {
                if (q.target.returns_something)
                {
                    Register rax = regs.get_register_by_id(Reg_rax);
                    Register reg = get_any_register_for(q.left.temp_id);
                    if (rax.id != reg.id)
                        fprintf(f, "\tmov %s, %s\n", rax.name.data, reg.name.data);
                }
                fprintf(f, "\tjmp .epi\n");
                break;
            }
        }
    }

    void gen_asm(Routine &r, FILE *out)
    {
        f = out;

        fprintf(f, "%s:\n", r.name.data);
        fprintf(f, "\tpush rbp\n");
        fprintf(f, "\tmov rbp, rsp\n");
        fprintf(f, "\n");

        int param_count = r.param_count;
        for (int i = 0; i < param_count; i++)
        {
            alloc_param(i);
        }

        for (uint32_t i = 0; i < r.n; i++)
        {
            gen_asm(r[i]);
        }

        fprintf(f, ".epi:\n");
        fprintf(f, "\tmov rsp, rbp\n");
        fprintf(f, "\tpop rbp\n");
        fprintf(f, "\tret\n\n");
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

    Routine *r = ir.routines->next;
    while (r)
    {
        fprintf(f, "\tglobal %s\n", r->name.data);
        r = r->next;
    }

    fprintf(f, "\tsection .text\n");

    r = ir.routines->next;
    while (r)
    {
        AsmGen gen;
        gen.gen_asm(*r, f);
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
