#include "decoder.h"
#include "printer.h"

#define ASMD_CURR_BYTE(_d) _d->memory[_d->decoder_cursor]
u8 ASMD_NEXT_BYTE(CPU *_d) { return _d->memory[++_d->decoder_cursor]; }
#define ASMD_NEXT_BYTE_WITHOUT_STEP(_d) _d->memory[_d->decoder_cursor+1]
#define ASMD_CURR_BYTE_INDEX(_d) _d->decoder_cursor

#define ARITHMETIC_OPCODE_LOOKUP(__byte, __opcode) \
     switch ((__byte >> 3) & 7) { \
        case 0: { __opcode = Opcode_add; break; } \
        case 5: { __opcode = Opcode_sub; break; } \
        case 7: { __opcode = Opcode_cmp; break; } \
        default: { \
            printf("[WARNING/%s:%d]: This arithmetic instruction is not handled yet!\n", __FUNCTION__, __LINE__); \
            assert(0); \
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

// @Todo: Rename to *_memory_operand
void immediate_to_operand(CPU *cpu, Instruction_Operand *operand, u8 is_signed, u8 is_16bit, u8 immediate_depends_from_signed)
{
    operand->type = Operand_Immediate;
    s16 immediate = ASMD_NEXT_BYTE(cpu);

    // If this an arithmetic operation then only can be an u16 or a s8 data type based on
    // that if it is wide and not signed immediate
    if (immediate_depends_from_signed) {
        if (is_16bit && !is_signed) {
            operand->immediate = BYTE_LOHI_TO_HILO(immediate, ASMD_NEXT_BYTE(cpu));
        } else {
            operand->immediate = immediate;
        }
        return;
    }

    if (is_16bit) {
        if (!is_signed) {
            operand->immediate = BYTE_LOHI_TO_HILO(immediate, ASMD_NEXT_BYTE(cpu));
        } else {
            operand->immediate = BYTE_LOHI_TO_HILO(immediate, ASMD_NEXT_BYTE(cpu));
        }
    } else {
        operand->immediate = immediate;
    }
}

// @Todo: Rename to *_memory_operand
void displacement_to_operand(CPU *cpu, Instruction_Operand *operand, u8 mod, u8 r_m)
{
    operand->type = Operand_Memory;
    operand->address.base = get_address_base(r_m, mod);
    operand->address.displacement = 0;

    if ((mod == 0x00 && r_m == 0x06) || mod == 0x02) { 
        cpu->instruction.flags |= Inst_Wide;
        operand->address.displacement = (s16)(BYTE_LOHI_TO_HILO(ASMD_NEXT_BYTE(cpu), ASMD_NEXT_BYTE(cpu)));
    } 
    else if (mod == 0x01) { 
        operand->address.displacement = (s8)ASMD_NEXT_BYTE(cpu);
    }
}

void register_memory_to_from_decode(CPU *d, u8 reg_dir)
{
    u8 byte = ASMD_NEXT_BYTE(d);
    Instruction *instruction = &d->instruction;

    u8 mod = (byte >> 6) & 0x03;
    u8 reg = (byte >> 3) & 0x07;
    u8 r_m = byte & 0x07;
    
    Register src;
    Register dest;

    if (mod == MOD_REGISTER) { 
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

void immediate_to_register_memory_decode(CPU *d, s8 is_signed, u8 is_16bit, u8 immediate_depends_from_signed)
{
    char byte = ASMD_NEXT_BYTE(d);
    Instruction *instruction = &d->instruction;

    u8 mod = (byte >> 6) & 0x03;
    u8 r_m = byte & 0x07;

    if (mod == MOD_REGISTER) {
        instruction->operands[0].type = Operand_Register;
        instruction->operands[0].reg  = r_m;
    } 
    else {
        displacement_to_operand(d, &instruction->operands[0], mod, r_m);
    }

    immediate_to_operand(d, &instruction->operands[1], is_signed, is_16bit, immediate_depends_from_signed);
}



void decode_next_instruction(CPU *cpu)
{
    cpu->decoder_cursor = cpu->ip;
    ZERO_MEMORY(&cpu->instruction, sizeof(Instruction)); 
    cpu->instruction.mem_address = cpu->decoder_cursor;

    u32 instruction_byte_start_offset = cpu->decoder_cursor;

    u8 byte = ASMD_CURR_BYTE(cpu);
    u8 reg_dir = 0;
    u8 is_16bit = 0;

    // DATA TRANSFER
    if (((byte >> 5) & 0b111) == 0) {
        // Segment
        if ((byte & 0b111) == 0b110) {
            cpu->instruction.opcode = Opcode_push;
        } else if ((byte & 0b111) == 0b110) {
            cpu->instruction.opcode = Opcode_pop;
        } else {
            printf("[ERROR]: Unknown opcode!\n");
            assert(0);
        }

        cpu->instruction.operands[0].type = Operand_Register;
        cpu->instruction.operands[0].reg  = (byte >> 3) & 0b11;
        cpu->instruction.flags |= Inst_Wide;
        cpu->instruction.flags |= Inst_Segment;
    }
    else if (((byte >> 4) & 0b11111) == 0b0101) {
        // Register
        if (!(byte & 0b1000)) {
            cpu->instruction.opcode = Opcode_push;
        } else if (byte & 0b1000) {
            cpu->instruction.opcode = Opcode_pop;
        } else {
            printf("[ERROR]: Unknown opcode!\n");
            assert(0);
        }
        
        cpu->instruction.operands[0].type = Operand_Register;
        cpu->instruction.operands[0].reg  = byte & 0b111;
        cpu->instruction.flags |= Inst_Wide;
    }
    else if ((byte >> 7) && (byte & 0b1111) == 0b1111) {
        // Register/Memory
        u8 opcode = (byte >> 4) & 0b111;

        u8 second_byte = ASMD_NEXT_BYTE(cpu);
        u8 opcode_signature = ((second_byte >> 3) & 0b111);

        if (opcode == 0b111 && opcode_signature == 0b110) {
            cpu->instruction.opcode = Opcode_push;
        } else if (opcode == 0b000 && opcode_signature == 0b000) {
            cpu->instruction.opcode = Opcode_pop;
        } else {
            printf("[ERROR]: Unknown opcode (%d)!\n", opcode);
            assert(0);
        }

        u8 mod = (second_byte >> 6) & 0b11;
        u8 r_m = (second_byte & 0b111);

        if (mod == MOD_REGISTER) {
            cpu->instruction.operands[0].type = Operand_Register;
            cpu->instruction.operands[0].reg  = r_m;
        } else {
            cpu->instruction.operands[0].type = Operand_Memory;
            
            displacement_to_operand(cpu, &cpu->instruction.operands[0], mod, r_m);
        }

        cpu->instruction.flags |= Inst_Wide;
    }
    // ARITHMETIC
    else if (((byte >> 6) & 7) == 0) {
        cpu->instruction.type = Instruction_Type_Arithmetic;

        ARITHMETIC_OPCODE_LOOKUP(byte, cpu->instruction.opcode);

        if (((byte >> 1) & 3) == 2) {
            // Immediate to accumulator                
            is_16bit = byte & 1;
            if (is_16bit) {
                cpu->instruction.flags |= Inst_Wide;
            }

            cpu->instruction.operands[0].type = Operand_Register;
            cpu->instruction.operands[0].reg = REG_ACCUMULATOR;

            immediate_to_operand(cpu, &cpu->instruction.operands[1], 0, is_16bit, 1);
        }
        else if (((byte >> 2) & 1) == 0) {
            reg_dir = (byte >> 1) & 1;

            is_16bit = byte & 1;
            if (is_16bit) {
                cpu->instruction.flags |= Inst_Wide;
            }

            register_memory_to_from_decode(cpu, reg_dir);
        } else {
            fprintf(stderr, "[ERROR]: Invalid opcode!\n");
            assert(0);
        }
    }
    else if (((byte >> 2) & 63) == 32) {
        cpu->instruction.type = Instruction_Type_Arithmetic;

        ARITHMETIC_OPCODE_LOOKUP(ASMD_NEXT_BYTE_WITHOUT_STEP(cpu), cpu->instruction.opcode);

        is_16bit = byte & 1;
        if (is_16bit) {
            cpu->instruction.flags |= Inst_Wide;
        }

        u8 is_signed = (byte >> 1) & 1;
        if (is_signed) {
            cpu->instruction.flags |= Inst_Sign;
        }

        immediate_to_register_memory_decode(cpu, is_signed, is_16bit, 1);
    }
    // MOV
    else if (((byte >> 2) & 63) == 34) {
        cpu->instruction.type = Instruction_Type_Move;

        cpu->instruction.opcode = Opcode_mov;
        
        reg_dir = (byte >> 1) & 1;
        is_16bit = byte & 1;
        if (is_16bit) {
            cpu->instruction.flags |= Inst_Wide;
        }

        register_memory_to_from_decode(cpu, reg_dir);
    }
    else if (((byte >> 1) & 127) == 99) {
        cpu->instruction.type = Instruction_Type_Move;

        cpu->instruction.opcode = Opcode_mov;

        char second_byte = ASMD_NEXT_BYTE_WITHOUT_STEP(cpu);
        assert(((second_byte >> 3) & 7) == 0);

        is_16bit = byte & 1;
        if (is_16bit) {
            cpu->instruction.flags |= Inst_Wide;
        }
        cpu->instruction.flags |= Inst_Sign;

        immediate_to_register_memory_decode(cpu, 1, is_16bit, 0);
    }
    else if (((byte >> 4) & 0x0F) == 0x0B) {
        cpu->instruction.type = Instruction_Type_Move;

        cpu->instruction.opcode = Opcode_mov;

        u8 reg = byte & 0x07;

        is_16bit = (byte >> 3) & 1;
        if (is_16bit) {
            cpu->instruction.flags |= Inst_Wide;
        }
        cpu->instruction.flags |= Inst_Sign;

        cpu->instruction.operands[0].type = Operand_Register;
        cpu->instruction.operands[0].reg = reg;

        immediate_to_operand(cpu, &cpu->instruction.operands[1], 1, is_16bit, 0);
    }
    else if (((byte >> 2) & 0x3F) == 0x28) {
        cpu->instruction.type = Instruction_Type_Move;

        cpu->instruction.opcode = Opcode_mov;

        // Memory to/from accumulator
        is_16bit = byte & 1;
        if (is_16bit) {
            cpu->instruction.flags |= Inst_Wide;
        }

        cpu->instruction.opcode = Opcode_mov;

        Instruction_Operand a_operand;
        a_operand.type = Operand_Memory;
        a_operand.address.base = Effective_Address_direct;

        a_operand.address.displacement = (s8)ASMD_NEXT_BYTE(cpu);
        if (is_16bit) {
            a_operand.address.displacement = (s16)BYTE_LOHI_TO_HILO(a_operand.address.displacement, ASMD_NEXT_BYTE(cpu));
        }

        Instruction_Operand b_operand;
        b_operand.type = Operand_Register;
        b_operand.reg = REG_ACCUMULATOR;

        reg_dir = (byte >> 1) & 1;
        if (reg_dir == 0) {
            cpu->instruction.operands[0] = b_operand;
            cpu->instruction.operands[1] = a_operand;
        } else {
            cpu->instruction.operands[0] = a_operand;
            cpu->instruction.operands[1] = b_operand;
        }
        
    }
    // Return from CALL (jumps)
    else if (((byte >> 4) & 15) == 0x07) {
        cpu->instruction.type = Instruction_Type_Jump;

        s8 ip_inc8 = ASMD_NEXT_BYTE(cpu); // 8 bit signed increment to instruction pointer
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

        cpu->instruction.opcode = opcode;

        // We have to add +2, because the relative displacement (offset) start from the second byte (ip_inc8)
        //  of instruction. 
        cpu->instruction.operands[0].type = Operand_Relative_Immediate;
        cpu->instruction.operands[0].immediate = ip_inc8+2;
    }
    else if (((byte >> 4) & 0x0F) == 0b1110) {
        cpu->instruction.type = Instruction_Type_Jump;

        s8 ip_inc8 = ASMD_NEXT_BYTE(cpu);
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

        cpu->instruction.opcode = opcode;

        // We have to add +2, because the relative displacement (offset) start from the second byte (ip_inc8)
        //  of instruction. 
        cpu->instruction.operands[0].type = Operand_Relative_Immediate;
        cpu->instruction.operands[0].immediate = ip_inc8+2;
    }
    else {
        fprintf(stderr, "[WARNING]: Instruction is not handled!\n");
        assert(0);
    }

    cpu->decoder_cursor++;

    cpu->instruction.size = cpu->decoder_cursor - instruction_byte_start_offset; 
}

