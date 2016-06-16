#ifndef AST_ALLOC_H
#define AST_ALLOC_H

#include "ast.h"
#include "token.h"

/**
 * Allocates AST nodes and expressions.
 * Uses the given instance of Alloc for the allocations.
 */
struct AstAlloc
{
    struct Alloc &a;

    AstAlloc(Alloc &a_)
    : a(a_)
    {}

    Str push_str(Str s);

    ArgList *alloc_arg();
    ParamList *alloc_param();
    StmtList *alloc_stmt();

    Expression *bool_exp(bool value);
    Expression *const_exp(uint64_t value, Type::Enum type);
    Expression *var_exp(Str ident);
    Expression *call_exp(Str ident, ArgList *args);
    Expression *unary_exp(Expression *operand, Token::Type op);
    Expression *binary_exp(Expression *left, Expression *right, Token::Type op);

    Node *empty_node();
    Node *exp_node(Expression *exp);
    Node *assign_node(Str ident, Expression *value);
    Node *decl_node(Type type, Str ident, Expression *init);
    Node *return_node(Expression *ret_val);
    Node *if_node(Expression *condition, Node *true_stmt, Node *else_stmt);
    Node *while_node(Expression *condition, Node *stmt);
    Node *block_node(StmtList *stmts);
    Node *func_def_node(Type ret_type, Str ident, ParamList *params, Node *body);
};

#endif // AST_ALLOC_H
