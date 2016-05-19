#include "ast.h"

#include <cstdio>


// assert that the node types are ok.

#define offs(Type, member) ((unsigned long long int)&(((Type *)0)->member))

#define ASSERT_TYPE_OFFSET(member) \
static_assert(offs(Node, member.type) == offs(Node, type), "offs(Node::" #member ".type) != offs(Node::type)")

// expressions
ASSERT_TYPE_OFFSET(boolean);
ASSERT_TYPE_OFFSET(constant);
ASSERT_TYPE_OFFSET(var);
ASSERT_TYPE_OFFSET(call);
ASSERT_TYPE_OFFSET(unary);
ASSERT_TYPE_OFFSET(binary);

// statements
ASSERT_TYPE_OFFSET(expr);
ASSERT_TYPE_OFFSET(assign);
ASSERT_TYPE_OFFSET(decl);
ASSERT_TYPE_OFFSET(ret);
ASSERT_TYPE_OFFSET(if_stmt);
ASSERT_TYPE_OFFSET(while_stmt);
ASSERT_TYPE_OFFSET(block);
ASSERT_TYPE_OFFSET(func_def);


//
//
//

void print_node(Node *node)
{
    if (node == 0)
        return;

    switch (node->type)
    {

    // expressions
    //
    case NodeType_BOOL:
        printf("%s", node->boolean.value ? "true" : "false");
        break;
    case NodeType_CONST:
        printf("%llu", node->constant.value);
        break;
    case NodeType_VAR:
        printf("%s", node->var.name.data);
        break;
    case NodeType_CALL:
        printf("%s(", node->call.func_name.data);
        if (node->call.args)
        {
            ArgList *arg = node->call.args;
            while (true)
            {
                print_node(arg->arg);
                arg = arg->next;
                if (!arg) break;
                printf(", ");
            }
        }
        printf(")");
        break;
    case NodeType_UNARY:
        printf("%s", Token::get_str(node->unary.op));
        print_node(node->unary.operand);
        break;
    case NodeType_BINARY:
        printf("(");
        print_node(node->binary.left);
        printf(" %s ", Token::get_str(node->binary.op));
        print_node(node->binary.right);
        printf(")");
        break;

    // statements
    //
    case NodeType_EMPTY:
        printf(";\n");
        break;
    case NodeType_EXPR:
        print_node(node->expr.expr);
        printf(";\n");
        break;
    case NodeType_ASSIGN:
        printf("%s = ", node->assign.var_name.data);
        print_node(node->assign.value);
        printf(";\n");
        break;
    case NodeType_DECL:
        // TODO: Type string.
        printf("%s %s", "<type>"/*Token::get_str(node->decl.var_type)*/,
                        node->decl.var_name.data);
        if (node->decl.init)
        {
            printf(" = ");
            print_node(node->decl.init);
        }
        printf(";\n");
        break;
    case NodeType_RETURN:
        printf("return");
        if (node->ret.value)
        {
            printf(" ");
            print_node(node->ret.value);
        }
        printf(";\n");
        break;
    case NodeType_IF:
        printf("if ");
        print_node(node->if_stmt.condition);
        printf(" ");
        print_node(node->if_stmt.true_stmt);
        if (node->if_stmt.else_stmt)
        {
            printf("else  ");
            print_node(node->if_stmt.else_stmt);
        }
        break;
    case NodeType_WHILE:
        printf("while ");
        print_node(node->while_stmt.condition);
        printf(" ");
        print_node(node->while_stmt.stmt);
        break;
    case NodeType_BLOCK:
    {
        printf("{\n");
        StmtList *stmt = node->block.stmts;
        while (stmt)
        {
            print_node(stmt->stmt);
            stmt = stmt->next;
        }
        printf("}\n");
        break;
    }
    case NodeType_FUNC_DEF:
    {
        printf("function %s(", node->func_def.name.data);
        if (node->func_def.params)
        {
            ParamList *param = node->func_def.params;
            while (true)
            {
                // TODO: Type string.
                printf("%s %s", "<type>"/*Token::get_str(param->type)*/, param->name.data);
                param = param->next;
                if (!param) break;
                printf(", ");
            }
        }
        printf(") ");
        if (node->func_def.ret_type.type != Type::VOID)
            // TODO: Type string.
            printf("-> %s ", "<type>"/*Token::get_str(node->func_def.ret_type)*/);
        print_node(node->func_def.body);
        break;
    }
    default:
        break;
    }
}
