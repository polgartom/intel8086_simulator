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

        if (inst->type == INST_MOVE) {
            if (inst->mod == MOD_REG) {
                u8 opcode = 0b10001000;
                opcode |= (W(inst) << 0); 
                opcode |= (inst->d << 1); 
                fwrite(&opcode, 1, 1, fp);

                u8 operands = 0;
                operands |= ((u8)inst->mod << 6);
                operands |= reg_rm(inst->a.reg) << 3;
                operands |= reg_rm(inst->b.reg) << 0;
                fwrite(&operands, 1, 1, fp);
            }
            else {
                if (inst->b.type == OPERAND_IMMEDIATE) {
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
                        fwrite(&disp, 1, 1, fp);
                    } 
                    else if (inst->a.type == OPERAND_MEMORY) {
                        operand |= mem_rm(inst->a.address, inst->mod);
                        fwrite(&operand, 1, 1, fp);

                        disp = inst->a.address.displacement;
                        fwrite(&disp, IS_16BIT(disp) ? 2 : 1, 1, fp);
                    }
                    else {
                        ASSERT(0, "What?");
                    }

                    u16 immediate = inst->b.immediate & 0xFFFF;
                    // if (W(inst)) immediate = BYTE_SWAP(immediate); 
                    fwrite(&immediate, inst->size, 1, fp);
                }
            }

        }
    }

    fclose(fp);
}