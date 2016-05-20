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

    bool operator == (Type t)
    {
        return (type == t.type);
    }
    bool operator != (Type t)
    {
        return !(*this == t);
    }
};

#endif // TYPE_H
