#include "sim86.h"

Register_Info *get_register_info(u8 is_16bit, u8 reg)
{
    static const u32 registers[2][8][3] = {
        // BYTE (8bit)
        { 
            {Register_al, 0, 1}, {Register_cl, 2, 1}, {Register_dl, 4, 1}, {Register_bl, 6, 1},
            {Register_ah, 1, 2}, {Register_ch, 3, 1}, {Register_dh, 5, 1}, {Register_bh, 7, 1}
        },
        // WORD (16bit)
        {
            {Register_ax, 0, 2}, {Register_cx, 2,  2}, {Register_dx, 4,  2}, {Register_bx, 6,  2},
            {Register_sp, 8, 2}, {Register_bp, 10, 2}, {Register_si, 12, 2}, {Register_di, 14, 2}
        }
    };

    return (Register_Info *)registers[is_16bit][reg];
}
