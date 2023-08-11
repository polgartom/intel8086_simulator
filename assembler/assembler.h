#ifndef H_ASSEMBLER
#define H_ASSEMBLER

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef char s8;
typedef short s16;
typedef int s32;
typedef long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

#define COLOR_DEFAULT "\033[0m"
#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_PURPLE "\033[0;35m"
#define COLOR_CYAN "\033[0;36m"

#define ZERO_MEMORY(dest, len) memset(((u8 *)dest), 0, (len))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr)[0])
#define CSTR_EQUAL(str1, str2) (strcmp(str1, str2) == 0)
#define CSTR_LEN(x) (x != NULL ? strlen(x) : 0)
#define XSTR(x) #x

#define ASSERT(__cond, __fmt_msg, ...) { \
    if (!(__cond)) { \
        printf(COLOR_RED __fmt_msg COLOR_DEFAULT "\n", ##__VA_ARGS__); \
        assert(__cond); \
    } \
}

typedef enum {
    W_UNDEFINED = 0,
    W_BYTE      = 1,
    W_WORD      = 2,
} Width;

typedef enum {
    M_MOV,
} Mnemonic;

typedef enum {
    REG_NONE,

    REG_AL,
    REG_CL,
    REG_DL,
    REG_BL,

    REG_AH,
    REG_CH,
    REG_DH,
    REG_BH,

    REG_AX, // accumulator
    REG_CX,
    REG_DX,
    REG_BX,

    REG_SP,
    REG_BP,
    REG_SI,
    REG_DI,

    REG_ES, // extra segment
    REG_CS, // code segment
    REG_SS, // stack segment
    REG_DS, // data segment

    REG_IP, // ip register

    REG_COUNT,
} Register;

typedef enum {
    MOD_MEM_MODE       = 0b00,
    MOD_MEM_8BIT_DISP  = 0b01, // DISP: displacement
    MOD_MEM_16BIT_DISP = 0b10,
    MOD_REG            = 0b11,
} MOD;

typedef enum {
  EFFECTIVE_ADDR_DIRECT,
  
  EFFECTIVE_ADDR_BX_SI,
  EFFECTIVE_ADDR_BX_DI,
  EFFECTIVE_ADDR_BP_SI,
  EFFECTIVE_ADDR_BP_DI,
  EFFECTIVE_ADDR_SI,
  EFFECTIVE_ADDR_DI,
  EFFECTIVE_ADDR_BP,
  EFFECTIVE_ADDR_BX,

} Effective_Address_Base;

typedef struct {

  Effective_Address_Base base;

  u16 segment;
  s32 displacement; // used as offset too.

} Effective_Address_Expression;

typedef enum {
    OPERAND_NONE,

    OPERAND_MEMORY,
    OPERAND_REGISTER,
    OPERAND_IMMEDIATE,
    OPERAND_RELATIVE_IMMEDIATE,

    OPERAND_COUNT
} Operand_Type;

typedef struct {
    Operand_Type type;

    union {
        Register reg;
        Effective_Address_Expression address;
        s32 immediate;
    };

} Operand;

typedef enum {
    INST_NONE,

    INST_MOVE,
    INST_ARITHMETIC,
    INST_CONTROL,
    INST_STRING,
    INST_IO,
    INST_FLOW,
    INST_STACK,
    INST_LOGICAL,

    INST_COUNT,
} Instruction_Type;

typedef struct {
    Mnemonic mnemonic;
    Instruction_Type type;
    
    Operand a;
    Operand b;

    Width size;

} Instruction;

Register decide_register(char *s)
{
    if (CSTR_EQUAL(s, "al")) return REG_AL;
    if (CSTR_EQUAL(s, "cl")) return REG_CL;
    if (CSTR_EQUAL(s, "dl")) return REG_DL;
    if (CSTR_EQUAL(s, "bl")) return REG_BL;
    if (CSTR_EQUAL(s, "ah")) return REG_AH;
    if (CSTR_EQUAL(s, "ch")) return REG_CH;
    if (CSTR_EQUAL(s, "dh")) return REG_DH;
    if (CSTR_EQUAL(s, "bh")) return REG_BH;
    if (CSTR_EQUAL(s, "ax")) return REG_AX;
    if (CSTR_EQUAL(s, "cx")) return REG_CX;
    if (CSTR_EQUAL(s, "dx")) return REG_DX;
    if (CSTR_EQUAL(s, "bx")) return REG_BX;
    if (CSTR_EQUAL(s, "sp")) return REG_SP;
    if (CSTR_EQUAL(s, "bp")) return REG_BP;
    if (CSTR_EQUAL(s, "si")) return REG_SI;
    if (CSTR_EQUAL(s, "di")) return REG_DI;
    if (CSTR_EQUAL(s, "es")) return REG_ES;
    if (CSTR_EQUAL(s, "cs")) return REG_CS;
    if (CSTR_EQUAL(s, "ss")) return REG_SS;
    if (CSTR_EQUAL(s, "ds")) return REG_DS;
    if (CSTR_EQUAL(s, "ip")) return REG_IP;

    // @Todo: proper error report
    ASSERT(0, "Unknown register -> %s\n", s);
    return REG_NONE;
}

inline Width register_size(Register r)
{
    // @Cleanup
    switch (r) {
        case REG_AL: return W_BYTE;
        case REG_CL: return W_BYTE;
        case REG_DL: return W_BYTE;
        case REG_BL: return W_BYTE;
        case REG_AH: return W_BYTE;
        case REG_CH: return W_BYTE;
        case REG_DH: return W_BYTE;
        case REG_BH: return W_BYTE;
        case REG_AX: return W_WORD;
        case REG_CX: return W_WORD;
        case REG_DX: return W_WORD;
        case REG_BX: return W_WORD;
        case REG_SP: return W_WORD;
        case REG_BP: return W_WORD;
        case REG_SI: return W_WORD;
        case REG_DI: return W_WORD;
        case REG_ES: return W_WORD;
        case REG_CS: return W_WORD;
        case REG_SS: return W_WORD;
        case REG_DS: return W_WORD;
        case REG_IP: return W_WORD;
    }
    
    return W_UNDEFINED;
}

#endif