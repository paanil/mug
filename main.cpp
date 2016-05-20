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
            if (!type_check(exp->unary.operand, sym))
                return false;
            // TODO: Check type is bool if op is not, etc.
            return true;
        case ExpType_BINARY:
            if (!type_check(exp->binary.left, sym))
                return false;
            if (!type_check(exp->binary.right, sym))
                return false;
            // TODO: Check op and operand compatibility.
            return true;
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
        return false;
    case NodeType_BLOCK:
    {
        StmtList *s = node->block.stmts;
        while (s)
        {
            if (!type_check(s->stmt, sym))
                return false;
            s = s->next;
        }
        return true;
    }
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

#define TEST_RESULT(input, result) \
    { Alloc a; Parser p(a); Ast ast = p.parse(input); tests += 1; \
      if (type_check(ast) != result) { fprintf(stderr, "type check test #%d failed.\n", tests); failed += 1; } }

#define TEST(input) TEST_RESULT(input, true)
#define TEST_FAIL(input) TEST_RESULT(input, false)

void run_type_check_tests()
{
    int tests = 0;
    int failed = 0;

    TEST("42;")
    TEST("-42;")
    TEST("42 + 5;")

    TEST_FAIL("x;") // fail
    TEST_FAIL("x + 13;") // fail
    TEST_FAIL("int x = x + 5;") // fail
    TEST_FAIL("f();") // fail
    TEST_FAIL("f() * 11;") // fail
    TEST_FAIL("f(25);") // fail

    TEST("int x;")
    TEST("int x; x + 13;")
    TEST("int x = 2; x * 7;")
    TEST("function f() { int x = -31; } f();")
    TEST_FAIL("function f() { int x = -31; } int x = f() * 11;") // fail
    TEST_FAIL("function f(int y) { int x = -31; } int x = f() * 11;") // fail
    TEST("function f() -> int { return 5 * 9; } int x = f() * 11;")

    fprintf(stdout, "ran %d type check tests: %d succeeded, %d failed.\n", tests, tests - failed, failed);
}

int main()
{
    run_lexer_tests();
    run_parser_tests();
    run_type_check_tests();

    return 0;
}
