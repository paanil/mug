#include "ir.h"
#include "ast.h"
#include "sym_table.h"
#include "alloc.h"
#include "assert.h"

#include <cstdio>


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

Quad *Routine::add(Alloc &a, Quad quad)
{
    uint32_t m = n % Quads::N;

    if (m == 0)
    {
        Quads *qs = a.allocate<Quads>();
        qs->next = nullptr;
        if (head == nullptr)
             head = qs;
        else tail->next = qs;
        tail = qs;
    }

    Quad *q = tail->quads + m;
    *q = quad; n++;
    return q;
}

Quad &Routine::operator [] (uint32_t index)
{
    assert(index < n);
    Quads *qs = head;
    uint32_t m = index / Quads::N;
    while (m --> 0) // haha
        qs = qs->next;
    return qs->quads[index % Quads::N];
}


//
//
//

struct IRGen
{
    uint32_t next_routine_id;
    Routine *tail;
    SymTable<Operand> sym;
    Alloc &a;

    IRGen(Alloc &a_)
    : next_routine_id()
    , tail()
    , a(a_)
    {}

    Routine *make_routine(Str name)
    {
        Routine *r = a.allocate<Routine>();
        *r = Routine(name, next_routine_id++);
        if (tail != nullptr)
            tail->next = r;
        tail = r;
        return r;
    }

    Operand gen_ir(Routine &r, Expression *exp)
    {
        switch (exp->type)
        {
            case ExpType_BOOL:
            {
                Operand result = r.make_temp();
                Operand operand;
                operand.int_value = exp->boolean.value ? 1 : 0;
                r.add(a, IR::MOV_IM, result, operand);
                return result;
            }

            case ExpType_CONST:
            {
                Operand result = r.make_temp();
                Operand operand;
                operand.int_value = exp->constant.value;
                r.add(a, IR::MOV_IM, result, operand);
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
                    r.add(a, IR::ARG, arg_idx, value);
                    arg = arg->next;
                }

                Operand result = r.make_temp();
                Operand routine;
                routine = sym.get(exp->call.func_name);
                r.add(a, IR::CALL, result, routine);
                return result;
            }

            case ExpType_UNARY:
            {
                Operand operand = gen_ir(r, exp->unary.operand);
                Operand result = r.make_temp();

                switch (exp->unary.op)
                {
                    case UnaryOp_NOT:
                    {
                        // TODO: IR::NOT?
                        Operand one;
                        one.int_value = 1;
                        r.add(a, IR::XOR_IM, result, operand, one);
                        break;
                    }
                    case UnaryOp_NEG:
                    {
                        r.add(a, IR::NEG, result, operand);
                        break;
                    }
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
                        r.add(a,
                              is_signed(exp) ? IR::IMUL : IR::MUL,
                              result, left, right);
                        break;
                    case BinaryOp_DIV:
                        r.add(a,
                              is_signed(exp) ? IR::IDIV : IR::DIV,
                              result, left, right);
                        break;
                    case BinaryOp_ADD:
                        r.add(a, IR::ADD, result, left, right);
                        break;
                    case BinaryOp_SUB:
                        r.add(a, IR::SUB, result, left, right);
                        break;
                    case BinaryOp_EQ:
                        r.add(a, IR::EQ, result, left, right);
                        break;
                    case BinaryOp_NE:
                        r.add(a, IR::NE, result, left, right);
                        break;
                    case BinaryOp_LT:
                        r.add(a,
                              is_signed(exp->binary.left) ? IR::LT : IR::BELOW,
                              result, left, right);
                        break;
                    case BinaryOp_GT:
                        r.add(a,
                              is_signed(exp->binary.left) ? IR::GT : IR::ABOVE,
                              result, left, right);
                        break;
                    case BinaryOp_LE:
                        r.add(a,
                              is_signed(exp->binary.left) ? IR::LE : IR::BE,
                              result, left, right);
                        break;
                    case BinaryOp_GE:
                        r.add(a,
                              is_signed(exp->binary.left) ? IR::GE : IR::AE,
                              result, left, right);
                        break;
                    case BinaryOp_AND:
                    {
                        r.add(a, IR::MOV, result, left);
                        Quad *jz_quad = r.add(a, IR::JZ, {}, result);
                        right = gen_ir(r, exp->binary.right);
                        r.add(a, IR::MOV, result, right);
                        jz_quad->target = r.make_label(a);
                        break;
                    }
                    case BinaryOp_OR:
                    {
                        r.add(a, IR::MOV, result, left);
                        Quad *jnz_quad = r.add(a, IR::JNZ, {}, result);
                        right = gen_ir(r, exp->binary.right);
                        r.add(a, IR::MOV, result, right);
                        jnz_quad->target = r.make_label(a);
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
                r.add(a, IR::MOV, var, value);
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
                    Operand var = r.make_temp();
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
                r.add(a, IR::RET, flag, value);
                break;
            }

            case NodeType_IF:
            {
                Operand condition = gen_ir(r, node->if_stmt.condition);
                Quad *jz_quad = r.add(a, IR::JZ, {}, condition);
                gen_ir(r, node->if_stmt.true_stmt);
                if (node->if_stmt.else_stmt)
                {
                    Quad *jmp_quad = r.add(a, IR::JMP);
                    jz_quad->target = r.make_label(a);
                    gen_ir(r, node->if_stmt.else_stmt);
                    jmp_quad->target = r.make_label(a);
                }
                else
                {
                    jz_quad->target = r.make_label(a);
                }
                break;
            }

            case NodeType_WHILE:
            {
                Operand label = r.make_label(a);
                Operand condition = gen_ir(r, node->while_stmt.condition);
                Quad *jz_quad = r.add(a, IR::JZ, {}, condition);
                gen_ir(r, node->while_stmt.stmt);
                r.add(a, IR::JMP, label);
                jz_quad->target = r.make_label(a);
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
                    Operand param = routine->make_temp();
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

void print_ir(Routine &r)
{
    fprintf(stdout, "\n%s#%u:\n", r.name.data, r.id);

    for(uint32_t i = 0; i < r.n; i++)
    {
        Quad q = r[i];

        fprintf(stdout, "%u \t%s \t", i, IR::get_str(q.op));

        switch (q.op)
        {
        case IR::MOV_IM:
            fprintf(stdout, "temp%u \t%llu \t-\n", q.target.temp_id, q.left.int_value);
            break;
        case IR::MOV:
            fprintf(stdout, "temp%u \ttemp%u \t-\n", q.target.temp_id, q.left.temp_id);
            break;
        case IR::NEG:
            fprintf(stdout, "temp%u \ttemp%u \t-\n", q.target.temp_id, q.left.temp_id);
            break;
        case IR::MUL:
        case IR::IMUL:
        case IR::DIV:
        case IR::IDIV:
        case IR::ADD:
        case IR::SUB:
        case IR::EQ: case IR::NE:
        case IR::LT: case IR::BELOW:
        case IR::GT: case IR::ABOVE:
        case IR::LE: case IR::BE:
        case IR::GE: case IR::AE:
            fprintf(stdout, "temp%u \ttemp%u \ttemp%u\n", q.target.temp_id, q.left.temp_id, q.right.temp_id);
            break;
        case IR::XOR_IM:
            fprintf(stdout, "temp%u \ttemp%u \t%llu\n", q.target.temp_id, q.left.temp_id, q.right.int_value);
            break;
        case IR::JMP:
            fprintf(stdout, "label%u \t- \t-\n", q.target.label);
            break;
        case IR::JZ:
        case IR::JNZ:
            fprintf(stdout, "label%u \ttemp%u \t-\n", q.target.label, q.left.temp_id);
            break;
        case IR::LABEL:
            fprintf(stdout, "label%u \t- \t-\n", q.target.label);
            break;
        case IR::CALL:
            fprintf(stdout, "temp%u \tfunc%u \t-\n", q.target.temp_id, q.left.func_id);
            break;
        case IR::RET:
            if (q.target.returns_something)
                fprintf(stdout, "temp%u \t- \t-\n", q.left.temp_id);
            else
                fprintf(stdout, "- \t- \t-\n");
            break;
        case IR::ARG:
            fprintf(stdout, "arg%u \ttemp%u \t-\n", q.target.arg_index, q.left.temp_id);
            break;
        }
    }
}

void print_ir(IR ir)
{
    Routine *r = ir.routines;
    while (r)
    {
        print_ir(*r);
        r = r->next;
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
