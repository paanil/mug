#ifndef PARSER_H
#define PARSER_H

#include "ast_alloc.h"

/*

Grammar

statement := ';'
statement := expression ';'
statement := ident '=' expression ';'
statement := type ident ('=' expression | <none>) ';'
statement := return (expression | <none>) ';'
statement := 'if' '(' expression ')' statement ('else' statement | <none>)
statement := 'while' '(' expression ')' statement
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
    AstAlloc a;
    bool error;

    Parser(Alloc &a_)
    : a(a_)
    {}

    Token::Type look_ahead();
    Token::Type peek();
    Token::Type get();

    bool accept(Token::Type tt);
    bool expect(Token::Type tt);
    void print_error(const char *message, const char *info);
    void expected_operand_error(Token::Type for_op);
    void expected_error(const char *what);

    Ast parse(const char *input);

    Node *parse_top_level();

    Node *parse_statement();
    Node *parse_statements();
    bool parse_type(Type *result);
    bool parse_parameters(ParamList &params);

    Expression *parse_expression();
    Expression *parse_and();
    Expression *parse_comparison();
    Expression *parse_sum();
    Expression *parse_term();
    Expression *parse_prefixed_factor();
    Expression *parse_factor();
    bool parse_arguments(ArgList &args);
};

void run_parser_tests();

#endif // PARSER_H
