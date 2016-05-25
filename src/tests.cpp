#include "ast.h"
#include "sym_table.h"
#include "alloc.h"

#include <cstdio>
#include <cassert>

#define PASTE_TYPES     \
    PASTE_TYPE(MOV_IM)  \
    PASTE_TYPE(MOV)     \
    PASTE_TYPE(MUL)     \
    PASTE_TYPE(IMUL)    \
    PASTE_TYPE(ADD)     \
    PASTE_TYPE(SUB)     \
    PASTE_TYPE(EQ)      \
    PASTE_TYPE(LT)      \
    PASTE_TYPE(JMP)     \
    PASTE_TYPE(JZ)      \
    PASTE_TYPE(CALL)    \
    PASTE_TYPE(RET)     \
    PASTE_TYPE(ARG)

struct IR
{
#define PASTE_TYPE(x) x,

    enum Type
    {
        PASTE_TYPES
    };

#undef PASTE_TYPE

#define PASTE_TYPE(x) #x,

    static const char *get_str(Type type)
    {
        static const char *ir_str[] =
        {
            PASTE_TYPES
        };

        return ir_str[type];
    }

#undef PASTE_TYPE

    struct Routine *routines;
};

// TODO: func id, arg index

struct Temp
{
    uint32_t id;
};

union Operand
{
    Temp temp;
    uint32_t jump; // TODO: Label.
    uint64_t int_value;
};

struct Quad
{
    IR::Type op;
    Operand target;
    Operand left;
    Operand right;

    Quad()
    {}

    Quad(IR::Type op_)
    : op(op_)
    {}

    Quad(IR::Type op_, Operand target_)
    : op(op_)
    , target(target_)
    {}

    Quad(IR::Type op_, Operand target_, Operand operand)
    : op(op_)
    , target(target_)
    , left(operand)
    {}

    Quad(IR::Type op_, Operand target_, Operand left_, Operand right_)
    : op(op_)
    , target(target_)
    , left(left_)
    , right(right_)
    {}
};

struct Quads
{
    static const uint32_t N = 64;
    Quad quads[N];
    Quads *next;
};

struct Routine
{
    uint32_t next_temp_id;

    uint32_t n;
    Quads *head;
    Quads *tail;

    Str name;
    uint32_t id;
    Routine *next;

    Routine(Str name_, uint32_t id_)
    : next_temp_id()
    , n()
    , head()
    , tail()
    , name(name_)
    , id(id_)
    , next()
    {}

    Operand make_temp()
    {
        Operand result;
        result.temp.id = next_temp_id++;
        return result;
    }

    Operand make_jump_target()
    {
        Operand result;
        result.jump = n;
        return result;
    }

    Quad *add(Quad quad, Alloc &a)
    {
        uint32_t m = n % Quads::N;

        if (m == 0)
        {
            Quads *qs = a.allocate<Quads>();
            qs->next = nullptr;
            if (head == nullptr)
                 head = qs;
            else tail->next = qs;
            tail = qs;
        }

        Quad *q = tail->quads + m;
        *q = quad; n++;
        return q;
    }

    void set_jump_target_here(Quad *quad)
    {
        quad->target.jump = n;
    }

    Quad &operator [] (uint32_t index)
    {
        assert(index < n);
        Quads *qs = head;
        uint32_t m = index / Quads::N;
        while (m --> 0) // haha
            qs = qs->next;
        return qs->quads[index % Quads::N];
    }
};

struct IRGen
{
    uint32_t next_routine_id;
    Routine *tail;
    SymTable<Temp> sym;
    Alloc &a;

    IRGen(Alloc &a_)
    : next_routine_id()
    , tail()
    , a(a_)
    {}

    Routine *make_routine(Str name)
    {
        Routine *r = a.allocate<Routine>();
        *r = Routine(name, next_routine_id++);
        if (tail != nullptr)
            tail->next = r;
        tail = r;
        return r;
    }

    Operand gen_ir(Routine &r, Expression *exp)
    {
        switch (exp->type)
        {
            case ExpType_BOOL:
            {
                Operand result = r.make_temp();
                Operand operand;
                operand.int_value = exp->boolean.value ? 1 : 0;
                r.add(Quad(IR::MOV_IM, result, operand), a);
                return result;
            }

            case ExpType_CONST:
            {
                Operand result = r.make_temp();
                Operand operand;
                operand.int_value = exp->constant.value;
                r.add(Quad(IR::MOV_IM, result, operand), a);
                return result;
            }

            case ExpType_VAR:
            {
                Operand result;
                result.temp = sym.get(exp->var.name);
                return result;
            }

            case ExpType_CALL:
            {
                ArgList *arg = exp->call.args;
                for (uint32_t index = 0; arg; index++)
                {
                    Operand arg_idx;
                    arg_idx.temp.id = index;
                    Operand value = gen_ir(r, arg->arg);
                    r.add(Quad(IR::ARG, arg_idx, value), a);
                    arg = arg->next;
                }

                Operand result = r.make_temp();
                Operand routine;
                routine.temp = sym.get(exp->call.func_name);
                r.add(Quad(IR::CALL, result, routine), a);
                return result;
            }

            case ExpType_UNARY:
            {
                switch (exp->unary.op)
                {
//                    case UnaryOp_NOT:
//                    {
//
//                    }

                    case UnaryOp_NEG:
                    {
                        Operand right = gen_ir(r, exp->unary.operand);
                        Operand left = r.make_temp();
                        Operand result = r.make_temp();

                        Operand zero;
                        zero.int_value = 0;
                        r.add(Quad(IR::MOV_IM, left, zero), a);

                        r.add(Quad(IR::SUB, result, left, right), a);
                        return result;
                    }

                    assert(0 && "invalid code path!");
                    break;
                }
            }

            case ExpType_BINARY:
            {
                Operand left = gen_ir(r, exp->binary.left);
                Operand right = gen_ir(r, exp->binary.right);
                Operand result = r.make_temp();

                switch (exp->binary.op)
                {
                    case BinaryOp_MUL:
                        if (exp->data_type.type == Type::INT)
                            r.add(Quad(IR::IMUL, result, left, right), a);
                        else
                            r.add(Quad(IR::MUL, result, left, right), a);
                        break;
                    case BinaryOp_ADD:
                        r.add(Quad(IR::ADD, result, left, right), a);
                        break;
                    case BinaryOp_SUB:
                        r.add(Quad(IR::SUB, result, left, right), a);
                        break;
                    case BinaryOp_EQ:
                        r.add(Quad(IR::EQ, result, left, right), a);
                        break;
                    case BinaryOp_LT:
                        r.add(Quad(IR::LT, result, left, right), a);
                        break;
                }

                return result;
            }
        }
    }

    void gen_ir(Routine &r, Node *node)
    {
        switch (node->type)
        {
            case NodeType_EMPTY:
                break;
            case NodeType_EXP:
                gen_ir(r, node->exp.exp);
                break;

            case NodeType_ASSIGN:
            {
                Operand var;
                var.temp = sym.get(node->assign.var_name);
                Operand value = gen_ir(r, node->assign.value);
                // TODO: If gen_ir(exp) always generates a quad with a new temp as target,
                // we could replace that quad's target with the var and not add this mov quad.
                // By doing that, we wouldn't get useless movs here, BUT
                // think about expressions that use vars e.g. "(x + y) * z"!
                // They would generate unnecessary movs.
                r.add(Quad(IR::MOV, var, value), a);
                break;
            }

            case NodeType_DECL:
            {
                if (node->decl.init)
                {
                    Operand init = gen_ir(r, node->decl.init);
                    sym.put(node->decl.var_name, init.temp);
                }
                else
                {
                    // TODO: Is this even necessary?
                    // We could sym.put(var_name, temp) when we first assign a value, BUT
                    // what if var is used without initializing or assigning?
                    Operand var = r.make_temp();
                    sym.put(node->decl.var_name, var.temp);
                }
                break;
            }

            case NodeType_RETURN:
            {
                Operand flag, value;
                flag.int_value = false; // a hack: if false, returns nothing
                if (node->ret.value)
                {
                    flag.int_value = true; // returns something
                    value = gen_ir(r, node->ret.value);
                }
                r.add(Quad(IR::RET, flag, value), a);
                break;
            }

            case NodeType_IF:
            {
                Operand dummy_target;
                Operand condition = gen_ir(r, node->if_stmt.condition);
                Quad *jz_quad = r.add(Quad(IR::JZ, dummy_target, condition), a);
                gen_ir(r, node->if_stmt.true_stmt);
                if (node->if_stmt.else_stmt)
                {
                    Quad *jmp_quad = r.add(Quad(IR::JMP), a);
                    r.set_jump_target_here(jz_quad);
                    gen_ir(r, node->if_stmt.else_stmt);
                    r.set_jump_target_here(jmp_quad);
                }
                else
                {
                    r.set_jump_target_here(jz_quad);
                }
                break;
            }

            case NodeType_WHILE:
            {
                Operand jump_target = r.make_jump_target();
                Operand condition = gen_ir(r, node->while_stmt.condition);
                Quad *jz_quad = r.add(Quad(IR::JZ, {}, condition), a);
                gen_ir(r, node->while_stmt.stmt);
                r.add(Quad(IR::JMP, jump_target), a);
                r.set_jump_target_here(jz_quad);
                break;
            }

            case NodeType_BLOCK:
            {
                sym.enter_scope();

                StmtList *s = node->block.stmts;
                while (s)
                {
                    gen_ir(r, s->stmt);
                    s = s->next;
                }

                sym.exit_scope();
                break;
            }

            case NodeType_FUNC_DEF:
            {
                Routine *routine = make_routine(node->func_def.name);
                Temp temp; // TODO: Routine id or...?
                temp.id = routine->id;
                sym.put(node->func_def.name, temp);

                sym.enter_scope();

                for (ParamList *p = node->func_def.params; p; p = p->next)
                {
                    Operand param = routine->make_temp();
                    sym.put(p->name, param.temp);
                }

                gen_ir(*routine, node->func_def.body);

                Operand flag;
                flag.int_value = false; // a hack: if false, return nothing
                routine->add(Quad(IR::RET, flag), a);

                sym.exit_scope();
                break;
            }
        }
    }

    IR gen_ir(Ast ast)
    {
        Routine *top_level = make_routine(Str::make("@top_level"));
        gen_ir(*top_level, ast.root);

        IR ir;
        ir.routines = top_level;
        return ir;
    }
};

void print_ir(Routine &r)
{
    printf("\n%s#%u:\n", r.name.data, r.id);

    for(uint32_t i = 0; i < r.n; i++)
    {
        Quad q = r[i];

        printf("%u \t%s \t", i, IR::get_str(q.op));

        switch (q.op)
        {
        case IR::MOV_IM:
            printf("temp%u \t%llu \t-\n", q.target.temp.id, q.left.int_value);
            break;
        case IR::MOV:
            printf("temp%u \ttemp%u \t-\n", q.target.temp.id, q.left.temp.id);
            break;
        case IR::MUL:
        case IR::IMUL:
        case IR::ADD:
        case IR::SUB:
        case IR::EQ:
        case IR::LT:
            printf("temp%u \ttemp%u \ttemp%u\n", q.target.temp.id, q.left.temp.id, q.right.temp.id);
            break;
        case IR::JMP:
            printf("%u \t- \t-\n", q.target.jump);
            break;
        case IR::JZ:
            printf("%u \ttemp%u \t-\n", q.target.jump, q.left.temp.id);
            break;
        case IR::CALL:
            printf("temp%u \tfunc%u \t-\n", q.target.temp.id, q.left.temp.id);
            break;
        case IR::RET:
            if (q.target.int_value)
                printf("temp%u \t- \t-\n", q.left.temp.id);
            else
                printf("- \t- \t-\n");
            break;
        case IR::ARG:
            printf("%u \ttemp%u \t-\n", q.target.temp.id, q.left.temp.id);
            break;
        }
    }
}

void print_ir(IR &ir)
{
    Routine *r = ir.routines;
    while (r)
    {
        print_ir(*r);
        r = r->next;
    }
}

void gen_ir(Ast ast, Alloc &a, const char *input)
{
    if (!ast.valid)
    {
        printf("ast not valid!");
        return;
    }

    IRGen gen(a);
    IR ir = gen.gen_ir(ast);
    printf("\"%s\"\n\n", input);
    print_ir(ir);
    return;
}


#include "check.h"
#include "parser.h"
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
    gen_ir(ast, a, input); \
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
    TEST("function f(int x) -> int { return x + 1; } int x = 0; while (x < 5) x = f(x);")

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
             "int int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 bool true false asdf 12345 6789u å",
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
         "asdf 56 + - 8\n \tint32 x = y * 9841 + 463;\nqwer ||9 if¤\ntruefalse true false;",
         3, 12, '¤')

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
