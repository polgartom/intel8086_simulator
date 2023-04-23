#include "sim86.h"

const char *get_opcode_name(Opcode opcode)
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

const char *get_register_name(Register reg)
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

void print_flags(u16 flags)
{
    if (flags & F_SIGNED) {
        printf("S");
    }

    if (flags & F_ZERO) {
        printf("Z");
    }
}

void print_instruction(Context *ctx, u8 with_end_line)
{
    FILE *dest = stdout;

    Instruction *instruction = &ctx->instruction;

    fprintf(dest, "[0x%02x]\t%s", instruction->mem_address, get_opcode_name(instruction->opcode));
    
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
                Register_Info *reg = get_register_info(instruction->flags & FLAG_IS_16BIT, op->reg);
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
