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

#define ALLOC_MEMORY(_type) (_type *)malloc(sizeof(_type));

#define NOT_DEFINED -1

// "Intel convention, if the displacement is two bytes, the most-significant 
// byte is stored second in the instruction."
#define BYTE_LOHI_TO_HILO(LO, HI) (((LO & 0x00FF) | ((HI << 8) & 0xFF00)))
#define BYTE_SWAP(__val) (((__val >> 8) & 0x00FF) | ((__val << 8) & 0xFF00))

///////////////////////////////////////////////////

typedef struct {
    u8 data[1024*1024]; // 1MB
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

    Register_count
} Register;

typedef struct {
    Register reg;
    u32 mem_offset;
    u32 mem_size;

} Register_Info;

typedef enum {
    Opcode_none,

    Opcode_mov,
    Opcode_add,
    Opcode_sub,
    Opcode_cmp,

    Opcode_jo,
    Opcode_js, 
    Opcode_jb, 
    Opcode_je, 
    Opcode_jbe,
    Opcode_jp, 
    Opcode_jnz,
    Opcode_jl, 
    Opcode_jle,
    Opcode_jnl,
    Opcode_jg, 
    Opcode_jae,
    Opcode_ja, 
    Opcode_jnp,
    Opcode_jno,
    Opcode_jns,

    Opcode_loop,
    Opcode_loopz,
    Opcode_loopnz,
    Opcode_jcxz,

    Opcode_count

} Opcode;

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

#define FLAG_IS_16BIT  0b00000001
#define FLAG_IS_SIGNED 0b00000100

#define F_SIGNED (1 << 8)
#define F_ZERO   (1 << 7)

typedef struct {
    u32 mem_offset;
    u8  size;

    Opcode opcode;
    
    Instruction_Operand operands[2];

    u32 flags;
} Instruction;

#define MAX_BINARY_FILE_SIZE 1024

typedef struct {
    // @Todo: Merge the loaded_bin and the Memory sturct
    u8 loaded_binary[1024]; // Loaded binary file
    u16 loaded_binary_size;
    u16 byte_offset;

    u8 memory[1024*1024];

    u32 ip_data; // current value of the instruction pointer register 

    Instruction *instruction; // current
    Instruction *instructions[2048];

    u16 flags;
} Context; // @Todo: Rename to CPU or what?

// @CleanUp:
static const u32 registers[2][8][3] = {
    // BYTE (8bit)
    { 
        // enum, offset, size
        {Register_al, 0, 1}, {Register_cl, 2, 1}, {Register_dl, 4, 1}, {Register_bl, 6, 1},
        {Register_ah, 1, 1}, {Register_ch, 3, 1}, {Register_dh, 5, 1}, {Register_bh, 7, 1}
    },
    // WORD (16bit)
    {
        // enum, offset, size
        {Register_ax, 0, 2}, {Register_cx, 2,  2}, {Register_dx, 4,  2}, {Register_bx, 6,  2},
        {Register_sp, 8, 2}, {Register_bp, 10, 2}, {Register_si, 12, 2}, {Register_di, 14, 2}
    }
};

#define REG_ACCUMULATOR 0
Register_Info *get_register_info(u8 is_16bit, u32 reg);

#endif
