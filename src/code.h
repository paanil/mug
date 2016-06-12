#ifndef CODE_H
#define CODE_H

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

#endif // CODE_H
