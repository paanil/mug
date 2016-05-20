#include "ast.h"

#include <cstdio>


// assert that the node types are ok.

#define offs(Type, member) ((unsigned long long int)&(((Type *)nullptr)->member))

#define ASSERT_OFFSET(T, member, attr) \
static_assert(offs(T, member) == offs(T, type), "offs(T::" #member "." #attr ") != offs(T::" #attr ")")

ASSERT_OFFSET(Expression, boolean,  type);
ASSERT_OFFSET(Expression, constant, type);
ASSERT_OFFSET(Expression, var,      type);
ASSERT_OFFSET(Expression, call,     type);
ASSERT_OFFSET(Expression, unary,    type);
ASSERT_OFFSET(Expression, binary,   type);

ASSERT_OFFSET(Expression, boolean,  data_type);
ASSERT_OFFSET(Expression, constant, data_type);
ASSERT_OFFSET(Expression, var,      data_type);
ASSERT_OFFSET(Expression, call,     data_type);
ASSERT_OFFSET(Expression, unary,    data_type);
ASSERT_OFFSET(Expression, binary,   data_type);

ASSERT_OFFSET(Node, exp,        type);
ASSERT_OFFSET(Node, assign,     type);
ASSERT_OFFSET(Node, decl,       type);
ASSERT_OFFSET(Node, ret,        type);
ASSERT_OFFSET(Node, if_stmt,    type);
ASSERT_OFFSET(Node, while_stmt, type);
ASSERT_OFFSET(Node, block,      type);
ASSERT_OFFSET(Node, func_def,   type);


//
//
//

void print_exp(Expression *exp)
{
    if (exp == nullptr)
        return;

    switch (exp->type)
    {
    case ExpType_BOOL:
        fprintf(stdout, "%s", exp->boolean.value ? "true" : "false");
        break;
    case ExpType_CONST:
        fprintf(stdout, "%llu", exp->constant.value);
        break;
    case ExpType_VAR:
        fprintf(stdout, "%s", exp->var.name.data);
        break;
    case ExpType_CALL:
        fprintf(stdout, "%s(", exp->call.func_name.data);
        if (exp->call.args)
        {
            ArgList *arg = exp->call.args;
            while (true)
            {
                print_exp(arg->arg);
                arg = arg->next;
                if (!arg) break;
                fprintf(stdout, ", ");
            }
        }
        fprintf(stdout, ")");
        break;
    case ExpType_UNARY:
        fprintf(stdout, "%s", Token::get_str(exp->unary.op));
        print_exp(exp->unary.operand);
        break;
    case ExpType_BINARY:
        fprintf(stdout, "(");
        print_exp(exp->binary.left);
        fprintf(stdout, " %s ", Token::get_str(exp->binary.op));
        print_exp(exp->binary.right);
        fprintf(stdout, ")");
        break;
    }
}

void print_node(Node *node)
{
    if (node == nullptr)
        return;

    switch (node->type)
    {
    case NodeType_EMPTY:
        fprintf(stdout, ";\n");
        break;
    case NodeType_EXP:
        print_exp(node->exp.exp);
        fprintf(stdout, ";\n");
        break;
    case NodeType_ASSIGN:
        fprintf(stdout, "%s = ", node->assign.var_name.data);
        print_exp(node->assign.value);
        fprintf(stdout, ";\n");
        break;
    case NodeType_DECL:
        // TODO: Type string.
        fprintf(stdout, "%s %s", "<type>"/*Token::get_str(node->decl.var_type)*/,
                        node->decl.var_name.data);
        if (node->decl.init)
        {
            fprintf(stdout, " = ");
            print_exp(node->decl.init);
        }
        fprintf(stdout, ";\n");
        break;
    case NodeType_RETURN:
        fprintf(stdout, "return");
        if (node->ret.value)
        {
            fprintf(stdout, " ");
            print_exp(node->ret.value);
        }
        fprintf(stdout, ";\n");
        break;
    case NodeType_IF:
        fprintf(stdout, "if ");
        print_exp(node->if_stmt.condition);
        fprintf(stdout, " ");
        print_node(node->if_stmt.true_stmt);
        if (node->if_stmt.else_stmt)
        {
            fprintf(stdout, "else  ");
            print_node(node->if_stmt.else_stmt);
        }
        break;
    case NodeType_WHILE:
        fprintf(stdout, "while ");
        print_exp(node->while_stmt.condition);
        fprintf(stdout, " ");
        print_node(node->while_stmt.stmt);
        break;
    case NodeType_BLOCK:
    {
        fprintf(stdout, "{\n");
        StmtList *stmt = node->block.stmts;
        while (stmt)
        {
            print_node(stmt->stmt);
            stmt = stmt->next;
        }
        fprintf(stdout, "}\n");
        break;
    }
    case NodeType_FUNC_DEF:
    {
        fprintf(stdout, "function %s(", node->func_def.name.data);
        if (node->func_def.params)
        {
            ParamList *param = node->func_def.params;
            while (true)
            {
                // TODO: Type string.
                fprintf(stdout, "%s %s", "<type>"/*Token::get_str(param->type)*/, param->name.data);
                param = param->next;
                if (!param) break;
                fprintf(stdout, ", ");
            }
        }
        fprintf(stdout, ") ");
        if (node->func_def.ret_type.type != Type::VOID)
            // TODO: Type string.
            fprintf(stdout, "-> %s ", "<type>"/*Token::get_str(node->func_def.ret_type)*/);
        print_node(node->func_def.body);
        break;
    }
    }
}
