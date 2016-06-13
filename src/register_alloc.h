#ifndef REGISTER_ALLOC_H
#define REGISTER_ALLOC_H

#include "assert.h"

#define PASTE_REGS  \
    PASTE_REG(rax)  \
    PASTE_REG(rcx)  \
    PASTE_REG(rdx)  \
    PASTE_REG(r8)   \
    PASTE_REG(r9)   \
    PASTE_REG(r10)  \
    PASTE_REG(r11)

#define PASTE_REG(r) Reg_##r,

enum RegID
{
    Reg_NONE = -1,

    PASTE_REGS

    Reg_COUNT
};

#undef PASTE_REG

struct Register
{
    RegID id; // Actually just an index.
    int32_t temp_id; // Temp currently in the register or -1 if none.

#define PASTE_REG(r) #r,

    static const char *get_str(RegID reg_id)
    {
        static const char *reg_str[] =
        {
            PASTE_REGS
        };

        return reg_str[reg_id];
    }

#undef PASTE_REG
};

#define PARAM_REG_COUNT 4

struct RegisterAlloc
{
    RegID param_registers[PARAM_REG_COUNT];
    RegID register_queue[Reg_COUNT]; // Used for least recently used allocation.
    Register registers[Reg_COUNT];

#define PASTE_REG(r) registers[Reg_##r] = (Register){ Reg_##r, -1 };

    void reset()
    {
        param_registers[0] = Reg_rcx;
        param_registers[1] = Reg_rdx;
        param_registers[2] = Reg_r8;
        param_registers[3] = Reg_r9;

        for (int i = 0; i < Reg_COUNT; i++)
            register_queue[i] = (RegID)i;

        PASTE_REGS
    }

#undef PASTE_REG

    void move_back_in_queue(int queue_index)
    {
        RegID reg_id = register_queue[queue_index];
        for (int i = queue_index + 1; i < Reg_COUNT; i++)
            register_queue[i - 1] = register_queue[i];
        register_queue[Reg_COUNT - 1] = reg_id;
    }

    Register alloc_register(RegID reg_id, int32_t temp_id)
    {
        assert(reg_id != Reg_NONE);
        assert(reg_id < Reg_COUNT);

        for (int i = 0; i < Reg_COUNT; i++)
        {
            if (register_queue[i] == reg_id)
            {
                move_back_in_queue(i);
                break;
            }
        }

        Register reg = registers[reg_id];
        registers[reg_id].temp_id = temp_id;
        return reg;
    }

    Register alloc_any_register(int32_t temp_id)
    {
        RegID reg_id = register_queue[0];

        move_back_in_queue(0);

        Register reg = registers[reg_id];
        registers[reg_id].temp_id = temp_id;
        return reg;
    }

    bool get_param_register(int param_index, Register *reg)
    {
        if(param_index >= PARAM_REG_COUNT)
            return false;

        RegID reg_id = param_registers[param_index];
        *reg = registers[reg_id];
        return true;
    }

    void dealloc_register(RegID reg_id)
    {
        assert(reg_id != Reg_NONE);
        assert(reg_id < Reg_COUNT);
        assert(registers[reg_id].temp_id >= 0);

        registers[reg_id].temp_id = -1;
    }

    Register get_register_by_id(RegID reg_id)
    {
        assert(reg_id != Reg_NONE);
        assert(reg_id < Reg_COUNT);

        return registers[reg_id];
    }
};

#endif // REGISTER_ALLOC_H
