#ifndef IR_GEN_H
#define IR_GEN_H

#define PASTE_TYPES     \
    PASTE_TYPE(MOV_IM)  \
    PASTE_TYPE(MOV)     \
    PASTE_TYPE(MUL)     \
    PASTE_TYPE(IMUL)    \
    PASTE_TYPE(ADD)     \
    PASTE_TYPE(SUB)     \
    PASTE_TYPE(EQ)      \
    PASTE_TYPE(LT)      \
    PASTE_TYPE(JMP)     \
    PASTE_TYPE(JZ)      \
    PASTE_TYPE(CALL)    \
    PASTE_TYPE(RET)     \
    PASTE_TYPE(ARG)

struct IR
{
#define PASTE_TYPE(x) x,

    enum Type
    {
        PASTE_TYPES
    };

#undef PASTE_TYPE

    static const char *get_str(Type type);

    struct Routine *routines;
};

void print_ir(IR ir);

IR gen_ir(struct Ast &ast, struct Alloc &a);

#endif // IR_GEN_H
