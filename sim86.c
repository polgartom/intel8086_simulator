#include "sim86.h"

Register_Info *get_register_info(u8 is_16bit, u32 reg)
{
    return (Register_Info *)registers[is_16bit][reg];
}
