#ifndef TOKEN_H
#define TOKEN_H

#include "str.h"

#define PASTE_TOKENS                       \
    PASTE_TOKEN(END, "<end>")              \
    PASTE_TOKEN(NOT, "!")                  \
    PASTE_TOKEN(STAR, "*")                 \
    PASTE_TOKEN(SLASH, "/")                \
    PASTE_TOKEN(PLUS, "+")                 \
    PASTE_TOKEN(MINUS, "-")                \
    PASTE_TOKEN(AMP, "&")                  \
    PASTE_TOKEN(PIPE, "|")                 \
    PASTE_TOKEN(EQ, "==")                  \
    PASTE_TOKEN(NE, "!=")                  \
    PASTE_TOKEN(LT, "<")                   \
    PASTE_TOKEN(GT, ">")                   \
    PASTE_TOKEN(LE, "<=")                  \
    PASTE_TOKEN(GE, ">=")                  \
    PASTE_TOKEN(AND, "&&")                 \
    PASTE_TOKEN(OR, "||")                  \
    PASTE_TOKEN(ASSIGN, "=")               \
    PASTE_TOKEN(ARROW, "->")               \
    PASTE_TOKEN(LPAREN, "(")               \
    PASTE_TOKEN(RPAREN, ")")               \
    PASTE_TOKEN(LBRACE, "{")               \
    PASTE_TOKEN(RBRACE, "}")               \
    PASTE_TOKEN(COMMA, ",")                \
    PASTE_TOKEN(SEMICOLON, ";")            \
    PASTE_TOKEN(IF, "if")                  \
    PASTE_TOKEN(ELSE, "else")              \
    PASTE_TOKEN(WHILE, "while")            \
    PASTE_TOKEN(FUNCTION, "function")      \
    PASTE_TOKEN(RETURN, "return")          \
    PASTE_TOKEN(INT, "int")                \
    PASTE_TOKEN(INT8, "int8")              \
    PASTE_TOKEN(INT16, "int16")            \
    PASTE_TOKEN(INT32, "int32")            \
    PASTE_TOKEN(INT64, "int64")            \
    PASTE_TOKEN(UINT, "uint")              \
    PASTE_TOKEN(UINT8, "uint8")            \
    PASTE_TOKEN(UINT16, "uint16")          \
    PASTE_TOKEN(UINT32, "uint32")          \
    PASTE_TOKEN(UINT64, "uint64")          \
    PASTE_TOKEN(BOOL, "bool")              \
    PASTE_TOKEN(TRUE, "true")              \
    PASTE_TOKEN(FALSE, "false")            \
    PASTE_TOKEN(IDENT, "<ident>")          \
    PASTE_TOKEN(INT_LIT, "<int literal>")  \
    PASTE_TOKEN(UINT_LIT, "<uint literal>")\
    PASTE_TOKEN(INVALID, "<invalid>")

struct Token
{
#define PASTE_TOKEN(tt, ts) tt,

    enum Type
    {
        PASTE_TOKENS
    };

#undef PASTE_TOKEN

    static const char *get_str(Type type);

    Type type;
    int line;
    int column;

    union
    {
        uint64_t value;
        Str text;
        char c;
    };
};

#endif // TOKEN_H
