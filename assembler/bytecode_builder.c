#include "assembler.h"

#define W(_inst) (_inst->size == W_WORD)

#define OUT(_data, _bytes) { u16 _out = (_data); fwrite(&_out, _bytes, 1, fp); }
#define OUTB(_data) OUT(_data, W_BYTE)
#define OUTW(_data) OUT(_data, W_WORD)

#define MOD_XXX_RM(_mod, _xxx, _rm) \
    OUTB(( 0b00000000 | ((_mod & 3) << 6) | ((_xxx & 7) << 3) | (_rm & 7) << 0 ));

// Encode displacement by the MOD field and the operand (memory) address type
#define DISP_MOD(_operand) { \
    if (_operand.type == OPERAND_MEMORY) { \
        u16 _disp = _operand.address.displacement; \
        if (inst->mod != MOD_MEM) { \
            /* 8 or 16 bit displacement */ \
            OUT(_operand.address.displacement, IS_16BIT(_disp) ? W_WORD : W_BYTE); \
        } else if (_disp != 0) { \
            OUT(_operand.address.displacement, W_WORD); \
        } \
    } \
} \

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
            OUTB(0b00100110 | (segreg(inst->segment_reg) << 3));
        }

        if (inst->type == INST_MOVE) {
            if (inst->a.is_segreg || inst->b.is_segreg) {
                u8 opcode = 0; 
                Operand sr, r_m;

                if (inst->a.is_segreg) {
                    // Register/memory to segment register
                    OUTB(0b10001110);
                    sr = inst->a;
                    r_m = inst->b;
                } else {
                    // Segment register to register/memory
                    OUTB(0b10001100);
                    sr = inst->b;
                    r_m = inst->a;
                }

                MOD_XXX_RM(inst->mod, segreg(sr.reg), reg_rm(r_m));
                DISP_MOD(r_m);
            }
            else if (inst->b.type != OPERAND_IMMEDIATE) {

                Operand reg = inst->a.type == OPERAND_REGISTER ? inst->a : inst->b;
                Operand r_m = inst->a.type == OPERAND_MEMORY   ? inst->a : inst->b;

                OUTB((0b10001000 | (inst->d << 1) | (W(inst) << 0)));
                MOD_XXX_RM(inst->mod, reg_rm(reg), reg_rm(r_m));
                DISP_MOD(r_m);
            }
            else {
                // In the 8086_family_Users_Manual_1_.pdf on page 164, we can encode 'immediate to register' in two ways.
                // However, we're using the 'Immediate to register/memory' for both memory and registers. 
                // This approach allows us to write less code.
                OUTB(0b11000110 | (W(inst) << 0));
                MOD_XXX_RM(inst->mod, 0b000, reg_rm(inst->a));
                DISP_MOD(inst->a);
                OUT(inst->b.immediate, inst->size);
            }

        }
    }

    fclose(fp);
}