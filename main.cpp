#include "parser.h"
#include <cstdio>

struct SymTable
{
    StrMap<Type> table;

    bool has(Str symbol, Type *result)
    {
        uint32_t idx = table.find(symbol);
        if (idx == NOT_FOUND) return false;
        *result = table.get(idx);
        return true;
    }
};

bool type_check(Expression *exp, SymTable *sym)
{
    switch (exp->type)
    {
        case ExpType_BOOL:
        case ExpType_CONST:
            return true;
        case ExpType_VAR:
        {
            Type sym_type;
            if (!sym->has(exp->var.name, &sym_type))
            {
                // TODO: Error message.
                return false;
            }
            exp->var.data_type = sym_type;
            return true;
        }
        case ExpType_CALL:
        {
            Type sym_type;
            if (!sym->has(exp->call.func_name, &sym_type))
            {
                // TODO: Error message.
                return false;
            }
            exp->call.data_type = sym_type;
            // TODO: Check args against params!
            return true;
        }
        case ExpType_UNARY:
        case ExpType_BINARY:
            return false;
    }

    return false;
}

bool type_check(Node *node, SymTable *sym)
{
    switch (node->type)
    {
    case NodeType_EMPTY:
        return true;
    case NodeType_EXP:
        return type_check(node->exp.exp, sym);
    case NodeType_ASSIGN:
    {
        Type sym_type;
        if (sym->has(node->assign.var_name, &sym_type))
        {
            // TODO: Error message.
            return false;
        }
        if (!type_check(node->assign.value, sym))
            return false;
        // TODO: Proper check!
        if (node->assign.value->data_type.type != sym_type.type)
        {
            // TODO: Error message.
            return false;
        }
        return true;
    }

    case NodeType_DECL:
    case NodeType_RETURN:
    case NodeType_IF:
    case NodeType_WHILE:
    case NodeType_BLOCK:
    case NodeType_FUNC_DEF:
        return false;
    }

    return false;
}

bool type_check(Ast ast)
{
    if (!ast.valid)
        return false;

    SymTable sym;

    return type_check(ast.root, &sym);
}

void run_type_check_tests()
{
    Alloc a;
    Parser p(a);
    Ast ast = p.parse("42;");
    if (!type_check(ast))
    {
        fprintf(stderr, "my glob\n");
    }
}

int main()
{
    run_lexer_tests();
    run_parser_tests();
    run_type_check_tests();

    return 0;
}
