#ifndef _H_PRINTER
#define _H_PRINTER

#include "sim86.h"

const char *mnemonic_name(Mneumonic m);
const char *register_name(Register reg);
void print_flags(u16 flags);
void print_instruction(CPU *cpu, u8 with_end_line);

#endif
