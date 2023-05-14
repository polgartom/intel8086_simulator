#ifndef _H_SIMULATOR
#define _H_SIMULATOR

#include "sim86.h"

s32 get_data_from_register(CPU *cpu, Register_Access *src_reg);

void load_executable(CPU *cpu, char *filename);
void boot(CPU *cpu);
void run(CPU *cpu);

#endif
