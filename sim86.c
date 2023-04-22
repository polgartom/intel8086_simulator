#include "sim86.h"
#include "decoder.h"

////////////////////////////////////////

Memory regmem; // Remove!!

s32 get_data_from_register(Register_Info *reg)
{
    u16 lower_mem_index = reg->mem_offset;
    if (reg->mem_size == 2) {
        s16 val_lo = regmem.data[lower_mem_index] << 0;
        s16 val_hi = regmem.data[lower_mem_index+1] << 8;

        return (val_hi | val_lo);
    }
 
    return regmem.data[lower_mem_index];
}


void set_data_to_register(Register_Info *dest, u16 data) 
{
    u16 lower_mem_index = dest->mem_offset;

    if (dest->mem_size == 2) {
        regmem.data[lower_mem_index] = ((data >> 0) & 0xFF);
        regmem.data[lower_mem_index+1] = ((data >> 8) & 0xFF);

        return;
    }

    regmem.data[lower_mem_index] = data;
}

u16 get_memory_address(Effective_Address_Expression *expr, u8 is_16bit)
{ 
    Register_Info *reg = NULL;
    u16 displacement = expr->displacement;
    u16 address = 0; // @Todo: segment stuff

    switch (expr->base) {
        case Effective_Address_direct:
            break;
        case Effective_Address_bx:
            reg = get_register(is_16bit, Register_bx);
            displacement = get_data_from_register(reg);
            break;
        default:
            printf("[%s]: This address calculation haven't handled yet!", __FUNCTION__);
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
        Register_Info *reg = get_register(is_16bit, op->reg);
        src_data = get_data_from_register(reg);
    } 
    else if (op->type == Operand_Immediate) {
        src_data = is_16bit ? op->immediate_s16 : op->immediate_u16;
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
        Register_Info *reg = get_register(is_16bit, op->reg);
        set_data_to_register(reg, data);

        printf(" %s: ", get_register_name(reg->reg));
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

    printf(" ;");

    if (i->opcode == Opcode_mov) {
        Instruction_Operand src_op = i->operands[1];
        u16 src_data = get_data_from_operand(ctx, &src_op, is_16bit);
        set_data_to_operand(ctx, &dest_op, is_16bit, src_data);
        
        ip_data_after += i->size; 
    }
    else if (i->opcode == Opcode_add || i->opcode == Opcode_sub || i->opcode == Opcode_cmp) {
        // @Cleanup: We can make an arithmetic flag to the instruction then we can use that to get to this branch
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
        if (ctx->flags & F_ZERO) {
            ip_data_after += i->size;
        } else {
            ip_data_after += i->operands[0].immediate_s16;
        }
    }

    ctx->ip_data = ip_data_after;

    printf(" | ip: %#02x (%d) -> %#02x (%d)\n", ip_data_before, ip_data_before, ctx->ip_data, ctx->ip_data);
}

void run(Context *ctx)
{
    do {
        Instruction *instruction = ctx->instructions[ctx->ip_data];
        ctx->instruction = instruction;

        print_instruction(instruction, 0);
        execute_instruction(ctx);

    } while (ctx->instructions[ctx->ip_data]);
}

int main(int argc, char **argv)
{
    if (argc < 2 || STR_LEN(argv[1]) == 0) {
        fprintf(stderr, "No binary file specified!\n");
    }
    printf("\nbinary: %s\n\n", argv[1]);

    Context ctx = {};
    ctx.loaded_binary_size = load_binary_file_to_memory(&ctx, argv[1]);

    regmem.size = 20;
    ZERO_MEMORY(regmem.data, regmem.size);

    decode(&ctx);
    run(&ctx);

    return 0;
}
