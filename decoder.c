#include "decoder.h"
#include "printer.h"

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


////////////////////////////////////////


void decode_memory_address_displacement(CPU *cpu, Instruction_Operand *operand)
{
    Instruction *inst = &cpu->instruction;

    operand->type = Operand_Memory;
    operand->address.base = get_address_base(inst->r_m, inst->mod);
    operand->address.displacement = 0;

    if ((inst->mod == 0x00 && inst->r_m == 0x06) || inst->mod == 0x02) { 
        operand->address.displacement = (s16)(BYTE_LOHI_TO_HILO(ASMD_NEXT_BYTE(cpu), ASMD_NEXT_BYTE(cpu)));
    } 
    else if (inst->mod == 0x01) { 
        operand->address.displacement = (s8)ASMD_NEXT_BYTE(cpu);
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
    if (arg == NULL) return;
    Instruction *inst = &cpu->instruction;

    // Will this cause a problem?
    if (arg[0] == 'e') {
        inst->flags |= Inst_Wide;
    }

    // @Cleanup!
    if (STR_EQUAL("AL", arg) || STR_EQUAL("eAX", arg)) {
        op->type = Operand_Register;
        op->reg  = 0; // encoded binary value of the reg
        return;
    } else if (STR_EQUAL("CL", arg) || STR_EQUAL("eCX", arg)) {
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
    } else if (STR_EQUAL("DL", arg) || STR_EQUAL("eDX", arg)) {
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
    } else if (STR_EQUAL("CS", arg)) {
        op->type = Operand_Register;
        op->reg = 1;

        assert(op->reg == ((ASMD_CURR_BYTE(cpu) >> 3) & 0b11));
    }

    for (u64 i = 0; i < STR_LEN(arg); i++) {
        if (arg[i] == 'A') {
            // Direct address. The instruction has no ModR/M byte; the address of the operand 
            // is encoded in the instruction. Applicable, e.g., to far JMP (opcode EA).
            
            assert(0);

        } else if (arg[i] == 'J') {
            // The instruction contains a relative offset to be added to the address of the 
            // subsequent instruction. Applicable, e.g., to short JMP (opcode EB), or LOOP.

            op->type = Operand_Relative_Immediate;
            op->immediate = (s8)(ASMD_NEXT_BYTE(cpu)+2);
        }
         else if (arg[i] == 'E') {
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

            s8 immediate = ASMD_NEXT_BYTE(cpu);
            op->type = Operand_Immediate;

            if (arg[++i] == 'v') {
                inst->flags |= Inst_Wide;
                op->immediate = (s16)BYTE_LOHI_TO_HILO(immediate, ASMD_NEXT_BYTE(cpu));
            } else {
                op->immediate = immediate;
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

            op->type |= (Inst_Segment | Inst_Wide);
            op->reg   = inst->reg;

        } else if (arg[i] == 'v' || arg[i] == 'w') {
            // Word argument. (The 'v' code has a more complex meaning in later x86 opcode maps, 
            // from which this was derived, but here it's just a synonym for the 'w' code.)

            inst->flags |= Inst_Wide;  
        
        } else if (arg[i] == 'b') {
            // Byte argument. This is the default value so we don't need to change here the flags.
        }
        else {
           assert(0);
        }
    }

}

static const char *i8086_inst_ext_table[][8][2] = {
    [Mneumonic_grp1]  = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}},
    [Mneumonic_grp2]  = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}},
    [Mneumonic_grp3a] = {{"Eb", "Ib"}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}},
    [Mneumonic_grp3b] = {{"Ev", "Iv"}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}},
    [Mneumonic_grp4]  = {{"Ev", "Iv"}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}},
    [Mneumonic_grp5]  = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {"Mb", NULL}, {NULL, NULL}, {NULL, "Mb"}, {NULL, NULL}, {NULL, NULL}},
};

void decode_next_instruction(CPU *cpu)
{
    cpu->decoder_cursor = cpu->ip;

    ZERO_MEMORY(&cpu->instruction, sizeof(Instruction)); 
    cpu->instruction.mem_address = cpu->decoder_cursor;

    u32 instruction_byte_start_offset = cpu->decoder_cursor;

    u8 byte = ASMD_CURR_BYTE(cpu);

    i8086_Inst_Table x = i8086_inst_table[byte];

    printf("%#02x ; %d ; %s ; %s\n", x.opcode, x.mnemonic, x.arg1, x.arg2);

    cpu->instruction.mnemonic = x.mnemonic;

    // @Todo: Replace the grouped mnemonic after this extension lookup
    // Override the opcode argument desciptors if arguments is defined in that table
    if (x.mnemonic >= Mneumonic_grp1) {
        mod_reg_rm(cpu, &cpu->instruction);

        const char *ext_arg1 = i8086_inst_ext_table[x.mnemonic][cpu->instruction.reg][0]; 
        const char *ext_arg2 = i8086_inst_ext_table[x.mnemonic][cpu->instruction.reg][1];
        if (ext_arg1 || ext_arg2) {
            x.arg1 = ext_arg1;
            x.arg2 = ext_arg2;
        }
    }

    Instruction *inst = &cpu->instruction;

    decode_arg(cpu, &inst->operands[0], x.arg1);
    decode_arg(cpu, &inst->operands[1], x.arg2);

    cpu->decoder_cursor++;
    cpu->instruction.size = cpu->decoder_cursor - instruction_byte_start_offset; 
}
