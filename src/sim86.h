#ifndef _H_SIM86
#define _H_SIM86 1

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char s8;
typedef short s16;
typedef int s32;
typedef long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

#define ZERO_MEMORY(dest, len) memset(((u8 *)dest), 0, (len))
#define STR_EQUAL(str1, str2) (strcmp(str1, str2) == 0)
#define STR_LEN(x) (x != NULL ? strlen(x) : 0)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr)[0])

#define ALLOC_MEMORY(_type) (_type *)malloc(sizeof(_type));

#define NOT_DEFINED -1

// "Intel convention, if the displacement is two bytes, the most-significant
// byte is stored second in the instruction."
#define BYTE_LOHI_TO_HILO(LO, HI) (((LO & 0x00FF) | ((HI << 8) & 0xFF00)))
#define BYTE_SWAP(__val) (((__val >> 8) & 0x00FF) | ((__val << 8) & 0xFF00))

#define XSTR(x) #x
#define _DEBUG_BREAK(text) { \
    char _dummy_input[12]; \
    printf(#text"\n"); \
    fgets(_dummy_input, sizeof(_dummy_input), stdin); \
}

///////////////////////////////////////////////////

#define MAX_MEMORY (1024 * 1024)

// These are the real place of the
#define F_CARRY      (1 << 0)
#define F_PARITY     (1 << 2)
#define F_AUXILIARY  (1 << 4)
#define F_ZERO       (1 << 6)
#define F_SIGNED     (1 << 7)
#define F_TRAP       (1 << 8)
#define F_INTERRUPT  (1 << 9)
#define F_DIRECTION  (1 << 10)
#define F_OVERFLOW   (1 << 11)

#define REG_IS_DEST 1
#define REG_IS_SRC 0

#define MOD_REGISTER 0x03

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

  Register_es, // extra segment
  Register_cs, // code segment
  Register_ss, // stack segment
  Register_ds, // data segment

  Register_ip, // ip register

  Register_none,
  Register_count
} Register;

typedef struct {
  Register reg;

  // They have to be u32 because we're using that number at register_acces() lookup table
  u32 index; // register memory index in the cpu regmem array
  u32 size;  // register memory size in the cpu regmem array
} Register_Access;

typedef enum {
  Operand_None,

  Operand_Memory,
  Operand_Register,
  Operand_Immediate,
  Operand_Relative_Immediate,

} Operand_Type;

const char *operand_type_to_cstr(Operand_Type type) {
  switch (type) { 
    case Operand_None: return XSTR(Operand_None);
    case Operand_Memory: return XSTR(Operand_Memory);
    case Operand_Register: return XSTR(Operand_Register);
    case Operand_Immediate: return XSTR(Operand_Immediate);
    case Operand_Relative_Immediate: return XSTR(Operand_Relative_Immediate);
  } 

  return "!!Operand_Unknown!!";
}

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

  u16 segment;
  s32 displacement; // used as offset too.

} Effective_Address_Expression;

typedef enum {
  Inst_Wide = (1 << 0),
  Inst_Segment = (1 << 1),
  Inst_Lock = (1 << 2),
  Inst_Repz = (1 << 3),
  Inst_Repnz = (1 << 4),
  Inst_Far = (1 << 5),
  Inst_Sign = (1 << 6)
} Instruction_Flag;

typedef struct {
    Operand_Type type;
    u16 flags;

    union {
        u8 reg;
        Effective_Address_Expression address;
        s32 immediate;
    };

} Instruction_Operand;

typedef enum Instruction_Type Instruction_Type;
enum Instruction_Type {
    Instruction_Type_None,

    Instruction_Type_move,
    Instruction_Type_arithmetic,
    Instruction_Type_control,
    Instruction_Type_string,
    Instruction_Type_io,
    Instruction_Type_flow,
    Instruction_Type_stack,
    Instruction_Type_logical,

    Instruction_Type_Count,
};

#include "i8086table.h"

typedef struct {
  u8 is_prefix;

  u32 mem_address;
  u8 size;

  Mnemonic mnemonic;
  Instruction_Type type;
  Instruction_Flag flags;
  Instruction_Operand operands[2];

  Register extend_with_this_segment;

  u8 mod;
  u8 reg;
  u8 r_m;
  u8 mod_reg_rm_decoded;

  u64 raw;

} Instruction;

typedef struct {
    u32 loaded_executable_size; // @Todo: Remove
    u32 exec_end;
    u32 decoder_cursor;

    Instruction instruction; // current instruction

    u16 ip;
    u16 flags;
    u8 regmem[64]; // The "accessible" register values are stored here

    u8* memory;

    u8 terminate;

    // Options
    u8 dump_out;
    u8 decode_only;
    u8 hide_inst_mem_addr;
    u8 show_raw_bin;
    u8 debug_mode;

    FILE *out; // @Debug

} CPU;

#define REG_ACCUMULATOR 0
Register_Access *register_access(u32 reg, u32 flags);
Register_Access *register_access_by_enum(Register reg);


#endif
