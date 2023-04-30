#include "printer.h"


const char *get_opcode_name(Opcode opcode)
{
    assert(opcode < Opcode_count);

    static const char* opcode_names[] = {
        "none",

        "mov",
        "add",
        "sub",
        "cmp",

        "push",
        "pop",

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

const char *mnemonic_name(Mneumonic m)
{
    static const char *mnemonic_name_lookup[] = {
        [Mneumonic_mov]     = "mov",
        [Mneumonic_add]     = "add",
        [Mneumonic_sub]     = "sub",
        [Mneumonic_cmp]     = "cmp",
        [Mneumonic_je]      = "je",
        [Mneumonic_jnz]     = "jnz",
        [Mneumonic_jl]      = "jl",
        [Mneumonic_jle]     = "jle",
        [Mneumonic_jb]      = "jb",
        [Mneumonic_jbe]     = "jbe",
        [Mneumonic_jp]      = "jp",
        [Mneumonic_jo]      = "jo",
        [Mneumonic_js]      = "js",
        [Mneumonic_jne]     = "jne",
        [Mneumonic_jnl]     = "jnl",
        [Mneumonic_jg]      = "jg",
        [Mneumonic_jnb]     = "jnb",
        [Mneumonic_ja]      = "ja",
        [Mneumonic_jnp]     = "jnp",
        [Mneumonic_jno]     = "jno",
        [Mneumonic_jns]     = "jns",
        [Mneumonic_loop]    = "loop",
        [Mneumonic_loopz]   = "loopz",
        [Mneumonic_loopnz]  = "loopnz",
        [Mneumonic_jcxz]    = "jcxz",
    };

    return mnemonic_name_lookup[m];
}

const char *register_name(Register reg)
{
    assert(reg < Register_count);

    static const char *register_names[] = {
        "al", "cl", "dl", "bl",
        "ah", "ch", "dh", "bh",
        "ax", "cx", "dx", "bx",
        "sp", "bp", "si", "di",
        "cs", "ds", "ss", "es",
        "ip"
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

void print_instruction(CPU *cpu, u8 with_end_line)
{
    FILE *dest = stdout;

    Instruction *instruction = &cpu->instruction;

    fprintf(dest, "[0x%02x]\t%s", instruction->mem_address, mnemonic_name(instruction->mnemonic));
    
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
                Register reg; 

                // @Hacky
                static const u32 segment_registers[] = {
                    Register_es,
                    Register_cs,
                    Register_ss,
                    Register_ds
                };

                if (instruction->flags & Inst_Segment) {
                    reg = segment_registers[op->reg];
                } else {
                    Register_Access *reg_access = register_access(op->reg, !!(instruction->flags & Inst_Wide));
                    reg = reg_access->reg; 
                }

                fprintf(dest, "%s", register_name(reg));

                break;
            }
            case Operand_Memory: {
                if (instruction->operands[0].type != Operand_Register) {
                    fprintf(dest, "%s ", (instruction->flags & Inst_Wide) ? "word" : "byte");
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
                fprintf(dest, "%d", op->immediate);

                break;
            }
            case Operand_Relative_Immediate: {
                fprintf(dest, "$%+d", op->immediate);

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
