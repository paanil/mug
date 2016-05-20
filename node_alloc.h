#ifndef NODE_ALLOC_H
#define NODE_ALLOC_H

#include "alloc.h"
#include "ast.h"

struct AstAlloc
{
    Alloc &a;

    AstAlloc(Alloc &a_)
    : a(a_)
    {}

    Str push_str(Str s)
    {
        char *data = a.allocate_array<char>(s.len + 1);
        memcpy(data, s.data, s.len);
        data[s.len] = 0;
        return Str::make(data, s.len);
    }

    ArgList *alloc_arg()
    {
        return a.allocate<ArgList>();
    }

    ParamList *alloc_param()
    {
        return a.allocate<ParamList>();
    }

    StmtList *alloc_stmt()
    {
        return a.allocate<StmtList>();
    }

#define EXPRESSION(T, Type) \
    T *exp = a.allocate<T>(); \
    exp->type = ExpType_##Type

    Expression *bool_exp(bool value)
    {
        EXPRESSION(BoolExp, BOOL);
        exp->value = value;
        exp->data_type.type = Type::BOOL;
        return (Expression *)exp;
    }

    Expression *const_exp(uint64_t value)
    {
        EXPRESSION(ConstExp, CONST);
        exp->value = value;
        exp->data_type.type = Type::UINT;
        return (Expression *)exp;
    }

    Expression *var_exp(Str ident)
    {
        EXPRESSION(VarExp, VAR);
        exp->name = push_str(ident);
        return (Expression *)exp;
    }

    Expression *call_exp(Str ident, ArgList *args)
    {
        EXPRESSION(CallExp, CALL);
        exp->func_name = push_str(ident);
        exp->args = args;
        return (Expression *)exp;
    }

    Expression *unary_exp(Expression *operand, Token::Type op)
    {
        EXPRESSION(UnaryExp, UNARY);
        exp->operand = operand;
        exp->op = op;
        return (Expression *)exp;
    }

    Expression *binary_exp(Expression *left, Expression *right, Token::Type op)
    {
        EXPRESSION(BinaryExp, BINARY);
        exp->left = left;
        exp->right = right;
        exp->op = op;
        return (Expression *)exp;
    }

#define NODE(T, Type) \
    T *node = a.allocate<T>(); \
    node->type = NodeType_##Type

    Node *empty_node()
    {
        NODE(Node, EMPTY);
        return node;
    }

    Node *exp_node(Expression *exp)
    {
        NODE(ExpNode, EXP);
        node->exp = exp;
        return (Node *)node;
    }

    Node *assign_node(Str ident, Expression *value)
    {
        NODE(AssignNode, ASSIGN);
        node->var_name = push_str(ident);
        node->value = value;
        return (Node *)node;
    }

    Node *decl_node(Type type, Str ident, Expression *init)
    {
        NODE(DeclNode, DECL);
        node->var_type = type;
        node->var_name = push_str(ident);
        node->init = init;
        return (Node *)node;
    }

    Node *return_node(Expression *ret_val)
    {
        NODE(ReturnNode, RETURN);
        node->value = ret_val;
        return (Node *)node;
    }

    Node *if_node(Expression *condition, Node *true_stmt, Node *else_stmt)
    {
        NODE(IfNode, IF);
        node->condition = condition;
        node->true_stmt = true_stmt;
        node->else_stmt = else_stmt;
        return (Node *)node;
    }

    Node *while_node(Expression *condition, Node *stmt)
    {
        NODE(WhileNode, WHILE);
        node->condition = condition;
        node->stmt = stmt;
        return (Node *)node;
    }

    Node *block_node(StmtList *stmts)
    {
        NODE(BlockNode, BLOCK);
        node->stmts = stmts;
        return (Node *)node;
    }

    Node *func_def_node(Type ret_type, Str ident, ParamList *params, Node *body)
    {
        NODE(FuncDefNode, FUNC_DEF);
        node->ret_type = ret_type;
        node->name = push_str(ident);
        node->params = params;
        node->body = body;
        return (Node *)node;
    }
};

#endif // NODE_ALLOC_H
