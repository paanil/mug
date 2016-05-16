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

    print_error("expected '%s'", Token::get_str(tt));
    return false;
}

void Parser::print_error(const char *message, const char *info)
{
    if (!error)
    {
        int line = token.line;
        int column = token.column;

        printf("error:%d:%d: ", line, column);
        printf(message, info);
        printf("\n");

        error = true;
    }
}

bool Parser::expected_operand_error(Token::Type for_op)
{
    print_error("expected operand for '%s'", Token::get_str(for_op));
    return false;
}

bool Parser::expected_error(const char *what)
{
    print_error("expected %s", what);
    return false;
}

//
// Parsing
//

bool Parser::parse(char *input)
{
    lexer.reset(input);
    get();
    get();

    current_node = 0;
    error = false;

    bool result = false;

    if (parse_top_level())
    {
        result = true;
    }

    return result;
}

bool Parser::parse_top_level()
{
    if (!parse_statements())
        return false;

    if (accept(Token::END))
        return true;

    print_error("unexpected token '%s'", Token::get_str(token.type));
    return false;
}

bool Parser::parse_statement()
{
    // empty statement
    if (accept(Token::SEMICOLON))
    {
        current_node = a.empty_node();
        return true;
    }

    // NOTE: Assign must be before expression!
    // assign statement
    if (peek() == Token::IDENT) // TODO: Not necessarily just ident.
                                // Could be dereferenced pointer for example.
    {
        if (look_ahead() == Token::ASSIGN)
        {
            int len = token.len;
            char *text = token.text;

            expect(Token::IDENT);
            expect(Token::ASSIGN);

            if (!parse_expression())
                return expected_error("expression after '='");

            if (!expect(Token::SEMICOLON))
                return false;

            current_node = a.assign_node(text, len, current_node);
            return true;
        }
    }

    // expression statement
    if (parse_expression())
    {
        if (!expect(Token::SEMICOLON))
            return false;

        current_node = a.expr_node(current_node);
        return true;
    }

    // variable declaration
    Token type = token;
    if (parse_type())
    {
        Token ident = token;

        if (!expect(Token::IDENT))
            return false;

        Node *init = 0;

        // variable init
        if (accept(Token::ASSIGN))
        {
            if (!parse_expression())
                return expected_error("expression after '='");

            init = current_node;
        }

        if (!expect(Token::SEMICOLON))
            return false;

        current_node = a.decl_node(type.type, ident.text, ident.len, init);
        return true;
    }

    // return statement
    if (accept(Token::RETURN))
    {
        Node *ret_val = 0;

        if (peek() != Token::SEMICOLON)
        {
            if (!parse_expression())
                return expected_error("return value");

            ret_val = current_node;
        }

        if (!expect(Token::SEMICOLON))
            return false;

        current_node = a.return_node(ret_val);
        return true;
    }

    // if statement
    if (accept(Token::IF))
    {
        if (!expect(Token::LPAREN))
            return false;

        if (!parse_expression())
            return expected_error("condition in parentheses");

        if (!expect(Token::RPAREN))
            return false;

        Node *condition = current_node;

        if (!parse_statement())
            return expected_error("statement for if");

        Node *true_stmt = current_node;
        Node *else_stmt = 0;

        // else
        if (accept(Token::ELSE))
        {
            if (!parse_statement())
                return expected_error("else statement");

            else_stmt = current_node;
        }

        current_node = a.if_node(condition, true_stmt, else_stmt);
        return true;
    }

    // while statement
    if (accept(Token::WHILE))
    {
        if (!expect(Token::LPAREN))
            return false;

        if (!parse_expression())
            return expected_error("condition in parentheses");

        if (!expect(Token::RPAREN))
            return false;

        Node *condition = current_node;

        if (!parse_statement())
            return expected_error("statement for while");

        current_node = a.while_node(condition, current_node);
        return true;
    }

    // statement block
    if (accept(Token::LBRACE))
    {
        if (!parse_statements())
            return false;

        if (!expect(Token::RBRACE))
            return false;

        return true;
    }

    // function definition
    if (accept(Token::FUNCTION))
    {
        Token ident = token;

        if (!expect(Token::IDENT))
            return false;

        if (!expect(Token::LPAREN))
            return false;

        ParamList params = {};

        if (!accept(Token::RPAREN))
        {
            if (!parse_parameters(params))
                return false;

            if (!expect(Token::RPAREN))
                return false;
        }

        Token ret_type = {};

        if (accept(Token::ARROW))
        {
            ret_type = token;

            if (!parse_type())
                return expected_error("return type after '->'");
        }

        if (!expect(Token::LBRACE))
            return false;

        if (!parse_statements())
            return false;

        if (!expect(Token::RBRACE))
            return false;

        current_node = a.func_def_node(ret_type.type, ident.text, ident.len, params.next, current_node);
        return true;
    }

    return false;
}

bool Parser::parse_type()
{
    if (Token::is_type(peek()))
    {
        get();
        return true;
    }
    return false;
}

bool Parser::parse_statements()
{
    StmtList stmts = {};
    StmtList *prev = &stmts;

    while (parse_statement())
    {
        StmtList *stmt = a.alloc_stmt();
        stmt->stmt = current_node;
        stmt->next = 0;
        prev->next = stmt;
        prev = stmt;
    }

    if (error)
        return false;

    current_node = a.block_node(stmts.next);
    return true;
}

bool Parser::parse_parameters(ParamList &params)
{
    if (!Token::is_type(peek()))
        return true;

    ParamList *prev = &params;

    do
    {
        Token type = token;

        if (!parse_type())
            return expected_error("parameter type after ','");

        Token ident = token;

        if (!expect(Token::IDENT))
            return false;

        ParamList *param = a.alloc_param();
        param->type = type.type;
        param->name = a.make_ident(ident.text, ident.len);
        param->next = 0;
        prev->next = param;
        prev = param;
    } while (accept(Token::COMMA));

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
        Node *left = current_node; \
        if (!parse_operand_func()) return expected_operand_error(op); \
        current_node = a.binary_node(left, current_node, op); \
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
    Token::Type op = token.type;

    if (accept(Token::NOT) ||
        accept(Token::MINUS))
    {
        if (!parse_factor())
            return false;

        current_node = a.unary_node(current_node, op);
        return true;
    }

    return parse_factor();
}

bool Parser::parse_factor()
{
    Token tmp = token;

    if (accept(Token::TRUE))
    {
        current_node = a.bool_node(true);
        return true;
    }
    if (accept(Token::FALSE))
    {
        current_node = a.bool_node(false);
        return true;
    }

    if (accept(Token::CONST))
    {
        current_node = a.const_node(tmp.value);
        return true;
    }

    if (accept(Token::IDENT))
    {
        if (accept(Token::LPAREN))
        {
            ArgList args = {};

            if (!accept(Token::RPAREN))
            {
                if (!parse_arguments(args))
                    return false;

                if (!expect(Token::RPAREN))
                    return false;
            }

            current_node = a.call_node(tmp.text, tmp.len, args.next);
            return true;
        }

        current_node = a.var_node(tmp.text, tmp.len);
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

bool Parser::parse_arguments(ArgList &args)
{
    ArgList *prev = &args;

    do
    {
        if (!parse_expression())
            return expected_error("argument");

        ArgList *arg = a.alloc_arg();
        arg->arg = current_node;
        arg->next = 0;
        prev->next = arg;
        prev = arg;

    } while (accept(Token::COMMA));

    return true;
}


//
// Parser tests
//

#define TEST_RESULT(input, result) \
    { Alloc a; char in[] = input; tests += 1; Parser p(a); \
      if (p.parse(in) != result) { printf("parser test #%d failed.\n", tests); failed += 1; } }

#define TEST(input) TEST_RESULT(input, true)
#define TEST_FAIL(input) TEST_RESULT(input, false)

void run_parser_tests()
{
    int tests = 0;
    int failed = 0;

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
    // TODO: More test cases that should fail.
    // TODO: Parser shouldn't automatically print errors (?)

    printf("ran %d parser tests: %d succeeded, %d failed.\n", tests, tests - failed, failed);
}
