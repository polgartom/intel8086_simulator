#include "assembler.h"

#define W(_inst) (_inst->size == W_WORD)

void build_bytecodes(Array instructions)
{
    FILE *fp = fopen("mock/a.out", "wb");
    if (fp == NULL) {
        printf("\n[ERROR]: Failed to open %s file. Probably it is not exists.\n", "a.out");
        assert(0);
    }

    for (int i = 0; i < instructions.count; i++) {
        Instruction *inst = (Instruction *)instructions.data[i];

        if (inst->prefixes & INST_PREFIX_SEGMENT) {
            u8 prefix = 0b00100110 | (reg_rm(inst->segment_reg) << 3);
            fwrite(&prefix, 1, 1, fp);
        }

        if (inst->type == INST_MOVE) {
            if (inst->a.is_segreg || inst->b.is_segreg) {
                Operand sr; Operand r_m;
                u8 opcode = 0; u8 operand = 0;

                if (inst->a.is_segreg) {
                    opcode = 0b10001110;
                    sr = inst->a;
                    r_m = inst->b;
                } else {
                    opcode = 0b10001100;
                    sr = inst->b;
                    r_m = inst->a;
                }

                fwrite(&opcode,  1, 1, fp);

                operand |= ((u8)inst->mod << 6);
                operand |= (reg_rm(sr.reg) << 3);

                if (r_m.type == OPERAND_REGISTER) {
                    operand |= (reg_rm(r_m.reg) << 0);
                    fwrite(&operand, 1, 1, fp);
                } else {
                    operand |= (mem_rm(r_m.address, inst->mod) << 0);
                    fwrite(&operand, 1, 1, fp);

                    u16 disp = r_m.address.displacement;
                    if (inst->mod != MOD_MEM) {
                        // 8 or 16 bit displacement
                        fwrite(&disp, IS_16BIT(disp) ? 2 : 1, 1, fp);
                    } else {
                        // Direct address
                        if (disp != 0) {
                            fwrite(&disp, 2, 1, fp);
                        }
                    }
                }
            }
            else if (inst->b.type != OPERAND_IMMEDIATE) {
                u8 opcode = 0b10001000;
                opcode |= (inst->d << 1);
                opcode |= (W(inst) << 0);
                fwrite(&opcode, 1, 1, fp);
                
                u8 operand = 0;
                operand |= ((u8)inst->mod << 6);

                if (inst->a.type == OPERAND_REGISTER && inst->b.type == OPERAND_REGISTER) {
                    operand |= reg_rm(inst->a.reg) << 3;
                    operand |= reg_rm(inst->b.reg) << 0;

                    fwrite(&operand, 1, 1, fp);
                }
                else {
                    Operand reg = inst->a.type == OPERAND_REGISTER ? inst->a : inst->b;
                    Operand r_m = inst->a.type == OPERAND_MEMORY   ? inst->a : inst->b;

                    operand |= reg_rm(reg.reg) << 3;
                    operand |= mem_rm(r_m.address, inst->mod) << 0;

                    fwrite(&operand, 1, 1, fp);

                    u16 disp = r_m.address.displacement;
                    if (inst->mod != MOD_MEM) {
                        // 8 or 16 bit displacement
                        fwrite(&disp, IS_16BIT(disp) ? 2 : 1, 1, fp);
                    } else {
                        // Direct address
                        if (disp != 0) {
                            fwrite(&disp, 2, 1, fp);
                        }
                    }
                }
            }
            else {
                // In the 8086_family_Users_Manual_1_.pdf on page 164, we can encode 'immediate to register' in two ways.
                // However, we're using the 'Immediate to register/memory' for both memory and registers. 
                // This approach allows us to write less code.

                u8 opcode = 0b11000110;
                opcode |= (W(inst) << 0);
                fwrite(&opcode, 1, 1, fp);

                u16 operand = 0;
                operand |= ((u8)inst->mod << 6);

                u16 disp = 0;

                if (inst->a.type == OPERAND_REGISTER) {
                    operand |= reg_rm(inst->a.reg);
                    fwrite(&operand, 1, 1, fp);

                    // empty 16bit displacement
                } 
                else if (inst->a.type == OPERAND_MEMORY) {
                    operand |= mem_rm(inst->a.address, inst->mod);
                    fwrite(&operand, 1, 1, fp);

                    fwrite(&inst->a.address.displacement, 2, 1, fp);
                }

                u16 immediate = inst->b.immediate & 0xFFFF;
                // if (W(inst)) immediate = BYTE_SWAP(immediate); 
                fwrite(&immediate, inst->size, 1, fp);
            }

        }
    }

    fclose(fp);
}