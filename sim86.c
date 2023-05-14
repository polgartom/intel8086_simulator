#include "sim86.h"

Register_Access *register_access(u32 reg, u32 flags)
{
    u8 index = (flags & Inst_Wide) ? 1 : 0;
    if (flags & Inst_Segment) {
        index = 2;
    }
    
    static u32 registers[3][8][3] = {
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
        },
        {
            {Register_es, 16, 2}, {Register_cs, 18,  2}, {Register_ss, 20,  2}, {Register_ds, 22,  2},
            {Register_count, 24, 2}, {Register_count, 26, 2}, {Register_count, 28, 2}, {Register_count, 30, 2} // Empty
        }
    };

    return (Register_Access *)registers[index][reg];
}

Register_Access *register_access_by_enum(Register reg_enum)
{
    u32 flags = 0;
    u32 reg = (u32)reg_enum;

    if (reg >= 16) {
        assert(reg <= 19);
        flags |= Inst_Segment;        
    } else if (reg > 7) {
        flags |= Inst_Wide;
        reg -= 8;
    }

    Register_Access *result = register_access(reg, flags); 
    assert(result->reg == reg_enum);

    return result;
}