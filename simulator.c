#include "simulator.h"
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

u16 get_memory_address(Effective_Address_Expression *expr, u8 is_16bit)
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

s32 get_data_from_operand(Context *ctx, Instruction_Operand *op, u8 is_16bit)
{
    s32 src_data = 0;

    if (op->type == Operand_Register) {
        Register_Info *reg = get_register_info(is_16bit, op->reg);
        src_data = get_data_from_register(reg);
    } 
    else if (op->type == Operand_Immediate) {
        if (ctx->instruction->flags & FLAG_IS_SIGNED) {
            src_data = op->immediate_s16;
        } else {
            src_data = op->immediate_u16;
        }
    } 
    else if (op->type == Operand_Memory) {
        u16 address = get_memory_address(&op->address, is_16bit);
        src_data = get_data_from_memory(ctx->memory, address, is_16bit);
    }

    return src_data;
}

void set_data_to_operand(Context *ctx, Instruction_Operand *op, u8 is_16bit, u16 data)
{
    s32 current_data = get_data_from_operand(ctx, op, is_16bit);

    if (op->type == Operand_Register) {
        Register_Info *reg = get_register_info(is_16bit, op->reg);
        set_data_to_register(reg, data);
    }
    else if (op->type == Operand_Memory) {
        u16 address = get_memory_address(&op->address, is_16bit);
        set_data_to_memory(ctx->memory, address, is_16bit, data);
    }

    printf(" %#02x -> %#02x |", current_data, data);
}

void execute_instruction(Context *ctx)
{
    Instruction *i = ctx->instruction;
    Instruction_Operand dest_op = i->operands[0];
    u8 is_16bit = i->flags & FLAG_IS_16BIT;

    u32 ip_data_before = ctx->ip_data; 
    u32 ip_data_after  = ctx->ip_data;

    //printf(" ;");

    if (i->opcode == Opcode_mov) {
        Instruction_Operand src_op = i->operands[1];
        u16 src_data = get_data_from_operand(ctx, &src_op, is_16bit);
        set_data_to_operand(ctx, &dest_op, is_16bit, src_data);
        
        ip_data_after += i->size; 
    }
    else if (i->opcode == Opcode_add || i->opcode == Opcode_sub || i->opcode == Opcode_cmp) {
        // @Cleanup: We can make an arithmetic flag for the instruction then we can use that to get to this branch
        Instruction_Operand src_op = i->operands[1];
        u16 src_data = get_data_from_operand(ctx, &src_op, is_16bit);
        s32 dest_data = get_data_from_operand(ctx, &dest_op, is_16bit);

        switch (i->opcode) {
            case Opcode_add: 
                dest_data += src_data;
                set_data_to_operand(ctx, &dest_op, is_16bit, dest_data);
                break;
            case Opcode_sub:
                dest_data -= src_data;
                set_data_to_operand(ctx, &dest_op, is_16bit, dest_data);
                break;
            case Opcode_cmp:
                dest_data -= src_data;
                break;
            default:
                assert(0);
        }

        u16 new_flags = ctx->flags;
        new_flags &= (~(F_SIGNED|F_ZERO)); // clear flags

        if (dest_data == 0) {
            new_flags |= F_ZERO;
        } 
        else if (dest_data < 0) {
            // @Todo: check the highest bit
            new_flags |= F_SIGNED;
        }

        printf(" flags: [");
        print_flags(ctx->flags);
        printf("] -> [");
        print_flags(new_flags);
        printf("]");

        ctx->flags = new_flags;
        ip_data_after += i->size; 
    }
    else if (i->opcode == Opcode_jnz) {
        // @Cleanup: We can make an jump flag for the instruction then we can use that to get to this branch
        if (ctx->flags & F_ZERO) {
            ip_data_after += i->size;
        } else {
            ip_data_after += i->operands[0].immediate_s16;
        }
    }

    // This instruction pointer data will provide us the next instruction location from the ctx->instructions array which indexed
    // based on the instruction byte index at loaded binary file.
    ctx->ip_data = ip_data_after;

    printf(" | ip: %#02x -> %#02x\n", ip_data_before, ctx->ip_data);
}

/*
void load_file(Context *ctx, char *filename)
{

}
*/

void run(Context *ctx)
{
    do {
        Instruction *instruction = ctx->instructions[ctx->ip_data];
        ctx->instruction = instruction;

        // @Todo: At this point we can't use the print out function here without calling the execute_instruction function
        // which will set the ip_data to pointing to the next instruction of the ctx->instructions set. Right now, the only 
        // place where we can call this is in the decode() function as the binary decoded.
        print_instruction(instruction, 0);
        execute_instruction(ctx);

    } while (ctx->instructions[ctx->ip_data]);
}

