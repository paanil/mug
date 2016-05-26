#include "ir.h"
#include "sym_table.h"

#include <cstdio>
#include <cassert>

union Value
{
    int64_t  ivalue;
    uint64_t uvalue;
};

struct Int
{
    Value retval;
    int env_index;
    Value env[20][100]; // max 20 nested calls, max 100 temps per call
    Routine *routines[20]; // max 20 routines


    Value get(Operand temp)
    {
        assert(temp.temp_id < 100);
        return env[env_index][temp.temp_id];
    }

    void set(Operand temp, uint64_t value)
    {
        assert(temp.temp_id < 100);
        env[env_index][temp.temp_id].uvalue = value;
    }

    void eval(Routine &r)
    {
        for (uint32_t i = 0; i < r.n; i++)
        {
            Quad q = r[i];
            switch (q.op)
            {
                case IR::MOV_IM:
                    set(q.target, q.left.int_value);
                    break;
                case IR::MOV:
                    set(q.target, get(q.left).uvalue);
                    break;
                case IR::MUL:
                    set(q.target, get(q.left).uvalue * get(q.right).uvalue);
                    break;
                case IR::IMUL:
                    set(q.target, get(q.left).ivalue * get(q.right).ivalue);
                    break;
                case IR::ADD:
                    set(q.target, get(q.left).uvalue + get(q.right).uvalue);
                    break;
                case IR::SUB:
                    set(q.target, get(q.left).uvalue - get(q.right).uvalue);
                    break;
                case IR::EQ:
                    set(q.target, get(q.left).uvalue == get(q.right).uvalue);
                    break;
                case IR::LT:
                    set(q.target, get(q.left).uvalue < get(q.right).uvalue);
                    break;
                case IR::JMP:
                    i = q.target.jump - 1; // for loop will i++
                    break;
                case IR::JZ:
                    if (get(q.left).uvalue == 0)
                        i = q.target.jump - 1; // for loop will i++
                    break;
                case IR::CALL:
                    assert(env_index < 20);
                    env_index++;
                    eval(*routines[q.left.func_id]);
                    env_index--;
                    set(q.target, retval.uvalue);
                    break;
                case IR::RET:
                    if (q.target.int_value)
                        retval = get(q.left);
                    return;
                case IR::ARG:
                {
                    assert(env_index < 20);
                    Value arg = get(q.left);
                    env_index++;
                    Operand temp;
                    temp.temp_id = q.target.arg_index;
                    set(temp, arg.uvalue);
                    env_index--;
                    break;
                }
            }
        }
    }

    Value eval(IR ir)
    {
        Routine *r = ir.routines;
        while (r)
        {
            assert(r->id < 20);
            routines[r->id] = r;
            r = r->next;
        }

        env_index = 0;
        eval(*routines[0]);

        return retval;
    }
};

void eval(IR ir)
{
    Int evaluator;
    Value val = evaluator.eval(ir);

    printf("returned %lld (int) / %llu (uint) / %s (bool)\n",
           val.ivalue, val.uvalue, val.uvalue ? "true" : "false");
}

#include "ir_gen.h"
#include "check.h"
#include "parser.h"
#include "lexer.h"
#include "alloc.h"
#include "error_context.h"

#include <cstdio>

//
// IR gen tests
//

#define TEST(input) { \
    tests += 1; \
    Alloc a; \
    ErrorContext ec(1); \
    Ast ast = parse(input, a, ec); \
    check(ast, ec); \
    IR ir = gen_ir(ast, a); \
    print_ir(ir); \
    eval(ir); \
}

void run_ir_gen_tests()
{
    int tests = 0;
    int failed = 0;

    fprintf(stdout, "running ir gen tests...\n\n");

//    TEST("5;")
//    TEST("5 + 5;")
//    TEST("int x = 5 + 5;")
//    TEST("int x = 5 + 5; int y = x * 10;")
//    TEST("int x; int y = 10; x = 5 + y;")
//    TEST("int x = -5;")
//    TEST("int x = 10 - 5;")
//    TEST("bool result = (5 < 6);")
//    TEST("bool result = false;")
//    TEST("if (5 == 6) true;")
//    TEST("int x = 0; if (5 == 6) { x = 5; } else { int y = 2; x = 5 * y; }")
//    TEST("int x = 1; int i = 0; while (i < 10) { x = x + x; i = i + 1; } int y = 5 * x;")
//    TEST("function f() { int x = 0; x = x + 5; } f();")
//    TEST("function f(int x) -> int { return x + 1; } int x = 0; while (x < 5) x = f(x);")
//    TEST("function f() -> bool { return false; } f();")
//    TEST("function f() -> int { return 1*2*3*4*-5+8; } f();")
    TEST("function g(int i) -> int { int x = 5; return i*i + x; }"
         "function f() -> int {"
         "  int val = 0;"
         "  int i = 0;"
         "  while (i < 3) {"
         "    val = val - g(i);"
         "    i = i + 1;"
         "  }"
         "  return val;"
         "}"
         "f();")

    fprintf(stdout, "------------------------------\n");
    fprintf(stdout, "ran %d ir gen tests: %d succeeded, %d failed.\n\n\n", tests, tests - failed, failed);
}

#undef TEST

//
// Static check tests
//

#define TEST_RESULT(input, result) { \
    tests += 1; \
    Alloc a; \
    ErrorContext ec(result ? 1 : 0); \
    Ast ast = parse(input, a, ec); \
    if (check(ast, ec) != result) { \
        fprintf(stderr, "static check test #%d failed.\n\n", tests); \
        failed += 1; \
    } \
}

#define TEST(input) TEST_RESULT(input, true)
#define TEST_FAIL(input) TEST_RESULT(input, false)

void run_static_check_tests()
{
    int tests = 0;
    int failed = 0;

    fprintf(stdout, "running static check tests...\n\n");

    TEST("42;")
    TEST("-42;")
    TEST("42 + 5;")
    TEST("true == false;");
    TEST("10 < 5;")
    TEST("int x;")
    TEST("int x; x + 13;")
    TEST("int x = 2; x * 7;")
    TEST("int x = 10; x < 5;")
    TEST("int x; { uint x = 5; }")
    TEST("{ int x = 5; } bool x = false;")
    TEST("function f() { } f();")
    TEST("function f() -> bool { return true; } f(); ")
    TEST("function f(int x) -> int { return x * 9; } int x = f(3) * 11;")
    TEST("function f(int x, int y) -> int { int x = 5; return x * y; } f(7, 9);")
    TEST("int x = 5; bool result = (x == 5);")
    TEST("int x = 25 / 7;"
        "bool result = false;"
        "if (x == 5) result = true;")
    TEST("uint x = 25 / 7;"
        "bool result;"
        "if (x != 5u) { x = 5; result = false; }"
        "else if (x != 10u) { x = 10; result = true; }")
    TEST("int x = 0; while (true) x = x + 1;");
    TEST("int x = 1; int i = 0; while (i < 10) { x = x + x; i = i + 1; }");
    TEST("5 < 10; 10u > 5u;");

    TEST_FAIL("x;")
    TEST_FAIL("x + 13;")
    TEST_FAIL("int x = x + 5;")
    TEST_FAIL("int x; int x = 5;")
    TEST_FAIL("{ int x = 5; } x * 5;")
    TEST_FAIL("true == 10")
    TEST_FAIL("true >= false")
    TEST_FAIL("true * false")
    TEST_FAIL("f();")
    TEST_FAIL("return 10;")
    TEST_FAIL("function f() -> int { return; }")
    TEST_FAIL("function f() -> int { return true; }")
    TEST_FAIL("function f() -> int { return 5; } f(10);")
    TEST_FAIL("function f(int x, int y) -> int { return 10; } f(5);")
    TEST_FAIL("function f() { int x = -31; } int x = f() * 11;")
    TEST_FAIL("bool result = false; if (5) result = true;")
    TEST_FAIL("if (true) x = 5;")
    TEST_FAIL("if (true) 5 + 5; else return 5;")
    TEST_FAIL("int x = 0; while (10) { int x = x + 1; }")
    TEST_FAIL("while (false) { int x = 0; x = x + y; }")
    TEST_FAIL("5 < 10u;")
    TEST_FAIL("function f() {} function f(int x) {}")

    fprintf(stdout, "------------------------------\n");
    fprintf(stdout, "ran %d static check tests: %d succeeded, %d failed.\n\n\n", tests, tests - failed, failed);
}

#undef TEST_RESULT
#undef TEST
#undef TEST_FAIL


//
// Parser tests
//

#define TEST_RESULT(input, result) { \
    tests += 1; \
    Alloc a; \
    ErrorContext ec(result ? 1 : 0); \
    Ast ast = parse(input, a, ec); \
    if (ast.valid != result) { \
        fprintf(stderr, "parser test #%d failed.\n\n", tests); \
        failed += 1; \
    } \
}

#define TEST(input) TEST_RESULT(input, true)
#define TEST_FAIL(input) TEST_RESULT(input, false)

void run_parser_tests()
{
    int tests = 0;
    int failed = 0;

    fprintf(stdout, "running parser tests...\n\n");

    TEST(";")
    TEST("1+1-2;")
    TEST("1+1-2*9/4;")
    TEST("1+1-2*9/(4+1);")
    TEST("1+1-2*9/(4+1)*-3;")
    TEST("9/(4+1)*-3 || !(2+6) && 0;")
    TEST("0==1 || 2!=3 && 4<5 && 7>6 && 8<=8 && 9>=8;")
    TEST("true == false;")
    TEST("asdf + x * 42;")
    TEST("f();")
    TEST("g(32 * (2 + x), false, y);")
    TEST("x = 30 + y;")
    TEST("int x = -1; uint y = 1; bool z;")
    TEST("return;")
    TEST("return 3*x*x + 2*x + 1;")
    TEST("if (x == false) f(5);")
    TEST("if (x == false) f(5); else f(10);")
    TEST("if (x == false) f(5); else if (y == true) f(10); else f(20);")
    TEST("while (i < 10) { x = x + f(i); i = i + 1; }")
    TEST("function f() { g(15*3 + 2); }")
    TEST("function f(int x) { g(15*x + 2); }")
    TEST("function f(int x, int y) -> int { if (x > y) return 1; else if (x < y) return -1; else return 0; }")

    TEST_FAIL("function f(); { g(15*3 + 2); }")
    TEST_FAIL("{if}")
    TEST_FAIL("1")
    TEST_FAIL("1+;")
    TEST_FAIL("if 5 > 4 ;")
    TEST_FAIL("if (3 & 1) ;")
    TEST_FAIL("while true {}")
    TEST_FAIL("((3 + 5) * 2;")
    TEST_FAIL("function g(int x) -> int;")
    TEST_FAIL("int uint32;")
    TEST_FAIL("3 = 4;")
    TEST_FAIL("1 != 2 && x = 4;")
    TEST_FAIL("{if(true){}else;")
    TEST_FAIL("{if(true){}else}")
    TEST_FAIL("int x,y,z;")
    TEST_FAIL("function h(3+5) {}")
    TEST_FAIL("function h(int 5) {}")
    TEST_FAIL("function h(int x, y) {}")

    fprintf(stdout, "------------------------------\n");
    fprintf(stdout, "ran %d parser tests: %d succeeded, %d failed.\n\n\n", tests, tests - failed, failed);
}

#undef TEST_RESULT
#undef TEST
#undef TEST_FAIL


//
// Lexer tests
//

template <int N>
int test_expected(const char *input, Token::Type (&expected_tokens)[N])
{
    Lexer lexer;
    lexer.reset(input);

    for (int i = 0; i < N; i++)
    {
        Token::Type expected = expected_tokens[i];
        Token::Type got = lexer.next_token().type;

        if (got != expected)
        {
            fprintf(stderr, "expected '%s', got '%s'\n", Token::get_str(expected), Token::get_str(got));
            return 1;
        }
    }
    return 0;
}

int test_invalid_token_at(const char *input, int line, int column, char c)
{
    Lexer lexer;
    lexer.reset(input);

    Token token = lexer.next_token();

    while (token.type != Token::END)
    {
        if (token.type == Token::INVALID)
        {
            if (token.line == line &&
                token.column == column &&
                token.c == c) break;
            return 1;
        }
        token = lexer.next_token();
    }

    while (token.type != Token::END)
    {
        token = lexer.next_token();
        if (token.type == Token::INVALID)
            return 1;
    }

    return 0;
}

#define TEST(test_func, input, ...) { \
    tests += 1; \
    if (test_func(input, ##__VA_ARGS__) != 0) { \
        fprintf(stderr, "lexer test #%d failed.\n\n", tests); \
        failed += 1; \
    } \
}

#define TT_LIST(tt) Token::tt
#define TT_LIST_2(tt, tt2) TT_LIST(tt), TT_LIST(tt2)
#define TT_LIST_4(tt, tt2, tt3, tt4) TT_LIST_2(tt, tt2), TT_LIST_2(tt3, tt4)
#define TT_LIST_8(tt, tt2, tt3, tt4, tt5, tt6, tt7, tt8) TT_LIST_4(tt, tt2, tt3, tt4), TT_LIST_4(tt5, tt6, tt7, tt8)

void run_lexer_tests()
{
    int tests = 0;
    int failed = 0;

    fprintf(stdout, "running lexer tests...\n\n");

    {
        Token::Type expected_tokens[] = {
            TT_LIST_8(NOT,STAR,SLASH,PLUS,MINUS,AMP,PIPE,EQ),
            TT_LIST_8(NE,LT,GT,LE,GE,AND,OR,ASSIGN),
            TT_LIST_8(ARROW,LPAREN,RPAREN,LBRACE,RBRACE,COMMA,SEMICOLON,IF),
            TT_LIST_8(ELSE,WHILE,FUNCTION,RETURN,INT,INT8,INT16,INT32),
            TT_LIST_8(INT64,UINT,UINT8,UINT16,UINT32,UINT64,BOOL,TRUE),
            TT_LIST_8(FALSE,IDENT,INT_LIT,UINT_LIT,INVALID,END,END,END)
        };

        TEST(test_expected,
             "!*/+-&|==!=<><=>=&&||= // single line comment\n"
             "->(){},;"  "if else while function return /* multi\n  line\n  comment */"
             "int int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 bool true false asdf 12345 6789u �",
             expected_tokens)
    }

    {
        Token::Type expected_tokens[] = {
            TT_LIST_8(IDENT,IDENT,IDENT,IDENT,IDENT,INT_LIT,IDENT,IDENT),Token::END
        };

        TEST(test_expected,
             "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ _ abcABC_1234567890 _abc123 123abc _abc123ABC",
             expected_tokens)
    }

    TEST(test_invalid_token_at,
         "asdf 56 + $- 8\n \tint32 x = y * 9841 + 463;\nqwer ||9 if\ntruefalse true false;",
         1, 11, '$')

    TEST(test_invalid_token_at,
         "asdf 56 + - 8\n \tint32 x = y * 9841 + 463;\nqwer ||9 if�\ntruefalse true false;",
         3, 12, '�')

    fprintf(stdout, "------------------------------\n");
    fprintf(stdout, "ran %d lexer tests: %d succeeded, %d failed.\n\n\n", tests, tests - failed, failed);
}


//
//
//

void run_tests()
{
    run_lexer_tests();
    run_parser_tests();
    run_static_check_tests();
    run_ir_gen_tests();
}
