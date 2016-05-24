#include "check.h"
#include "sym_table.h"
#include "error_context.h"

bool is_bool(Expression *exp)
{
    return (exp->data_type.type == Type::BOOL);
}

bool is_signed(Expression *exp)
{
    return (exp->data_type.type == Type::INT);
}

bool is_void(Expression *exp)
{
    return (exp->data_type.type == Type::VOID);
}

bool can_cast(Type from, Type to)
{
    switch (from.type)
    {
        case Type::VOID: return false;
        case Type::INT:  return (to.type == Type::INT || to.type == Type::UINT);
        case Type::UINT: return (to.type == Type::INT || to.type == Type::UINT);
        case Type::BOOL: return (to.type == Type::BOOL);
        case Type::FUNC: return false;
    }
    return false;
}

struct Checker
{
    SymTable<Type> sym;
    ErrorContext &ec;

    Checker(ErrorContext &ec_)
    : sym()
    , ec(ec_)
    {}

    bool type_check(Expression *exp);
    bool type_check(Node *node);
};

bool Checker::type_check(Expression *exp)
{
    switch (exp->type)
    {
        case ExpType_BOOL:
        case ExpType_CONST:
            return true;

        case ExpType_VAR:
        {
            Type sym_type;
            if (!sym.has(exp->var.name, &sym_type))
            {
                ec.print_error("variable '%s' is not defined", exp->var.name.data);
                return false;
            }
            exp->data_type = sym_type;
            return true;
        }

        case ExpType_CALL:
        {
            Type sym_type;
            if (!sym.has(exp->call.func_name, &sym_type))
            {
                ec.print_error("function '%s' is not defined",
                               exp->call.func_name.data);
                return false;
            }
            if (sym_type.type != Type::FUNC)
            {
                ec.print_error("cannot call '%s'; it's not a function",
                               exp->call.func_name.data);
                return false;
            }

            ArgList *a = exp->call.args;
            ParamList *p = sym_type.func->params;

            while (p)
            {
                if (a == nullptr)
                {
                    ec.print_error("calling function '%s' with too few arguments",
                                   exp->call.func_name.data);
                    return false;
                }

                if (!type_check(a->arg))
                    return false;

                if (!can_cast(a->arg->data_type, p->type))
                {
                    ec.print_error("incompatible argument type; "
                                   "function takes %s, but %s was given",
                                   Type::get_str(p->type), Type::get_str(a->arg->data_type));
                    return false;
                }

                p = p->next;
                a = a->next;
            }

            if (a != nullptr)
            {
                ec.print_error("calling function '%s' with too many arguments",
                               exp->call.func_name.data);
                return false;
            }

            exp->data_type = sym_type.func->ret_type;
            return true;
        }

        case ExpType_UNARY:
            if (!type_check(exp->unary.operand))
                return false;
            if (is_void(exp->unary.operand))
            {
                ec.print_error("cannot do unary operations with void");
                return false;
            }

            switch (exp->unary.op)
            {
            case UnaryOp_NOT:
                if (!is_bool(exp->unary.operand))
                {
                    ec.print_error("incompatible type for unary not");
                    return false;
                }
                exp->data_type.type = Type::BOOL;
                break;
            case UnaryOp_NEG:
                if (is_bool(exp->unary.operand))
                {
                    ec.print_error("boolean cannot be negated");
                    return false;
                }
                exp->data_type.type = Type::INT;
                break;
            }

            return true;

        case ExpType_BINARY:
            if (!type_check(exp->binary.left))
                return false;
            if (!type_check(exp->binary.right))
                return false;
            if (is_void(exp->binary.left) || is_void(exp->binary.right))
            {
                ec.print_error("cannot do binary operations with voids");
                return false;
            }

            switch (exp->binary.op)
            {
            case BinaryOp_MUL: case BinaryOp_DIV:
            case BinaryOp_ADD: case BinaryOp_SUB:
                if (is_bool(exp->binary.left) || is_bool(exp->binary.right))
                {
                    ec.print_error("boolean used in binary arithmetic");
                    return false;
                }
                if (is_signed(exp->binary.left) || is_signed(exp->binary.right))
                    exp->data_type.type = Type::INT;
                else
                    exp->data_type.type = Type::UINT;
                break;

            case BinaryOp_EQ: case BinaryOp_NE:
            case BinaryOp_LT: case BinaryOp_GT:
            case BinaryOp_LE: case BinaryOp_GE:
                if (exp->binary.op == BinaryOp_EQ || exp->binary.op == BinaryOp_NE)
                {
                    if (is_bool(exp->binary.left) != is_bool(exp->binary.right))
                    {
                        ec.print_error("cannot equ nor nequ boolean and numeric value");
                        return false;
                    }
                }
                else
                {
                    if (is_bool(exp->binary.left) || is_bool(exp->binary.right))
                    {
                        ec.print_error("only boolean equality or inequality can be tested");
                        return false;
                    }
                }
                if (is_signed(exp->binary.left) != is_signed(exp->binary.right))
                {
                    ec.print_error("comparison of signed and unsigned values");
                    return false;
                }
                exp->data_type.type = Type::BOOL;
                break;

            case BinaryOp_AND:
            case BinaryOp_OR:
                if (!is_bool(exp->binary.left) || !is_bool(exp->binary.right))
                {
                    ec.print_error("logical && and || can be used with booleans only");
                    return false;
                }
                exp->data_type.type = Type::BOOL;
                break;
            }

            return true;
    }

    return false;
}

bool Checker::type_check(Node *node)
{
    switch (node->type)
    {
        case NodeType_EMPTY:
            return true;
        case NodeType_EXP:
            return type_check(node->exp.exp);

        case NodeType_ASSIGN:
        {
            Type sym_type;
            if (!sym.has(node->assign.var_name, &sym_type))
            {
                ec.print_error("variable '%s' is not defined",
                               node->assign.var_name.data);
                return false;
            }
            if (!type_check(node->assign.value))
                return false;
            if (!can_cast(node->assign.value->data_type, sym_type))
            {
                ec.print_error("incompatible types in assignment; "
                               "trying to assign %s to %s",
                               Type::get_str(node->assign.value->data_type),
                               Type::get_str(sym_type));
                return false;
            }
            return true;
        }

        case NodeType_DECL:
        {
            if (sym.in_current_scope(node->decl.var_name))
            {
                ec.print_error("variable '%s' is already defined in current scope",
                               node->decl.var_name.data);
                return false;
            }
            if (node->decl.init)
            {
                if (!type_check(node->decl.init))
                    return false;
                if (!can_cast(node->decl.init->data_type, node->decl.var_type))
                {
                    ec.print_error("incompatible types in variable declaration; "
                                   "trying to initialize %s with %s",
                                   Type::get_str(node->decl.var_type),
                                   Type::get_str(node->decl.init->data_type));
                    return false;
                }
            }
            sym.put(node->decl.var_name, node->decl.var_type);
            return true;
        }

        case NodeType_RETURN:
        {
            Type sym_type;
            if (!sym.has(Str::make("@return"), &sym_type))
            {
                ec.print_error("'return' outside a function");
                return false;
            }
            if (node->ret.value)
            {
                if (!type_check(node->ret.value))
                    return false;
                if (!can_cast(node->ret.value->data_type, sym_type))
                {
                    ec.print_error("return value doesn't match function's return type");
                    return false;
                }
            }
            else if (sym_type.type != Type::VOID)
            {
                ec.print_error("non-void function should return something");
                return false;
            }

            return true;
        }

        case NodeType_IF:
            if (!type_check(node->if_stmt.condition))
                return false;
            if (!is_bool(node->if_stmt.condition))
            {
                ec.print_error("condition is not a boolean");
                return false;
            }
            if (!type_check(node->if_stmt.true_stmt))
                return false;
            if (node->if_stmt.else_stmt)
            {
                if (!type_check(node->if_stmt.else_stmt))
                    return false;
            }
            return true;

        case NodeType_WHILE:
            if (!type_check(node->while_stmt.condition))
                return false;
            if (!is_bool(node->while_stmt.condition))
            {
                ec.print_error("condition is not a boolean");
                return false;
            }
            return type_check(node->while_stmt.stmt);

        case NodeType_BLOCK:
        {
            sym.enter_scope();

            StmtList *s = node->block.stmts;
            while (s)
            {
                if (!type_check(s->stmt))
                    return false;
                s = s->next;
            }

            sym.exit_scope();
            return true;
        }

        case NodeType_FUNC_DEF:
        {
            if (sym.has(node->func_def.name))
            {
                ec.print_error("function '%s' has already been defined",
                               node->func_def.name.data);
                return false;
            }

            Type type = { Type::FUNC, &node->func_def };
            sym.put(node->func_def.name, type);

            sym.enter_scope();
            sym.put(Str::make("@return"), node->func_def.ret_type);

            for (ParamList *p = node->func_def.params; p; p = p->next)
                sym.put(p->name, p->type);

            if (!type_check(node->func_def.body))
                return false;

            if (node->func_def.ret_type.type != Type::VOID)
            {
                // TODO: Check that something is returned.
            }

            sym.exit_scope();
            return true;
        }
    }

    return false;
}


//
//
//

bool check(Ast ast, ErrorContext &ec)
{
    if (!ast.valid)
        return false;

    Checker checker(ec);
    return checker.type_check(ast.root);
}
