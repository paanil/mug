#include "parser.h"

struct SymTable
{
    StrMap<Type> symbol_types;

};

bool type_check(Node *node, SymTable *sym)
{
    uint32_t idx = NOT_FOUND;

    switch (node->type)
    {
        case NodeType_BOOL:
//            node->boolean.exp_type = (Type){Type::BOOL}; // could be done when parsing.
            return true;
        case NodeType_CONST:
//            node->constant.exp_type = (Type){Type::UINT}; // could be done when parsing.
            return true;
        case NodeType_VAR:
            idx = sym->symbol_types.find(node->var.name);
            if (idx == NOT_FOUND)
            {
                // TODO: Error message.
                return false;
            }
//            node->var.exp_type = sym->symbol_types.get(idx);
            return true;
        case NodeType_CALL:
            idx = sym->symbol_types.find(node->call.func_name);
            if (idx == NOT_FOUND)
            {
                // TODO: Error message.
                return false;
            }
//            node->call.exp_type = sym->symbol_types.get(idx);
            // TODO: Check args against params!
            return true;
//        NodeType_UNARY,
//        NodeType_BINARY,
//
//        NodeType_EMPTY,
//        NodeType_EXPR,
//        NodeType_ASSIGN,
//        NodeType_DECL,
//        NodeType_RETURN,
//        NodeType_IF,
//        NodeType_WHILE,
//        NodeType_BLOCK,
//        NodeType_FUNC_DEF,
    }
}

bool type_check(Ast ast)
{
    if (!ast.valid)
        return false;

    SymTable sym;

    return type_check(ast.root, &sym);
}

void run_type_check_tests()
{
    Alloc a;
    Parser p(a);
    Ast ast = p.parse("42;");
    if (!type_check(ast));
}

int main()
{
    run_lexer_tests();
    run_parser_tests();

    return 0;
}
