#include "sim86.h"

Register_Access *register_access(u32 reg, u8 is_wide)
{
    assert(is_wide <= 1);
    assert(reg < 8);

    static u32 registers[2][8][3] = {
        // BYTE (8bit)
        { 
            // enum, offset, size
            {Register_al, 0, 1}, {Register_cl, 2, 1}, {Register_dl, 4, 1}, {Register_bl, 6, 1},
            {Register_ah, 1, 1}, {Register_ch, 3, 1}, {Register_dh, 5, 1}, {Register_bh, 7, 1}
        },
        // WORD (16bit)
        {
            // enum, offset, size
            {Register_ax, 0, 2}, {Register_cx, 2,  2}, {Register_dx, 4,  2}, {Register_bx, 6,  2},
            {Register_sp, 8, 2}, {Register_bp, 10, 2}, {Register_si, 12, 2}, {Register_di, 14, 2}
        }
    };

    return (Register_Access *)registers[is_wide][reg];
}

Register_Access *register_access_by_enum(Register reg)
{
    u8 is_wide = (u32)reg > 7;
    u32 reg_index = (u32)reg;
    if (is_wide) {
        reg_index -= 8;
    }

    Register_Access *result = register_access(reg_index, is_wide); 
    assert(result->reg == reg);

    return result;
}