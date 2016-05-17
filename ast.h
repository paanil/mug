#ifndef AST_H
#define AST_H

#include "lexer.h"

#define DEF_NODE(Name) \
struct Name { \
    NodeType type;

#define END_NODE \
};

enum NodeType
{
    // expressions
    NodeType_BOOL,
    NodeType_CONST,
    NodeType_VAR,
    NodeType_CALL,
    NodeType_UNARY,
    NodeType_BINARY,

    // statements
    NodeType_EMPTY,
    NodeType_EXPR,
    NodeType_ASSIGN,
    NodeType_DECL,
    NodeType_RETURN,
    NodeType_IF,
    NodeType_WHILE,
    NodeType_BLOCK,
    NodeType_FUNC_DEF,
};

struct ArgList
{
    struct Node *arg;
    ArgList *next;
};

struct ParamList
{
    Token::Type type;
    Str name;
    ParamList *next;
};

struct StmtList
{
    struct Node *stmt;
    StmtList *next;
};

// expressions

DEF_NODE(BoolNode)
    bool value;
END_NODE

DEF_NODE(ConstNode)
    uint64_t value;
END_NODE

DEF_NODE(VarNode)
    Str name;
END_NODE

DEF_NODE(CallNode)
    Str func_name;
    ArgList *args;
END_NODE

DEF_NODE(UnaryNode)
    struct Node *operand;
    Token::Type op;
END_NODE

DEF_NODE(BinaryNode)
    struct Node *left;
    struct Node *right;
    Token::Type op;
END_NODE

// statements

DEF_NODE(ExprNode)
    struct Node *expr;
END_NODE

DEF_NODE(AssignNode)
    Str var_name;
    struct Node *value;
END_NODE

DEF_NODE(DeclNode)
    Token::Type var_type;
    Str var_name;
    struct Node *init;
END_NODE

DEF_NODE(ReturnNode)
    struct Node *value;
END_NODE

DEF_NODE(IfNode)
    struct Node *condition;
    struct Node *true_stmt;
    struct Node *else_stmt;
END_NODE

DEF_NODE(WhileNode)
    struct Node *condition;
    struct Node *stmt;
END_NODE

DEF_NODE(BlockNode)
    StmtList *stmts;
END_NODE

DEF_NODE(FuncDefNode)
    Token::Type ret_type;
    Str name;
    ParamList *params;
    struct Node *body;
END_NODE

//

struct Node
{
    union
    {
        // expressions
        NodeType    type;
        BoolNode    boolean;
        ConstNode   constant;
        VarNode     var;
        CallNode    call;
        UnaryNode   unary;
        BinaryNode  binary;

        // statements
        ExprNode    expr;
        AssignNode  assign;
        DeclNode    decl;
        ReturnNode  ret;
        IfNode      if_stmt;
        WhileNode   while_stmt;
        BlockNode   block;
        FuncDefNode func_def;
    };
};

void print_node(Node *node);

#endif // AST_H
