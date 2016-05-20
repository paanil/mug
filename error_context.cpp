#include "error_context.h"

#include <cstdio>

void ErrorContext::print_error(const char *message)
{
    if (errors < max_print)
    {
        fprintf(stderr, "%s\n", message);
    }

    errors++;
}

void ErrorContext::print_error(const char *message, const char *info)
{
    if (errors < max_print)
    {
        fprintf(stderr, message, info);
        fprintf(stderr, "\n");
    }

    errors++;
}

void ErrorContext::print_error(int line, int column,
                               const char *message, const char *info)
{
    if (errors < max_print)
    {
        fprintf(stderr, "error:%d:%d: ", line, column);
    }

    print_error(message, info);
}
