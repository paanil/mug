#include "check.h"
#include "parser.h"
#include "alloc.h"
#include "error_context.h"

#include <cstdio>

//
// Type check tests
//

#define TEST_RESULT(input, result) { \
    tests += 1; \
    Alloc a; \
    ErrorContext ec(result ? 1 : 0); \
    Ast ast = parse(input, a, ec); \
    if (check(ast, ec) != result) { \
        fprintf(stderr, "type check test #%d failed.\n\n", tests); \
        failed += 1; \
    } \
}

#define TEST(input) TEST_RESULT(input, true)
#define TEST_FAIL(input) TEST_RESULT(input, false)

void run_type_check_tests()
{
    int tests = 0;
    int failed = 0;

    fprintf(stdout, "running type check tests...\n");

/*  1 */TEST("42;")
/*  2 */TEST("-42;")
/*  3 */TEST("42 + 5;")
/*  4 */TEST("true == false;");
/*  5 */TEST("10 < 5;")
/*  6 */TEST("int x;")
/*  7 */TEST("int x; x + 13;")
/*  8 */TEST("int x = 2; x * 7;")
/*  9 */TEST("int x = 10; x < 5;") // TODO: 5 -> signed, 5u -> unsigned
/* 10 */TEST("int x; { uint x = 5; }")
/* 11 */TEST("{ int x = 5; } bool x = false;")
/* 12 */TEST("function f() { }")
/* 13 */TEST("function f() -> bool { return true; }")
/* 14 */TEST("function f(int x) -> int { return x * 9; } int x = f(3) * 11;")
/* 15 */TEST("function f(int x) { int x = 5; return; }")
/* 16 */TEST_FAIL("x;")
/* 17 */TEST_FAIL("x + 13;")
/* 18 */TEST_FAIL("int x = x + 5;")
/* 19 */TEST_FAIL("int x; int x = 5;")
/* 20 */TEST_FAIL("{ int x = 5; } x * 5;")
/* 21 */TEST_FAIL("true == 10")
/* 22 */TEST_FAIL("true >= false")
/* 23 */TEST_FAIL("true * false")
/* 24 */TEST_FAIL("f();")
/* 25 */TEST_FAIL("return;")
/* 26 */TEST_FAIL("return 10;")
/* 27 */TEST_FAIL("function f() -> int {}")
/* 28 */TEST_FAIL("function f() -> int { return; }")
/* 29 */TEST_FAIL("function f() -> int { return true; }")
/* 30 */TEST_FAIL("function f() { int x = -31; } int x = f() * 11;")
/* 31 */TEST_FAIL("function f(int x) { return x * 9; } int x = f() * 11;")

    fprintf(stdout, "------------------------------\n");
    fprintf(stdout, "ran %d type check tests: %d succeeded, %d failed.\n\n\n", tests, tests - failed, failed);
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

    fprintf(stdout, "running parser tests...\n");

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

    fprintf(stdout, "running lexer tests...\n");

    {
        Token::Type expected_tokens[] = {
            TT_LIST_8(NOT,STAR,SLASH,PLUS,MINUS,AMP,PIPE,EQ),
            TT_LIST_8(NE,LT,GT,LE,GE,AND,OR,ASSIGN),
            TT_LIST_8(ARROW,LPAREN,RPAREN,LBRACE,RBRACE,COMMA,SEMICOLON,IF),
            TT_LIST_8(ELSE,WHILE,FUNCTION,RETURN,INT,INT8,INT16,INT32),
            TT_LIST_8(INT64,UINT,UINT8,UINT16,UINT32,UINT64,BOOL,TRUE),
            TT_LIST_8(FALSE,IDENT,CONST,INVALID,END,END,END,END)
        };

        TEST(test_expected,
             "!*/+-&|==!=<><=>=&&||= // single line comment\n"
             "->(){},;"  "if else while function return /* multi\n  line\n  comment */"
             "int int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 bool true false asdf 12345 �",
             expected_tokens)
    }

    {
        Token::Type expected_tokens[] = {
            TT_LIST_8(IDENT,IDENT,IDENT,IDENT,IDENT,CONST,IDENT,IDENT),Token::END
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
    run_type_check_tests();
}
