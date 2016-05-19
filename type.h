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
    };

    Enum type;
};

#endif // TYPE_H
