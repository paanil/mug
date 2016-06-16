#ifndef IR_GEN_H
#define IR_GEN_H

#define PASTE_TYPES     \
    PASTE_TYPE(MOV_IM)  \
    PASTE_TYPE(MOV)     \
    PASTE_TYPE(NOT)     \
    PASTE_TYPE(NEG)     \
    PASTE_TYPE(MUL)     \
    PASTE_TYPE(IMUL)    \
    PASTE_TYPE(DIV)     \
    PASTE_TYPE(IDIV)    \
    PASTE_TYPE(ADD)     \
    PASTE_TYPE(SUB)     \
    PASTE_TYPE(EQ)      \
    PASTE_TYPE(NE)      \
    PASTE_TYPE(LT)      \
    PASTE_TYPE(BELOW)   \
    PASTE_TYPE(GT)      \
    PASTE_TYPE(ABOVE)   \
    PASTE_TYPE(LE)      \
    PASTE_TYPE(BE)      \
    PASTE_TYPE(GE)      \
    PASTE_TYPE(AE)      \
    PASTE_TYPE(JMP)     \
    PASTE_TYPE(JZ)      \
    PASTE_TYPE(JNZ)     \
    PASTE_TYPE(LABEL)   \
    PASTE_TYPE(CALL)    \
    PASTE_TYPE(RET)     \
    PASTE_TYPE(ARG)

/**
 * IR has a linked list of routines.
 */
struct IR
{
#define PASTE_TYPE(x) x,

    enum Type
    {
        PASTE_TYPES
    };

#undef PASTE_TYPE

    static const char *get_str(Type type);

    struct Routine *routines; // linked list of routines. first one is the top level
};

void print_ir(IR ir);

/**
 * Generates intermediate code from the given AST.
 */
IR gen_ir(struct Ast &ast, struct Alloc &a);

#endif // IR_GEN_H
