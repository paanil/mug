#include "ir.h"
#include "check.h"
#include "parser.h"
#include "alloc.h"
#include "error_context.h"

#include <cstdio>
#include <cassert>

struct Register
{
    Str name; // We might want to put registers in a StrMap (?) => not just c-string.
    int32_t id; // Actually just an index.
    int32_t temp_id; // Temp currently in the register or -1 if none.
};

#define MAX_REGISTERS 20
#define MAX_PARAM_REGISTERS 10

struct RegisterAlloc
{
    int32_t register_count;
    int32_t return_register;
    int32_t param_register_count;
    int32_t param_registers[MAX_PARAM_REGISTERS];
    int32_t register_queue[MAX_REGISTERS]; // Used for least recently used allocation.
    Register registers[MAX_REGISTERS];

    RegisterAlloc()
    : register_count()
    , return_register()
    , param_register_count()
    {}

    void add_register(const char *name)
    {
        assert(register_count < MAX_REGISTERS);

        int32_t index = register_count++;

        registers[index].name = Str::make(name);
        registers[index].id = index;
        registers[index].temp_id = -1;
        register_queue[index] = index;
    }

    void add_return_register(const char *name)
    {
        return_register = register_count;
        add_register(name);
    }

    void add_param_register(const char *name)
    {
        assert(param_register_count < MAX_PARAM_REGISTERS);

        param_registers[param_register_count++] = register_count;
        add_register(name);
    }

    void move_back_in_queue(int32_t index)
    {
        int32_t reg_id = register_queue[index];
        for (int i = index + 1; i < register_count; i++)
            register_queue[i - 1] = register_queue[i];
        register_queue[register_count - 1] = reg_id;
    }

    Register alloc_register(int32_t temp_id)
    {
        int32_t reg_id = register_queue[0];

        move_back_in_queue(0);

        Register reg = registers[reg_id];
        registers[reg_id].temp_id = temp_id;
        return reg;
    }

    bool alloc_param_register(int32_t param_index, int32_t *reg)
    {
        if(param_index >= param_register_count)
            return false;

        int32_t reg_id = param_registers[param_index];

        for (int i = 0; i < register_count; i++)
        {
            if (register_queue[i] == reg_id)
            {
                move_back_in_queue(i);
                break;
            }
        }

        registers[reg_id].temp_id = param_index;
        *reg = reg_id;
        return true;
    }

    Register get_register(int32_t reg_id)
    {
        assert(reg_id < register_count);
        for (int i = 0; i < register_count; i++)
        {
            if (register_queue[i] == reg_id)
            {
                move_back_in_queue(i);
                break;
            }
        }
        return registers[reg_id];
    }

    Register get_return_register()
    {
        return registers[return_register];
    }
};

struct AsmGen
{
    struct Temp
    {
        int32_t reg_id; // Register that has the temp in it or -1 if none.
//        int32_t stack_offset;
//        bool spilled;
    };

    RegisterAlloc regs;
    Temp temps[100];

    AsmGen()
    {
        regs.add_param_register("rcx");
        regs.add_param_register("rdx");
        regs.add_param_register("r8");
        regs.add_param_register("r9");
        regs.add_return_register("rax");
        regs.add_register("r10");
        regs.add_register("r11");

        for (int i = 0; i < 100; i++)
        {
            temps[i].reg_id= -1;
        }
    }

    void alloc_param(int32_t param_index)
    {
        int32_t reg_id;
        if (regs.alloc_param_register(param_index, &reg_id))
        {
            temps[param_index].reg_id = reg_id;
        }
        else
        {
//            temps[param_index].stack_offset = 8 * param_index;
//            temps[param_index].spilled = true;
        }
    }

    Register get_register(int32_t temp_id = -1)
    {
        Register reg = regs.alloc_register(temp_id);
        if (reg.temp_id >= 0)
        {
            // spill
        }
        return reg;
    }

    Register get_register_for(int32_t temp_id)
    {
        Temp temp = temps[temp_id];
        if (temp.reg_id >= 0)
            return regs.get_register(temp.reg_id);

        Register reg = get_register(temp_id);
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
                Register reg = get_register_for(q.target.temp_id);
                printf("\tmov %s, %llu\n", reg.name.data, q.left.int_value);
                break;
            }
            case IR::ADD:
            {
                Register target = get_register_for(q.target.temp_id);
                Register left = get_register_for(q.left.temp_id);
                Register right = get_register_for(q.right.temp_id);
                printf("\tmov %s, %s\n", target.name.data, left.name.data);
                printf("\tadd %s, %s\n", target.name.data, right.name.data);
                break;
            }
            case IR::EQ: case IR::NE:
            case IR::LT: case IR::BELOW:
            case IR::GT: case IR::ABOVE:
            case IR::LE: case IR::BE:
            case IR::GE: case IR::AE:
            {
                Register target = get_register_for(q.target.temp_id);
                Register left = get_register_for(q.left.temp_id);
                Register right = get_register_for(q.right.temp_id);
                Register temp = get_register();
                printf("\txor %s, %s\n", target.name.data, target.name.data);
                printf("\tmov %s, 1\n", temp.name.data);
                printf("\tcmp %s, %s\n", left.name.data, right.name.data);
                switch (q.op)
                {
                    case IR::EQ:
                        printf("\tcmove %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::NE:
                        printf("\tcmovne %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::LT:
                        printf("\tcmovl %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::BELOW:
                        printf("\tcmovb %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::GT:
                        printf("\tcmovg %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::ABOVE:
                        printf("\tcmova %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::LE:
                        printf("\tcmovle %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::BE:
                        printf("\tcmovbe %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::GE:
                        printf("\tcmovge %s, %s\n", target.name.data, temp.name.data);
                        break;
                    case IR::AE:
                        printf("\tcmovae %s, %s\n", target.name.data, temp.name.data);
                        break;
                    default:
                        assert(0 && "invalid default case!");
                        break;
                }
                break;
            }
            case IR::JZ:
            {
                Register left = get_register_for(q.left.temp_id);
                printf("\tcmp %s, 0\n", left.name.data);
                printf("\tje .l%u\n", q.target.label);
                break;
            }
            case IR::LABEL:
            {
                printf(".l%u:\n", q.target.label);
                break;
            }
            case IR::RET:
            {
                if (q.target.returns_something)
                {
                    Register ret = regs.get_return_register();
                    Register reg = get_register_for(q.left.temp_id);
                    if (ret.id != reg.id)
                        printf("\tmov %s, %s\n", ret.name.data, reg.name.data);
                }
                printf("\tjmp .epi\n");
                break;
            }
        }
    }

    void gen_asm(Routine &r)
    {
        printf("%s:\n", r.name.data);
        printf("\tpush rbp\n");
        printf("\tmov rbp, rsp\n");
        printf("\n");

        for (uint32_t i = 0; i < r.param_count; i++)
        {
            alloc_param(i);
        }

        for (uint32_t i = 0; i < r.n; i++)
        {
            gen_asm(r[i]);
        }

        printf(".epi:\n");
        printf("\tmov rsp, rbp\n");
        printf("\tpop rbp\n");
        printf("\tret\n\n");
    }
};

void gen_asm(IR ir)
{
    if (ir.routines == nullptr || ir.routines->next == nullptr)
    {
        printf("ir is null!\n");
        return;
    }

//    print_ir(ir);
//    printf("\n\n");

    Routine *r = ir.routines->next;
    while (r)
    {
        printf("\tglobal %s\n", r->name.data);
        r = r->next;
    }

    printf("\tsection .text\n");

    r = ir.routines->next;
    while (r)
    {
        AsmGen gen;
        gen.gen_asm(*r);
        r = r->next;
    }
}

int compile(const char *file)
{
    Alloc a;
    char *buf;

    {
        FILE *f = fopen(file, "rb");
        if (f == nullptr)
        {
            fprintf(stderr, "error: couldn't open file '%s'", file);
            return 1;
        }

        fseek(f, 0, SEEK_END);
        unsigned size = ftell(f);
        fseek(f, 0, SEEK_SET);

        buf = a.allocate_array<char>(size + 1);
        buf[size] = 0;

        if (fread(buf, 1, size, f) != size)
        {
            fprintf(stderr, "error: couldn't read file '%s'", file);
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
    gen_asm(ir);
    return 0;
}
