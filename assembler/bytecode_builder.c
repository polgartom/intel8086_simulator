#include "assembler.h"

#define W(_inst) (_inst->size == W_WORD)

#define OUT(_data, _bytes) { u16 _out = (_data); fwrite(&_out, _bytes, 1, fp); }
#define OUTB(_data) OUT(_data, W_BYTE)
#define OUTW(_data) OUT(_data, W_WORD)

#define MOD_XXX_RM(_mod, _xxx, _rm) \
    OUTB(( 0b00000000 | ((_mod & 3) << 6) | ((_xxx & 7) << 3) | ((_rm & 7) << 0) ));

// Encode displacement by the MOD field and the operand (memory) address type
// @Testit
#define DISP_MOD(_operand) { \
    if (_operand.type == OPERAND_MEMORY) { \
        u16 _disp = _operand.address.displacement; \
        if (inst->mod != MOD_MEM) { \
            /* 8 or 16 bit displacement @todo */ \
            if (_operand.address.base == EFFECTIVE_ADDR_BP) { \
                OUT(_operand.address.displacement, IS_16BIT(_disp) ? W_WORD : W_BYTE); \
            } else { \
                if (_disp) {\
                    OUT(_operand.address.displacement, IS_16BIT(_disp) ? W_WORD : W_BYTE); \
                }\
            }\
        } else if (_disp != 0) { \
            OUT(_operand.address.displacement, W_WORD); \
        } \
    } \
} \

#define DECIDE_REG(_inst) _inst->a.type == OPERAND_REGISTER ? _inst->a : _inst->b
// DECIDE_RM will return a register type operand if both a and b is register type
#define DECIDE_RM(_inst)  _inst->a.type == OPERAND_MEMORY   ? _inst->a : _inst->b
 
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

        switch (inst->mnemonic) {
            case M_MOV: {
                if (IS_OPERAND_REG(inst->a, REG_AX) && IS_OPERAND_MEM(inst->b) && inst->b.address.base == EFFECTIVE_ADDR_DIRECT) {
                    // Memory to accumulator
                    OUTB(0b10100000 | W(inst));
                    DISP_MOD(inst->b);
                }
                else if (IS_OPERAND_MEM(inst->a) && inst->b.address.base == EFFECTIVE_ADDR_DIRECT && IS_OPERAND_REG(inst->b, REG_AX)) {
                    // Accumulator to memory
                    OUTB(0b10100010 | W(inst));
                    DISP_MOD(inst->a);
                }
                else if (inst->b.type == OPERAND_IMMEDIATE) {
                    if (inst->a.type == OPERAND_REGISTER) {
                        // Immediate to register
                        OUTB(0b10110000 | (W(inst)<<3) | reg_rm(inst->a));
                    } else {
                        // When are we using this to encode immediate to register???

                        // Immediate to register / memory
                        OUTB(0b11000110 | W(inst));
                        MOD_XXX_RM(inst->mod, 0b000, reg_rm(inst->a));
                        DISP_MOD(inst->a);
                    }

                    OUT(inst->b.immediate, inst->size);

                } else {
                    Operand reg_or_sr, r_m;

                    if (IS_SEGREG(inst->a.reg)) {
                        // Register/memory to segment register
                        reg_or_sr  = inst->a;
                        r_m        = inst->b;
                        OUTB(0b10001110);                    
                    } else if (IS_SEGREG(inst->b.reg)) {
                        // Segment register to register/memory
                        reg_or_sr = inst->b;
                        r_m       = inst->a; 
                        OUTB(0b10001100);
                    } else {
                        // Register/memory to/from register
                        reg_or_sr = DECIDE_REG(inst);
                        r_m       = DECIDE_RM(inst);
                        OUTB((0b10001000 | (inst->d << 1) | W(inst)));
                    }

                    MOD_XXX_RM(inst->mod, reg_rm(reg_or_sr), reg_rm(r_m));
                    DISP_MOD(r_m);
                }

                break;
            }
            case M_ADD: {
                if (inst->b.type == OPERAND_IMMEDIATE) {
                    
                    u8 s = 0; // @Incomplete: immediate size by 's' and 'w' field
                    OUTB(0b10000000 | (s << 1) | W(inst));
                    MOD_XXX_RM(inst->mod, 0b000, reg_rm(inst->a));
                    DISP_MOD(inst->a);
                    OUT(inst->b.immediate, inst->size); // @Incomplete: s: w=01

                } else {
                    
                    // Register/memory with register to either
                    OUTB(0b00000000 | (inst->d << 1) | W(inst));

                    Operand reg = DECIDE_REG(inst);
                    Operand r_m = DECIDE_RM(inst);

                    MOD_XXX_RM(inst->mod, reg_rm(reg), reg_rm(r_m));
                    DISP_MOD(r_m);

                }
                break;
            }
        }

    }

    fclose(fp);
}