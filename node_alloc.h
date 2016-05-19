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

    Node *var_node(Str ident)
    {
        NODE(VarNode, VAR);
        node->name = push_str(ident);
        return (Node *)node;
    }

    Node *call_node(Str ident, ArgList *args)
    {
        NODE(CallNode, CALL);
        node->func_name = push_str(ident);
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

    Node *assign_node(Str ident, Node *value)
    {
        NODE(AssignNode, ASSIGN);
        node->var_name = push_str(ident);
        node->value = value;
        return (Node *)node;
    }

    Node *decl_node(Token::Type type, Str ident, Node *init)
    {
        NODE(DeclNode, DECL);
        node->var_type = (Type){Type::INT};//type; //TODO: Something.
        node->var_name = push_str(ident);
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

    Node *func_def_node(Token::Type ret_type, Str ident,
                        ParamList *params, Node *body)
    {
        NODE(FuncDefNode, FUNC_DEF);
        node->ret_type = (Type){Type::INT};//ret_type; // TODO: Something.
        node->name = push_str(ident);
        node->params = params;
        node->body = body;
        return (Node *)node;
    }
};

#endif // NODE_ALLOC_H
