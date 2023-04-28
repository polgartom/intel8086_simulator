#ifndef _H_PRINTER
#define _H_PRINTER

#include "sim86.h"

const char *get_opcode_name(Opcode opcode);
const char *register_name(Register reg);
void print_flags(u16 flags);
void print_instruction(CPU *cpu, u8 with_end_line);

#endif
