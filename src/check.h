#ifndef CHECK_H
#define CHECK_H

#include "ast.h"

struct ErrorContext;

bool check(Ast &ast, ErrorContext &ec);

#endif // CHECK_H
