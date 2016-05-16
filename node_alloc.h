#ifndef NODE_ALLOC_H
#define NODE_ALLOC_H

#include "alloc.h"
#include "ast.h"

struct NodeAlloc
{
    Alloc &a;

    NodeAlloc(Alloc &a_)
    : a(a_)
    {}

    Ident make_ident(const char *text, int len)
    {
        char *ident = a.allocate_array<char>(len + 1);
        for (int i = 0; i < len; i++)
            ident[i] = text[i];
        ident[len] = 0;
        return (Ident){ ident };
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

#define NODE(Type, NType) \
    Type *node = a.allocate<Type>(); \
    node->type = NodeType_##NType

    //
    // Expressions
    //

    Node *bool_node(bool value)
    {
        NODE(BoolNode, BOOL);
        node->value = value;
        return (Node *)node;
    }

    Node *const_node(uint64_t value)
    {
        NODE(ConstNode, CONST);
        node->value = value;
        return (Node *)node;
    }

    Node *var_node(const char *text, int len)
    {
        NODE(VarNode, VAR);
        node->name = make_ident(text, len);
        return (Node *)node;
    }

    Node *call_node(const char *text, int len, ArgList *args)
    {
        NODE(CallNode, CALL);
        node->func_name = make_ident(text, len);
        node->args = args;
        return (Node *)node;
    }

    Node *unary_node(Node *operand, Token::Type op)
    {
        NODE(UnaryNode, UNARY);
        node->operand = operand;
        node->op = op;
        return (Node *)node;
    }

    Node *binary_node(Node *left, Node *right, Token::Type op)
    {
        NODE(BinaryNode, BINARY);
        node->left = left;
        node->right = right;
        node->op = op;
        return (Node *)node;
    }

    //
    // Statements
    //

    Node *empty_node()
    {
        NODE(Node, EMPTY);
        return node;
    }

    Node *expr_node(Node *expr)
    {
        NODE(ExprNode, EXPR);
        node->expr = expr;
        return (Node *)node;
    }

    Node *assign_node(const char *text, int len, Node *value)
    {
        NODE(AssignNode, ASSIGN);
        node->var_name = make_ident(text, len);
        node->value = value;
        return (Node *)node;
    }

    Node *decl_node(Token::Type type, const char *text, int len, Node *init)
    {
        NODE(DeclNode, DECL);
        node->var_type = type;
        node->var_name = make_ident(text, len);
        node->init = init;
        return (Node *)node;
    }

    Node *return_node(Node *ret_val)
    {
        NODE(ReturnNode, RETURN);
        node->value = ret_val;
        return (Node *)node;
    }

    Node *if_node(Node *condition, Node *true_stmt, Node *else_stmt)
    {
        NODE(IfNode, IF);
        node->condition = condition;
        node->true_stmt = true_stmt;
        node->else_stmt = else_stmt;
        return (Node *)node;
    }

    Node *while_node(Node *condition, Node *stmt)
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

    Node *func_def_node(Token::Type ret_type, const char *text, int len,
                        ParamList *params, Node *body)
    {
        NODE(FuncDefNode, FUNC_DEF);
        node->ret_type = ret_type;
        node->name = make_ident(text, len);
        node->params = params;
        node->body = body;
        return (Node *)node;
    }
};

#endif // NODE_ALLOC_H
