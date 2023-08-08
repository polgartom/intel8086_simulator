#ifndef _H_SIMULATOR
#define _H_SIMULATOR

#include "sim86.h"

u32 calc_inst_pointer_address(CPU *cpu);
u32 calc_stack_pointer_address(CPU *cpu);

u16 get_data_from_register(CPU *cpu, Register_Access *src_reg);

void load_executable(CPU *cpu, char *filename);
void boot(CPU *cpu);
void run(CPU *cpu);

#endif
