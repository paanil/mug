#ifndef LEXER_H
#define LEXER_H

#include <cstdint>

#define PASTE_TTS                   \
    PASTE_TT(END, "<end>")          \
    PASTE_TT(NOT, "!")              \
    PASTE_TT(STAR, "*")             \
    PASTE_TT(SLASH, "/")            \
    PASTE_TT(PLUS, "+")             \
    PASTE_TT(MINUS, "-")            \
    PASTE_TT(AMP, "&")              \
    PASTE_TT(PIPE, "|")             \
    PASTE_TT(EQ, "==")              \
    PASTE_TT(NE, "!=")              \
    PASTE_TT(LT, "<")               \
    PASTE_TT(GT, ">")               \
    PASTE_TT(LE, "<=")              \
    PASTE_TT(GE, ">=")              \
    PASTE_TT(AND, "&&")             \
    PASTE_TT(OR, "||")              \
    PASTE_TT(ASSIGN, "=")           \
    PASTE_TT(ARROW, "->")           \
    PASTE_TT(LPAREN, "(")           \
    PASTE_TT(RPAREN, ")")           \
    PASTE_TT(LBRACE, "{")           \
    PASTE_TT(RBRACE, "}")           \
    PASTE_TT(COMMA, ",")            \
    PASTE_TT(SEMICOLON, ";")        \
    PASTE_TT(IF, "if")              \
    PASTE_TT(ELSE, "else")          \
    PASTE_TT(WHILE, "while")        \
    PASTE_TT(FUNCTION, "function")  \
    PASTE_TT(RETURN, "return")      \
    PASTE_TT(INT, "int")            \
    PASTE_TT(INT8, "int8")          \
    PASTE_TT(INT16, "int16")        \
    PASTE_TT(INT32, "int32")        \
    PASTE_TT(INT64, "int64")        \
    PASTE_TT(UINT, "uint")          \
    PASTE_TT(UINT8, "uint8")        \
    PASTE_TT(UINT16, "uint16")      \
    PASTE_TT(UINT32, "uint32")      \
    PASTE_TT(UINT64, "uint64")      \
    PASTE_TT(BOOL, "bool")          \
    PASTE_TT(TRUE, "true")          \
    PASTE_TT(FALSE, "false")        \
    PASTE_TT(IDENT, "<ident>")      \
    PASTE_TT(CONST, "<const>")      \
    PASTE_TT(INVALID, "<invalid>")

struct Token
{
#define PASTE_TT(tt, ts) tt,

    enum Type
    {
        PASTE_TTS
    };

#undef PASTE_TT

    static const char *get_str(Type type);
    static bool is_type(Type type);

    Type type;
    int line;
    int column;

    union
    {
        uint64_t value;
        struct
        {
            char *text;
            int len;
        };
        char c;
    };
};

struct Lexer
{
    struct HashItem
    {
        const char *kw;
        Token::Type tt;
    };

    char *input;
    int line;
    int column;
    // NOTE: Prime numbers are propably good for hash tables.
    HashItem keyword_hash_table[67];

    Lexer() = default;
    Lexer(char *input_);

    char peek();
    char get();

    Token make_token(Token::Type type);
    Token ident_token(char *text, int len);
    Token const_token(uint64_t value);
    Token invalid_token(char c);

    Token next_token();
};

void run_lexer_tests();

#endif // LEXER_H
