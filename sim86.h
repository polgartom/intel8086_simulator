#ifndef _H_SIM86
#define _H_SIM86

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

typedef char  s8;
typedef short s16;
typedef int   s32;
typedef long  s64;

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

#define ZERO_MEMORY(dest, len) memset(((u8*)dest), 0, (len))
#define STR_EQUAL(str1, str2) (strcmp(str1, str2) == 0)
#define STR_LEN(x) (x != NULL ? strlen(x) : 0)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr)[0])

#define NOT_DEFINED -1

// "Intel convention, if the displacement is two bytes, the most-significant 
// byte is stored second in the instruction."
#define BYTE_SWAP_16BIT(LO, HI) ((LO & 0x00FF) | ((HI << 8) & 0xFF00))

///////////////////////////////////////////////////

#define MAX_MEMORY_SIZE 0xfffff // 1mb

typedef struct {
    u8 data[1024];
    u32 size;
} Memory;


#define REG_IS_DEST 1
#define REG_IS_SRC  0

typedef enum {

    Register_al,
    Register_cl,
    Register_dl,
    Register_bl,

    Register_ah,
    Register_ch,
    Register_dh,
    Register_bh,

    Register_ax, // accumulator
    Register_cx,
    Register_dx,
    Register_bx,

    Register_sp,
    Register_bp,
    
    Register_si,
    Register_di,

} Register;

typedef struct {
    Register reg;
    u32 mem_offset;
    u32 mem_size;

} Register_Info;

typedef enum  {
    Operand_None,

    Operand_Memory,
    Operand_Register,
    Operand_Immediate,
    Operand_Relative_Immediate,

} Operand_Type;

typedef enum {
    Effective_Address_direct,
    
    Effective_Address_bx_si,
    Effective_Address_bx_di,
    Effective_Address_bp_si,
    Effective_Address_bp_di,
    Effective_Address_si,
    Effective_Address_di,
    Effective_Address_bp,
    Effective_Address_bx,

} Effective_Address_Base;

typedef struct {
    
    Effective_Address_Base base;
    s32 displacement;

} Effective_Address_Expression;

typedef struct {
    Operand_Type type;  

    union {
        u8 reg;
        Effective_Address_Expression address;
        u16 immediate_u16;
        s16 immediate_s16;
    };

} Instruction_Operand;

// @Todo: Remove those!
#define FLAG_IS_16BIT  0b00000001
#define FLAG_IS_SIGNED 0b00000100

#define F_SIGNED (1 << 8)
#define F_ZERO   (1 << 7)

typedef struct {
    u16 byte_offset;
    u8  size;
    const char *opcode;
    
    Instruction_Operand operands[2];

    u32 flags;
} Instruction;

typedef struct {
    Memory memory;
    u32 mem_index;
    Instruction *instruction; // current
    u16 flags;
} Decoder_Context;

///////////////////////////////////////////////////

// Mode (MOD) lookup table
const char* const mode[4] = {
    "Memory mode",  // Memory mode, no displacement follows
    "8bit memory mode",  // Memory mode 8 bit
    "16bit memory mode",  // Memory mode 16 bit
    "Register to register",  // Register to (no displacement) 
};

#endif
