#include "printer.h"

// @Todo: Remove the reg param
const char *mnemonic_name(Mneumonic m)
{
    static const char *mnemonic_name_lookup[] = {
        [Mneumonic_mov]     = "mov",
        [Mneumonic_add]     = "add",
        [Mneumonic_adc]     = "adc",
        [Mneumonic_inc]     = "inc",
        [Mneumonic_dec]     = "dec",
        [Mneumonic_sub]     = "sub",
        [Mneumonic_cmp]     = "cmp",
        [Mneumonic_and]     = "and",
        [Mneumonic_aaa]     = "aaa",
        [Mneumonic_aad]     = "aad",
        [Mneumonic_aas]     = "aas",
        [Mneumonic_das]     = "das",
        [Mneumonic_daa]     = "daa",
        [Mneumonic_aam]     = "aam",
        [Mneumonic_sbb]     = "sbb",

        [Mneumonic_test]    = "test",
        [Mneumonic_or]      = "or",
        [Mneumonic_xor]     = "xor",

        [Mneumonic_movsw]   = "movsw",
        [Mneumonic_movsb]   = "movsb",
        [Mneumonic_cmpsb]   = "cmpsb",
        [Mneumonic_scasb]   = "scasb",
        [Mneumonic_lodsb]   = "lodsb",
        [Mneumonic_cmpsw]   = "cmpsw",
        [Mneumonic_scasw]   = "scasw",
        [Mneumonic_lodsw]   = "lodsw",
        [Mneumonic_stosb]   = "stosb",
        [Mneumonic_stosw]   = "stosw",
        [Mneumonic_int]     = "int",
        [Mneumonic_into]    = "into",
        [Mneumonic_iret]    = "iret",
        [Mneumonic_clc]     = "clc",
        [Mneumonic_cmc]     = "cmc",
        [Mneumonic_stc]     = "stc",
        [Mneumonic_cld]     = "cld",
        [Mneumonic_std]     = "std",
        [Mneumonic_cli]     = "cli",
        [Mneumonic_sti]     = "sti",
        [Mneumonic_hlt]     = "hlt",
        [Mneumonic_wait]    = "wait",
        [Mneumonic_cbw]     = "cbw",
        [Mneumonic_cwd]     = "cwd",
        [Mneumonic_ret]     = "ret",
        [Mneumonic_retf]    = "retf",
        [Mneumonic_call]    = "call",
        [Mneumonic_push]    = "push",
        [Mneumonic_pop]     = "pop",
        [Mneumonic_popf]    = "popf",
        [Mneumonic_lahf]    = "lahf",
        [Mneumonic_sahf]    = "sahf",
        [Mneumonic_pushf]   = "pushf",
        [Mneumonic_xchg]    = "xchg",
        [Mneumonic_nop]     = "nop",
        [Mneumonic_xlat]    = "xlat",
        [Mneumonic_in]      = "in",
        [Mneumonic_out]     = "out",
        [Mneumonic_lea]     = "lea",
        [Mneumonic_lds]     = "lds",
        [Mneumonic_les]     = "les",
        [Mneumonic_jmp]     = "jmp",
        [Mneumonic_jz]      = "jz",
        [Mneumonic_jnz]     = "jnz",
        [Mneumonic_jl]      = "jl",
        [Mneumonic_jle]     = "jle",
        [Mneumonic_jb]      = "jb",
        [Mneumonic_jbe]     = "jbe",
        [Mneumonic_jp]      = "jp",
        [Mneumonic_jo]      = "jo",
        [Mneumonic_js]      = "js",
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

        [Mneumonic_rol]     = "rol",
        [Mneumonic_ror]     = "ror",
        [Mneumonic_rcl]     = "rcl",
        [Mneumonic_rcr]     = "rcr",
        [Mneumonic_shl]     = "shl",
        [Mneumonic_shr]     = "shr",
        [Mneumonic_sar]     = "sar",
        [Mneumonic_not]     = "not",
        [Mneumonic_neg]     = "neg",
        [Mneumonic_mul]     = "mul",
        [Mneumonic_imul]    = "imul",
        [Mneumonic_div]     = "div",
        [Mneumonic_idiv]    = "idiv",

        // DB (define-byte) pseudo-instruction
        [Mneumonic_db]      = "db",

        // Prefixes
        [Mneumonic_repz]    = "repz",
        [Mneumonic_repnz]   = "repnz",
        [Mneumonic_lock]    = "lock",
        [Mneumonic_cs]      = "cs", // segment register
        [Mneumonic_ds]      = "ds", // segment register
        [Mneumonic_es]      = "es", // segment register
        [Mneumonic_ss]      = "ss", // segment register

        [Mneumonic_invalid] = "???",

        [Mneumonic_grp1]    = "grp1",
        [Mneumonic_grp2]    = "grp2",
        [Mneumonic_grp3a]   = "grp3a",
        [Mneumonic_grp3b]   = "grp3b",
        [Mneumonic_grp4]    = "grp4",
        [Mneumonic_grp5]    = "grp5"
    };

    if (mnemonic_name_lookup[m] == NULL) {
        printf("[WARNING]: mneumonic not handled: %d\n", m);
        assert(0);
    }

    return mnemonic_name_lookup[m];
}

const char *register_name(Register reg)
{
    static const char *register_names[] = {
        "al", "cl", "dl", "bl",
        "ah", "ch", "dh", "bh",
        "ax", "cx", "dx", "bx",
        "sp", "bp", "si", "di",
        "es", "cs", "ss", "ds",
        "ip"
    };

    assert(reg < ARRAY_SIZE(register_names) || reg > ARRAY_SIZE(register_names));

    return register_names[reg];
}

void print_flags(u16 flags)
{
    if (flags & F_SIGNED) {
        printf(" SF");
    }
    if (flags & F_ZERO) {
        printf(" ZF");
    }
    if (flags & F_CARRY) {
        printf(" CF");
    }
    if (flags & F_PARITY) {
        printf(" PF");
    }
    if (flags & F_OVERFLOW) {
        printf(" OF");
    }
    if (flags & F_AUXILIARY) {
        printf(" AF");
    }
    if (flags & F_INTERRUPT) {
        printf(" IF");
    }
    if (flags & F_DIRECTION) {
        printf(" DF");
    }
    if (flags & F_TRAP) {
        printf(" TF");
    }
}

void print_out_formated_flags(u16 old_flags, u16 new_flags)
{
    printf("\n\t\t@flags: [");
    print_flags(old_flags);
    printf(" ] -> [");
    print_flags(new_flags);
    printf(" ]");
}

void print_instruction(CPU *cpu, u8 with_end_line)
{
    FILE *dest = stdout;

    Instruction *instruction = &cpu->instruction;

    fprintf(dest, "%08X\t", instruction->mem_address);
    fprintf(dest, "%s", mnemonic_name(instruction->mnemonic));

    if (instruction->flags & Inst_Lock) {
        fprintf(dest, "lock ");
    }

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
                Register_Access *reg_access = register_access(op->reg, op->flags);
                Register reg_enum = reg_access->reg;

                fprintf(dest, "%s", register_name(reg_enum));

                break;
            }
            case Operand_Memory: {
                // @Cleanup:
                if (&instruction->operands[0] == op && !(instruction->flags & Inst_Far)) {
                    fprintf(dest, "%s ", (instruction->flags & Inst_Wide) ? "word" : "byte");
                }

                // @Todo: CleanUp
                if (instruction->flags & Inst_Segment) {
                    if (instruction->extend_with_this_segment != Register_none) {
                        // segment prefix
                        fprintf(dest, "%s:", register_name(instruction->extend_with_this_segment));
                    } else {
                        // segment at direct address
                        u16 segment = op->address.segment;
                        u16 offset = op->address.displacement;
                        fprintf(dest, "%d:%d", segment, offset);
                        break;
                    }
                }

                if (&instruction->operands[0] == op && instruction->flags & Inst_Far) {
                    fprintf(dest, "far ");
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
                fprintf(dest, "$%+d", op->immediate+instruction->size);

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
