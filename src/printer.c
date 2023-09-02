#include "printer.h"

// @Todo: Remove the reg param
const char *mnemonic_name(Mnemonic m)
{
    static const char *mnemonic_name_lookup[] = {
        [Mnemonic_mov]     = "mov",
        [Mnemonic_add]     = "add",
        [Mnemonic_adc]     = "adc",
        [Mnemonic_inc]     = "inc",
        [Mnemonic_dec]     = "dec",
        [Mnemonic_sub]     = "sub",
        [Mnemonic_cmp]     = "cmp",
        [Mnemonic_and]     = "and",
        [Mnemonic_aaa]     = "aaa",
        [Mnemonic_aad]     = "aad",
        [Mnemonic_aas]     = "aas",
        [Mnemonic_das]     = "das",
        [Mnemonic_daa]     = "daa",
        [Mnemonic_aam]     = "aam",
        [Mnemonic_sbb]     = "sbb",

        [Mnemonic_test]    = "test",
        [Mnemonic_or]      = "or",
        [Mnemonic_xor]     = "xor",

        [Mnemonic_movsw]   = "movsw",
        [Mnemonic_movsb]   = "movsb",
        [Mnemonic_cmpsb]   = "cmpsb",
        [Mnemonic_scasb]   = "scasb",
        [Mnemonic_lodsb]   = "lodsb",
        [Mnemonic_cmpsw]   = "cmpsw",
        [Mnemonic_scasw]   = "scasw",
        [Mnemonic_lodsw]   = "lodsw",
        [Mnemonic_stosb]   = "stosb",
        [Mnemonic_stosw]   = "stosw",
        [Mnemonic_int]     = "int",
        [Mnemonic_into]    = "into",
        [Mnemonic_iret]    = "iret",
        [Mnemonic_clc]     = "clc",
        [Mnemonic_cmc]     = "cmc",
        [Mnemonic_stc]     = "stc",
        [Mnemonic_cld]     = "cld",
        [Mnemonic_std]     = "std",
        [Mnemonic_cli]     = "cli",
        [Mnemonic_sti]     = "sti",
        [Mnemonic_hlt]     = "hlt",
        [Mnemonic_wait]    = "wait",
        [Mnemonic_cbw]     = "cbw",
        [Mnemonic_cwd]     = "cwd",
        [Mnemonic_ret]     = "ret",
        [Mnemonic_retf]    = "retf",
        [Mnemonic_call]    = "call",
        [Mnemonic_push]    = "push",
        [Mnemonic_pop]     = "pop",
        [Mnemonic_popf]    = "popf",
        [Mnemonic_lahf]    = "lahf",
        [Mnemonic_sahf]    = "sahf",
        [Mnemonic_pushf]   = "pushf",
        [Mnemonic_xchg]    = "xchg",
        [Mnemonic_nop]     = "nop",
        [Mnemonic_xlat]    = "xlat",
        [Mnemonic_in]      = "in",
        [Mnemonic_out]     = "out",
        [Mnemonic_lea]     = "lea",
        [Mnemonic_lds]     = "lds",
        [Mnemonic_les]     = "les",
        [Mnemonic_jmp]     = "jmp",
        [Mnemonic_jz]      = "jz",
        [Mnemonic_jnz]     = "jnz",
        [Mnemonic_jl]      = "jl",
        [Mnemonic_jle]     = "jle",
        [Mnemonic_jb]      = "jb",
        [Mnemonic_jbe]     = "jbe",
        [Mnemonic_jp]      = "jp",
        [Mnemonic_jo]      = "jo",
        [Mnemonic_js]      = "js",
        [Mnemonic_jnl]     = "jnl",
        [Mnemonic_jg]      = "jg",
        [Mnemonic_jnb]     = "jnb",
        [Mnemonic_ja]      = "ja",
        [Mnemonic_jnp]     = "jnp",
        [Mnemonic_jno]     = "jno",
        [Mnemonic_jns]     = "jns",
        [Mnemonic_loop]    = "loop",
        [Mnemonic_loopz]   = "loopz",
        [Mnemonic_loopnz]  = "loopnz",
        [Mnemonic_jcxz]    = "jcxz",

        [Mnemonic_rol]     = "rol",
        [Mnemonic_ror]     = "ror",
        [Mnemonic_rcl]     = "rcl",
        [Mnemonic_rcr]     = "rcr",
        [Mnemonic_shl]     = "shl",
        [Mnemonic_shr]     = "shr",
        [Mnemonic_sar]     = "sar",
        [Mnemonic_not]     = "not",
        [Mnemonic_neg]     = "neg",
        [Mnemonic_mul]     = "mul",
        [Mnemonic_imul]    = "imul",
        [Mnemonic_div]     = "div",
        [Mnemonic_idiv]    = "idiv",

        // DB (define-byte) pseudo-instruction
        [Mnemonic_db]      = "db",

        // Prefixes
        [Mnemonic_repz]    = "repz",
        [Mnemonic_repnz]   = "repnz",
        [Mnemonic_lock]    = "lock",
        [Mnemonic_cs]      = "cs", // segment register
        [Mnemonic_ds]      = "ds", // segment register
        [Mnemonic_es]      = "es", // segment register
        [Mnemonic_ss]      = "ss", // segment register

        [Mnemonic_invalid] = "???",

        [Mnemonic_grp1]    = "grp1",
        [Mnemonic_grp2]    = "grp2",
        [Mnemonic_grp3a]   = "grp3a",
        [Mnemonic_grp3b]   = "grp3b",
        [Mnemonic_grp4]    = "grp4",
        [Mnemonic_grp5]    = "grp5"
    };

    if (mnemonic_name_lookup[m] == NULL) {
        printf("[WARNING]: mnemonic not handled: %d\n", m);
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

    if (instruction->flags & Inst_Repz) {
        fprintf(dest, "repz ");
    }
    if (instruction->flags & Inst_Repnz) {
        fprintf(dest, "repnz ");
    }

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
