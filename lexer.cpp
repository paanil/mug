#include "lexer.h"

#include <cassert>
#include <cstdio>


//
// Token
//

const char *Token::get_str(Token::Type type)
{
#define PASTE_TT(tt, ts) ts,

    static const char *token_str[] =
    {
        PASTE_TTS
    };

#undef PASTE_TT

    return token_str[type];
}

bool Token::is_type(Token::Type type)
{
    return (Token::INT <= type && type <= BOOL);
}


//
// Lexer
//

Lexer::Lexer()
{
#define PASTE_TT(tt, ts) if (Token::IF <= Token::tt && Token::tt <= Token::FALSE) keyword_map.set(Str::make(ts), Token::tt);

    PASTE_TTS

#undef PASTE_TT
}

char Lexer::peek()
{
    return *input;
}

char Lexer::get()
{
    column++;
    return *input++;
}

Token Lexer::make_token(Token::Type type)
{
    Token token;
    token.type = type;
    token.line = line;
    token.column = column;
    return token;
}

Token Lexer::ident_token(Str s)
{
    Token token = make_token(Token::IDENT);
    token.text = s;
    return token;
}

Token Lexer::const_token(uint64_t value)
{
    Token token = make_token(Token::CONST);
    token.value = value;
    return token;
}

Token Lexer::invalid_token(char c)
{
    Token token = make_token(Token::INVALID);
    token.c = c;
    return token;
}

#define CASE_ONE(c, tt) case c: get(); return make_token(Token::tt)
#define CASE_TWO(c1, c2, tt1, tt2) \
    case c1:    \
        get();  \
        if (peek() == c2) { get(); return make_token(Token::tt2); } \
        return make_token(Token::tt1)

Token Lexer::next_token()
{
    while (true)
    {
        switch (peek())
        {
            case 0:
                return make_token(Token::END);

            CASE_TWO('!', '=', NOT, NE);
            CASE_ONE('*', STAR);
            CASE_ONE('+', PLUS);
            CASE_TWO('-', '>', MINUS, ARROW);
            CASE_TWO('&', '&', AMP, AND);
            CASE_TWO('|', '|', PIPE, OR);
            CASE_TWO('=', '=', ASSIGN, EQ);
            CASE_TWO('<', '=', LT, LE);
            CASE_TWO('>', '=', GT, GE);
            CASE_ONE('(', LPAREN);
            CASE_ONE(')', RPAREN);
            CASE_ONE('{', LBRACE);
            CASE_ONE('}', RBRACE);
            CASE_ONE(',', COMMA);
            CASE_ONE(';', SEMICOLON);

            case '/':
                get();
                if (peek() == '/') // single line comment
                {
                    get();
                    while (peek() && peek() != '\n') get();
                    break;
                }
                if (peek() == '*') // multi line comment
                {
                    bool terminated = false;

                    get();
                    while (peek())
                    {
                        char c = get();
                        if (c == '*')
                        {
                            if (peek() == '/')
                            {
                                terminated = true;

                                get();
                                break;
                            }
                        }
                        else if (c == '\n')
                        {
                            get();
                            line++;
                            column = 0;
                        }
                    }

                    if (!terminated)
                    {
                        // TODO: Error!
                    }

                    break;
                }
                return make_token(Token::SLASH);

            case '_':
            case 'a'...'z':
            case 'A'...'Z':
            {
                int len = 0;
                const char *text = input;

                while (true)
                {
                    len++;
                    get();

                    switch (peek())
                    {
                        case '_':
                        case 'a'...'z':
                        case 'A'...'Z':
                        case '0'...'9':
                            break;

                        default:
                        {
                            Str s = Str::make(text, len);
                            uint32_t idx = keyword_map.find(s);
                            if (idx == NOT_FOUND)
                                return ident_token(s);
                            Token::Type tt = keyword_map.get(idx);
                            return make_token(tt);
                        }
                    }
                }

                assert(0 && "invalid code path!");
            }

            case '0'...'9':
            {
                uint64_t value = 0;

                while (true)
                {
                    value *= 10;
                    value += get() - '0';

                    switch (peek())
                    {
                        case '0'...'9':
                            break;

                        default:
                            return const_token(value);
                    }
                }

                assert(0 && "invalid code path!");
            }

            case ' ': case '\t': case '\r':
                get();
                break;

            case '\n':
                get();
                line++;
                column = 0;
                break;

            default:
                return invalid_token(get());
        }
    }

    assert(0 && "invalid code path!");
}


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

#define TEST(test_func, input, ...) \
    { tests += 1; if (test_func(input, ##__VA_ARGS__) != 0) { fprintf(stderr, "lexer test #%d failed.\n\n", tests); failed += 1; } }

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
        Token::Type expected_tokens[] =
        {
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
             "int int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 bool true false asdf 12345 å",
             expected_tokens)
    }

    {
        Token::Type expected_tokens[] =
        {
            Token::IDENT,Token::IDENT,Token::IDENT,Token::IDENT,Token::IDENT,Token::CONST,Token::IDENT,Token::IDENT,Token::END
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
