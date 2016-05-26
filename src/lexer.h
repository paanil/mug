#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "str_map.h"

struct Lexer
{
    const char *input;
    int line;
    int column;
    StrMap<Token::Type> keyword_map;

    Lexer();

    void reset(const char *input_)
    {
        input = input_;
        line = 1;
        column = 0;
    }

    char peek();
    char get();

    Token make_token(Token::Type type);
    Token ident_token(Str s);
    Token intliteral_token(uint64_t value);
    Token uintliteral_token(uint64_t value);
    Token invalid_token(char c);

    Token next_token();
};

#endif // LEXER_H
