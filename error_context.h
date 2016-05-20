#ifndef ERROR_CONTEXT_H
#define ERROR_CONTEXT_H

struct ErrorContext
{
    int errors;
    int max_print;

    ErrorContext(int max_print_ = 10)
    : errors()
    , max_print(max_print_)
    { }

    void print_error(const char *message);
    void print_error(const char *message, const char *info);
    void print_error(int line, int column, const char *message, const char *info);
};

#endif // ERROR_CONTEXT_H
