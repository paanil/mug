#include "parser.h"
#include "lexer.h"
#include "ast_alloc.h"
#include "error_context.h"

struct Parser
{
    Lexer lexer;
    Token token;
    Token next_token;
    AstAlloc a;
    ErrorContext &ec;
    bool error;

    Parser(Alloc &a_, ErrorContext &ec_)
    : a(a_)
    , ec(ec_)
    {}

    void print_error(const char *message, const char *info)
    {
        ec.print_error(token.line, token.column, message, info);
        error = true;
    }
    void expected_operand_error(Token::Type for_op)
    {
        print_error("expected operand for '%s'", Token::get_str(for_op));
    }
    void expected_error(const char *what)
    {
        print_error("expected %s", what);
    }

    Token::Type look_ahead()
    {
        return next_token.type;
    }
    Token::Type peek()
    {
        return token.type;
    }
    Token::Type get()
    {
        token = next_token;
        next_token = lexer.next_token();
        return token.type;
    }

    bool accept(Token::Type tt)
    {
        if (peek() == tt)
        {
            get();
            return true;
        }
        return false;
    }
    bool expect(Token::Type tt)
    {
        if (accept(tt))
            return true;

        print_error("expected '%s'", Token::get_str(tt));
        return false;
    }

    Ast parse(const char *input);

    Node *parse_top_level();

    Node *parse_statement();
    Node *parse_function_def();
    Node *parse_statements();
    bool parse_type(Type *result);
    bool parse_parameters(ParamList &params);

    Expression *parse_expression();
    Expression *parse_and();
    Expression *parse_comparison();
    Expression *parse_sum();
    Expression *parse_term();
    Expression *parse_prefixed_factor();
    Expression *parse_factor();
    bool parse_arguments(ArgList &args);
};


Ast Parser::parse(const char *input)
{
    lexer.reset(input);
    get();
    get();

    error = false;

    Ast result;
    result.root = parse_top_level();
    result.valid = result.root != nullptr;
    return result;
}

Node *Parser::parse_top_level()
{
    StmtList stmts = {};
    StmtList *prev = &stmts;

    while (true)
    {
        Node *s = parse_statement();
        if (s == nullptr)
        {
            if (error) break;

            s = parse_function_def();
            if (s == nullptr) break;
        }

        StmtList *stmt = a.alloc_stmt();
        stmt->stmt = s;
        stmt->next = nullptr;
        prev->next = stmt;
        prev = stmt;
    }

    if (error)
        return nullptr;

    if (accept(Token::END))
        return a.block_node(stmts.next);

    print_error("unexpected token '%s'", Token::get_str(token.type));
    return nullptr;
}

Node *Parser::parse_statement()
{
    // empty statement
    if (accept(Token::SEMICOLON))
        return a.empty_node();

    // NOTE: Assign must be before expression!
    // assign statement
    if (peek() == Token::IDENT) // TODO: Not necessarily just ident.
                                // Could be dereferenced pointer for example.
    {
        if (look_ahead() == Token::ASSIGN)
        {
            Str ident = token.text;

            expect(Token::IDENT);
            expect(Token::ASSIGN);

            Expression *value = parse_expression();
            if (value == nullptr)
            {
                expected_error("expression after '='");
                return nullptr;
            }

            if (!expect(Token::SEMICOLON))
                return nullptr;

            return a.assign_node(ident, value);
        }
    }

    // expression statement
    Expression *exp = parse_expression();
    if (exp != nullptr)
    {
        if (!expect(Token::SEMICOLON))
            return nullptr;

        return a.exp_node(exp);
    }

    // variable declaration
    Type type;
    if (parse_type(&type))
    {
        Str ident = token.text;

        if (!expect(Token::IDENT))
            return nullptr;

        Expression *init = nullptr;

        // variable init
        if (accept(Token::ASSIGN))
        {
            init = parse_expression();
            if (init == nullptr)
            {
                expected_error("expression after '='");
                return nullptr;
            }
        }

        if (!expect(Token::SEMICOLON))
            return nullptr;

        return a.decl_node(type, ident, init);
    }

    // return statement
    if (accept(Token::RETURN))
    {
        Expression *ret_val = nullptr;

        if (peek() != Token::SEMICOLON)
        {
            ret_val = parse_expression();
            if (ret_val == nullptr)
            {
                expected_error("return value");
                return nullptr;
            }
        }

        if (!expect(Token::SEMICOLON))
            return nullptr;

        return a.return_node(ret_val);
    }

    // if statement
    if (accept(Token::IF))
    {
        if (!expect(Token::LPAREN))
            return nullptr;

        Expression *condition = parse_expression();
        if (condition == nullptr)
        {
            expected_error("condition in parentheses");
            return nullptr;
        }

        if (!expect(Token::RPAREN))
            return nullptr;

        Node *true_stmt = parse_statement();
        if (true_stmt == nullptr)
        {
            expected_error("statement for if");
            return nullptr;
        }

        Node *else_stmt = nullptr;

        // else
        if (accept(Token::ELSE))
        {
            else_stmt = parse_statement();
            if (else_stmt == nullptr)
            {
                expected_error("else statement");
                return nullptr;
            }
        }

        return a.if_node(condition, true_stmt, else_stmt);
    }

    // while statement
    if (accept(Token::WHILE))
    {
        if (!expect(Token::LPAREN))
            return nullptr;

        Expression *condition = parse_expression();
        if (condition == nullptr)
        {
            expected_error("condition in parentheses");
            return nullptr;
        }

        if (!expect(Token::RPAREN))
            return nullptr;

        Node *stmt = parse_statement();
        if (stmt == nullptr)
        {
            expected_error("statement for while");
            return nullptr;
        }

        return a.while_node(condition, stmt);
    }

    // statement block
    if (accept(Token::LBRACE))
    {
        Node *statements = parse_statements();
        if (statements == nullptr)
            return nullptr;

        if (!expect(Token::RBRACE))
            return nullptr;

        return statements;
    }

    return nullptr;
}

Node *Parser::parse_function_def()
{
    bool external = accept(Token::EXTERN);

    if (accept(Token::FUNCTION))
    {
        Str ident = token.text;

        if (!expect(Token::IDENT))
            return nullptr;

        if (!expect(Token::LPAREN))
            return nullptr;

        ParamList params = {};

        if (!accept(Token::RPAREN))
        {
            if (!parse_parameters(params))
                return nullptr;

            if (!expect(Token::RPAREN))
                return nullptr;
        }

        Type ret_type = {};

        if (accept(Token::ARROW))
        {
            if (!parse_type(&ret_type))
            {
                expected_error("return type after '->'");
                return nullptr;
            }
        }

        if (external)
        {
            if (!expect(Token::SEMICOLON))
                return nullptr;

            // hack: if body == nullptr, the function is external
            return a.func_def_node(ret_type, ident, params.next, nullptr);
        }

        if (!expect(Token::LBRACE))
            return nullptr;

        Node *statements = parse_statements();
        if (statements == nullptr)
            return nullptr;

        if (!expect(Token::RBRACE))
            return nullptr;

        return a.func_def_node(ret_type, ident, params.next, statements);
    }
    else if (external)
    {
        expected_error("function declaration after 'extern'");
        return nullptr;
    }

    return nullptr;
}

Node *Parser::parse_statements()
{
    StmtList stmts = {};
    StmtList *prev = &stmts;

    for (Node *s; (s = parse_statement()) != nullptr; )
    {
        StmtList *stmt = a.alloc_stmt();
        stmt->stmt = s;
        stmt->next = nullptr;
        prev->next = stmt;
        prev = stmt;
    }

    if (error)
        return nullptr;

    return a.block_node(stmts.next);
}

bool Parser::parse_type(Type *result)
{
    switch (peek())
    {
    case Token::INT...Token::INT64:
        result->type = Type::INT; break;
    case Token::UINT...Token::UINT64:
        result->type = Type::UINT; break;
    case Token::BOOL:
        result->type = Type::BOOL; break;
    default:
        return false;
    }

    get();
    return true;
}

bool Parser::parse_parameters(ParamList &params)
{
    Token::Type tt = peek();
    if (tt < Token::INT || Token::BOOL < tt)
        return true;

    ParamList *prev = &params;

    do
    {
        Type type = {};
        if (!parse_type(&type))
        {
            expected_error("parameter type after ','");
            return false;
        }

        Str ident = token.text;
        if (!expect(Token::IDENT))
            return false;

        ParamList *param = a.alloc_param();
        param->type = type;
        param->name = a.push_str(ident);
        param->next = nullptr;
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
Expression *Parser::func_name() { \
    Expression *left = parse_operand_func(); \
    if (left == nullptr) return nullptr; \
    while (true) { \
        Token::Type op = token.type; \
        if (!accept_any(this, __VA_ARGS__)) return left; \
        Expression *right = parse_operand_func(); \
        if (right == nullptr) { \
            expected_operand_error(op); \
            return nullptr; \
        } \
        left = a.binary_exp(left, right, op); \
    } \
}

PARSE_BINOP(parse_expression, parse_and, Token::OR)
PARSE_BINOP(parse_and, parse_comparison, Token::AND)
PARSE_BINOP(parse_comparison, parse_sum, Token::EQ, Token::NE, Token::LT, Token::GT, Token::LE, Token::GE)
PARSE_BINOP(parse_sum, parse_term, Token::PLUS, Token::MINUS)
PARSE_BINOP(parse_term, parse_prefixed_factor, Token::STAR, Token::SLASH)

//
//
//

Expression *Parser::parse_prefixed_factor()
{
    Token::Type op = token.type;

    if (accept(Token::NOT) ||
        accept(Token::MINUS))
    {
        Expression *operand = parse_factor();
        if (operand == nullptr)
        {
            expected_operand_error(op);
            return nullptr;
        }

        return a.unary_exp(operand, op);
    }

    return parse_factor();
}

Expression *Parser::parse_factor()
{
    if (accept(Token::TRUE))
        return a.bool_exp(true);
    if (accept(Token::FALSE))
        return a.bool_exp(false);

    uint64_t value = token.value;
    if (accept(Token::INT_LIT))
        return a.const_exp(value, Type::INT);
    if (accept(Token::UINT_LIT))
        return a.const_exp(value, Type::UINT);

    Str ident = token.text;
    if (accept(Token::IDENT))
    {
        if (accept(Token::LPAREN))
        {
            ArgList args = {};

            if (!accept(Token::RPAREN))
            {
                if (!parse_arguments(args))
                    return nullptr;

                if (!expect(Token::RPAREN))
                    return nullptr;
            }

            return a.call_exp(ident, args.next);
        }

        return a.var_exp(ident);
    }

    if (accept(Token::LPAREN))
    {
        Expression *exp = parse_expression();
        if (exp == nullptr)
        {
            expected_error("expression in parentheses");
            return nullptr;
        }

        return expect(Token::RPAREN) ? exp : nullptr;
    }

    return nullptr;
}

bool Parser::parse_arguments(ArgList &args)
{
    ArgList *prev = &args;

    do
    {
        Expression *argument = parse_expression();
        if (argument == nullptr)
        {
            expected_error("argument");
            return false;
        }

        ArgList *arg = a.alloc_arg();
        arg->arg = argument;
        arg->next = nullptr;
        prev->next = arg;
        prev = arg;

    } while (accept(Token::COMMA));

    return true;
}


//
//
//

Ast parse(const char *input, Alloc &a, ErrorContext &ec)
{
    Parser p(a, ec);
    return p.parse(input);
}
