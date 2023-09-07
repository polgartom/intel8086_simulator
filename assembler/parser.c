#include "assembler.h"
#include "new_string.h"

typedef struct {
    int byte_offset; // we have to track this because of jumps

    Array instructions;

    Array tokens;
    u64 ti; // tokens iterator index
    Token *last_token;
} Parser;

Parser parser; // @XXX: Multi-Thread

#define NEW_INST() \
    Instruction *inst = NEW(Instruction); \
    ZERO_MEMORY(inst, sizeof(Instruction)); \
    inst->a.inst = inst; \
    inst->b.inst = inst; \
    array_add(&parser.instructions, inst); \

inline Token *current_token()
{
    if (parser.ti >= parser.tokens.count) return NULL;
    return (Token *)parser.tokens.data[parser.ti];
}

inline void eat_token()
{
    parser.ti += 1;
}

inline Token *eat_and_get_next_token()
{
    eat_token();
    return current_token();
}

inline Token *peak_next_token()
{
    ASSERT(parser.ti+1 < parser.tokens.count, "Next token is not exists!");
    return (Token *)parser.tokens.data[parser.ti+1];
}

Register decide_register(String s)
{
    if (string_equal_cstr(s, "al")) return REG_AL;
    if (string_equal_cstr(s, "cl")) return REG_CL;
    if (string_equal_cstr(s, "dl")) return REG_DL;
    if (string_equal_cstr(s, "bl")) return REG_BL;
    if (string_equal_cstr(s, "ah")) return REG_AH;
    if (string_equal_cstr(s, "ch")) return REG_CH;
    if (string_equal_cstr(s, "dh")) return REG_DH;
    if (string_equal_cstr(s, "bh")) return REG_BH;
    if (string_equal_cstr(s, "ax")) return REG_AX;
    if (string_equal_cstr(s, "cx")) return REG_CX;
    if (string_equal_cstr(s, "dx")) return REG_DX;
    if (string_equal_cstr(s, "bx")) return REG_BX;
    if (string_equal_cstr(s, "sp")) return REG_SP;
    if (string_equal_cstr(s, "bp")) return REG_BP;
    if (string_equal_cstr(s, "si")) return REG_SI;
    if (string_equal_cstr(s, "di")) return REG_DI;
    if (string_equal_cstr(s, "es")) return REG_ES;
    if (string_equal_cstr(s, "cs")) return REG_CS;
    if (string_equal_cstr(s, "ss")) return REG_SS;
    if (string_equal_cstr(s, "ds")) return REG_DS;
    if (string_equal_cstr(s, "ip")) return REG_IP;

    // @Todo: proper error report
    ASSERT(0, "Unknown register -> '"SFMT"'", SARG(s));
    return REG_NONE;
}

int operator_scores[] = {
    [T_PLUS_OP]  = 1,
    [T_MINUS_OP] = 1,
    [T_MULTIPLY_OP] = 2,
    [T_DIVIDE_OP] = 2,
};

int eval_numeric_expr()
{
    // @Incomplete

    // #define VALID_NUM_T(_t) (_t == T_NUMERIC_LITERAL || _t == T_PLUS_OP || _t == T_MINUS_OP || _t == T_MULTIPLY_OP || _t == T_DIVIDE_OP || _t == T_LEFT_ROUND_BRACKET || _t == T_RIGHT_ROUND_BRACKET)

    Token *t = current_token();
    bool is_signed = t->type == T_MINUS_OP;
    if (t->type == T_PLUS_OP || t->type == T_MINUS_OP) {
        t = eat_and_get_next_token();
    }

    ASSERT(t && t->type == T_NUMERIC_LITERAL, "Invalid token!");

    bool failed = false;
    int num = string_atoi(t->value, &failed);
    ASSERT(!failed, "Failed atoi() -> '"SFMT"'", SARG(t->value));

    if (is_signed) {
        ASSERT(!is_signed, "Signed numbers are not implemented yet!");
    }

    return num;
}

void parse_effective_addr_expr(Instruction *inst, Operand *operand)
{
    assert(current_token()->type == T_LEFT_BLOCK_BRACKET);

    operand->type = OPERAND_MEMORY;

    Token *t = eat_and_get_next_token();
    if (t->type == T_IDENTIFIER) {

        if (string_equal_cstr(t->value, "bx")) {
            operand->address.base = EFFECTIVE_ADDR_BX;

            t = eat_and_get_next_token();

            if (t->type == T_PLUS_OP) {
                t = eat_and_get_next_token();

                if (string_equal_cstr(t->value, "si")) {
                    operand->address.base = EFFECTIVE_ADDR_BX_SI;
                }
                else if (string_equal_cstr(t->value, "di")) {
                    operand->address.base = EFFECTIVE_ADDR_BX_DI;
                } 
                else if (t->type == T_NUMERIC_LITERAL) {
                    operand->address.displacement = eval_numeric_expr();
                }
                else {
                    ASSERT(0, "Invalid memory address at -> "SFMT " ; %s", SARG(t->value), TOKSTR(t->type));
                }

                t = eat_and_get_next_token();
                if (t->type == T_PLUS_OP) {
                    t = eat_and_get_next_token();
                    ASSERT(t->type == T_NUMERIC_LITERAL, "Expect number as displacement");

                    operand->address.displacement = eval_numeric_expr();
                    t = eat_and_get_next_token();
                }
            }

        }
        else if (string_equal_cstr(t->value, "bp")) {
            operand->address.base = EFFECTIVE_ADDR_BP;

            t = eat_and_get_next_token();

            if (t->type == T_PLUS_OP) {
                t = eat_and_get_next_token();

                if (string_equal_cstr(t->value, "si")) {
                    operand->address.base = EFFECTIVE_ADDR_BP_SI;
                }
                else if (string_equal_cstr(t->value, "di")) {
                    operand->address.base = EFFECTIVE_ADDR_BP_DI;
                }
                else if (t->type == T_NUMERIC_LITERAL) {
                    operand->address.displacement = eval_numeric_expr();
                }
                else {
                    ASSERT(0, "Invalid memory address at -> "SFMT " ; %s", SARG(t->value), TOKSTR(t->type));
                }

                t = eat_and_get_next_token();
                if (t->type == T_PLUS_OP) {
                    t = eat_and_get_next_token();
                    ASSERT(t->type == T_NUMERIC_LITERAL, "Expect number as displacement");

                    operand->address.displacement = eval_numeric_expr();
                    t = eat_and_get_next_token();
                }
            }
        }
        else if (string_equal_cstr(t->value, "si")) {
            operand->address.base = EFFECTIVE_ADDR_SI;

            t = eat_and_get_next_token();

            if (t->type == T_PLUS_OP) {
                operand->address.displacement = eval_numeric_expr();
                t = eat_and_get_next_token();
            }
        }
        else if (string_equal_cstr(t->value, "di")) {
            operand->address.base = EFFECTIVE_ADDR_DI;

            t = eat_and_get_next_token();

            if (t->type == T_PLUS_OP) {
                operand->address.displacement = eval_numeric_expr();
                t = eat_and_get_next_token();
            }
        }
        else {
            ASSERT(0, "Unexpected token -> "SFMT"\n", SARG(t->value));
        }

        inst->mod = (IS_16BIT(operand->address.displacement) ? MOD_MEM_16BIT_DISP : MOD_MEM_8BIT_DISP);

    }
    else if (t->type == T_NUMERIC_LITERAL) {
        // expect direct address
        operand->address.base = EFFECTIVE_ADDR_DIRECT;
        operand->address.displacement = eval_numeric_expr();
        inst->mod = MOD_MEM;

        t = eat_and_get_next_token();

    } else {
        ASSERT(0, "Unexpected token -> "SFMT" ; type: %s\n", SARG(t->value), TOKSTR(t->type));
    }

    t = current_token();
    Token *pt = (Token *)parser.tokens.data[parser.ti-1];
    ASSERT(t->type == T_RIGHT_BLOCK_BRACKET, "Unexpected token -> "SFMT"\n ; prev: "SFMT " , type: %s", SARG(t->value), SARG(pt->value), TOKSTR(pt->type));
    return;
}

void parse_basic_reg_mem_imm(Instruction *inst)
{
    Token *t = eat_and_get_next_token();

    if (t->type == T_IDENTIFIER) {
        // Specification of operand type
        if (string_equal_cstr(t->value, "byte")) {
            inst->size = W_BYTE;
            t = eat_and_get_next_token();
        }
        else if (string_equal_cstr(t->value, "word")) {
            inst->size = W_WORD;
            t = eat_and_get_next_token();
        }
    }

    if (t->type == T_IDENTIFIER) {
        if (peak_next_token()->type == T_COLON) {
            eat_token();

            inst->segment_reg = decide_register(t->value);
            inst->prefixes |= INST_PREFIX_SEGMENT;

            ASSERT(IS_SEGREG(inst->segment_reg), "Invalid segment override -> '"SFMT"' is invalid segment register!", SARG(t->value));
            t = eat_and_get_next_token(); // expected token -> '['

            ASSERT(t->type == T_LEFT_BLOCK_BRACKET, "An effective address expected after segment register, but '"SFMT"' (%s) was received instead!", SARG(t->value), TOKSTR(t->type));

            parse_effective_addr_expr(inst, &inst->a);

        } else {
            inst->a.reg  = decide_register(t->value);
            inst->a.type = OPERAND_REGISTER;
            inst->a.is_segreg = IS_SEGREG(inst->a.reg);

            inst->size = register_size(inst->a.reg);
        }

    } 
    else if (t->type == T_LEFT_BLOCK_BRACKET) {
        inst->a.type = OPERAND_MEMORY;
        parse_effective_addr_expr(inst, &inst->a);
    }
    else {
        ASSERT(0, "Unexpected identifier -> "SFMT, SARG(t->value));
    }

    ASSERT(eat_and_get_next_token()->type == T_COMMA, "Expect ',' after first operand");
    t = eat_and_get_next_token();

    if (t->type == T_IDENTIFIER) {
        if (peak_next_token()->type == T_COLON) {
            ASSERT(inst->a.type == OPERAND_REGISTER, "Invalid combination of opcode and operands");
            
            eat_token();

            inst->segment_reg = decide_register(t->value);
            inst->prefixes |= INST_PREFIX_SEGMENT;

            ASSERT(IS_SEGREG(inst->segment_reg), "Invalid segment override -> "SFMT" is invalid segment register!", SARG(t->value));
            t = eat_and_get_next_token(); // expected token -> '['

            ASSERT(t->type == T_LEFT_BLOCK_BRACKET, "An effective address expected after segment register, but %s was received instead!", TOKSTR(t->type));

            inst->d = REG_FIELD_IS_DEST;
            parse_effective_addr_expr(inst, &inst->b);
        }
        else {
            inst->b.reg  = decide_register(t->value);
            inst->b.type = OPERAND_REGISTER;
            inst->b.is_segreg = IS_SEGREG(inst->b.reg);

            if (inst->a.type == OPERAND_REGISTER) {
                ASSERT(register_size(inst->a.reg) == register_size(inst->b.reg), "Invalid combination of opcode and operands");
                inst->d = REG_FIELD_IS_DEST;
                inst->mod = MOD_REG;
            }
            else if (inst->a.type == OPERAND_MEMORY) {
                inst->size = register_size(inst->b.reg);
                inst->d = REG_FIELD_IS_SRC;
            }
        }

    }
    else if (t->type == T_LEFT_BLOCK_BRACKET) {
        ASSERT(inst->a.type == OPERAND_REGISTER, "Invalid combination of opcode and operands");

        inst->d = REG_FIELD_IS_DEST;
        parse_effective_addr_expr(inst, &inst->b);
    }
    else if (t->type == T_NUMERIC_LITERAL || t->type == T_PLUS_OP || t->type == T_MINUS_OP) {

        inst->b.type      = OPERAND_IMMEDIATE;
        inst->b.immediate = eval_numeric_expr();
        ASSERT(!(inst->b.immediate & 0xFFFF0000), "The value can't be larger, than 65535");

        inst->d = REG_FIELD_IS_DEST;

        if (inst->a.type == OPERAND_REGISTER) {
            inst->mod = MOD_REG;
            inst->size = register_size(inst->a.reg);
        } 
        else if (inst->a.type = OPERAND_MEMORY)  {
            // inst->size = IS_16BIT(inst->b.immediate) ? W_WORD : W_BYTE;
        }
        else {
            assert(0);
        }
    }
    
    ASSERT(inst->size != W_UNDEFINED, "Operation size is not specified!");
}

void parse_mov()
{
    NEW_INST();
    inst->mnemonic = M_MOV;
    inst->type     = INST_MOVE;

    parse_basic_reg_mem_imm(inst);

    ASSERT(
        !(inst->mod == MOD_REG && IS_SEGREG(inst->a.reg) && IS_SEGREG(inst->b.reg)),
        "Move to segment to segment register is invalid. At first, you need to move the data to a general register"
    );
}

void parse_add()
{
    NEW_INST();
    inst->mnemonic = M_ADD;
    inst->type     = INST_ARITHMETIC;

    parse_basic_reg_mem_imm(inst);
}

void parse_sub()
{
    NEW_INST();
    inst->mnemonic = M_SUB;
    inst->type     = INST_ARITHMETIC;

    parse_basic_reg_mem_imm(inst);
}

void parse_tokens()
{
    parser.instructions = array_create(32);

    parser.tokens = lexer.tokens;
    parser.last_token = (Token *)array_last_item(&lexer.tokens);

    Token *t = NULL;
    while (t = current_token()) {

        if (t->type == T_IDENTIFIER) {
            if (string_equal_cstr(t->value, "mov")) {
                parse_mov();
            } else if (string_equal_cstr(t->value, "add")) {
                parse_add();
            } else if (string_equal_cstr(t->value, "sub")) {
                parse_sub();
            } else {
                ASSERT(0, "Unexpected identifier -> "SFMT, SARG(t->value));
            }
        }
        else if (t->type == T_LABEL) {
        }
        else if (t->type == T_COMMENT) {
        }
        else {
            ASSERT(0, "Unexpected token -> "SFMT " ; %s", SARG(t->value), TOKSTR(t->type));
        }

        t = eat_and_get_next_token();
        ASSERT(!t || t->type == T_LINE_BREAK, "Expect line break after instruction");
        eat_token();
    };

    // for (int i = 0; i < parser.instructions.count; i++) {
    //     Instruction* inst = parser.instructions.data[i];
    // }

    // printf("DONE!\n");
}