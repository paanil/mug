#include "ir.h"
#include "ast.h"
#include "sym_table.h"
#include "alloc.h"
#include "assert.h"

#include <cstdio>
#include <new> // to placement new routines


//
// IR
//

#define PASTE_TYPE(x) #x,

const char *IR::get_str(Type type)
{
    static const char *ir_str[] =
    {
        PASTE_TYPES
    };

    return ir_str[type];
}

#undef PASTE_TYPE


//
// Routine
//

Quad *Routine::add(Quad quad)
{
    uint32_t n = quad_count % Quads::N;

    if (n == 0)
    {
        Quads *quads = a.allocate<Quads>();
        quads->next = nullptr;
        if (head == nullptr)
             head = quads;
        else tail->next = quads;
        tail = quads;
    }

    Quad *q = tail->quads + n;
    *q = quad;
    quad_count++;
    return q;
}

Quad &Routine::operator [] (uint32_t index)
{
    assert(index < quad_count);
    Quads *quads = head;
    uint32_t n = index / Quads::N;
    while (n --> 0) // haha
        quads = quads->next;
    return quads->quads[index % Quads::N];
}


/**
 * Generates intermediate code from AST.
 */
struct IRGen
{
    uint32_t next_routine_id;
    Routine *tail; // The current tail of the linked list of routines.
    SymTable<Operand> sym;
    Alloc &a;

    IRGen(Alloc &a_)
    : next_routine_id()
    , tail()
    , a(a_)
    {}

    Routine *make_routine(Str name)
    {
        Routine *routine = a.allocate<Routine>();
        // TODO: Get rid of this placement new.
        routine = new (routine) Routine(name, next_routine_id++, a);
        if (tail != nullptr)
            tail->next = routine;
        tail = routine;
        return routine;
    }

    /**
     * Returns the value of the expression as a temp.
     * Even vars are temps.
     */
    Operand gen_ir(Routine &r, Expression *exp)
    {
        switch (exp->type)
        {
            case ExpType_BOOL:
            {
                Operand result = r.make_temp();
                Operand operand;
                operand.int_value = exp->boolean.value ? 1 : 0;
                r.add(IR::MOV_IM, result, operand);
                return result;
            }

            case ExpType_CONST:
            {
                Operand result = r.make_temp();
                Operand operand;
                operand.int_value = exp->constant.value;
                r.add(IR::MOV_IM, result, operand);
                return result;
            }

            case ExpType_VAR:
            {
                Operand result;
                result = sym.get(exp->var.name);
                return result;
            }

            case ExpType_CALL:
            {
                ArgList *arg = exp->call.args;
                for (uint32_t index = 0; arg; index++)
                {
                    Operand arg_idx;
                    arg_idx.arg_index = index;
                    Operand value = gen_ir(r, arg->arg);
                    r.add(IR::ARG, arg_idx, value);
                    arg = arg->next;
                }

                Operand result = r.make_temp();
                Operand routine;
                routine = sym.get(exp->call.func_name);
                r.add(IR::CALL, result, routine);
                return result;
            }

            case ExpType_UNARY:
            {
                Operand operand = gen_ir(r, exp->unary.operand);
                Operand result = r.make_temp();

                switch (exp->unary.op)
                {
                    case UnaryOp_NOT:
                        r.add(IR::NOT, result, operand);
                        break;
                    case UnaryOp_NEG:
                        r.add(IR::NEG, result, operand);
                        break;
                }

                return result;
            }

            case ExpType_BINARY:
            {
                Operand result = r.make_temp();
                Operand left = gen_ir(r, exp->binary.left);
                Operand right;
                if (exp->binary.op != BinaryOp_AND && exp->binary.op != BinaryOp_OR)
                {
                    right = gen_ir(r, exp->binary.right);
                }

                switch (exp->binary.op)
                {
                    case BinaryOp_MUL:
                        r.add(is_signed(exp) ? IR::IMUL : IR::MUL,
                              result, left, right);
                        break;
                    case BinaryOp_DIV:
                        r.add(is_signed(exp) ? IR::IDIV : IR::DIV,
                              result, left, right);
                        break;
                    case BinaryOp_ADD:
                        r.add(IR::ADD, result, left, right);
                        break;
                    case BinaryOp_SUB:
                        r.add(IR::SUB, result, left, right);
                        break;
                    case BinaryOp_EQ:
                        r.add(IR::EQ, result, left, right);
                        break;
                    case BinaryOp_NE:
                        r.add(IR::NE, result, left, right);
                        break;
                    case BinaryOp_LT:
                        r.add(is_signed(exp->binary.left) ? IR::LT : IR::BELOW,
                              result, left, right);
                        break;
                    case BinaryOp_GT:
                        r.add(is_signed(exp->binary.left) ? IR::GT : IR::ABOVE,
                              result, left, right);
                        break;
                    case BinaryOp_LE:
                        r.add(is_signed(exp->binary.left) ? IR::LE : IR::BE,
                              result, left, right);
                        break;
                    case BinaryOp_GE:
                        r.add(is_signed(exp->binary.left) ? IR::GE : IR::AE,
                              result, left, right);
                        break;
                    case BinaryOp_AND:
                    {
                        r.add(IR::MOV, result, left);
                        Quad *jz_quad = r.add(IR::JZ, {}, result);
                        right = gen_ir(r, exp->binary.right);
                        r.add(IR::MOV, result, right);
                        jz_quad->target = r.make_label();
                        break;
                    }
                    case BinaryOp_OR:
                    {
                        r.add(IR::MOV, result, left);
                        Quad *jnz_quad = r.add(IR::JNZ, {}, result);
                        right = gen_ir(r, exp->binary.right);
                        r.add(IR::MOV, result, right);
                        jnz_quad->target = r.make_label();
                        break;
                    }
                }

                return result;
            }
        }

        InvalidCodePath;
        return {};
    }

    void gen_ir(Routine &r, Node *node)
    {
        switch (node->type)
        {
            case NodeType_EMPTY:
                break;
            case NodeType_EXP:
                gen_ir(r, node->exp.exp);
                break;

            case NodeType_ASSIGN:
            {
                Operand var;
                var = sym.get(node->assign.var_name);
                Operand value = gen_ir(r, node->assign.value);
                // TODO: If gen_ir(exp) always generates a quad with a new temp as target,
                // we could replace that quad's target with the var and not add this mov quad.
                // By doing that, we wouldn't get useless movs here, BUT
                // think about expressions that use vars e.g. "(x + y) * z"!
                // They would generate unnecessary movs.
                r.add(IR::MOV, var, value);
                break;
            }

            case NodeType_DECL:
            {
                if (node->decl.init)
                {
                    Operand init = gen_ir(r, node->decl.init);
                    sym.put(node->decl.var_name, init);
                }
                else
                {
                    // TODO: Is this even necessary?
                    // We could sym.put(var_name, temp) when we first assign a value, BUT
                    // what if var is used without initializing or assigning?
                    Operand var = r.make_temp(); // variable is a temp
                    sym.put(node->decl.var_name, var);
                }
                break;
            }

            case NodeType_RETURN:
            {
                Operand flag, value;
                flag.returns_something = false;
                if (node->ret.value)
                {
                    flag.returns_something = true;
                    value = gen_ir(r, node->ret.value);
                }
                r.add(IR::RET, flag, value);
                break;
            }

            case NodeType_IF:
            {
                Operand condition = gen_ir(r, node->if_stmt.condition);
                Quad *jz_quad = r.add(IR::JZ, {}, condition);
                gen_ir(r, node->if_stmt.true_stmt);
                if (node->if_stmt.else_stmt)
                {
                    Quad *jmp_quad = r.add(IR::JMP);
                    jz_quad->target = r.make_label();
                    gen_ir(r, node->if_stmt.else_stmt);
                    jmp_quad->target = r.make_label();
                }
                else
                {
                    jz_quad->target = r.make_label();
                }
                break;
            }

            case NodeType_WHILE:
            {
                Operand label = r.make_label();
                Operand condition = gen_ir(r, node->while_stmt.condition);
                Quad *jz_quad = r.add(IR::JZ, {}, condition);
                gen_ir(r, node->while_stmt.stmt);
                r.add(IR::JMP, label);
                jz_quad->target = r.make_label();
                break;
            }

            case NodeType_BLOCK:
            {
                sym.enter_scope();

                StmtList *s = node->block.stmts;
                while (s)
                {
                    gen_ir(r, s->stmt);
                    s = s->next;
                }

                sym.exit_scope();
                break;
            }

            case NodeType_FUNC_DEF:
            {
                Routine *routine = make_routine(node->func_def.name);
                Operand func;
                func.func_id = routine->id;
                sym.put(node->func_def.name, func);

                sym.enter_scope();

                uint32_t param_count = 0;

                for (ParamList *p = node->func_def.params; p; p = p->next)
                {
                    Operand param = routine->make_temp(); // params are temps
                    sym.put(p->name, param);
                    param_count++;
                }

                routine->param_count = param_count;

                gen_ir(*routine, node->func_def.body);

                sym.exit_scope();
                break;
            }
        }
    }

    IR gen_ir(Ast ast)
    {
        Routine *top_level = make_routine(Str::make("@top_level"));
        gen_ir(*top_level, ast.root);

        IR ir;
        ir.routines = top_level;
        return ir;
    }
};


//
//
//

void print_ir(Routine *routine)
{
    fprintf(stdout, "\n%s#%u:\n", routine->name.data, routine->id);

    int quad_count = routine->quad_count;
    for(int i = 0; i < quad_count; i++)
    {
        Quad quad = (*routine)[i];

        fprintf(stdout, "%u \t%s \t", i, IR::get_str(quad.op));

        switch (quad.op)
        {
        case IR::MOV_IM:
            fprintf(stdout, "temp%u \t%llu \t-\n", quad.target.temp_id, quad.left.int_value);
            break;
        case IR::MOV:
            fprintf(stdout, "temp%u \ttemp%u \t-\n", quad.target.temp_id, quad.left.temp_id);
            break;
        case IR::NOT:
        case IR::NEG:
            fprintf(stdout, "temp%u \ttemp%u \t-\n", quad.target.temp_id, quad.left.temp_id);
            break;
        case IR::MUL: case IR::IMUL:
        case IR::DIV: case IR::IDIV:
        case IR::ADD: case IR::SUB:
        case IR::EQ: case IR::NE:
        case IR::LT: case IR::BELOW:
        case IR::GT: case IR::ABOVE:
        case IR::LE: case IR::BE:
        case IR::GE: case IR::AE:
            fprintf(stdout, "temp%u \ttemp%u \ttemp%u\n", quad.target.temp_id, quad.left.temp_id, quad.right.temp_id);
            break;
        case IR::JMP:
            fprintf(stdout, "label%u \t- \t-\n", quad.target.label);
            break;
        case IR::JZ:
        case IR::JNZ:
            fprintf(stdout, "label%u \ttemp%u \t-\n", quad.target.label, quad.left.temp_id);
            break;
        case IR::LABEL:
            fprintf(stdout, "label%u \t- \t-\n", quad.target.label);
            break;
        case IR::CALL:
            fprintf(stdout, "temp%u \tfunc%u \t-\n", quad.target.temp_id, quad.left.func_id);
            break;
        case IR::RET:
            if (quad.target.returns_something)
                fprintf(stdout, "temp%u \t- \t-\n", quad.left.temp_id);
            else
                fprintf(stdout, "- \t- \t-\n");
            break;
        case IR::ARG:
            fprintf(stdout, "arg%u \ttemp%u \t-\n", quad.target.arg_index, quad.left.temp_id);
            break;
        }
    }
}

void print_ir(IR ir)
{
    Routine *routine = ir.routines;
    while (routine)
    {
        print_ir(routine);
        routine = routine->next;
    }
}

IR gen_ir(Ast &ast, Alloc &a)
{
    if (!ast.valid)
    {
        return {};
    }

    IRGen gen(a);
    return gen.gen_ir(ast);
}
