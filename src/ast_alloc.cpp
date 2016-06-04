#include "ast_alloc.h"
#include "alloc.h"
#include "assert.h"

Str AstAlloc::push_str(Str s)
{
    char *data = a.allocate_array<char>(s.len + 1);
    memcpy(data, s.data, s.len);
    data[s.len] = 0;
    return Str::make(data, s.len);
}

ArgList *AstAlloc::alloc_arg()
{
    return a.allocate<ArgList>();
}

ParamList *AstAlloc::alloc_param()
{
    return a.allocate<ParamList>();
}

StmtList *AstAlloc::alloc_stmt()
{
    return a.allocate<StmtList>();
}

#define EXPRESSION(T, Type) \
    T *exp = a.allocate<T>(); \
    exp->type = ExpType_##Type

Expression *AstAlloc::bool_exp(bool value)
{
    EXPRESSION(BoolExp, BOOL);
    exp->value = value;
    exp->data_type.type = Type::BOOL;
    return (Expression *)exp;
}

Expression *AstAlloc::const_exp(uint64_t value, Type::Enum type)
{
    EXPRESSION(ConstExp, CONST);
    exp->value = value;
    exp->data_type.type = type;
    return (Expression *)exp;
}

Expression *AstAlloc::var_exp(Str ident)
{
    EXPRESSION(VarExp, VAR);
    exp->name = push_str(ident);
    return (Expression *)exp;
}

Expression *AstAlloc::call_exp(Str ident, ArgList *args)
{
    EXPRESSION(CallExp, CALL);
    exp->func_name = push_str(ident);
    exp->args = args;
    return (Expression *)exp;
}

Expression *AstAlloc::unary_exp(Expression *operand, Token::Type op)
{
    EXPRESSION(UnaryExp, UNARY);
    exp->operand = operand;

    switch (op)
    {
    case Token::NOT:   exp->op = UnaryOp_NOT; break;
    case Token::MINUS: exp->op = UnaryOp_NEG; break;
    InvalidDefaultCase;
    }

    return (Expression *)exp;
}

Expression *AstAlloc::binary_exp(Expression *left, Expression *right, Token::Type op)
{
    EXPRESSION(BinaryExp, BINARY);
    exp->left = left;
    exp->right = right;

    switch (op)
    {
        case Token::STAR:  exp->op = BinaryOp_MUL; break;
        case Token::SLASH: exp->op = BinaryOp_DIV; break;
        case Token::PLUS:  exp->op = BinaryOp_ADD; break;
        case Token::MINUS: exp->op = BinaryOp_SUB; break;
        case Token::EQ:    exp->op = BinaryOp_EQ;  break;
        case Token::NE:    exp->op = BinaryOp_NE;  break;
        case Token::LT:    exp->op = BinaryOp_LT;  break;
        case Token::GT:    exp->op = BinaryOp_GT;  break;
        case Token::LE:    exp->op = BinaryOp_LE;  break;
        case Token::GE:    exp->op = BinaryOp_GE;  break;
        case Token::AND:   exp->op = BinaryOp_AND; break;
        case Token::OR:    exp->op = BinaryOp_OR;  break;
        InvalidDefaultCase;
    }

    return (Expression *)exp;
}

#define NODE(T, Type) \
    T *node = a.allocate<T>(); \
    node->type = NodeType_##Type

Node *AstAlloc::empty_node()
{
    NODE(Node, EMPTY);
    return node;
}

Node *AstAlloc::exp_node(Expression *exp)
{
    NODE(ExpNode, EXP);
    node->exp = exp;
    return (Node *)node;
}

Node *AstAlloc::assign_node(Str ident, Expression *value)
{
    NODE(AssignNode, ASSIGN);
    node->var_name = push_str(ident);
    node->value = value;
    return (Node *)node;
}

Node *AstAlloc::decl_node(Type type, Str ident, Expression *init)
{
    NODE(DeclNode, DECL);
    node->var_type = type;
    node->var_name = push_str(ident);
    node->init = init;
    return (Node *)node;
}

Node *AstAlloc::return_node(Expression *ret_val)
{
    NODE(ReturnNode, RETURN);
    node->value = ret_val;
    return (Node *)node;
}

Node *AstAlloc::if_node(Expression *condition, Node *true_stmt, Node *else_stmt)
{
    NODE(IfNode, IF);
    node->condition = condition;
    node->true_stmt = true_stmt;
    node->else_stmt = else_stmt;
    return (Node *)node;
}

Node *AstAlloc::while_node(Expression *condition, Node *stmt)
{
    NODE(WhileNode, WHILE);
    node->condition = condition;
    node->stmt = stmt;
    return (Node *)node;
}

Node *AstAlloc::block_node(StmtList *stmts)
{
    NODE(BlockNode, BLOCK);
    node->stmts = stmts;
    return (Node *)node;
}

Node *AstAlloc::func_def_node(Type ret_type, Str ident, ParamList *params, Node *body)
{
    NODE(FuncDefNode, FUNC_DEF);
    node->ret_type = ret_type;
    node->name = push_str(ident);
    node->params = params;
    node->body = body;
    return (Node *)node;
}
