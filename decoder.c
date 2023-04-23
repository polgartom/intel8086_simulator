#include "decoder.h"
#include "printer.h"

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
    s16 immediate = ASMD_NEXT_BYTE(ctx);

    // If this an arithmetic operation then only can be an u16 or a s8 data type based on
    // that if it is wide and not signed immediate
    if (immediate_depends_from_signed) {
        if (is_16bit && !is_signed) {
            operand->immediate_u16 = BYTE_LOHI_TO_HILO(immediate, ASMD_NEXT_BYTE(ctx));
        } else {
            operand->immediate_s16 = immediate;
        }
        return;
    }

    if (is_16bit) {
        if (!is_signed) {
            operand->immediate_u16 = BYTE_LOHI_TO_HILO(immediate, ASMD_NEXT_BYTE(ctx));
        } else {
            operand->immediate_s16 = BYTE_LOHI_TO_HILO(immediate, ASMD_NEXT_BYTE(ctx));
        }
    } else {
        operand->immediate_s16 = immediate;
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
            ctx->instruction->type = Instruction_Type_Arithmetic;

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
            ctx->instruction->type = Instruction_Type_Arithmetic;

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
            ctx->instruction->type = Instruction_Type_Move;

            ctx->instruction->opcode = Opcode_mov;
            
            reg_dir = (byte >> 1) & 1;
            is_16bit = byte & 1;
            if (is_16bit) {
                ctx->instruction->flags |= FLAG_IS_16BIT;
            }

            register_memory_to_from_decode(ctx, reg_dir);
        }
        else if (((byte >> 1) & 127) == 99) {
            ctx->instruction->type = Instruction_Type_Move;

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
            ctx->instruction->type = Instruction_Type_Move;

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
            ctx->instruction->type = Instruction_Type_Move;

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
            ctx->instruction->type = Instruction_Type_Jump;

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
            ctx->instruction->type = Instruction_Type_Jump;

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

