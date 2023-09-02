#include "assembler.h"
#include "new_string.h"

// typedef struct Ast
// {

// };

// #define NEW_AST() (Ast *)malloc(sizeof(Ast));

typedef struct {
    int byte_offset; // we have to track this because of jumps

    Instruction instructions[4096]; // @Incomplete
    int inst_index;

    Token *tokens;
} Parser;

Parser parser; // @XXX: Multi-Thread

// @Temporary
#define NEW_INST() Instruction *inst = parser.instructions+(parser.inst_index++);

inline Token *current_token()
{
    // @Todo: Dynamic tokens size
    if (lexer.ti == 4096-1) return NULL;
    return parser.tokens;
}

inline void eat_token()
{
    parser.tokens += 1;
}

inline Token *eat_and_get_next_token()
{
    eat_token();
    return current_token();
}

inline Token *peak_next_token()
{
    // @Todo: Proper error message at assertion
    // @Todo: Dynamic tokens size
    if (lexer.ti == 4096-1) return NULL;
    return parser.tokens+1;
}

inline Token *peak_prev_token()
{
    if (parser.tokens == lexer.tokens) return NULL;
    return parser.tokens-1;
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
    ASSERT(0, "Unknown register -> "SFMT, SARG(s));
    return REG_NONE;
}

#define IS_NUM_16BIT(_num) (_num & 0xff00)

int eval_numeric_expr()
{
    // @Incomplete: Precedence of parenthesis

    Token *t = current_token();
    bool is_signed = t->type == T_MINUS_OP;
    if (t->type != T_NUMERIC_LITERAL) eat_token();

    int num = 0;
    while (t && t->type == T_NUMERIC_LITERAL) {
        bool failed = false; 
        int num = string_atoi(t->value, &failed);
        if (failed) ASSERT(0, "Failed to convert '"SFMT"' to numeric value.", SARG(t->value));

        t = eat_and_get_next_token();
    }

    return num;
}

void parse_effective_addr_expr(Instruction *inst, Operand *operand)
{
    #define GUESS_MOD(__num) (IS_NUM_16BIT(__num) ? MOD_MEM_16BIT_DISP : MOD_MEM_8BIT_DISP);

    assert(current_token()->type == T_LEFT_BLOCK_BRACKET);

    operand->type = OPERAND_MEMORY;

    Token *t = eat_and_get_next_token();
    if (t->type == T_IDENTIFIER) {

        if (string_equal_cstr(t->value, "bx")) {
            operand->address.base = EFFECTIVE_ADDR_BX;

            t = eat_and_get_next_token();
            
            if (string_equal_cstr(t->value, "si")) {
                operand->address.base = EFFECTIVE_ADDR_BX_SI;
                t = eat_and_get_next_token();
            }
            else if (string_equal_cstr(t->value, "di")) {
                operand->address.base = EFFECTIVE_ADDR_BX_DI;
                t = eat_and_get_next_token();
            }

            if (t->type == T_PLUS_OP || t->type == T_MINUS_OP) {
                operand->address.displacement = eval_numeric_expr();
            }

        }
        else if (string_equal_cstr(t->value, "bp")) {
            operand->address.base = EFFECTIVE_ADDR_BP;

            t = eat_and_get_next_token();

            if (string_equal_cstr(t->value, "si")) {
                operand->address.base = EFFECTIVE_ADDR_BP_SI;
                t = eat_and_get_next_token();
            }
            else if (string_equal_cstr(t->value, "di")) {
                operand->address.base = EFFECTIVE_ADDR_BP_DI;
                t = eat_and_get_next_token();
            }

            if (t->type == T_PLUS_OP || t->type == T_MINUS_OP) {
                operand->address.displacement = eval_numeric_expr();
            } else {
                ASSERT(operand->address.base != EFFECTIVE_ADDR_BP, "Invalid effective address");
            }
        }
        else if (string_equal_cstr(t->value, "si")) {
            operand->address.base = EFFECTIVE_ADDR_SI;

            t = eat_and_get_next_token();
            if (t->type == T_PLUS_OP || t->type == T_MINUS_OP) {
                eat_token();
                operand->address.displacement = eval_numeric_expr();
            }
        }
        else if (string_equal_cstr(t->value, "di")) {
            operand->address.base = EFFECTIVE_ADDR_BP;

            t = eat_and_get_next_token();
            if (t->type == T_PLUS_OP || t->type == T_MINUS_OP) {
                eat_token();
                operand->address.displacement = eval_numeric_expr();
            }
        }
        else {
            ASSERT(0, "Unexpected token -> "SFMT"\n", SARG(t->value));
        }
    }
    else if (t->type == T_NUMERIC_LITERAL) {
        // expect direct address
        operand->address.base = EFFECTIVE_ADDR_DIRECT;
        operand->address.displacement = eval_numeric_expr();
    }

    t = current_token();
    ASSERT(t->type == T_RIGHT_BLOCK_BRACKET, "Unexpected token -> "SFMT"\n", SARG(t->value));
    return;
}

void mov_inst()
{
    NEW_INST();
    inst->mnemonic = M_MOV;
    inst->type     = INST_MOVE;

    Token *t = eat_and_get_next_token();

    if (t->type == T_IDENTIFIER) {
        // Specification of operand type
        if (string_equal_cstr(t->value, "byte")) {
            inst->size = W_BYTE;
        }
        else if (string_equal_cstr(t->value, "word")) {
            inst->size = W_WORD;
        }
    }

    if (t->type == T_LEFT_BLOCK_BRACKET) {
        parse_effective_addr_expr(inst, &inst->a);
    }
    else if (t->type == T_IDENTIFIER) {
        // @Incomplete: It can be macro?
        inst->a.type = OPERAND_REGISTER;
        inst->a.reg  = decide_register(t->value);
    }
    else {
        ASSERT(0, "Unexpected identifier -> "SFMT, SARG(t->value));
    }

    t = eat_and_get_next_token();
    ASSERT(t->type == T_COMMA, "Expect ',' after first operand");

    if (t->type == T_LEFT_BLOCK_BRACKET) {
        ASSERT(inst->a.type != OPERAND_MEMORY, "Memory to memory is not allowed -> "SFMT, SARG(t->value));

        parse_effective_addr_expr(inst, &inst->b);
    }
    else if (t->type == T_IDENTIFIER) {
        inst->b.type = OPERAND_REGISTER;
        inst->b.reg  = decide_register(t->value);
    }
    else if (t->type == T_NUMERIC_LITERAL || t->type == T_PLUS_OP || t->type == T_MINUS_OP) {
        ASSERT(inst->a.type != OPERAND_MEMORY, "Immediate to memory is not allowed! At first, you must move the immediate to a register -> "SFMT, SARG(t->value));

        inst->b.type      = OPERAND_IMMEDIATE;
        inst->b.immediate = eval_numeric_expr();
        ASSERT(!(inst->b.immediate & 0xFFFF0000), "The value can't be larger, than 65535");
    }
    
    if (inst->a.type == OPERAND_REGISTER) {
        if (inst->b.type == OPERAND_REGISTER || inst->b.type == OPERAND_IMMEDIATE) {
            inst->mod = MOD_REG;
        }
    }

    // @Todo: Set the rm field

    t = eat_and_get_next_token();
    ASSERT(t->type == T_LINE_BREAK, "Expect line break after instruction");

    ASSERT(inst->size != W_UNDEFINED, "Operation size is not specified!");
}

void parse_tokens()
{
    parser.tokens = lexer.tokens;

    Token *end = lexer.tokens+lexer.ti;
    Token *t = NULL;
    while ((t = current_token()) && t != end) {
        print_token(t);

        if (t->type == T_IDENTIFIER) {
            if (string_equal_cstr(t->value, "mov")) {
                mov_inst();
            }

            ASSERT(0, "Unexpected identifier -> "SFMT, SARG(t->value));
        }
        else if (t->type == T_LABEL) {
        }
        else if (t->type == T_COMMENT) {
        }

        ASSERT(0, "Unexpected token -> "SFMT, SARG(t->value));

        eat_token();
    };
}