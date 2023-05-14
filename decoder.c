#include "decoder.h"
#include "printer.h"
#include "simulator.h"

#define ASMD_CURR_BYTE(_d) _d->memory[_d->decoder_cursor]
u8 ASMD_NEXT_BYTE(CPU *_d) { return _d->memory[++_d->decoder_cursor]; }
#define ASMD_NEXT_BYTE_WITHOUT_STEP(_d) _d->memory[_d->decoder_cursor+1]
#define ASMD_CURR_BYTE_INDEX(_d) _d->decoder_cursor

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
    }

    fprintf(stderr, "[ERROR]: Invalid effective address expression!\n");
    assert(0);
    return Effective_Address_direct;
}


void decode_memory_address_displacement(CPU *cpu, Instruction_Operand *operand)
{
    Instruction *inst = &cpu->instruction;

    operand->type = Operand_Memory;
    operand->address.base = get_address_base(inst->r_m, inst->mod);
    operand->address.displacement = 0;

    if ((inst->mod == 0x00 && inst->r_m == 0x06) || inst->mod == 0x02) { 
        if (operand->address.base == Effective_Address_direct) {
            operand->address.displacement = (u16)(BYTE_LOHI_TO_HILO(ASMD_NEXT_BYTE(cpu), ASMD_NEXT_BYTE(cpu)));
        } else {
            operand->address.displacement = (s16)(BYTE_LOHI_TO_HILO(ASMD_NEXT_BYTE(cpu), ASMD_NEXT_BYTE(cpu)));
        }
    } 
    else if (inst->mod == 0x01) { 
        if (operand->address.base == Effective_Address_direct) {
            operand->address.displacement = (u8)ASMD_NEXT_BYTE(cpu);
        } else {
            operand->address.displacement = (s8)ASMD_NEXT_BYTE(cpu);
        }
    }
}

void mod_reg_rm(CPU *cpu, Instruction *inst) 
{
    if (inst->mod_reg_rm_decoded == 0) {
        u8 byte = ASMD_NEXT_BYTE(cpu);

        inst->mod = (byte >> 6) & 0b11;
        inst->reg = (byte >> 3) & 0b111;
        inst->r_m = byte & 0b111;

        inst->mod_reg_rm_decoded = 1;
    }
}

void decode_arg(CPU *cpu, Instruction_Operand *op, const char *arg)
{
    Instruction *inst = &cpu->instruction;
    
    if (arg == NULL) {
        return;
    }

    if (arg[0] == 'e' || arg[strlen(arg)-1] == 'X') {
        inst->flags |= Inst_Wide;
    }

    // @Cleanup!
    if (STR_EQUAL("AL", arg) || STR_EQUAL("eAX", arg)) {
        op->type = Operand_Register;
        op->reg  = 0; // encoded binary value of the reg
        return;
    } else if (STR_EQUAL("CL", arg) || STR_EQUAL("eCX", arg)) {
        if (STR_EQUAL("CL", arg)) {
            // @Cleanup: Temp solution 
            // This one of the exceptions. SAL/SHL && SAR && SHR && ROL && ROR && RCL && RCR instructions are using it.
            op->use_lower_reg = 1;
        }
        op->type = Operand_Register;
        op->reg  = 1; 
        return;
    } else if (STR_EQUAL("AH", arg) || STR_EQUAL("eSP", arg)) {
        op->type = Operand_Register;
        op->reg  = 4;
        return; 
    } else if (STR_EQUAL("CH", arg) || STR_EQUAL("eBP", arg)) {
        op->type = Operand_Register;
        op->reg  = 5;
        return;    
    } else if (STR_EQUAL("DL", arg) || STR_EQUAL("eDX", arg) || STR_EQUAL("DX", arg)) {
        op->type = Operand_Register;
        op->reg  = 2;
        return;
    } else if (STR_EQUAL("DH", arg) || STR_EQUAL("eSI", arg)) {
        op->type = Operand_Register;
        op->reg  = 6;
        return;
    } else if (STR_EQUAL("BL", arg) || STR_EQUAL("eBX", arg)) {
        op->type = Operand_Register;
        op->reg  = 3;
        return;    
    } else if (STR_EQUAL("BH", arg) || STR_EQUAL("eDI", arg)) {
        op->type = Operand_Register;
        op->reg  = 7;
        return;
    } else if (STR_EQUAL("ES", arg)) {
        op->type = Operand_Register;
        op->is_segment_reg = 1;
        op->reg = 0; // @Incomplete @Cleanup: Lookup for the static const u32 segment_registers[] in printer.c
        return;
    } else if (STR_EQUAL("CS", arg)) {
        op->type = Operand_Register;
        op->is_segment_reg = 1;
        op->reg = 1; // @Incomplete @Cleanup: Lookup for the static const u32 segment_registers[] in printer.c
        return;
    } else if (STR_EQUAL("SS", arg)) {
        op->type = Operand_Register;
        op->is_segment_reg = 1;
        op->reg = 2; // @Incomplete @Cleanup: Lookup for the static const u32 segment_registers[] in printer.c
        return;
    } else if (STR_EQUAL("DS", arg)) {
        op->type = Operand_Register;
        op->is_segment_reg = 1;
        op->reg = 3; // @Incomplete @Cleanup: Lookup for the static const u32 segment_registers[] in printer.c
        return;
    }

    u8 arg_strlen = STR_LEN(arg);
    for (u64 i = 0; i < arg_strlen; i++) {
        if (arg[i] == 'A') {
            // Direct address. The instruction has no ModR/M byte; the address of the operand 
            // is encoded in the instruction. Applicable, e.g., to far JMP (opcode EA).
            assert(arg[++i] == 'p');

            inst->flags |= Inst_Segment | Inst_Far;

            op->type = Operand_Memory;
            op->address.base = Effective_Address_direct;        
            // this is the offset
            op->address.displacement = (u16)(BYTE_LOHI_TO_HILO(ASMD_NEXT_BYTE(cpu), ASMD_NEXT_BYTE(cpu)));
            // segment are encoded next to the offset
            op->address.segment = (u16)(BYTE_LOHI_TO_HILO(ASMD_NEXT_BYTE(cpu), ASMD_NEXT_BYTE(cpu)));
            
            // the result will be segment:offset
            
        } else if (arg[i] == 'J') {
            // The instruction contains a relative offset to be added to the address of the 
            // subsequent instruction. Applicable, e.g., to short JMP (opcode EB), or LOOP.

            op->type = Operand_Relative_Immediate;
            if (arg[++i] == 'v') {
                // @Todo: Remove this 3. This is the isntruction size btw
                op->immediate = (s16)(BYTE_LOHI_TO_HILO(ASMD_NEXT_BYTE(cpu), ASMD_NEXT_BYTE(cpu))+3);
            } else {
                // @Todo: Remove this 2. This is the isntruction size btw
                op->immediate = (s8)(ASMD_NEXT_BYTE(cpu)+2); 
            }
        
        } else if (arg[i] == 'E') {
            // A ModR/M byte follows the opcode and specifies the operand. The operand is either a general-
            // purpose register or a memory address. If it is a memory address, the address is computed from a 
            // segment register and any of the following values: a base register, an index register, a displacement.
            
            mod_reg_rm(cpu, inst);
    
            if (inst->mod == MOD_REGISTER) {
                op->type = Operand_Register;
                op->reg = inst->r_m;
            } else {
                decode_memory_address_displacement(cpu, op);
            }

        } else if (arg[i] == 'G') {
            // The reg field of the ModR/M byte selects a general register.

            mod_reg_rm(cpu, inst);

            op->type = Operand_Register;
            op->reg = inst->reg;

        } else if (arg[i] == 'I') {
            // Immediate data. The operand value is encoded in subsequent bytes of the instruction.

            s16 immediate = ASMD_NEXT_BYTE(cpu);
            op->type = Operand_Immediate;

            assert(arg[i+1] != '\0');

            char next_char = arg[++i];
            if (next_char == 'v' || next_char == 'w') {
                inst->flags |= Inst_Wide;
                op->immediate = (s16)BYTE_LOHI_TO_HILO(immediate, ASMD_NEXT_BYTE(cpu));
            } else if (next_char == '0') {
                // @Todo: This is ok?
                op->type = Operand_Immediate;
                s16 immediate = ASMD_NEXT_BYTE(cpu);
                if (immediate == 0xa) continue;
                op->immediate = immediate;
            } else if (next_char == 'b') {
                op->immediate = immediate;
            } else {
                assert(0);
            }

        } else if (arg[i] == 'O') {
            // The instruction has no ModR/M byte; the offset of the operand is encoded as a WORD in the instruction. 
            // Applicable, e.g., to certain MOVs (opcodes A0 through A3).

            op->type = Operand_Memory;
            op->address.base = Effective_Address_direct;
            op->address.displacement = (s32)BYTE_LOHI_TO_HILO(ASMD_NEXT_BYTE(cpu), ASMD_NEXT_BYTE(cpu));

        } else if (arg[i] == 'S') {
            // The reg field of the ModR/M byte selects a segment register.

            mod_reg_rm(cpu, inst);

            inst->flags |= Inst_Segment;
            op->is_segment_reg = 1;
            op->type = Operand_Register;
            op->reg  = inst->reg;

        } else if (arg[i] == 'M') {
            // The ModR/M byte may refer only to memory. Applicable, e.g., to LES and LDS.
            
            mod_reg_rm(cpu, inst);
            decode_memory_address_displacement(cpu, op);

        } else if (arg[i] == 'v' || arg[i] == 'w') {
            // Word argument. (The 'v' code has a more complex meaning in later x86 opcode maps, 
            // from which this was derived, but here it's just a synonym for the 'w' code.)

            inst->flags |= Inst_Wide;  
        
        } else if (arg[i] == 'b') {
            // Byte argument. This is the default value so we don't need to change here the flags.

        } else if (arg[i] == 'p') {
            // 32-bit segment:offset pointer.
            inst->flags |= Inst_Far;

        } else if (arg[i] == '1') {
            // @Todo: This is ok? Maybe we should create a new opcode type like Operand_Constant?
            op->type = Operand_Immediate;
            op->reg = 1;
            
        } else if (arg[i] == '3') {
            // @Todo: This is ok? Maybe we should create a new opcode type like Operand_Constant?
            op->type = Operand_Immediate;
            op->reg = 3;

        } else {
            printf(">> %s\n", arg);

            assert(0);
        }
    }

}

static Mneumonic extended_mneumonic_lookup[][8] = {
    [Mneumonic_grp1]  = {Mneumonic_add, Mneumonic_or, Mneumonic_adc, Mneumonic_sbb, Mneumonic_and, Mneumonic_sub, Mneumonic_xor, Mneumonic_cmp},
    [Mneumonic_grp2]  = {Mneumonic_rol, Mneumonic_ror, Mneumonic_rcl, Mneumonic_rcr, Mneumonic_shl, Mneumonic_shr, Mneumonic_invalid, Mneumonic_sar},
    [Mneumonic_grp3b] = {Mneumonic_test, Mneumonic_invalid, Mneumonic_not, Mneumonic_neg, Mneumonic_mul, Mneumonic_imul, Mneumonic_div, Mneumonic_idiv},
    [Mneumonic_grp3a] = {Mneumonic_test, Mneumonic_invalid, Mneumonic_not, Mneumonic_neg, Mneumonic_mul, Mneumonic_imul, Mneumonic_div, Mneumonic_idiv},
    [Mneumonic_grp4]  = {Mneumonic_inc, Mneumonic_dec, Mneumonic_invalid, Mneumonic_invalid, Mneumonic_invalid, Mneumonic_invalid, Mneumonic_invalid, Mneumonic_invalid},
    [Mneumonic_grp5]  = {Mneumonic_inc, Mneumonic_dec, Mneumonic_call, Mneumonic_call, Mneumonic_jmp, Mneumonic_jmp, Mneumonic_push, Mneumonic_invalid}
};

static const char *i8086_inst_ext_table[][8][2] = {
    [Mneumonic_grp1]  = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}},
    [Mneumonic_grp2]  = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}},
    [Mneumonic_grp3a] = {{"Eb", "Ib"}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}},
    [Mneumonic_grp3b] = {{"Ev", "Iv"}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}},
    [Mneumonic_grp4]  = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}},
    [Mneumonic_grp5]  = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {"Mp", NULL}, {NULL, NULL}, {"Mp", NULL}, {NULL, NULL}, {NULL, NULL}},
};

void decode_next_instruction(CPU *cpu)
{
    Instruction *inst = &cpu->instruction;
    cpu->decoder_cursor = cpu->ip;

    if (!inst->is_prefix) {
        ZERO_MEMORY(inst, sizeof(Instruction));
        inst->extend_with_this_segment = Register_none;
    }
    inst->is_prefix = 0;

    u8 byte = ASMD_CURR_BYTE(cpu);
    inst->mem_address = cpu->decoder_cursor;
    u32 instruction_byte_start_offset = cpu->decoder_cursor;

    i8086_Inst_Table x = i8086_inst_table[byte];
    inst->mnemonic = x.mnemonic;
    inst->type = x.type;
    
    // Overwrite the arguments if the extenstion table lookup is find something
    if (x.mnemonic >= Mneumonic_grp1) {
        mod_reg_rm(cpu, inst);

        inst->mnemonic = extended_mneumonic_lookup[x.mnemonic][inst->reg];
        
        // @Todo: Handle this! // @Cleanup:
        switch (inst->mnemonic) {
            case Mneumonic_inc: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_dec: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_add: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_sub: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_mul: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_imul: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_div: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_idiv: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_or:  inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_xor: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_and: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_not: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_test: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_cmp: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_neg: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_adc: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_sbb: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_shl: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_shr: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_sar: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_rol: inst->type = Instruction_Type_arithmetic; break;
            case Mneumonic_ror: inst->type = Instruction_Type_arithmetic; break;

            case Mneumonic_jmp: inst->type = Instruction_Type_jump; break;

            default:
                printf("[WARNING]: Not handled instruction (grouped) type definition.\n");
                assert(0);
        }

        const char *ext_arg1 = i8086_inst_ext_table[x.mnemonic][inst->reg][0]; 
        const char *ext_arg2 = i8086_inst_ext_table[x.mnemonic][inst->reg][1];
        if (ext_arg1 || ext_arg2) {
            x.arg1 = ext_arg1;
            x.arg2 = ext_arg2;
        }
    }

    //printf("\n%#02x ; %d ; %s ; %s -> \t|", x.opcode, x.mnemonic, x.arg1, x.arg2);

    decode_arg(cpu, &inst->operands[0], x.arg1);
    decode_arg(cpu, &inst->operands[1], x.arg2);

    // Set prefixes
    // @Todo: Handle more prefixes
    if (inst->mnemonic == Mneumonic_lock) {
        inst->is_prefix = 1;
        inst->flags |= Inst_Lock;
    } 
    else if (inst->mnemonic == Mneumonic_cs) {
        inst->is_prefix = 1;
        inst->flags |= Inst_Segment;
        inst->extend_with_this_segment = Register_cs;
    }
    else if (inst->mnemonic == Mneumonic_ds) {
        inst->is_prefix = 1;
        inst->flags |= Inst_Segment;
        inst->extend_with_this_segment = Register_ds;
    }
    else if (inst->mnemonic == Mneumonic_es) {
        inst->is_prefix = 1;
        inst->flags |= Inst_Segment;
        inst->extend_with_this_segment = Register_es;
    }
    else if (inst->mnemonic == Mneumonic_ss) {
        inst->is_prefix = 1;
        inst->flags |= Inst_Segment;
        inst->extend_with_this_segment = Register_ss;
    }
    
    if ((inst->flags & Inst_Lock) && inst->is_prefix == 0) {
        // Flip memory, register because the lock prefix must be follow a memory operand   
        if (inst->operands[0].type != Operand_Memory) {
            assert(inst->operands[1].type == Operand_Memory);
            Instruction_Operand temp = inst->operands[0];
            inst->operands[0] = inst->operands[1];
            inst->operands[1] = temp;
        }
    }
    
    cpu->decoder_cursor++;
    cpu->instruction.size = cpu->decoder_cursor - instruction_byte_start_offset; 
}