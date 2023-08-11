#include "assembler.h"
#include "new_string.h"

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

u32 eval_numeric_expr()
{
    // @Todo:
    
    Token *t = current_token();
    bool is_signed = t->type == MINUS_OP;
    if (t->type != NUMERIC_LITERAL) eat_token();
    
    return 0;
}

void parse_effective_addr_expr(Operand *operand)
{
    assert(current_token()->type == LEFT_BLOCK_BRACKET);

    operand->type = OPERAND_MEMORY;

    Token *t = eat_and_get_next_token();
    if (t->type == IDENTIFIER) {
    
        if (string_equal_cstr(t->value, "bx")) {
            operand->address.base = EFFECTIVE_ADDR_BX;

            t = eat_and_get_next_token();
            if (string_equal_cstr(t->value, "si")) {
                operand->address.base = EFFECTIVE_ADDR_BX_SI;
            } 
            else if (string_equal_cstr(t->value, "di")) {
                operand->address.base = EFFECTIVE_ADDR_BX_DI;
            } 
            else if (t->type == RIGHT_BLOCK_BRACKET) {
                return;
            }
            else if (t->type == PLUS_OP || t->type == MINUS_OP) {
                u32 displacement = eval_numeric_expr();
            }
            
            ASSERT(0, "Unexpected token -> "SFMT"\n", SARG(t->value));
        }
        else if (string_equal_cstr(t->value, "bp")) {
            operand->address.base = EFFECTIVE_ADDR_BP;
            
            t = eat_and_get_next_token();
            if (string_equal_cstr(t->value, "si")) {
                operand->address.base = EFFECTIVE_ADDR_BP_SI;
            } 
            else if (string_equal_cstr(t->value, "di")) {
                operand->address.base = EFFECTIVE_ADDR_BP_DI;
            } 
            else if (t->type == RIGHT_BLOCK_BRACKET) {
                return;
            }
            else if (t->type == PLUS_OP || t->type == MINUS_OP) {
                u32 displacement = eval_numeric_expr();
            }
            
            ASSERT(0, "Unexpected token -> "SFMT"\n", SARG(t->value));
        }
        else if (string_equal_cstr(t->value, "si")) {
            operand->address.base = EFFECTIVE_ADDR_BP_SI;
            
            t = eat_and_get_next_token();
            if (t->type == RIGHT_BLOCK_BRACKET) return;
        }
        else if (string_equal_cstr(t->value, "di")) {
            operand->address.base = EFFECTIVE_ADDR_BP_DI;
            
            t = eat_and_get_next_token();
            if (t->type == RIGHT_BLOCK_BRACKET) return;
        } 
        else {
            ASSERT(0, "Unexpected token -> "SFMT"\n", SARG(t->value));
        }
    }
    else if (t->type == NUMERIC_LITERAL) {
        // expect direct address
        operand->address.base = EFFECTIVE_ADDR_DIRECT;
    }
}

void mov_inst()
{
    NEW_INST();
    inst->mnemonic = M_MOV; 
    inst->type     = INST_MOVE;

    Token *t = eat_and_get_next_token();

    if (t->type == IDENTIFIER) {
        if (string_equal_cstr(t->value, "byte")) {
            inst->size = W_BYTE;            
        }
        else if (string_equal_cstr(t->value, "word")) {
            inst->size = W_WORD;            
        }
    }

    if (t->type == LEFT_BLOCK_BRACKET) {
        parse_effective_addr_expr(&inst->a);
    }
    else if (t->type == IDENTIFIER) {
        inst->a.reg = decide_register(t->value.data);
    }
    else {
        ASSERT(0, "Unexpected identifier -> "SFMT, SARG(t->value));
    }

    t = eat_and_get_next_token();
    ASSERT(t->type == COMMA, "Expect ',' after first operand");
    
    t = eat_and_get_next_token();
    ASSERT(t->type == LINE_BREAK, "Expect line break after instruction");
    
    ASSERT(inst->size != W_UNDEFINED, "Operation size is not specified!");
}

void parse_tokens()
{
    parser.tokens = lexer.tokens;

    Token *end = lexer.tokens+lexer.ti;
    Token *t = NULL;
    while ((t = current_token()) && t != end) {
        print_token(t);

        if (t->type == IDENTIFIER) {
            if (string_equal_cstr(t->value, "mov")) {
                mov_inst();
            }

            ASSERT(0, "Unexpected identifier -> "SFMT, SARG(t->value));
        }
        else if (t->type == LABEL) {
        }
        else if (t->type == COMMENT) {
        }

        ASSERT(0, "Unexpected token -> "SFMT, SARG(t->value));

        eat_token();
    };
}