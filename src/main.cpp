#include "tests.h"
#include "code_gen.h"
#include "ir_gen.h"
#include "check.h"
#include "parser.h"
#include "alloc.h"
#include "error_context.h"

int compile(const char *, const char *);

int main(int argc, const char **argv)
{
    /*const char *args[] = {
        "", "-p", "tests/code_gen_tests.mug"
    };
    argc = sizeof(args) / sizeof(*args);
    argv = args;*/

    if (argc < 2)
        run_tests();
    else
    {
        const char *s = nullptr;
        const char *o = nullptr;
        bool print_only = false;
        char buf[256];

        for (int i = 1; i < argc; i++)
        {
            const char *arg = argv[i];
            if (arg[0] == '-')
            {
                if (arg[1] == 'o')
                {
                    if (arg[2])
                        o = arg + 2;
                    else if (i + 1 < argc)
                        o = argv[++i];
                    else
                    {
                        fprintf(stderr, "error: -o flag requires output file!\n");
                        return 1;
                    }
                }
                else if (arg[1] == 'p')
                {
                    print_only = true;
                }
                else
                {
                    fprintf(stderr, "warning: unrecognized parameter %s\n", arg);
                }
            }
            else
            {
                if (s != nullptr)
                {
                    fprintf(stderr, "error: mug can handle only one source file at a time!\n");
                    return 1;
                }

                s = arg;
            }
        }

        if (s == nullptr)
        {
            fprintf(stderr, "error: source file not given!\n");
            return 1;
        }

        if (o == nullptr && !print_only)
        {
            char *dest = buf;
            for (unsigned i = 0; i < sizeof(buf) - 3; i++)
            {
                if (s[i] == 0 || s[i] == '.') break;
                *dest++ = s[i];
            }
            *dest++ = '.';
            *dest++ = 's';
            *dest++ = 0;

            o = buf;
        }

        return compile(s, print_only ? nullptr : o);
    }

    return 0;
}

//
//
//

int compile(const char *source_file, const char *output_file)
{
    Alloc a;
    char *buf;

    {
        FILE *f = fopen(source_file, "rb");
        if (f == nullptr)
        {
            fprintf(stderr, "error: couldn't open file '%s'\n", source_file);
            return 1;
        }

        fseek(f, 0, SEEK_END);
        unsigned size = ftell(f);
        fseek(f, 0, SEEK_SET);

        buf = a.allocate_array<char>(size + 1);
        buf[size] = 0;

        if (fread(buf, 1, size, f) != size)
        {
            fprintf(stderr, "error: couldn't read file '%s'\n", source_file);
            fclose(f);
            return 1;
        }

        fclose(f);
    }

    ErrorContext ec;
    Ast ast = parse(buf, a, ec);
    if (!check(ast, ec))
    {
        return 0;
    }

    IR ir = gen_ir(ast, a);

    if (output_file)
    {
        FILE *f = fopen(output_file, "w");
        if (f == nullptr)
        {
            fprintf(stderr, "error: couldn't open file '%s' for writing\n", output_file);
            return 1;
        }

        gen_code(ir, f);

        fclose(f);
    }
    else
    {
        gen_code(ir, stdout);
    }

    return 0;
}
