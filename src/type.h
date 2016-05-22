#ifndef TYPE_H
#define TYPE_H

struct Type
{
    enum Enum
    {
        VOID,
        INT,
        UINT,
        BOOL,
        FUNC,
    };

    static const char *get_str(Type t)
    {
        static const char *type_str[] =
        {
            "void",
            "int",
            "uint",
            "bool",
            "function",
        };

        return type_str[t.type];
    }

    Enum type;
    struct FuncDefNode *func;
};

#endif // TYPE_H
