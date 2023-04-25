#include "simulator.h"
#include "decoder.h"
#include "printer.h"


// @Todo: Collective memory as like in the real hardware 
u8 regmem[64] = {0}; 

s32 get_data_from_register(Register_Info *reg)
{
    u16 lower_mem_index = reg->mem_offset;
    if (reg->mem_size == 2) {
        s16 val_lo = regmem[lower_mem_index] << 0;
        s16 val_hi = regmem[lower_mem_index+1] << 8;

        return (val_hi | val_lo);
    }
 
    return regmem[lower_mem_index];
}

void set_data_to_register(Register_Info *dest, u16 data) 
{
    u16 lower_mem_index = dest->mem_offset;

    if (dest->mem_size == 2) {
        regmem[lower_mem_index] = ((data >> 0) & 0xFF);
        regmem[lower_mem_index+1] = ((data >> 8) & 0xFF);

        return;
    }

    regmem[lower_mem_index] = data;
}

u16 get_memory_address(Effective_Address_Expression *expr)
{ 
    Register_Info *reg = NULL;
    u16 displacement = expr->displacement;
    u16 address = 0; // @Todo: segment stuff

    // @Cleanup
    switch (expr->base) {
        case Effective_Address_direct:
            // In this case the address will be equal to the displacement
            break;
        case Effective_Address_bx_si:
            reg = get_register_info_by_enum(Register_bx);
            address = get_data_from_register(reg);
            reg = get_register_info_by_enum(Register_si);
            address += get_data_from_register(reg);

            break;
        case Effective_Address_bx_di:
            reg = get_register_info_by_enum(Register_bx);
            address = get_data_from_register(reg);
            reg = get_register_info_by_enum(Register_di);
            address += get_data_from_register(reg);

            break;
        case Effective_Address_bp_si:
            reg = get_register_info_by_enum(Register_bp);
            address = get_data_from_register(reg);
            reg = get_register_info_by_enum(Register_si);
            address += get_data_from_register(reg);

            break;
        case Effective_Address_bp_di:
            reg = get_register_info_by_enum(Register_bp);
            address = get_data_from_register(reg);
            reg = get_register_info_by_enum(Register_di);
            address += get_data_from_register(reg);

            break;
        case Effective_Address_si:
            reg = get_register_info_by_enum(Register_si);
            address = get_data_from_register(reg);

            break;
        case Effective_Address_di:
            reg = get_register_info_by_enum(Register_di);
            address = get_data_from_register(reg);

            break;
        case Effective_Address_bp:
            reg = get_register_info_by_enum(Register_bp);
            address = get_data_from_register(reg);

            break;
        case Effective_Address_bx:
            reg = get_register_info_by_enum(Register_bx);
            address = get_data_from_register(reg);

            break;
        default:
            assert(0);
    }

    return address + displacement;
}

s16 get_data_from_memory(u8 *memory, u32 address, u8 is_16bit)
{
    s16 data = (u8)memory[address];
    if (is_16bit) {
        data = BYTE_LOHI_TO_HILO(data, (u8)memory[address+1]);
    }

    return data;
}

void set_data_to_memory(u8 *memory, u32 address, u8 is_16bit, u16 data)
{
    memory[address] = (u8)(data & 0x00FF);
    if (is_16bit) {
        memory[address+1] = (u8)((data >> 8) & 0x00FF);
    }
}

s32 get_data_from_operand(CPU *cpu, Instruction_Operand *op, u8 is_16bit)
{
    s32 src_data = 0;

    if (op->type == Operand_Register) {
        Register_Info *reg = get_register_info(is_16bit, op->reg);
        src_data = get_data_from_register(reg);
    } 
    else if (op->type == Operand_Immediate) {
        if (cpu->instruction.flags & FLAG_IS_SIGNED) {
            src_data = op->immediate_s16;
        } else {
            src_data = op->immediate_u16;
        }
    } 
    else if (op->type == Operand_Memory) {
        u16 address = get_memory_address(&op->address);
        src_data = get_data_from_memory(cpu->memory, address, is_16bit);
    }

    return src_data;
}

void set_data_to_operand(CPU *cpu, Instruction_Operand *op, u8 is_16bit, u16 data)
{
    s32 current_data = get_data_from_operand(cpu, op, is_16bit);

    if (op->type == Operand_Register) {
        Register_Info *reg = get_register_info(is_16bit, op->reg);
        set_data_to_register(reg, data);
    }
    else if (op->type == Operand_Memory) {
        u16 address = get_memory_address(&op->address);
        set_data_to_memory(cpu->memory, address, is_16bit, data);
    }
    else {
        printf("[ERROR]: How do you wannna put value in the immediate?\n");
        assert(0);
    }

    printf(" %#02x -> %#02x |", current_data, data);
}

void execute_instruction(CPU *cpu)
{
    Instruction *i = &cpu->instruction;
    Instruction_Operand dest_op = i->operands[0];
    u8 is_16bit = i->flags & FLAG_IS_16BIT;

    u32 ip_data_before = cpu->ip_data; 
    u32 ip_data_after  = cpu->ip_data;

    printf(" ;");

    if (i->type == Instruction_Type_Move) {
        Instruction_Operand src_op = i->operands[1];
        u16 src_data = get_data_from_operand(cpu, &src_op, is_16bit);
        set_data_to_operand(cpu, &dest_op, is_16bit, src_data);
        
        ip_data_after += i->size; 
    }
    else if (i->type == Instruction_Type_Arithmetic) {
        Instruction_Operand src_op = i->operands[1];
        u16 src_data = get_data_from_operand(cpu, &src_op, is_16bit);
        s32 dest_data = get_data_from_operand(cpu, &dest_op, is_16bit);

        switch (i->opcode) {
            case Opcode_add: 
                dest_data += src_data;
                set_data_to_operand(cpu, &dest_op, is_16bit, dest_data);
                break;
            case Opcode_sub:
                dest_data -= src_data;
                set_data_to_operand(cpu, &dest_op, is_16bit, dest_data);
                break;
            case Opcode_cmp:
                dest_data -= src_data;
                break;
            default:
                printf("[WARNING]: This opcode: %s is have not handled yet!", get_opcode_name(i->opcode));
                assert(0);
        }

        u16 new_flags = cpu->flags;
        new_flags &= (~(F_SIGNED|F_ZERO)); // clear flags

        if (dest_data == 0) {
            new_flags |= F_ZERO;
        } 
        else if (dest_data >> 15) {
            new_flags |= F_SIGNED;
        }

        printf(" flags: [");
        print_flags(cpu->flags);
        printf("] -> [");
        print_flags(new_flags);
        printf("]");

        cpu->flags = new_flags;
        ip_data_after += i->size; 
    }
    else if (i->type == Instruction_Type_Jump) {
        switch (i->opcode) {
            case Opcode_jnz: {
                if (cpu->flags & F_ZERO) {
                    ip_data_after += i->size;
                } else {
                    ip_data_after += i->operands[0].immediate_s16;
                }
                break;
            }
            case Opcode_loop: {
                Register_Info *cx = get_register_info_by_enum(Register_cx);
                s16 cx_data = (s16)get_data_from_register(cx);
                cx_data -= 1;
                set_data_to_register(cx, cx_data);
                
                if (cx_data != 0) {
                    ip_data_after += i->operands[0].immediate_s16;
                } else {
                    ip_data_after += i->size;
                }

                break;
            }

            default:
                printf("\n[ERROR]: This opcode: '%s' is have not handled yet!\n", get_opcode_name(i->opcode));
                assert(0);
        }
    }

    // This instruction pointer data will provide us the next instruction location from the cpu->instructions array which indexed
    // based on the instruction byte index at loaded binary file.
    cpu->ip_data = ip_data_after;

    printf(" | ip: %#02x -> %#02x\n", ip_data_before, cpu->ip_data);
}

/*
void *allocate_memory(u16 size)
{
    
}
*/

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

    assert(fsize+1 <= MAX_BINARY_FILE_SIZE);

    // @Todo: Dynamic memory allocation  
    fread(&cpu->memory[0], fsize, 1, fp);
    fclose(fp);

    cpu->loaded_executable_size = fsize;
    cpu->ip_data = 0; // cpu->memory[0] memory index
}

void run(CPU *cpu)
{
    // @Todo: Check the loaded executable memory address, but now we always put the executable to beginning of the memory
    cpu->ip_data = 0;

    char input[128] = {0};

    printf(">> Press enter to the next instruction\n");
    do {
        decode_next_instruction(cpu);

//        if (cpu->decode_only) {
//            print_instruction(cpu, 1);
//            
//            We have to update this "manually", because here we only printing and not executing, so the ip_data won't update! 
//            cpu->ip_data = cpu->decoder_cursor;
//         }

        // @Todo: The i8086/88 contains the debug flag so later we simulate this too
        if (0) {
__de:;
            fgets(input, sizeof(input), stdin);
            if (input[0] != '\n') {
                goto __de;
            }
        }

        print_instruction(cpu, 0);
        execute_instruction(cpu);

    // @Todo: Another option to check end of the executable?
    } while (cpu->ip_data != cpu->loaded_executable_size);
}
