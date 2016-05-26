#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

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

Ast parse(const char *input, struct Alloc &a, struct ErrorContext &ec);

#endif // PARSER_H
