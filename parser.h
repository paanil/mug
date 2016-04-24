#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

/*

Grammar

statement := ';'
statement := expression ';'
statement := ident '=' expression ';'
statement := type ident ('=' expression | <none>) ';'
statement := return (expression | <none>) ';'
statement := 'if' expression statement ('else' statement | <none>)
statement := 'while' expression statement
statement := '{' statements '}'
statement := 'function' ident '(' parameters ')' ('->' | <none>) '{' statements '}'
type := 'int' | 'uint' | 'bool'
statements := statement statements | <none>
parameters := type ident (',' parameters | <none>) | <none>

expression := and ('||' expression | <none>)
and := comparison ('&&' and | <none>)
comparison := sum (('==' | '!=' | '<' | '>' | '<=' | '>=') comparison | <none>)
sum := term (('+' | '-') sum | <none>)
term := prefixed_factor (('*' |'/') term | <none>)
prefixed_factor := ('-' | '!' | <none>) factor
factor := ('true' | 'false') | const | ident | ident '(' arguments ')' | '(' expression ')'
arguments := expression (',' arguments | <none>) | <none>

*/

struct Parser
{
    Lexer lexer;
    Token token;
    Token next_token;

    Token::Type look_ahead();
    Token::Type peek();
    Token::Type get();

    bool accept(Token::Type tt);
    bool expect(Token::Type tt);

    bool expected_operand_error(Token::Type for_op);
    bool expected_error(const char *what);

    bool parse(char *input);

    bool parse_top_level();

    bool parse_statement();
    bool parse_type();
    bool parse_parameters();

    bool parse_expression();
    bool parse_and();
    bool parse_comparison();
    bool parse_sum();
    bool parse_term();
    bool parse_prefixed_factor();
    bool parse_factor();
    bool parse_arguments();
};

void run_parser_tests();

#endif // PARSER_H
