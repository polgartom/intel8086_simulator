#include "simulator.h"
#include "decoder.h"
#include "printer.h"

#include <time.h>
#include <sys/timeb.h>
#include <memory.h>

#ifdef GRAPHICS_ENABLED

#include "SDL.h"
#define VIDEO_RAM_SIZE 0x10000
#define GRAPHICS_UPDATE_DELAY 360000

#endif

#define SIGN_BIT(__wide) (__wide ? (1 << 15) : (1 << 7))
#define MASK_BY_WIDTH(__wide) (__wide ? 0xffff : 0xff)
#define SEGMENT_MASK 0xFFFFF // 20bit

// @Cleanup: remove this register_access mess
u16 get_data_from_register(CPU *cpu, Register_Access *src_reg)
{
    u16 index = src_reg->index;
    if (src_reg->size == 2) {
        u16 data = *(u16 *)(cpu->regmem+index);
        return BYTE_SWAP(data);
    }

    return cpu->regmem[index];
}

// @Cleanup: remove this register_access mess
void set_data_to_register(CPU *cpu, Register_Access *dest_reg, u16 data)
{
    // @Debug
    u16 current_data = get_data_from_register(cpu, dest_reg);
    printf(" \n\t\t@%s: %#02x -> %#02x ", register_name(dest_reg->reg), current_data, data);

    u16 index = dest_reg->index;
    if (dest_reg->size == 2) {
        *(u16 *)(cpu->regmem+index) = BYTE_SWAP(data);
        return;
    }

    cpu->regmem[index] = data & 0xFF; 
}

#define get_from_register(_cpu, _reg_enum) \
    get_data_from_register(_cpu, register_access_by_enum(_reg_enum))
    
#define set_to_register(_cpu, _reg_enum, _data) \
    set_data_to_register(_cpu, register_access_by_enum(_reg_enum), _data)

u32 calc_absolute_memory_address(CPU *cpu, Effective_Address_Expression *expr)
{
    u16 address = 0;
    u16 segment = expr->segment; // This a constant segment value like in the asm: jmp 5312:2891
    u32 mask = 0xFFFF; // 16bit mask

    Register extended_with_this_segment_reg = cpu->instruction.extend_with_this_segment;
    if ((cpu->instruction.flags & Inst_Segment) && extended_with_this_segment_reg != Register_none) {
        segment = get_from_register(cpu, extended_with_this_segment_reg);
        mask = SEGMENT_MASK;
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

    u32 result = ((segment << 4) + address + expr->displacement) & mask;

    //printf("\n\t\t*[%#02x]", result);
    return result;
}

u32 calc_inst_pointer_address(CPU *cpu)
{
    u16 segment = get_from_register(cpu, Register_cs);
    u16 offset  = cpu->ip;

    return (((segment << 4) + offset)) & SEGMENT_MASK;
}

u32 calc_stack_pointer_address(CPU *cpu)
{
    u16 segment = get_from_register(cpu, Register_ss);
    u16 offset  = get_from_register(cpu, Register_sp);

    return (((segment << 4) + offset)) & SEGMENT_MASK;
}

u16 get_data_from_memory(CPU *cpu, u32 address)
{
    address = address & SEGMENT_MASK;

    if (cpu->instruction.flags & Inst_Wide) {
        u16 data = *(u16 *)(cpu->memory+address);
        return BYTE_SWAP(data);
    }

    return cpu->memory[address];
}

void set_data_to_memory(CPU *cpu, u32 address, u16 data)
{
    address = address & SEGMENT_MASK; 
    
    // @Todo: @Debug: Print out the memory address in this format 0000:0xFFF, so with the segment and the offset
    u16 current_data = get_data_from_memory(cpu, address); // @Debug
    printf("\n\t\t[%d]: %#02x -> %#02x", address, current_data, data);

    if (cpu->instruction.flags & Inst_Wide) {
        *(u16 *)(cpu->memory+address) = BYTE_SWAP(data);
        return;
    }

    cpu->memory[address] = data & 0xFF;
} 

u16 get_from_operand(CPU *cpu, Instruction_Operand *op)
{
    u16 data = 0;

    if (op->type == Operand_Register) {
        Register_Access *reg = register_access(op->reg, op->flags);
        data = get_data_from_register(cpu, reg);
    }
    else if (op->type == Operand_Immediate) {
        data = op->immediate;
    }
    else if (op->type == Operand_Memory) {
        u32 address = calc_absolute_memory_address(cpu, &op->address);
        data = get_data_from_memory(cpu, address);
    }

    return data;
}

void set_to_operand(CPU *cpu, Instruction_Operand *op, u16 data)
{
    if (op->type == Operand_Register) {
        Register_Access *dest_reg = register_access(op->reg, op->flags);
        set_data_to_register(cpu, dest_reg, data);
    }
    else if (op->type == Operand_Memory) {
        u32 address = calc_absolute_memory_address(cpu, &op->address);
        set_data_to_memory(cpu, address, data);
    }
    else {
        printf("\n[ERROR]: How do you wannna put value in the immediate?\n");
        assert(0);
    }
}

void stack_push(CPU *cpu, u16 data)
{
    u16 sp_val = get_from_register(cpu, Register_sp);
    sp_val -= 2;
    set_to_register(cpu, Register_sp, sp_val);

    u32 absolute_address = calc_stack_pointer_address(cpu);
    set_data_to_memory(cpu, absolute_address, data);
}

u16 stack_pop(CPU *cpu)
{
    u32 absolute_address = calc_stack_pointer_address(cpu);
    u16 data = get_data_from_memory(cpu, absolute_address);

    u16 sp_val = get_from_register(cpu, Register_sp);
    sp_val += 2;
    set_to_register(cpu, Register_sp, sp_val);

    return data;
}

void stack_push_flags(CPU *cpu)
{
    stack_push(cpu, cpu->flags);
}

void stack_pop_flags(CPU *cpu)
{
    u16 old_flags = cpu->flags;

    cpu->flags = 0;
    cpu->flags |= stack_pop(cpu);

    print_out_formated_flags(old_flags, cpu->flags);
}

void execute_interrupt(CPU *cpu, u16 interrupt_type)
{
    stack_push_flags(cpu);
    stack_push(cpu, get_from_register(cpu, Register_cs));
    stack_push(cpu, cpu->ip);
    cpu->flags &= ~(F_TRAP|F_INTERRUPT);

    // The interrupt pointer address is 4 byte, the first 2 byte refer to the offset (ip)
    // and the remained 2 byte is the segment (cs) of the address.
    u16 interrupt_address = interrupt_type * 4;
    u16 ip_val = get_data_from_memory(cpu, interrupt_address);
    u16 cs_val = get_data_from_memory(cpu, interrupt_address+2);

    cpu->ip = ip_val;
    set_to_register(cpu, Register_cs, cs_val);
}

void update_parity_flag(CPU *cpu, u32 result)
{
    cpu->flags &= ~(F_PARITY);

    u8 ones = 0;
    u8 size = (cpu->instruction.flags & Inst_Wide) ? 16 : 8;
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

    s32 left_val  = get_from_operand(cpu, left_op);
    s32 right_val = get_from_operand(cpu, right_op);

    u32 sign_bit = SIGN_BIT(is_wide);
    u32 mask = MASK_BY_WIDTH(is_wide);

    // @Debug
    u32 ip_before = cpu->ip;
    u32 ip_after  = cpu->ip;

    switch (i->mnemonic) {
        // :Arithmatic
        case Mneumonic_mov: {
            set_to_operand(cpu, left_op, right_val);
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
            set_to_operand(cpu, left_op, result);

            break;
        }
        case Mneumonic_sub: {
            u32 result = left_val - right_val;
            set_to_operand(cpu, left_op, result);

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
            u16 divisior = left_val;
            
            if (divisior == 0) {
                // @Todo: execute an interrupt? @Incomplete
                assert(0);
            } 

            if (is_wide) {
                u16 divident = get_from_register(cpu, Register_ax);
                u32 unmasked_result = (u32)divident / (u32)divisior;
                u16 remainder = divident % divisior;
                
                set_to_register(cpu, Register_ax, unmasked_result);
                set_to_register(cpu, Register_dx, remainder);
                
                update_arith_flags(cpu, unmasked_result, 0, 0);
            } else {
                u16 divident = get_from_register(cpu, Register_ax);
                u32 unmasked_result = (u32)divident / (u32)divisior;
                u8 remainder = divident % divisior;
                
                set_to_register(cpu, Register_al, unmasked_result);
                set_to_register(cpu, Register_ah, remainder);
                
                update_arith_flags(cpu, unmasked_result, 0, 0);
            }

            break;
        }
        case Mneumonic_inc: {
            u32 result = left_val+1;
            set_to_operand(cpu, left_op, result);
            update_arith_flags(cpu, result, 0, 0);
            break;
        }
        case Mneumonic_dec: {
            u32 result = left_val-1;
            set_to_operand(cpu, left_op, result);
            update_arith_flags(cpu, result, 0, 0);
            break;
        }
        case Mneumonic_mul: {
            u32 result = left_val * right_val;
            set_to_operand(cpu, left_op, result);
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
            set_to_operand(cpu, left_op, result);
            update_log_flags(cpu, result);
            break;
        }
        case Mneumonic_not: {
            u32 result = ~left_val;
            set_to_operand(cpu, left_op, result);
            break;
        }
        case Mneumonic_or: {
            u32 result = left_val |= right_val;
            set_to_operand(cpu, left_op, result);
            update_log_flags(cpu, result);
            break;
        }
        case Mneumonic_xor: {
            u32 result = left_val ^= right_val;
            set_to_operand(cpu, left_op, result);
            update_log_flags(cpu, result);
            break;
        }
        // :Flow
        case Mneumonic_jmp: {
            assert(!(i->flags & Inst_Far)); // @Todo: Unimplemented
            ip_after += i->operands[0].immediate;
            break;
        }
        case Mneumonic_jl: {
            u8 SF = !!(cpu->flags & F_SIGNED);
            u8 OF = !!(cpu->flags & F_OVERFLOW);
            if (SF ^ OF) {
                ip_after += i->operands[0].immediate;
            }
            break;
        }
        case Mneumonic_jle: {
            u8 SF = !!(cpu->flags & F_SIGNED);
            u8 OF = !!(cpu->flags & F_OVERFLOW);
            u8 ZF = !!(cpu->flags & F_ZERO);
            if (((SF ^ OF) | ZF) == 1) {
                ip_after += i->operands[0].immediate;
            }
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
            u16 cx_data = get_from_register(cpu, Register_cx);
            cx_data -= 1;
            set_to_register(cpu, Register_cx, cx_data);

            if (cx_data != 0) {
                ip_after += i->operands[0].immediate;
            }

            break;
        }
        // :Stack
        case Mneumonic_push: {
            u16 data = get_from_operand(cpu, left_op);
            stack_push(cpu, data);
            break;
        }
        case Mneumonic_pushf: {
            stack_push_flags(cpu);
            break;
        }
        case Mneumonic_pop: {
            u16 data = stack_pop(cpu);
            set_to_operand(cpu, left_op, data);
            break;
        }
        case Mneumonic_popf: {
            stack_pop_flags(cpu);
            break;
        }
        // ???
        case Mneumonic_cld: {
            cpu->flags &= ~F_DIRECTION;
            break;
        }
        // :Interrupt
        // case Mneumonic_int3: // We're decoding the int3 as int and 3 immediate value
        case Mneumonic_int: {
            u16 interrupt_type = get_from_operand(cpu, left_op);
            execute_interrupt(cpu, interrupt_type);
            break;
        }
        case Mneumonic_into: {
            if (cpu->flags & F_OVERFLOW) {
                execute_interrupt(cpu, 4);
            }
            break;
        }
        case Mneumonic_iret: {
            cpu->ip = stack_pop(cpu);
            u16 cs_val = stack_pop(cpu);
            set_to_register(cpu, Register_cs, cs_val);

            stack_pop_flags(cpu);

            break;
        }
        // :String
        case Mneumonic_movsb: {
            // @Todo: rep
            u16 ds_val = get_from_register(cpu, Register_ds);
            u16 si_val = get_from_register(cpu, Register_si);
            u32 src_address = (ds_val << 4) + si_val;

            u16 di_val = get_from_register(cpu, Register_di);
            u16 es_val = get_from_register(cpu, Register_es);
            u32 dest_adress = (es_val << 4) + di_val;

            u16 data = get_data_from_memory(cpu, src_address);
            set_data_to_memory(cpu, dest_adress, data);

            if (cpu->flags & F_DIRECTION) {
                si_val -= 1;
                di_val -= 1;
            } else {
                si_val += 1;
                di_val += 1;
            }

            break;
        }
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
                set_data_to_memory(cpu, absolute_address, ax);

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
                set_data_to_memory(cpu, absolute_address, al);

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
            u16 port = left_val;
            u16 data = right_val;

            // @Debug
            fprintf(cpu->out, "%d : %d\n", port, data);
        
            break;
        }
        default: {
            printf("\n[WARNING]: This instruction: %s is not handled yet!\n", mnemonic_name(i->mnemonic));
            cpu->terminate = 1; // @Temporary
        }
    }

    // This instruction pointer data will provide us the next instruction location from the cpu->instructions array which indexed
    // based on the instruction byte index at loaded binary file.
    // @Bug @Todo: This will cause a bug if the ip address overflow, because if we incremented the ip value,
    //  then the cs register value has to be incremented too if the ip register value is left his range.
    ip_after += i->size;
    cpu->ip = ip_after;

    printf("\n\t\t@ip: %#02x -> %#02x\n", ip_before, cpu->ip);
    printf("\n");
}

void load_executable(CPU *cpu, char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("\n[ERROR]: Failed to open %s file. Probably it is not exists.\n", filename);
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
    set_to_register(cpu, Register_cs, 0xf000);
    printf("\n");
    cpu->ip = 0x0100;
}

#ifdef GRAPHICS_ENABLED
void swapFramebufferVertically(u16* framebuffer, int width, int height) {
    int rowSize = width * sizeof(u16);
    u32 tempRow[rowSize];

    for (int row = 0; row < height / 2; row++) {
        int topRow = row;
        int bottomRow = height - 1 - row;

        // Calculate the start indices of the rows
        int topRowStart = topRow * width;
        int bottomRowStart = bottomRow * width;

        // Swap the rows
        memcpy(tempRow, &framebuffer[topRowStart], rowSize);
        memcpy(&framebuffer[topRowStart], &framebuffer[bottomRowStart], rowSize);
        memcpy(&framebuffer[bottomRowStart], tempRow, rowSize);
    }
}
#endif

void run(CPU *cpu)
{
    char input[128] = {0};

    if (cpu->decode_only) {
        printf("bits 16\n\n");
    }

#ifdef GRAPHICS_ENABLED
    u16 GRAPHICS_X = 256;
    u16 GRAPHICS_Y = GRAPHICS_X;

    SDL_Surface *sdl_screen;
    SDL_Event sdl_event;

    SDL_Init(SDL_INIT_VIDEO);
    sdl_screen = SDL_SetVideoMode(GRAPHICS_X, GRAPHICS_Y, 8*2, 0);
    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(500, 30);

    int pixel_colors[16];
    int vid_addr_lookup[VIDEO_RAM_SIZE];
    u32 *vid_mem_base; 

    vid_mem_base = (u32*)&cpu->memory[0];

	for (int i = 0; i < 16; i++) {
        pixel_colors[i] = 0xFF*(((i & 1) << 24) + ((i & 2) << 15) + ((i & 4) << 6) + ((i & 8) >> 3));
    }

	for (int i = 0; i < GRAPHICS_X * GRAPHICS_Y / 4; i++) {
		vid_addr_lookup[i] = i / GRAPHICS_X * (GRAPHICS_X / 8) + (i / 2) % (GRAPHICS_X / 8) + 0x2000*((4 * i / GRAPHICS_X) % 4);
    }
#endif

    u32 timer = 0;

    do {
        timer++;
        decode_next_instruction(cpu);

        // @Todo: The i8086 contains the debug flag so later we simulate this too
        // instead of this boolean
        if (cpu->debug_mode) {
            //printf(">> Press enter to the next instruction\n");
__de:;
            fgets(input, sizeof(input), stdin);
            if (STR_EQUAL("exit\n", input) || input[0] == 'q') {
                return;
            }
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

            continue;
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

            // @Temporary
            if (cpu->terminate) {
                return;
            }
        }

#ifdef GRAPHICS_ENABLED
        if ((timer % GRAPHICS_UPDATE_DELAY) == 0) {
            u32 pixels = GRAPHICS_X*GRAPHICS_Y;

/*
            // 1x scale
            for (u32 i = 0; i < pixels; i++) {
                u16 *pxptr = ((u16*)sdl_screen->pixels); 
                u16 color = ((u16*)vid_mem_base)[i];
                pxptr[i] = BYTE_SWAP(color);
            } 
*/

            // 2x scale
            u32 y = 0;
            u32 j = 0;
            for (u32 i = 0; i < (GRAPHICS_X/2)*(GRAPHICS_Y/2); i++) {
                u16 *pxptr = ((u16*)sdl_screen->pixels); 
                u16 color = BYTE_SWAP(((u16*)vid_mem_base)[i]);

                if (j!=0 && ((j) % GRAPHICS_X) == 0) {
                    j += GRAPHICS_X;
                }

                pxptr[j] = color;
                pxptr[j+1] = color;
                pxptr[j+GRAPHICS_X] = color;
                pxptr[j+GRAPHICS_X+1] = color;

                j+=2;
            }

            swapFramebufferVertically((u16*)sdl_screen->pixels, GRAPHICS_X, GRAPHICS_Y);
            SDL_Flip(sdl_screen);
        }
#endif

    // @Todo: Another option to check end of the executable?
    } while (calc_inst_pointer_address(cpu) < cpu->exec_end);

}
