#ifndef AST_H
#define AST_H

#include "lexer.h"
#include "type.h"

#define DEF_EXP(Name) \
struct Name { \
    ExpType type; \
    Type data_type;

#define END_EXP \
};

#define DEF_NODE(Name) \
struct Name { \
    NodeType type;

#define END_NODE \
};

enum ExpType
{
    ExpType_BOOL,
    ExpType_CONST,
    ExpType_VAR,
    ExpType_CALL,
    ExpType_UNARY,
    ExpType_BINARY,
};

enum UnaryOp
{
    UnaryOp_NOT,
    UnaryOp_NEG,
};

enum BinaryOp
{
    BinaryOp_MUL,
    BinaryOp_DIV,
    BinaryOp_ADD,
    BinaryOp_SUB,
    BinaryOp_EQ,
    BinaryOp_NE,
    BinaryOp_LT,
    BinaryOp_GT,
    BinaryOp_LE,
    BinaryOp_GE,
    BinaryOp_AND,
    BinaryOp_OR,
};

enum NodeType
{
    NodeType_EMPTY,
    NodeType_EXP,
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
    struct Expression *arg;
    ArgList *next;
};

struct ParamList
{
    Type type;
    Str name;
    ParamList *next;
};

struct StmtList
{
    struct Node *stmt;
    StmtList *next;
};

// expressions

DEF_EXP(BoolExp)
    bool value;
END_EXP

DEF_EXP(ConstExp)
    uint64_t value;
END_EXP

DEF_EXP(VarExp)
    Str name;
END_EXP

DEF_EXP(CallExp)
    Str func_name;
    ArgList *args;
END_EXP

DEF_EXP(UnaryExp)
    struct Expression *operand;
    UnaryOp op;
END_EXP

DEF_EXP(BinaryExp)
    struct Expression *left;
    struct Expression *right;
    BinaryOp op;
END_EXP

// statements

DEF_NODE(ExpNode)
    struct Expression *exp;
END_NODE

DEF_NODE(AssignNode)
    Str var_name;
    struct Expression *value;
END_NODE

DEF_NODE(DeclNode)
    Type var_type;
    Str var_name;
    struct Expression *init;
END_NODE

DEF_NODE(ReturnNode)
    struct Expression *value;
END_NODE

DEF_NODE(IfNode)
    struct Expression *condition;
    struct Node *true_stmt;
    struct Node *else_stmt;
END_NODE

DEF_NODE(WhileNode)
    struct Expression *condition;
    struct Node *stmt;
END_NODE

DEF_NODE(BlockNode)
    StmtList *stmts;
END_NODE

DEF_NODE(FuncDefNode)
    Type ret_type;
    Str name;
    ParamList *params;
    struct Node *body;
END_NODE

//

struct Expression
{
    union
    {
        struct
        {
            ExpType type;
            Type data_type;
        };

        BoolExp     boolean;
        ConstExp    constant;
        VarExp      var;
        CallExp     call;
        UnaryExp    unary;
        BinaryExp   binary;
    };
};

struct Node
{
    union
    {
        NodeType    type;

        ExpNode     exp;
        AssignNode  assign;
        DeclNode    decl;
        ReturnNode  ret;
        IfNode      if_stmt;
        WhileNode   while_stmt;
        BlockNode   block;
        FuncDefNode func_def;
    };
};

const char *UnaryOp_get_str(UnaryOp op);
const char *BinaryOp_get_str(BinaryOp op);

void print_node(Node *node);

struct Ast
{
    Node *root;
    bool valid;
};

#endif // AST_H
