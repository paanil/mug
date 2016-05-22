#include "lexer.h"

#include <cassert>


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
