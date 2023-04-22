#include "sim86.h"

#define ASMD_CURR_BYTE(_d) _d->loaded_binary[_d->byte_offset]
u8 ASMD_NEXT_BYTE(Context *_d) { return _d->loaded_binary[++_d->byte_offset]; }
#define ASMD_NEXT_BYTE_WITHOUT_STEP(_d) _d->loaded_binary[_d->byte_offset+1]
#define ASMD_CURR_BYTE_INDEX(_d) _d->byte_offset

#define ARITHMETIC_OPCODE_LOOKUP(__byte, __opcode) \
     switch ((__byte >> 3) & 7) { \
        case 0: { __opcode = Opcode_add; break; } \
        case 5: { __opcode = Opcode_sub; break; } \
        case 7: { __opcode = Opcode_cmp; break; } \
        default: { \
            printf("[WARNING/%s:%d]: This arithmetic instruction is not handled yet!\n", __FUNCTION__, __LINE__); \
            goto _debug_parse_end; \
        } \
    } \

////////////////////////////////////////

Memory regmem; // Remove!!

#define REG_ACCUMULATOR 0
static Register_Info *get_register(u8 is_16bit, u8 reg)
{
    // [byte/word][register_index_by_table][meta_data]
    static const u32 registers[2][8][3] = {
        // BYTE (8bit)
        { 
        //   Register_index, mem_offset (byte), mem_size (byte)
            {Register_al, 0, 1}, {Register_cl, 2, 1}, {Register_dl, 4, 1}, {Register_bl, 6, 1},
            {Register_ah, 1, 2}, {Register_ch, 3, 1}, {Register_dh, 5, 1}, {Register_bh, 7, 1}
        },
        // WORD (16bit)
        {
            {Register_ax, 0, 2}, {Register_cx, 2,  2}, {Register_dx, 4,  2}, {Register_bx, 6,  2},
            {Register_sp, 8, 2}, {Register_bp, 10, 2}, {Register_si, 12, 2}, {Register_di, 14, 2}
        }
    };

    return (Register_Info *)registers[is_16bit][reg];
}

static const char *get_opcode_name(Opcode opcode)
{
    assert(opcode < Opcode_count);

    static const char* opcode_names[] = {
        "none",

        "mov",
        "add",
        "sub",
        "cmp",

        "jo",
        "js",
        "jb",
        "je",
        "jbe",
        "jp",
        "jnz",
        "jl",
        "jle",
        "jnl",
        "jg",
        "jae",
        "ja",
        "jnp",
        "jno",
        "jns",

        "loop",
        "loopz",
        "loopnz",
        "jcxz"
    };

    return opcode_names[opcode];
}

static const char *get_register_name(Register reg)
{
    assert(reg < Register_count);

    static const char *register_names[] = {
        "al", "cl", "dl", "bl",
        "ah", "ch", "dh", "bh",
        "ax", "cx", "dx", "bx",
        "sp", "bp", "si", "di"
    };

    return register_names[reg];
}

size_t load_binary_file_to_memory(Context *ctx, char *filename)
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

    ZERO_MEMORY(ctx->loaded_binary, fsize+1);

    fread(ctx->loaded_binary, fsize, 1, fp);
    fclose(fp);

    return fsize;
}

Effective_Address_Base get_address_base(u8 r_m, u8 mod) 
{
    switch (r_m) {
        case 0x00: {return Effective_Address_bx_si;}
        case 0x01: {return Effective_Address_bx_di;}
        case 0x02: {return Effective_Address_bp_si;}
        case 0x03: {return Effective_Address_bp_di;}
        case 0x04: {return Effective_Address_si;   }
        case 0x05: {return Effective_Address_di;   }
        case 0x06: {
            if (mod == 0x00) {
                return Effective_Address_direct; 
            }
            return Effective_Address_bp;
        }
        case 0x07: {return Effective_Address_bx;}
        default: {
            fprintf(stderr, "[ERROR]: Invalid effective address expression!\n");
            assert(0);
        } 
    }
}

void immediate_to_operand(Context *ctx, Instruction_Operand *operand, u8 is_signed, u8 is_16bit, u8 immediate_depends_from_signed)
{
    operand->type = Operand_Immediate;
    s8 immediate8 = ASMD_NEXT_BYTE(ctx);

    // If this an arithmetic operation then only can be an u16 or a s8 data type based on
    // that if it is wide and not signed immediate
    if (immediate_depends_from_signed) {
        if (is_16bit && !is_signed) {
            operand->immediate_u16 = BYTE_LOHI_TO_HILO(immediate8, ASMD_NEXT_BYTE(ctx));
        } else {
            operand->immediate_s16 = immediate8;
        }
        return;
    }

    if (is_16bit) {
        if (!is_signed) {
            operand->immediate_u16 = BYTE_LOHI_TO_HILO(immediate8, ASMD_NEXT_BYTE(ctx));
        } else {
            operand->immediate_s16 = BYTE_LOHI_TO_HILO(immediate8, ASMD_NEXT_BYTE(ctx));
        }
    } else {
        operand->immediate_s16 = immediate8;
    }
}

void displacement_to_operand(Context *d, Instruction_Operand *operand, u8 mod, u8 r_m)
{
    operand->type = Operand_Memory;
    operand->address.base = get_address_base(r_m, mod);
    operand->address.displacement = 0;

    if ((mod == 0x00 && r_m == 0x06) || mod == 0x02) { 
        operand->address.displacement = (s16)(BYTE_LOHI_TO_HILO(ASMD_NEXT_BYTE(d), ASMD_NEXT_BYTE(d)));
    } 
    else if (mod == 0x01) { 
        operand->address.displacement = (s8)ASMD_NEXT_BYTE(d);
    }
}

void register_memory_to_from_decode(Context *d, u8 reg_dir)
{
    u8 byte = ASMD_NEXT_BYTE(d);
    Instruction *instruction = d->instruction;

    u8 mod = (byte >> 6) & 0x03;
    u8 reg = (byte >> 3) & 0x07;
    u8 r_m = byte & 0x07;
    
    Register src;
    Register dest;

    if (mod == 0x03) { 
        dest = r_m;
        src = reg;
        if (reg_dir == REG_IS_DEST) {
            dest = reg;
            src = r_m;
        }

        instruction->operands[0].type = Operand_Register;
        instruction->operands[1].type = Operand_Register;
        instruction->operands[0].reg = dest;
        instruction->operands[1].reg = src;

        return;
    }

    Instruction_Operand a_operand;
    displacement_to_operand(d, &a_operand, mod, r_m);

    Instruction_Operand b_operand;
    b_operand.type = Operand_Register;
    b_operand.reg = reg;

    if (reg_dir == REG_IS_DEST) {
        instruction->operands[0] = b_operand;
        instruction->operands[1] = a_operand;
    } else {
        instruction->operands[0] = a_operand;
        instruction->operands[1] = b_operand;
    }
}

void immediate_to_register_memory_decode(Context *d, s8 is_signed, u8 is_16bit, u8 immediate_depends_from_signed)
{
    char byte = ASMD_NEXT_BYTE(d);
    Instruction *instruction = d->instruction;

    u8 mod = (byte >> 6) & 0x03;
    u8 r_m = byte & 0x07;

    if (mod == 0x03) {
        instruction->operands[0].type = Operand_Register;
        instruction->operands[0].reg  = r_m;
    } 
    else {
        displacement_to_operand(d, &instruction->operands[0], mod, r_m);
    }

    immediate_to_operand(d, &instruction->operands[1], is_signed, is_16bit, immediate_depends_from_signed);
}

void print_flags(u16 flags)
{
    if (flags & F_SIGNED) {
        printf("S");
    }

    if (flags & F_ZERO) {
        printf("Z");
    }
}

void print_instruction(Instruction *instruction, u8 with_end_line)
{
    FILE *dest = stdout;

    fprintf(dest, "[0x%02x]\t%s", instruction->mem_offset, get_opcode_name(instruction->opcode));
    
    const char *separator = " ";
    for (u8 j = 0; j < 2; j++) {
        Instruction_Operand *op = &instruction->operands[j];
        if (op->type == Operand_None) {
            continue;
        }
        
        fprintf(dest, "%s", separator);
        separator = ", ";

        switch (op->type) {
            case Operand_None: {
                break;
            }
            case Operand_Register: {
                Register_Info *reg = get_register(instruction->flags & FLAG_IS_16BIT, op->reg);
                fprintf(dest, "%s", get_register_name(reg->reg));

                break;
            }
            case Operand_Memory: {
                if (instruction->operands[0].type != Operand_Register) {
                    fprintf(dest, "%s ", (instruction->flags & FLAG_IS_16BIT) ? "word" : "byte");
                }

                char const *r_m_base[] = {"","bx+si","bx+di","bp+si","bp+di","si","di","bp","bx"};

                fprintf(dest, "[%s", r_m_base[op->address.base]);
                if (op->address.displacement) {
                    fprintf(dest, "%+d", op->address.displacement);
                }
                fprintf(dest, "]");

                break;
            }
            case Operand_Immediate: {
                if ((instruction->flags & FLAG_IS_16BIT) && !(instruction->flags & FLAG_IS_SIGNED)) {
                    fprintf(dest, "%d", op->immediate_u16);
                } else {
                    fprintf(dest, "%d", op->immediate_s16);
                }

                break;
            }
            case Operand_Relative_Immediate: {
                fprintf(dest, "$%+d", op->immediate_s16);

                break;
            }
            default: {
                fprintf(stderr, "[WARNING]: I found a not operand at print out!\n");
            }
        }

    }

    if (with_end_line) {
        fprintf(dest, "\n");
    }

}

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

void decode(Context *ctx)
{
    do {
        u8 byte = ASMD_CURR_BYTE(ctx);
        u8 reg_dir = 0;
        u8 is_16bit = 0;

        u32 instruction_byte_start_offset = ctx->byte_offset;

        ctx->instruction = ALLOC_MEMORY(Instruction);
        ctx->instruction->mem_offset = ctx->byte_offset;

        ctx->instructions[ctx->byte_offset] = ctx->instruction;
        
        // ARITHMETIC
        if (((byte >> 6) & 7) == 0) {
            ARITHMETIC_OPCODE_LOOKUP(byte, ctx->instruction->opcode);

            if (((byte >> 1) & 3) == 2) {
                // Immediate to accumulator                
                is_16bit = byte & 1;
                if (is_16bit) {
                    ctx->instruction->flags |= FLAG_IS_16BIT;
                }

                ctx->instruction->operands[0].type = Operand_Register;
                ctx->instruction->operands[0].reg = REG_ACCUMULATOR;

                immediate_to_operand(ctx, &ctx->instruction->operands[1], 0, is_16bit, 1);
            }
            else if (((byte >> 2) & 1) == 0) {
                reg_dir = (byte >> 1) & 1;

                is_16bit = byte & 1;
                if (is_16bit) {
                    ctx->instruction->flags |= FLAG_IS_16BIT;
                }

                register_memory_to_from_decode(ctx, reg_dir);
            } else {
                fprintf(stderr, "[ERROR]: Invalid opcode!\n");
                assert(0);
            }
        }
        else if (((byte >> 2) & 63) == 32) {
            ARITHMETIC_OPCODE_LOOKUP(ASMD_NEXT_BYTE_WITHOUT_STEP(ctx), ctx->instruction->opcode);

            is_16bit = byte & 1;
            if (is_16bit) {
                ctx->instruction->flags |= FLAG_IS_16BIT;
            }

            u8 is_signed = (byte >> 1) & 1;
            if (is_signed) {
                ctx->instruction->flags |= FLAG_IS_SIGNED;
            }

            immediate_to_register_memory_decode(ctx, is_signed, is_16bit, 1);
        }
        // MOV
        else if (((byte >> 2) & 63) == 34) {
            ctx->instruction->opcode = Opcode_mov;
            
            reg_dir = (byte >> 1) & 1;
            is_16bit = byte & 1;
            if (is_16bit) {
                ctx->instruction->flags |= FLAG_IS_16BIT;
            }

            register_memory_to_from_decode(ctx, reg_dir);
        }
        else if (((byte >> 1) & 127) == 99) {
            ctx->instruction->opcode = Opcode_mov;

            char second_byte = ASMD_NEXT_BYTE_WITHOUT_STEP(ctx);
            assert(((second_byte >> 3) & 7) == 0);

            is_16bit = byte & 1;
            if (is_16bit) {
                ctx->instruction->flags |= FLAG_IS_16BIT;
            }
            ctx->instruction->flags |= FLAG_IS_SIGNED;

            immediate_to_register_memory_decode(ctx, 1, is_16bit, 0);
        }
        else if (((byte >> 4) & 0x0F) == 0x0B) {
            ctx->instruction->opcode = Opcode_mov;

            u8 reg = byte & 0x07;

            is_16bit = (byte >> 3) & 1;
            if (is_16bit) {
                ctx->instruction->flags |= FLAG_IS_16BIT;
            }
            ctx->instruction->flags |= FLAG_IS_SIGNED;

            ctx->instruction->operands[0].type = Operand_Register;
            ctx->instruction->operands[0].reg = reg;

            immediate_to_operand(ctx, &ctx->instruction->operands[1], 1, is_16bit, 0);
        }
        else if (((byte >> 2) & 0x3F) == 0x28) {
            ctx->instruction->opcode = Opcode_mov;

            // Memory to/from accumulator
            is_16bit = byte & 1;
            if (is_16bit) {
                ctx->instruction->flags |= FLAG_IS_16BIT;
            }

            ctx->instruction->opcode = Opcode_mov;

            Instruction_Operand a_operand;
            a_operand.type = Operand_Memory;
            a_operand.address.base = Effective_Address_direct;

            a_operand.address.displacement = (s8)ASMD_NEXT_BYTE(ctx);
            if (is_16bit) {
                a_operand.address.displacement = (s16)BYTE_LOHI_TO_HILO(a_operand.address.displacement, ASMD_NEXT_BYTE(ctx));
            }

            Instruction_Operand b_operand;
            b_operand.type = Operand_Register;
            b_operand.reg = REG_ACCUMULATOR;

            reg_dir = (byte >> 1) & 1;
            if (reg_dir == 0) {
                ctx->instruction->operands[0] = b_operand;
                ctx->instruction->operands[1] = a_operand;
            } else {
                ctx->instruction->operands[0] = a_operand;
                ctx->instruction->operands[1] = b_operand;
            }
            
        }
        // Return from CALL (jumps)
        else if (((byte >> 4) & 15) == 0x07) {
            s8 ip_inc8 = ASMD_NEXT_BYTE(ctx); // 8 bit signed increment to instruction pointer
            Opcode opcode = Opcode_none;
            switch (byte & 0x0F) {
                case 0b0000: { opcode = Opcode_jo;  break;               }
                case 0b1000: { opcode = Opcode_js;  break;               }
                case 0b0010: { opcode = Opcode_jb;  break; /* or jnae */ }
                case 0b0100: { opcode = Opcode_je;  break; /* or jz */   }
                case 0b0110: { opcode = Opcode_jbe; break; /* or jna */  }
                case 0b1010: { opcode = Opcode_jp;  break; /* or jpe */  }
                case 0b0101: { opcode = Opcode_jnz; break; /* or jne */  }
                case 0b1100: { opcode = Opcode_jl;  break; /* or jnge */ }
                case 0b1110: { opcode = Opcode_jle; break; /* or jng */  }
                case 0b1101: { opcode = Opcode_jnl; break; /* or jge */  }
                case 0b1111: { opcode = Opcode_jg;  break; /* or jnle */ }
                case 0b0011: { opcode = Opcode_jae; break; /* or jnle */ }
                case 0b0111: { opcode = Opcode_ja;  break; /* or jnbe */ }
                case 0b1011: { opcode = Opcode_jnp; break; /* or jpo */  }
                case 0b0001: { opcode = Opcode_jno; break;               }
                case 0b1001: { opcode = Opcode_jns; break;               }
                default: {
                    fprintf(stderr, "[ERROR]: This invalid opcode\n");
                    assert(0);
                }
            }

            ctx->instruction->opcode = opcode;

            // We have to add +2, because the relative displacement (offset) start from the second byte (ip_inc8)
            //  of instruction. 
            ctx->instruction->operands[0].type = Operand_Relative_Immediate;
            ctx->instruction->operands[0].immediate_u16 = ip_inc8+2;
        }
        else if (((byte >> 4) & 0x0F) == 0b1110) {
            s8 ip_inc8 = ASMD_NEXT_BYTE(ctx);
            Opcode opcode = Opcode_none;
            switch (byte & 0x0F) {
                case 0b0010: { opcode = Opcode_loop; break;                    }
                case 0b0001: { opcode = Opcode_loopz; break;  /* or loope */   }
                case 0b0000: { opcode = Opcode_loopnz; break; /* or loopne */  }
                case 0b0011: { opcode = Opcode_jcxz; break;                    }
                default: {
                    fprintf(stderr, "[ERROR]: This is invalid opcode\n");
                    assert(0);
                }
            }

            ctx->instruction->opcode = opcode;

            // We have to add +2, because the relative displacement (offset) start from the second byte (ip_inc8)
            //  of instruction. 
            ctx->instruction->operands[0].type = Operand_Relative_Immediate;
            ctx->instruction->operands[0].immediate_u16 = ip_inc8+2;
        }
        else {
            fprintf(stderr, "[WARNING]: Instruction is not handled!\n");
            break;
        }

        ctx->byte_offset++;

        ctx->instruction->size = ctx->byte_offset - instruction_byte_start_offset; 

    } while(ctx->loaded_binary_size != ctx->byte_offset);

_debug_parse_end:;

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
