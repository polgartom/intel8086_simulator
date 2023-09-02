#ifndef _H_PRINTER
#define _H_PRINTER

#include "sim86.h"

const char *mnemonic_name(Mnemonic m);
const char *register_name(Register reg);

void print_flags(u16 flags);
void print_out_formated_flags(u16 old_flags, u16 new_flags);

void print_instruction(CPU *cpu, u8 with_end_line);

/*
static void int_to_bin_str(u64 val, u8 size)
{
    printf("0b");
    for (int i = size-1; i > -1; i--) {
        if (val & (1<<i)) printf("1");
        else              printf("0");
    }
    printf("\n");
}
*/


#endif
