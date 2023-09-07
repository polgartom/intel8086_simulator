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

#define ASSERT(__cond, __fmt_msg, ...) { \
    if (!(__cond)) { \
        printf(COLOR_RED __fmt_msg COLOR_DEFAULT "\n", ##__VA_ARGS__); \
        assert(__cond); \
    } \
}

#define ZERO_MEMORY(dest, len) memset(((u8 *)dest), 0, (len))
#define NEW(_type) ((_type *)malloc(sizeof(_type)))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr)[0])
#define CSTR_EQUAL(str1, str2) (strcmp(str1, str2) == 0)
#define CSTR_LEN(x) (x != NULL ? strlen(x) : 0)
#define XSTR(x) #x

#define BYTE_SWAP(__val) (((__val >> 8)) | ((__val << 8)))
#define IS_16BIT(_num) (_num & 0xFF00)

//////////////////

#define REG_FIELD_IS_SRC  0
#define REG_FIELD_IS_DEST 1

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
    MOD_MEM            = 0b00,
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

typedef enum {
    INST_PREFIX_NONE = 0,

    INST_PREFIX_LOCK    = 0b0001,
    INST_PREFIX_SEGMENT = 0b0010,
    INST_PREFIX_REPE    = 0b0100,
    INST_PREFIX_REPNE   = 0b1000,

} Instruction_Prefix;

typedef struct {
    Mnemonic mnemonic;
    Instruction_Type type;

    u16 prefixes;

    Width size;
    bool d; // direction
    MOD  mod;
    u8   rm; 

    Operand a;
    Operand b;

    Register segment_reg;

} Instruction;

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

u8 reg_rm(Register reg)
{
    static u8 rm[] = {
        [REG_AL] = 0b000, [REG_AX] = 0b000,
        [REG_CL] = 0b001, [REG_CX] = 0b001,
        [REG_DL] = 0b010, [REG_DX] = 0b010,
        [REG_BL] = 0b011, [REG_BX] = 0b011,
        [REG_AH] = 0b100, [REG_SP] = 0b100,
        [REG_CH] = 0b101, [REG_BP] = 0b101,
        [REG_DH] = 0b110, [REG_SI] = 0b110,
        [REG_BH] = 0b111, [REG_DI] = 0b111,

        [REG_ES] = 0b0000,  [REG_CS] = 0b0001,
        [REG_SS] = 0b0010,  [REG_DS] = 0b0011,
    };

    return rm[(s32)reg];
}

u8 mem_rm(Effective_Address_Expression address, MOD mod)
{
    assert(mod >= 0 && mod <= 3);
    if (mod != MOD_MEM) assert(address.base != EFFECTIVE_ADDR_DIRECT);

    static u8 rm[3][9] = {
        {
            // Memory mode, no displacement follows
            [EFFECTIVE_ADDR_BX_SI]  = 0b000,
            [EFFECTIVE_ADDR_BX_DI]  = 0b001,
            [EFFECTIVE_ADDR_BP_SI]  = 0b010,
            [EFFECTIVE_ADDR_BP_DI]  = 0b011,
            [EFFECTIVE_ADDR_SI]     = 0b100,
            [EFFECTIVE_ADDR_DI]     = 0b101,
            [EFFECTIVE_ADDR_DIRECT] = 0b110,
            [EFFECTIVE_ADDR_BX]     = 0b111,
            [EFFECTIVE_ADDR_BP]     = 0b000, // @Todo: throw an error
        },
        {
            // Memory mode, 8 bit displacement follows
            [EFFECTIVE_ADDR_BX_SI]  = 0b000,
            [EFFECTIVE_ADDR_BX_DI]  = 0b001,
            [EFFECTIVE_ADDR_BP_SI]  = 0b010,
            [EFFECTIVE_ADDR_BP_DI]  = 0b011,
            [EFFECTIVE_ADDR_SI]     = 0b100,
            [EFFECTIVE_ADDR_DI]     = 0b101,
            [EFFECTIVE_ADDR_BP]     = 0b110,
            [EFFECTIVE_ADDR_BX]     = 0b111,
            [EFFECTIVE_ADDR_DIRECT] = 0b000, // @Todo: throw an error
        },
        {
            // Memory mode, 16 bit displacement follows
            [EFFECTIVE_ADDR_BX_SI]  = 0b000,
            [EFFECTIVE_ADDR_BX_DI]  = 0b001,
            [EFFECTIVE_ADDR_BP_SI]  = 0b010,
            [EFFECTIVE_ADDR_BP_DI]  = 0b011,
            [EFFECTIVE_ADDR_SI]     = 0b100,
            [EFFECTIVE_ADDR_DI]     = 0b101,
            [EFFECTIVE_ADDR_BP]     = 0b110,
            [EFFECTIVE_ADDR_BX]     = 0b111,
            [EFFECTIVE_ADDR_DIRECT] = 0b000, // @Todo: throw an error
        }
    };

    return rm[(s32)mod][(s32)address.base];
}

#define IS_SEGREG(reg) (reg >= REG_ES && reg <= REG_DS)

#endif