#ifndef _H_SIMULATOR
#define _H_SIMULATOR

#include "sim86.h"

void load_executable(CPU *cpu, char *filename);
void boot(CPU *cpu);
void run(CPU *cpu);

#endif
