#include "sim86.h"

// @Todo: Flip this params order
Register_Info *get_register_info(u8 is_16bit, u32 reg)
{
    return (Register_Info *)registers[is_16bit][reg];
}

// @Todo: dude...
Register_Info *get_register_info_by_enum(Register reg_enum)
{
    for (u32 i = 0; i < 2; i++) {
        for (u32 j = 0; j < 8; j++) {
            if (registers[i][j][0] == reg_enum) {
                return (Register_Info *)registers[i][j];
            }
        }
        
    }

    assert(0);
}
