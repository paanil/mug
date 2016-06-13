#ifndef CODE_H
#define CODE_H

#include "list.h"

#define PASTE_INSTRS        \
    PASTE_INSTR(STORE)      \
    PASTE_INSTR(LOAD)       \
    PASTE_INSTR(MOV_IM)     \
    PASTE_INSTR(MOV)        \
    PASTE_INSTR(XOR_IM)     \
    PASTE_INSTR(XOR)        \
    PASTE_INSTR(NEG)        \
    PASTE_INSTR(CQO)        \
    PASTE_INSTR(MUL)        \
    PASTE_INSTR(IMUL)       \
    PASTE_INSTR(DIV)        \
    PASTE_INSTR(IDIV)       \
    PASTE_INSTR(ADD)        \
    PASTE_INSTR(SUB)        \
    PASTE_INSTR(CMP_IM)     \
    PASTE_INSTR(CMP)        \
    PASTE_INSTR(CMOVE)      \
    PASTE_INSTR(CMOVNE)     \
    PASTE_INSTR(CMOVL)      \
    PASTE_INSTR(CMOVB)      \
    PASTE_INSTR(CMOVG)      \
    PASTE_INSTR(CMOVA)      \
    PASTE_INSTR(CMOVLE)     \
    PASTE_INSTR(CMOVBE)     \
    PASTE_INSTR(CMOVGE)     \
    PASTE_INSTR(CMOVAE)     \
    PASTE_INSTR(LABEL)      \
    PASTE_INSTR(JMP_EPI)    \
    PASTE_INSTR(JMP)        \
    PASTE_INSTR(JE)         \
    PASTE_INSTR(JNE)        \
    PASTE_INSTR(PUSH_ARG)   \
    PASTE_INSTR(CALL)


struct Instr
{
#define PASTE_INSTR(i) i,

    enum Type
    {
        PASTE_INSTRS
    };

#undef PASTE_INSTR

    union Operand
    {
        RegID reg_id;
        int32_t offset;
        uint32_t label;
        uint32_t routine;
        uint64_t value;
    };

    Type type;
    Operand oper1;
    Operand oper2;
};

struct Code
{
    List<Str> routines;
    List<Instr> instructions;

    FILE *f;

    Code(FILE *out)
    : f(out)
    {}

    void routine(Str name)
    {
        routines.push(name);
    }

#define CASE_REG(Type, name) \
    case Instr::Type: \
        fprintf(f, "\t" #name " %s\n", \
                Register::get_str(instr.oper1.reg_id)); \
        break
#define CASE_REG_REG(Type, name) \
    case Instr::Type: \
        fprintf(f, "\t" #name " %s, %s\n", \
                Register::get_str(instr.oper1.reg_id), \
                Register::get_str(instr.oper2.reg_id)); \
        break
#define CASE_REG_VAL(Type, name) \
    case Instr::Type: \
        fprintf(f, "\t" #name " %s, %llu\n", \
                Register::get_str(instr.oper1.reg_id), \
                instr.oper2.value); \
        break

    void write_routine(Str routine_name, uint32_t stack_bytes)
    {
        fprintf(f, "%s:\n", routine_name.data);
        fprintf(f, "\t" "push rbp\n");
        fprintf(f, "\t" "mov rbp, rsp\n");
        fprintf(f, "\t" "sub rsp, %u\n", stack_bytes);
        fprintf(f, "\n");

        int instr_count = instructions.get_size();
        for (int i = 0; i < instr_count; i++)
        {
            Instr instr = instructions[i];
            switch (instr.type)
            {
                case Instr::STORE:
                    fprintf(f, "\t" "mov [rbp%+d], %s\n",
                            instr.oper1.offset,
                            Register::get_str(instr.oper2.reg_id));
                    break;
                case Instr::LOAD:
                    fprintf(f, "\t" "mov %s, [rbp%+d]\n",
                            Register::get_str(instr.oper1.reg_id),
                            instr.oper2.offset);
                    break;
                CASE_REG_VAL(MOV_IM, mov);
                CASE_REG_REG(MOV, mov);
                CASE_REG_VAL(XOR_IM, xor);
                CASE_REG_REG(XOR, xor);
                CASE_REG(NEG, neg);
                case Instr::CQO:
                    fprintf(f, "\t" "cqo\n");
                    break;
                CASE_REG(MUL, mul);
                CASE_REG(IMUL, imul);
                CASE_REG(DIV, div);
                CASE_REG(IDIV, idiv);
                CASE_REG_REG(ADD, add);
                CASE_REG_REG(SUB, sub);
                CASE_REG_VAL(CMP_IM, cmp);
                CASE_REG_REG(CMP, cmp);
                CASE_REG_REG(CMOVE, cmove);
                CASE_REG_REG(CMOVNE, cmovne);
                CASE_REG_REG(CMOVL, cmovl);
                CASE_REG_REG(CMOVB, cmovb);
                CASE_REG_REG(CMOVG, cmovg);
                CASE_REG_REG(CMOVA, cmova);
                CASE_REG_REG(CMOVLE, cmovle);
                CASE_REG_REG(CMOVBE, cmovbe);
                CASE_REG_REG(CMOVGE, cmovge);
                CASE_REG_REG(CMOVAE, cmovae);
                case Instr::LABEL:
                    fprintf(f, ".l%u:\n", instr.oper1.label);
                    break;
                case Instr::JMP_EPI:
                    fprintf(f, "\t" "jmp .epi\n");
                    break;
                case Instr::JMP:
                    fprintf(f, "\t" "jmp .l%u\n", instr.oper1.label);
                    break;
                case Instr::JE:
                    fprintf(f, "\t" "je .l%u\n", instr.oper1.label);
                    break;
                case Instr::JNE:
                    fprintf(f, "\t" "jne .l%u\n", instr.oper1.label);
                    break;
                case Instr::PUSH_ARG:
                    fprintf(f, "\t" "mov [rsp%+d], %s\n",
                            instr.oper1.offset,
                            Register::get_str(instr.oper2.reg_id));
                    break;
                case Instr::CALL:
                    fprintf(f, "\t" "call %s\n", routines[instr.oper1.routine].data);
                    break;
            }
        }

        fprintf(f, ".epi:\n");
        fprintf(f, "\t" "mov rsp, rbp\n");
        fprintf(f, "\t" "pop rbp\n");
        fprintf(f, "\t" "ret\n\n");

        instructions.resize(0);
    }

#define ADD0(Type) \
    Instr i; \
    i.type = Instr::Type; \
    instructions.push(i)
#define ADD1(Type, e1) \
    Instr i; \
    i.type = Instr::Type; \
    i.oper1.e1; \
    instructions.push(i)
#define ADD2(Type, e1, e2) \
    Instr i; \
    i.type = Instr::Type; \
    i.oper1.e1; \
    i.oper2.e2; \
    instructions.push(i)

    void store(int32_t dest, Register source)
    {
        ADD2(STORE, offset = dest, reg_id = source.id);
    }

    void load(Register dest, int32_t source)
    {
        ADD2(LOAD, reg_id = dest.id, offset = source);
    }

    void mov(Register dest, uint64_t value)
    {
        ADD2(MOV_IM, reg_id = dest.id, value = value);
    }

    void xor_(Register reg, uint64_t value)
    {
        ADD2(XOR_IM, reg_id = reg.id, value = value);
    }

    void cmp(Register reg, uint64_t value)
    {
        ADD2(CMP_IM, reg_id = reg.id, value = value);
    }

    void zero(Register reg)
    {
        ADD2(XOR, reg_id = reg.id, reg_id = reg.id);
    }

    void sign_extend_rax_to_rdx()
    {
        ADD0(CQO);
    }

    void label(uint32_t label)
    {
        ADD1(LABEL, label = label);
    }

    void jmp_epi()
    {
        ADD0(JMP_EPI);
    }

    void jmp(uint32_t label)
    {
        ADD1(JMP, label = label);
    }

    void je(uint32_t label)
    {
        ADD1(JE, label = label);
    }

    void jne(uint32_t label)
    {
        ADD1(JNE, label = label);
    }

    void push_arg(int32_t stack_offset, Register reg)
    {
        ADD2(PUSH_ARG, offset = stack_offset, reg_id = reg.id);
    }

    void call(uint32_t routine_id)
    {
        ADD1(CALL, routine = routine_id);
    }

#define INSTRUCTION(instr_name, Type) \
    void instr_name(Register r) { \
        ADD1(Type, reg_id = r.id); \
    }

    INSTRUCTION(neg, NEG)
    INSTRUCTION(mul, MUL)
    INSTRUCTION(imul, IMUL)
    INSTRUCTION(div, DIV)
    INSTRUCTION(idiv, IDIV)

#define INSTRUCTION2(instr_name, Type) \
    void instr_name(Register a, Register b) { \
        ADD2(Type, reg_id = a.id, reg_id = b.id); \
    }

    INSTRUCTION2(mov, MOV)
    INSTRUCTION2(add, ADD)
    INSTRUCTION2(sub, SUB)
    INSTRUCTION2(cmp, CMP)
    INSTRUCTION2(cmove, CMOVE)
    INSTRUCTION2(cmovne, CMOVNE)
    INSTRUCTION2(cmovl, CMOVL)
    INSTRUCTION2(cmovb, CMOVB)
    INSTRUCTION2(cmovg, CMOVG)
    INSTRUCTION2(cmova, CMOVA)
    INSTRUCTION2(cmovle, CMOVLE)
    INSTRUCTION2(cmovbe, CMOVBE)
    INSTRUCTION2(cmovge, CMOVGE)
    INSTRUCTION2(cmovae, CMOVAE)
};

#endif // CODE_H
