#include "assembler.h"

void build_bytecodes(Array instructions)
{
    FILE *fp = fopen("mock/a.out", "wb");
    if (fp == NULL) {
        printf("\n[ERROR]: Failed to open %s file. Probably it is not exists.\n", "a.out");
        assert(0);
    }

    for (int i = 0; i < instructions.count; i++) {
        Instruction *inst = (Instruction *)instructions.data[i];

        if (inst->type == INST_MOVE) {
            if (inst->a.type == OPERAND_REGISTER) {
                if (inst->b.type == OPERAND_REGISTER) {
                    u8 opcode = 0b10001000;
                    u8 w = inst->size == W_WORD ? 1 : 0;
                    opcode |= (w << 0); 
                    opcode |= (inst->d << 1); 
                    fwrite(&opcode, 1, 1, fp);

                    u8 operands = 0;
                    operands |= ((u8)inst->mod << 6);
                    operands |= reg_rm(inst->a.reg) << 3;
                    operands |= reg_rm(inst->b.reg) << 0;
                    fwrite(&operands, 1, 1, fp);
                }
                else if (inst->b.type == OPERAND_IMMEDIATE) {
                    u8 opcode = 0b10110000;
                    u8 w = inst->size == W_WORD ? 1 : 0;
                    opcode |= (w << 3);
                    opcode |= reg_rm(inst->a.reg) << 0;
                    fwrite(&opcode, 1, 1, fp);

                    if (w) {
                        // u16 operands = BYTE_SWAP(inst->b.immediate);
                        u16 operands = (inst->b.immediate);
                        fwrite(&operands, 2, 1, fp);
                    } else {
                        u8 operand = inst->b.immediate & 0x00FF;
                        fwrite(&operand, 1, 1, fp);
                    }
                }
            }

        }
    }

    fclose(fp);
}