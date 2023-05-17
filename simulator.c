#include "simulator.h"
#include "decoder.h"
#include "printer.h"

#define SIGN_BIT(__wide) (__wide ? (1 << 15) : (1 << 7))
#define MASK_BY_WIDTH(__wide) (__wide ? 0xffff : 0xff)

u16 get_data_from_register(CPU *cpu, Register_Access *src_reg)
{
    u16 lower_mem_index = src_reg->index;
    if (src_reg->size == 2) {
        s16 val_lo = cpu->regmem[lower_mem_index] << 0;
        s16 val_hi = cpu->regmem[lower_mem_index+1] << 8;

        return (val_hi | val_lo);
    }

    return cpu->regmem[lower_mem_index];
}

void set_data_to_register(CPU *cpu, Register_Access *dest_reg, u16 data)
{
    // @Debug
    u16 current_data = get_data_from_register(cpu, dest_reg);
    printf(" \n\t\t@%s: %#02x -> %#02x ", register_name(dest_reg->reg), current_data, data);

    u16 lower_mem_index = dest_reg->index;
    if (dest_reg->size == 2) {
        cpu->regmem[lower_mem_index] = ((data >> 0) & 0xFF);
        cpu->regmem[lower_mem_index+1] = ((data >> 8) & 0xFF);

        return;
    }

    cpu->regmem[lower_mem_index] = data;
}

#define get_from_register(_cpu, _reg_enum) get_data_from_register(_cpu, register_access_by_enum(_reg_enum))
#define set_to_register(_cpu, _reg_enum, _data) set_data_to_register(_cpu, register_access_by_enum(_reg_enum), _data)


u32 calc_absolute_memory_address(CPU *cpu, Effective_Address_Expression *expr)
{
    u16 address = 0;
    u16 segment = expr->segment; // This a constant segment value like in the asm: jmp 5312:2891

    if ((cpu->instruction.flags & Inst_Segment) && cpu->extend_with_this_segment != Register_none) {
        Register_Access *reg_access = register_access_by_enum(cpu->extend_with_this_segment);
        segment = get_data_from_register(cpu, reg_access);
    }

    switch (expr->base) {
        case Effective_Address_direct:
            break;
        case Effective_Address_bx_si:
            address = get_from_register(cpu, Register_bx);
            address += get_from_register(cpu, Register_si);

            break;
        case Effective_Address_bx_di:
            address = get_from_register(cpu, Register_bx);
            address += get_from_register(cpu, Register_di);

            break;
        case Effective_Address_bp_si:
            address = get_from_register(cpu, Register_bp);
            address += get_from_register(cpu, Register_si);

            break;
        case Effective_Address_bp_di:
            address = get_from_register(cpu, Register_bp);
            address += get_from_register(cpu, Register_di);

            break;
        case Effective_Address_si:
            address = get_from_register(cpu, Register_si);

            break;
        case Effective_Address_di:
            address = get_from_register(cpu, Register_di);

            break;
        case Effective_Address_bp:
            address = get_from_register(cpu, Register_bp);

            break;
        case Effective_Address_bx:
            address = get_from_register(cpu, Register_bx);

            break;
        default:
            assert(0);
    }

    return (segment << 4) + address + expr->displacement;
}

u32 calc_inst_pointer_address(CPU *cpu)
{
    u16 segment = get_from_register(cpu, Register_cs);
    u16 offset  = cpu->ip;

    return (segment << 4) + offset;
}

u32 calc_stack_pointer_address(CPU *cpu)
{
    u16 segment = get_from_register(cpu, Register_ss);
    u16 offset  = get_from_register(cpu, Register_sp);

    return (segment << 4) + offset;
}

s32 get_data_from_memory(u8 *memory, u32 address, u8 is_16bit)
{
    s16 data = (u8)memory[address];
    if (is_16bit) {
        data = BYTE_LOHI_TO_HILO(data, (u8)memory[address+1]);
    }

    return data;
}

void set_data_to_memory(u8 *memory, u32 address, u8 is_16bit, u16 data)
{
    s32 current_data = get_data_from_memory(memory, address, is_16bit); // @Debug

    // @Todo: @Debug: Print out the memory address in this format 0000:0xFFF, so with the segment and the offset
    printf("\n\t\t[%d]: %#02x -> %#02x", address, current_data, data);

    memory[address] = (u8)(data & 0x00FF);
    if (is_16bit) {
        memory[address+1] = (u8)((data >> 8) & 0x00FF);
    }
}

s32 get_data_from_operand(CPU *cpu, Instruction_Operand *op, u8 is_16bit)
{
    s32 src_data = 0;

    if (op->type == Operand_Register) {
        Register_Access *src_reg = register_access(op->reg, op->flags);
        src_data = get_data_from_register(cpu, src_reg);
    }
    else if (op->type == Operand_Immediate) {
        src_data = op->immediate;
    }
    else if (op->type == Operand_Memory) {
        u16 address = calc_absolute_memory_address(cpu, &op->address);
        src_data = get_data_from_memory(cpu->memory, address, is_16bit);
    }

    return src_data;
}

void set_data_to_operand(CPU *cpu, Instruction_Operand *op, u8 is_16bit, u16 data)
{
    if (op->type == Operand_Register) {
        Register_Access *dest_reg = register_access(op->reg, op->flags);
        set_data_to_register(cpu, dest_reg, data);
    }
    else if (op->type == Operand_Memory) {
        u16 address = calc_absolute_memory_address(cpu, &op->address);
        set_data_to_memory(cpu->memory, address, is_16bit, data);
    }
    else {
        printf("[ERROR]: How do you wannna put value in the immediate?\n");
        assert(0);
    }
}

void update_parity_flag(CPU *cpu, u32 result)
{
    cpu->flags &= ~(F_PARITY);

    u8 ones = 0;
    u8 size = (cpu->instruction.flags & Inst_Wide) ? 16 : 8;
    u8 i = size;
    for (u8 i = 0; i < size; i++) {
        if (result & (1<<i)) ones++;
    }

    cpu->flags |= ((ones % 2) == 0) ? F_PARITY : 0;
}

void update_common_flags(CPU *cpu, u32 result)
{
    u8 is_wide = cpu->instruction.flags & Inst_Wide;
    u32 sign_bit = SIGN_BIT(is_wide);

    cpu->flags &= (~(F_SIGNED|F_ZERO));

    cpu->flags |= (result & sign_bit ? F_SIGNED : 0);
    cpu->flags |= (result & MASK_BY_WIDTH(is_wide)) == 0 ? F_ZERO : 0;

    update_parity_flag(cpu, result);
}

void update_log_flags(CPU *cpu, u32 result)
{
    cpu->flags &= (~(F_OVERFLOW|F_CARRY|F_AUXILIARY));
    update_common_flags(cpu, result);
}

void update_arith_flags(CPU *cpu, u32 result, u32 OF, u32 AF)
{
    u32 flags_before = cpu->flags;

    u32 sign_bit = SIGN_BIT(cpu->instruction.flags & Inst_Wide);
    u32 CF = result & (sign_bit << 1);

    cpu->flags &= (~(F_CARRY|F_OVERFLOW|F_AUXILIARY));
    cpu->flags |= CF ? F_CARRY : 0;
    cpu->flags |= OF ? F_OVERFLOW : 0;
    cpu->flags |= AF ? F_AUXILIARY : 0;

    update_common_flags(cpu, result);

    print_out_formated_flags(flags_before, cpu->flags);
}

void execute_instruction(CPU *cpu)
{
    Instruction *i = &cpu->instruction;
    u8 is_wide = (i->flags & Inst_Wide) ? 1 : 0;

    Instruction_Operand *left_op  = &i->operands[0];
    Instruction_Operand *right_op = &i->operands[1];

    s32 left_val  = get_data_from_operand(cpu, left_op, is_wide);
    s32 right_val = get_data_from_operand(cpu, right_op, is_wide);

    u32 sign_bit = SIGN_BIT(is_wide);
    u32 mask = MASK_BY_WIDTH(is_wide);

    // @Debug
    u32 ip_before = cpu->ip;
    u32 ip_after  = cpu->ip;

    switch (i->mnemonic) {
        // :Arithmatic
        case Mneumonic_mov: {
            set_data_to_operand(cpu, left_op, is_wide, right_val);
            break;
        }
        case Mneumonic_add: {
            u32 result = (left_val & mask) + (right_val & mask);

            u32 OF = (~(left_val ^ right_val) & (left_val ^ result)) & sign_bit;

            // This is the same as above, just "unpacked" for visibility purpose
            // u32 OF = 0;
            // if ((left_val & sign_bit) == (right_val & sign_bit)) {
            //    // The commented out part with /**/ is not required, because we already know at this point that the left and right
            //    // sign extension the same
            //    if ((left_val & sign_bit) != (result & sign_bit) /*&& (right_val & sign_bit) != (result & sign_bit)*/) {
            //        OF = 1;
            //    }
            // }

            u32 AF = ((left_val & 0xf) + (right_val & 0xf)) & 0x10;

            update_arith_flags(cpu, result, OF, AF);
            set_data_to_operand(cpu, left_op, is_wide, result);

            break;
        }
        case Mneumonic_sub: {
            u32 result = left_val - right_val;
            set_data_to_operand(cpu, left_op, is_wide, result);

            u32 OF = ((left_val ^ right_val) & (left_val ^ result)) & sign_bit;
            u32 AF = ((left_val & 0xf) - (right_val & 0xf)) & 0x10;

            update_arith_flags(cpu, result, OF, AF);

            break;
        }
        case Mneumonic_cmp: {
            u32 result = left_val - right_val;

            u32 OF = ((left_val ^ right_val) & (left_val ^ result)) & sign_bit;
            u32 AF = ((left_val & 0xf) - (right_val & 0xf)) & 0x10;
            update_arith_flags(cpu, result, OF, AF);

            break;
        }
        case Mneumonic_div: {
            if (left_val == 0) {
                // @Todo: execute an interrupt? @Incomplete
                assert(0);
            } else {
                u32 result = left_val / right_val;
                set_data_to_operand(cpu, left_op, is_wide, result);
                update_arith_flags(cpu, result, 0, 0);
            }

            break;
        }
        case Mneumonic_inc: {
            u32 result = left_val+1;
            set_data_to_operand(cpu, left_op, is_wide, result);
            update_arith_flags(cpu, result, 0, 0);
            break;
        }
        case Mneumonic_dec: {
            u32 result = left_val-1;
            set_data_to_operand(cpu, left_op, is_wide, result);
            update_arith_flags(cpu, result, 0, 0);
            break;
        }
        case Mneumonic_mul: {
            u32 result = left_val * right_val;
            set_data_to_operand(cpu, left_op, is_wide, result);
            update_arith_flags(cpu, result, 0, 0);
            break;
        }
        // :Logical
        case Mneumonic_test: {
            u32 result = left_val &= right_val;
            update_log_flags(cpu, result);
            break;
        }
        case Mneumonic_and: {
            u32 result = left_val &= right_val;
            set_data_to_operand(cpu, left_op, is_wide, result);
            update_log_flags(cpu, result);
            break;
        }
        case Mneumonic_not: {
            u32 result = ~left_val;
            set_data_to_operand(cpu, left_op, is_wide, result);
            break;
        }
        case Mneumonic_or: {
            u32 result = left_val |= right_val;
            set_data_to_operand(cpu, left_op, is_wide, result);
            update_log_flags(cpu, result);
            break;
        }
        case Mneumonic_xor: {
            u32 result = left_val ^= right_val;
            set_data_to_operand(cpu, left_op, is_wide, result);
            update_log_flags(cpu, result);
            break;
        }
        // :Flow
        case Mneumonic_jmp: {
            assert(!(i->flags & Inst_Far)); // @Todo: Unimplemented
            ip_after += i->operands[0].immediate;
            break;
        }
        case Mneumonic_jz: {
            if (cpu->flags & F_ZERO) {
                ip_after += i->operands[0].immediate;
            }
            break;
        }
        case Mneumonic_jnz: {
            if (!(cpu->flags & F_ZERO)) {
                ip_after += i->operands[0].immediate;
            }
            break;
        }
        case Mneumonic_ja: {
            if (!(cpu->flags & F_ZERO) && !(cpu->flags & F_CARRY)) {
                ip_after += i->operands[0].immediate;
            }
            break;
        }
        case Mneumonic_loop: {
            // loop instruction is always using the cx register value which will be decremented by one
            // every time when loop instruction are called
            Register_Access *reg_access = register_access_by_enum(Register_cx);
            s16 cx_data = get_data_from_register(cpu, reg_access);
            cx_data -= 1;
            set_data_to_register(cpu, reg_access, cx_data);

            if (cx_data != 0) {
                ip_after += i->operands[0].immediate;
            }

            break;
        }
        // :Stack
        case Mneumonic_push: {
            s32 data = get_data_from_operand(cpu, left_op, 1);

            u16 sp_val = get_from_register(cpu, Register_sp);
            sp_val -= 2;
            set_to_register(cpu, Register_sp, sp_val);

            u32 absolute_address = calc_stack_pointer_address(cpu);
            set_data_to_memory(cpu->memory, absolute_address, 1, data);

            break;
        }
        case Mneumonic_pushf: {
            u16 data = cpu->flags;

            u16 sp_val = get_from_register(cpu, Register_sp);
            sp_val -= 2;
            set_to_register(cpu, Register_sp, sp_val);

            u32 absolute_address = calc_stack_pointer_address(cpu);
            set_data_to_memory(cpu->memory, absolute_address, 1, data);

            break;
        }
        case Mneumonic_pop: {
            u32 absolute_address = calc_stack_pointer_address(cpu);
            s32 data = get_data_from_memory(cpu->memory, absolute_address, 1);

            set_data_to_operand(cpu, left_op, 1, data);

            u16 sp_val = get_from_register(cpu, Register_sp);
            sp_val += 2;
            set_to_register(cpu, Register_sp, sp_val);

            break;
        }
        case Mneumonic_popf: {
            u32 absolute_address = calc_stack_pointer_address(cpu);
            s32 data = get_data_from_memory(cpu->memory, absolute_address, 1);
            cpu->flags = data;

            u16 sp_val = get_from_register(cpu, Register_sp);
            sp_val += 2;
            set_to_register(cpu, Register_sp, sp_val);

            break;
        }
        // ???
        case Mneumonic_cld: {
            cpu->flags &= ~F_DIRECTION;
            break;
        }
        // :String
        case Mneumonic_stosw: {
            // @Todo: Repeat if the repeat Prefix (REP/REPE/REPZ or REPNE/REPNZ)
            //  (repeated based on the value in the CX register)
            assert(!(i->flags & Inst_Repnz)); // @Todo

            u16 cx = 1;
            if (i->flags & Inst_Repz) {
                cx = get_from_register(cpu, Register_cx);
            }

            while (cx != 0) {
                Effective_Address_Expression expr = {.base=Effective_Address_di};
                u32 absolute_address = calc_absolute_memory_address(cpu, &expr);

                s32 ax = get_from_register(cpu, Register_ax);
                set_data_to_memory(cpu->memory, absolute_address, 1, ax);

                u16 di = get_from_register(cpu, Register_di);
                if (cpu->flags & F_DIRECTION) di -= 2;
                else                          di += 2;

                set_to_register(cpu, Register_di, di);

                cx -= 1;
            };

            if (i->flags & Inst_Repz) {
                set_to_register(cpu, Register_cx, cx);
            }

            break;
        }
        case Mneumonic_stosb: {
            // @Todo: Repeat if the repeat Prefix (REP/REPE/REPZ or REPNE/REPNZ)
            //  (repeated based on the value in the CX register)
            assert(!(i->flags & Inst_Repnz)); // @Todo

            u16 cx = 1;
            if (i->flags & Inst_Repz) {
                cx = get_from_register(cpu, Register_cx);
            }

            while (cx != 0) {
                Effective_Address_Expression expr = {.base=Effective_Address_di};
                u32 absolute_address = calc_absolute_memory_address(cpu, &expr);

                s32 al = get_from_register(cpu, Register_al);
                set_data_to_memory(cpu->memory, absolute_address, 0, al);

                u16 di = get_from_register(cpu, Register_di);
                if (cpu->flags & F_DIRECTION) di -= 1;
                else                          di += 1;

                set_to_register(cpu, Register_di, di);

                cx -= 1;
            };

            if (i->flags & Inst_Repz) {
                set_to_register(cpu, Register_cx, cx);
            }

            break;
        }
        // :IO
        case Mneumonic_out: {
            // @Todo:
            break;
        }
        default: {
            printf("[WARNING]: This instruction: %s is have not handled yet!\n", mnemonic_name(i->mnemonic));
            assert(0);
        }
    }

    // This instruction pointer data will provide us the next instruction location from the cpu->instructions array which indexed
    // based on the instruction byte index at loaded binary file.
    ip_after += i->size;
    cpu->ip = ip_after;

    printf("\n\t\t@ip: %#02x -> %#02x\n", ip_before, cpu->ip);
}

void load_executable(CPU *cpu, char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("[ERROR]: Failed to open %s file. Probably it is not exists.\n", filename);
        assert(0);
    }

    fseek(fp, 0, SEEK_END);
    u32 fsize = ftell(fp);
    rewind(fp);

    assert(fsize+1 <= MAX_MEMORY);

    u32 inst_absolute_address = calc_inst_pointer_address(cpu);
    fread(&cpu->memory[inst_absolute_address], fsize, 1, fp);
    fclose(fp);

    cpu->loaded_executable_size = fsize;
    cpu->exec_end = inst_absolute_address + fsize;
}

void boot(CPU *cpu)
{
    cpu->memory = (u8*)malloc(MAX_MEMORY);
    ZERO_MEMORY(cpu->memory, MAX_MEMORY);
    ZERO_MEMORY(cpu->regmem, 64);

    // @Cleanup: This is a little-bit wierdo, two different register set
    set_to_register(cpu, Register_cs, 0xF000);
    printf("\n");
    cpu->ip = 0x0100;
}

void run(CPU *cpu)
{
    char input[128] = {0};

    if (cpu->decode_only) {
        printf("bits 16\n\n");
    }

    do {
decode_next:;
        decode_next_instruction(cpu);

        // @Todo: The i8086 contains the debug flag so later we simulate this too
        // instead of this boolean
        if (cpu->debug_mode) {
            //printf(">> Press enter to the next instruction\n");
__de:;
            fgets(input, sizeof(input), stdin);
            if (input[0] != '\n') {
                goto __de;
            }
        }

        if (cpu->instruction.is_prefix) {
            // If we have a prefix, then we don't want to run or print it. We will print at at the next
            // instruction decode, because we're using the nasm syntax.

            // This is a special case, the cpu->decoder_cursor have an absolute address, so we have to "reverse" this absolute address
            // which are calculated with the segment register and the instruction pointer (ip) register offset.
            u16 cs_segment = get_from_register(cpu, Register_cs);
            cpu->ip = cpu->decoder_cursor - (cs_segment << 4);

            goto decode_next;
        }

        if (cpu->decode_only) {
            // We have to update this "manually", because here we only printing and not executing, so the ip won't update!
            // @Incomplete: The problem will appear if the in the instruction the DW or DB directive is defined, because those
            // just raw memory but we must guess somehow which is code and which is just raw data. Maybe we can make context analysis,
            // example, check which segment of the code won't run in any case, maybe we check the jmp instructions for this at first.

            // This is a special case, the cpu->decoder_cursor have an absolute address, so we have to "reverse" this absolute address
            // which are calculated with the segment register and the instruction pointer (ip) register offset.
            u16 cs_segment = get_from_register(cpu, Register_cs);
            cpu->ip = cpu->decoder_cursor - (cs_segment << 4);

            print_instruction(cpu, 1);

        } else {
            print_instruction(cpu, 0);
            execute_instruction(cpu);
        }

    // @Todo: Another option to check end of the executable?
    } while (calc_inst_pointer_address(cpu) < cpu->exec_end);

}
