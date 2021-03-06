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
statements := statement statements | <none>
type := 'int' | 'uint' | 'bool'
parameters := type ident (',' parameters | <none>) | <none>
function_def := 'function' ident '(' parameters ')' ('->' type | <none>) '{' statements '}'
extern_function := 'extern' 'function' ident '(' parameters ')' ('->' type | <none>) ';'
top_level := (statement | function_def | extern_function) top_level | <none>

expression := and ('||' expression | <none>)
and := comparison ('&&' and | <none>)
comparison := sum (('==' | '!=' | '<' | '>' | '<=' | '>=') comparison | <none>)
sum := term (('+' | '-') sum | <none>)
term := prefixed_factor (('*' |'/') term | <none>)
prefixed_factor := ('-' | '!' | <none>) factor
factor := ('true' | 'false') | const | ident | ident '(' arguments ')' | '(' expression ')'
arguments := expression (',' arguments | <none>) | <none>

*/

/**
 * Parses the input according to the grammar and returns valid or invalid AST.
 */
Ast parse(const char *input, struct Alloc &a, struct ErrorContext &ec);

#endif // PARSER_H
