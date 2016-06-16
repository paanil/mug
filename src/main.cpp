#include "tests.h"
#include "code_gen.h"
#include "ir_gen.h"
#include "check.h"
#include "parser.h"
#include "alloc.h"
#include "error_context.h"

#include <cstdlib>

enum OutputMode
{
    OutputMode_EXE,
    OutputMode_ASM,
    OutputMode_OBJ,
};

/**
 * Generates assembly from the source file and places it into the output file.
 * Implemented below main function.
 */
int compile(const char *source_file, const char *output_file);

int invoke(const char *command)
{
    return system(command);
}

int invoke(const char *fmt, const char *param)
{
    char command[256];
    snprintf(command, sizeof(command), fmt, param);
    return system(command);
}

void print_help()
{
    fprintf(stdout,
            "Usage: mug [-s|-c] [-o <output-file>] <source-file>\n\n"
            "By default, mug runs three phases:\n"
            "(1) <source-file> ==> mug ==> out.s\n"
            "(2) out.s ==> nasm ==> out.o\n"
            "(3) out.o ==> gcc ==> out.exe\n\n"
            "If -s is given, mug stops after phase (1).\n"
            "If -c is given, mug stops after phase (2).\n"
            "If both -s and -c are given, only the last one is effective.\n"
            "If -o is given, the final output of mug is <output-file>.\n\n"
            "Example usage: mug -c -o banana.o banana.mug\n");
}

int main(int argc, const char **argv)
{
#ifdef TEST_BUILD
    run_tests();
    return 0;
#endif // TEST_BUILD

    if (argc < 2)
    {
        print_help();
        return 0;
    }

    // Parse arguments.

    const char *source = nullptr;
    const char *output = nullptr;
    OutputMode mode = OutputMode_EXE;

    for (int i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        if (arg[0] == '-')
        {
            switch (arg[1])
            {
                case 's':
                    mode = OutputMode_ASM;
                    break;
                case 'c':
                    mode = OutputMode_OBJ;
                    break;
                case 'o':
                {
                    if (arg[2])
                        output = arg + 2;
                    else if (i + 1 < argc)
                        output = argv[++i];
                    else
                    {
                        fprintf(stderr, "error: -o requires output file\n");
                        return 1;
                    }
                    break;
                }
                default:
                    fprintf(stderr, "warning: unrecognized parameter %s\n", arg);
                    break;
            }
        }
        else
        {
            if (source != nullptr)
            {
                fprintf(stderr, "error: mug can handle only one source file at a time!\n");
                return 1;
            }

            source = arg;
        }
    }

    if (source == nullptr)
    {
        fprintf(stderr, "error: source file not given!\n");
        return 1;
    }

    // Do the job.

    if (mode == OutputMode_ASM)
        return compile(source, output ? output : "out.s");

    if (compile(source, "out.s") != 0)
        return 1;

    if (mode == OutputMode_OBJ)
        return invoke("yasm -f win64 -o %s out.s", output ? output : "out.o");

    if (invoke("yasm -f win64 -o out.o out.s") != 0)
        return 1;

    return invoke("gcc -o %s out.o", output ? output : "out.exe");
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

    FILE *f = fopen(output_file, "w");
    if (f == nullptr)
    {
        fprintf(stderr, "error: couldn't open file '%s' for writing\n", output_file);
        return 1;
    }

    gen_code(ir, f);

    fclose(f);

    return 0;
}
