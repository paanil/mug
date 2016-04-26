#include "parser.h"

#include <cstdio>


//
// Parser
//

Token::Type Parser::look_ahead()
{
    return next_token.type;
}

Token::Type Parser::peek()
{
    return token.type;
}

Token::Type Parser::get()
{
    token = next_token;
    next_token = lexer.next_token();
    return token.type;
}

bool Parser::accept(Token::Type tt)
{
    if (peek() == tt)
    {
        get();
        return true;
    }
    return false;
}

bool Parser::expect(Token::Type tt)
{
    if (accept(tt))
        return true;

    int line = token.line;
    int column = token.column;
    const char *s = Token::get_str(tt);

    printf("error:%d:%d: expected '%s'\n", line, column, s);

    return false;
}

bool Parser::expected_operand_error(Token::Type for_op)
{
    int line = token.line;
    int column = token.column;
    const char *s = Token::get_str(for_op);

    printf("error:%d:%d: expected operand for '%s'\n", line, column, s);

    return false;
}

bool Parser::expected_error(const char *what)
{
    int line = token.line;
    int column = token.column;

    printf("error:%d:%d: expected %s\n", line, column, what);

    return false;
}

bool Parser::parse(char *input)
{
    lexer = Lexer(input);
    get();
    get();

    return parse_top_level();
}

bool Parser::parse_top_level()
{
    while (parse_statement()) ;

    if (!accept(Token::END))
    {
        int line = token.line;
        int column = token.column;
        const char *s = Token::get_str(token.type);

        printf("error:%d:%d: unexpected token '%s'\n", line, column, s);

        return false;
    }

    return true;
}

bool Parser::parse_statement()
{
    // empty statement
    if (accept(Token::SEMICOLON))
    {
        return true;
    }

    // assign statement
    if (peek() == Token::IDENT) // TODO: Not necessarily just ident.
                                // Could be dereferenced pointer for example.
    {
        if (look_ahead() == Token::ASSIGN)
        {
//                Ident ident = Ident(token.text, token.len);
            expect(Token::IDENT);
            expect(Token::ASSIGN);

            if (!parse_expression())
                return expected_error("expression for assignment");

            if (!expect(Token::SEMICOLON))
                return false;

            return true;
        }
    }

    // expression statement
    if (parse_expression())
    {
        if (expect(Token::SEMICOLON))
            return true;
        return false;
    }

    // variable declaration statement
    if (parse_type())
    {
        if (!expect(Token::IDENT))
            return false;

        // variable init
        if (accept(Token::ASSIGN))
        {
            if (!parse_expression())
                return expected_error("expression for assignment");
        }

        if (!expect(Token::SEMICOLON))
            return false;

        return true;
    }

    // return statement
    if (accept(Token::RETURN))
    {
        if (peek() != Token::SEMICOLON)
        {
            if (!parse_expression())
                return expected_error("expression for return");
        }

        if (!expect(Token::SEMICOLON))
            return false;

        return true;
    }

    // if statement
    if (accept(Token::IF))
    {
        if (!parse_expression())
            return expected_error("test expression for if");

        if (!parse_statement())
            return expected_error("statement for if");

        // else
        if (accept(Token::ELSE))
        {
            if (!parse_statement())
                return expected_error("statement for else");
        }

        return true;
    }

    // while statement
    if (accept(Token::WHILE))
    {
        if (!parse_expression())
            return expected_error("test expression for while");

        if (!parse_statement())
            return expected_error("statement for while");

        return true;
    }

    // statement block
    if (accept(Token::LBRACE))
    {
        while (parse_statement()) ;

        if (!expect(Token::RBRACE))
            return false;

        return true;
    }

    // function definition
    if (accept(Token::FUNCTION))
    {
        if (!expect(Token::IDENT))
            return false;

        if (!expect(Token::LPAREN))
            return false;

        if (!accept(Token::RPAREN))
        {
            if (!parse_parameters())
                return false;

            if (!expect(Token::RPAREN))
                return false;
        }

        if (accept(Token::ARROW))
        {
            if (!parse_type())
                return expected_error("return type after '->'");
        }

        if (!expect(Token::LBRACE))
            return false;

        while (parse_statement()) ;

        if (!expect(Token::RBRACE))
            return false;

        return true;
    }

    return false;
}

bool Parser::parse_type()
{
    return (accept(Token::INT) ||
            accept(Token::UINT) ||
            accept(Token::BOOL));
}

bool Parser::parse_parameters()
{
    if (!parse_type())
        return true;

    if (!expect(Token::IDENT))
        return false;

    while (accept(Token::COMMA))
    {
        if (!parse_type())
            return expected_error("parameter type after ','");

        if (!expect(Token::IDENT))
            return false;
    }

    return true;
}


//
// Binary ops
//

inline bool accept_any(Parser *p, Token::Type tt)
{ return p->accept(tt); }
template <class... Args>
inline bool accept_any(Parser *p, Token::Type tt, Args... args)
{ return (p->accept(tt) || accept_any(p, args...)); }

#define PARSE_BINOP(func_name, parse_operand_func, ...) \
bool Parser::func_name() { \
    if (!parse_operand_func()) return false; \
    while (true) { \
        Token::Type op = token.type; \
        if (!accept_any(this, __VA_ARGS__)) return true; \
        if (!parse_operand_func()) return expected_operand_error(op); \
    } \
}

PARSE_BINOP(parse_expression, parse_and, Token::OR)
PARSE_BINOP(parse_and, parse_comparison, Token::AND)
PARSE_BINOP(parse_comparison, parse_sum, Token::EQ, Token::NE, Token::LT, Token::GT, Token::LE, Token::GE)
PARSE_BINOP(parse_sum, parse_term, Token::PLUS, Token::MINUS)
PARSE_BINOP(parse_term, parse_prefixed_factor, Token::STAR, Token::SLASH)


//
// Factors
//

bool Parser::parse_prefixed_factor()
{
    if (accept(Token::NOT)) ;
    else if (accept(Token::MINUS)) ;

    if (!parse_factor())
        return false;

    return true;
}

bool Parser::parse_factor()
{
//        Token tmp = token;

    if (accept(Token::TRUE))
    {
//            bool value = true;
        return true;
    }
    if (accept(Token::FALSE))
    {
//            bool value = false;
        return true;
    }

    if (accept(Token::CONST))
    {
//            uint64_t value = tmp.value;
        return true;
    }

    if (accept(Token::IDENT))
    {
//            Ident ident = Ident(tmp.text, tmp.len);
        if (accept(Token::LPAREN))
        {
            if (accept(Token::RPAREN))
                return true;

            if (!parse_arguments())
                return false;

            if (!expect(Token::RPAREN))
                return false;

            return true;
        }
        return true;
    }

    if (accept(Token::LPAREN))
    {
        if (!parse_expression())
            return expected_error("expression in parentheses");

        return expect(Token::RPAREN);
    }

    return false;
}

bool Parser::parse_arguments()
{
    do
    {
        if (!parse_expression())
            return false;
    } while (accept(Token::COMMA));

    return true;
}


//
// Parser tests
//

#define TEST_RESULT(input, result) \
    { char in[] = input; tests += 1; Parser p; \
      if (p.parse(in) != result) { printf("parser test #%d failed.\n", tests); failed += 1; } }

#define TEST(input) TEST_RESULT(input, true)
#define TEST_FAIL(input) TEST_RESULT(input, false)

void run_parser_tests()
{
    int tests = 0;
    int failed = 0;

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
    TEST("return 3*x*x + 2*x + 1;")
    TEST("if x == false f(5);")
    TEST("if x == false f(5); else f(10);")
    TEST("if x == false f(5); else if y == true f(10); else f(20);")
    TEST("while i < 10 { x = x + f(i); i = i + 1; }")
    TEST("function f() { g(15*3 + 2); }")
    TEST("function f(int x) { g(15*x + 2); }")
    TEST("function f(int x, int y) -> int { if (x > y) return 1; else if (x < y) return -1; else return 0; }")

    TEST_FAIL("function f(); { g(15*3 + 2); }")
    // TODO: More test cases that should fail.
    // TODO: Parser shouldn't automatically print errors (?)

    printf("ran %d parser tests: %d succeeded, %d failed.\n", tests, tests - failed, failed);
}
