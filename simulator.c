#include "simulator.h"
#include "decoder.h"
#include "printer.h"

s32 get_data_from_register(CPU *cpu, Register_Access *src_reg)
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
    u16 lower_mem_index = dest_reg->index;
    if (dest_reg->size == 2) {
        cpu->regmem[lower_mem_index] = ((data >> 0) & 0xFF);
        cpu->regmem[lower_mem_index+1] = ((data >> 8) & 0xFF);

        return;
    }

    cpu->regmem[lower_mem_index] = data;
}

#define get_data_from_register_by_enum(_cpu, _src_reg_enum) get_data_from_register(_cpu, register_access_by_enum(_src_reg_enum))
#define set_data_to_register_by_enum(_cpu, _dest_reg_enum, _data) set_data_to_register(_cpu, register_access_by_enum(_src_reg_enum, _data))


u32 calc_absolute_memory_address(CPU *cpu, Effective_Address_Expression *expr)
{ 
    u16 displacement = expr->displacement;
    u16 address = 0; // @Todo: segment stuff

    // @Cleanup
    switch (expr->base) {
        case Effective_Address_direct:
            // In this case the address will be equal to the displacement
            break;
        case Effective_Address_bx_si:
            address = get_data_from_register_by_enum(cpu, Register_bx); 
            address += get_data_from_register_by_enum(cpu, Register_si);

            break;
        case Effective_Address_bx_di:
            address = get_data_from_register_by_enum(cpu, Register_bx); 
            address += get_data_from_register_by_enum(cpu, Register_di); 

            break;
        case Effective_Address_bp_si:
            address = get_data_from_register_by_enum(cpu, Register_bp); 
            address += get_data_from_register_by_enum(cpu, Register_si); 

            break;
        case Effective_Address_bp_di:
            address = get_data_from_register_by_enum(cpu, Register_bp); 
            address += get_data_from_register_by_enum(cpu, Register_di); 

            break;
        case Effective_Address_si:
            address = get_data_from_register_by_enum(cpu, Register_si);

            break;
        case Effective_Address_di:
            address = get_data_from_register_by_enum(cpu, Register_di);

            break;
        case Effective_Address_bp:
            address = get_data_from_register_by_enum(cpu, Register_bp);

            break;
        case Effective_Address_bx:
            address = get_data_from_register_by_enum(cpu, Register_bx);

            break;
        default:
            assert(0);
    }

    return address + displacement;
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
    memory[address] = (u8)(data & 0x00FF);
    if (is_16bit) {
        memory[address+1] = (u8)((data >> 8) & 0x00FF);
    }
}

s32 get_data_from_operand(CPU *cpu, Instruction_Operand *op, u8 is_16bit)
{
    s32 src_data = 0;

    if (op->type == Operand_Register) {
        Register_Access *src_reg = register_access(op->reg, is_16bit);
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
    s32 current_data = get_data_from_operand(cpu, op, is_16bit);

    if (op->type == Operand_Register) {
        Register_Access *dest_reg = register_access(op->reg, is_16bit);
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

    printf(" %#02x -> %#02x |", current_data, data);
}

void execute_instruction(CPU *cpu)
{
    Instruction *i = &cpu->instruction;
    Instruction_Operand dest_op = i->operands[0];
    u8 is_16bit = !!(i->flags & Inst_Wide);

    u32 ip_before = cpu->ip; 
    u32 ip_after  = cpu->ip;

    printf(" ;");

    if (i->type == Instruction_Type_Move) {
        Instruction_Operand src_op = i->operands[1];
        u16 src_data = get_data_from_operand(cpu, &src_op, is_16bit);
        set_data_to_operand(cpu, &dest_op, is_16bit, src_data);
        
        ip_after += i->size; 
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
        ip_after += i->size; 
    }
    else if (i->type == Instruction_Type_Jump) {
        switch (i->opcode) {
            case Opcode_jnz: {
                if (cpu->flags & F_ZERO) {
                    ip_after += i->size;
                } else {
                    ip_after += i->operands[0].immediate;
                }
                break;
            }
            case Opcode_loop: {
                // loop instruction is always using the cx register value which will be decremented by one
                // every time when loop instruction are called
                Register_Access *reg_access = register_access_by_enum(Register_cx);
                s16 cx_data = get_data_from_register(cpu, reg_access);
                cx_data -= 1;
                set_data_to_register(cpu, reg_access, cx_data);
                
                printf("> loop[cx]: %d -> %d\n",  cx_data+1, cx_data);

                if (cx_data != 0) {
                    ip_after += i->operands[0].immediate;
                } else {
                    ip_after += i->size;
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
    cpu->ip = ip_after;

    printf(" | ip: %#02x -> %#02x\n", ip_before, cpu->ip);
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

    assert(fsize+1 <= MAX_MEMORY);

    // @Todo: Dynamic memory allocation  
    fread(&cpu->memory[0], fsize, 1, fp);
    fclose(fp);

    cpu->loaded_executable_size = fsize;
    cpu->ip = 0; // cpu->memory[0] memory index
}

void boot(CPU *cpu)
{
    cpu->memory = (u8*)malloc(MAX_MEMORY);
    
    ZERO_MEMORY(cpu->memory, MAX_MEMORY);
    ZERO_MEMORY(cpu->regmem, 64);

    // @Todo: Check the loaded executable memory address, but now we always put the executable to beginning of the memory
    cpu->ip = 0;
    
    // @Todo: Specify the max stack size
    cpu->ss = 65535; // The stack ends at this memory address
}

void run(CPU *cpu)
{
    char input[128] = {0};

    do {
        decode_next_instruction(cpu);

        // @Todo: The i8086/88 contains the debug flag so later we simulate this too
        // instead of this boolean
        if (cpu->debug_mode) {
            printf(">> Press enter to the next instruction\n");
__de:;
            fgets(input, sizeof(input), stdin);
            if (input[0] != '\n') {
                goto __de;
            }
        }

        if (cpu->decode_only) {
            print_instruction(cpu, 1);

            //We have to update this "manually", because here we only printing and not executing, so the ip won't update! 
            cpu->ip = cpu->decoder_cursor;
        }
        else {
            print_instruction(cpu, 0);
            execute_instruction(cpu);
        }

    // @Todo: Another option to check end of the executable?
    } while (cpu->ip != cpu->loaded_executable_size);
}
