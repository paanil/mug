#include "parser.h"
#include "sym_table.h"

struct TypeChecker
{
    SymTable sym;
    ErrorContext &ec;

    TypeChecker(ErrorContext &ec_)
    : sym()
    , ec(ec_)
    {}

    bool type_check(Expression *exp)
    {
        switch (exp->type)
        {
            case ExpType_BOOL:
            case ExpType_CONST:
                return true;
            case ExpType_VAR:
            {
                Type sym_type;
                if (!sym.has(exp->var.name, &sym_type))
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
                if (!sym.has(exp->call.func_name, &sym_type))
                {
                    // TODO: Error message.
                    return false;
                }
                exp->call.data_type = sym_type;
                // TODO: Check args against params!
                return true;
            }
            case ExpType_UNARY:
                if (!type_check(exp->unary.operand))
                    return false;
                // TODO: Check type is bool if op is not, etc.
                return true;
            case ExpType_BINARY:
                if (!type_check(exp->binary.left))
                    return false;
                if (!type_check(exp->binary.right))
                    return false;
                // TODO: Check op and operand compatibility.
                return true;
        }

        return false;
    }

    bool type_check(Node *node)
    {
        switch (node->type)
        {
        case NodeType_EMPTY:
            return true;
        case NodeType_EXP:
            return type_check(node->exp.exp);
        case NodeType_ASSIGN:
        {
            Type sym_type;
            if (sym.has(node->assign.var_name, &sym_type))
            {
                // TODO: Error message.
                return false;
            }
            if (!type_check(node->assign.value))
                return false;
            // TODO: Proper check!
            if (node->assign.value->data_type != sym_type)
            {
                // TODO: Error message.
                return false;
            }
            return true;
        }
        case NodeType_DECL:
        {
            if (sym.in_current_scope(node->decl.var_name))
            {
                ec.print_error("variable '%s' already defined in current scope",
                               node->decl.var_name.data);
                return false;
            }
            if (node->decl.init)
            {
                if (!type_check(node->decl.init))
                    return false;
                // TODO: Check properly!
                if (node->decl.var_type != node->decl.init->data_type)
                {
                    // TODO: Better error message.
                    ec.print_error("incompatible types in variable declaration");
                    return false;
                }
            }
            sym.put(node->decl.var_name, node->decl.var_type);
            return true;
        }
        case NodeType_RETURN:
        case NodeType_IF:
        case NodeType_WHILE:
            return false;
        case NodeType_BLOCK:
        {
            sym.enter_scope();

            StmtList *s = node->block.stmts;
            while (s)
            {
                if (!type_check(s->stmt))
                    return false;
                s = s->next;
            }

            sym.exit_scope();
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

        return type_check(ast.root);
    }
};

#include <cstdio>

#define TEST_RESULT(input, result) \
    { tests += 1; Alloc a; ErrorContext ec(result ? 1 : 0); \
      Parser p(a, ec); Ast ast = p.parse(input); TypeChecker tc(ec); \
      if (tc.type_check(ast) != result) { fprintf(stderr, "type check test #%d failed.\n\n", tests); failed += 1; } }

#define TEST(input) TEST_RESULT(input, true)
#define TEST_FAIL(input) TEST_RESULT(input, false)

void run_type_check_tests()
{
    int tests = 0;
    int failed = 0;

    fprintf(stdout, "running type check tests...\n");

/*  1 */ TEST("42;")
/*  2 */ TEST("-42;")
/*  3 */ TEST("42 + 5;")
/*    */
/*  4 */ TEST_FAIL("x;") // fail
/*  5 */ TEST_FAIL("x + 13;") // fail
/*  6 */ TEST_FAIL("int x = x + 5;") // fail
/*  7 */ TEST_FAIL("f();") // fail
/*  8 */ TEST_FAIL("f() * 11;") // fail
/*  9 */ TEST_FAIL("f(25);") // fail
/*    */
/* 10 */ TEST("int x;")
/* 11 */ TEST("int x; x + 13;")
/* 12 */ TEST("int x = 2; x * 7;")
/* 13 */ TEST_FAIL("int x; int x = 5;")
/* 14 */ TEST("int x; { uint x = 5; }")
/* 15 */ TEST("function f() { int x = -31; } f();")
/* 16 */ TEST_FAIL("function f() { int x = -31; } int x = f() * 11;") // fail
/* 17 */ TEST_FAIL("function f(int y) { int x = -31; } int x = f() * 11;") // fail
/* 18 */ TEST("function f() -> int { return 5 * 9; } int x = f() * 11;")

    fprintf(stdout, "------------------------------\n");
    fprintf(stdout, "ran %d type check tests: %d succeeded, %d failed.\n\n\n", tests, tests - failed, failed);
}

int main()
{
    run_lexer_tests();
    run_parser_tests();
    run_type_check_tests();

    return 0;
}
