#ifndef CHECK_H
#define CHECK_H

/**
 * Type checks the given AST.
 * Returns true, if the AST is OK.
 */
bool check(struct Ast &ast, struct ErrorContext &ec);

#endif // CHECK_H
