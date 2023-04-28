#ifndef _H_SIM86
#define _H_SIM86

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

///////////////////////////////////////////////////

#define MAX_MEMORY (1024 * 1024)

#define F_SIGNED (1 << 8)
#define F_ZERO (1 << 7)

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

  Register_cs, // code segment
  Register_ds, // data segment
  Register_ss, // stack segment
  Register_es, // extra segment

  Register_ip, // ip register

  Register_count
} Register;

typedef struct {
  Register reg;
  u32 mem_offset;
  u32 mem_size;
} Register_Access;

typedef enum {
  Opcode_none,

  Opcode_mov,
  Opcode_add,
  Opcode_sub,
  Opcode_cmp,

  Opcode_push,
  Opcode_pop,

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

typedef enum {
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

typedef enum {
  Inst_Lock = (1 << 0),
  Inst_Rep = (1 << 1),
  Inst_Segment = (1 << 2),
  Inst_Wide = (1 << 3),
  Inst_Far = (1 << 4),
  Inst_Sign = (1 << 5)
} Instruction_Flag;

typedef struct {
  Operand_Type type;

  union {
      u8 reg;
      Effective_Address_Expression address;
      s32 immediate;
  };

} Instruction_Operand;

typedef enum {
  Instruction_Type_None,

  Instruction_Type_Move,
  Instruction_Type_Arithmetic,
  Instruction_Type_Jump,

  Instruction_Type_Count,
} Instruction_Type;

typedef struct {
  u32 mem_address;
  u8 size;

  Opcode opcode;
  Instruction_Type type;
  Instruction_Flag flags;
  Instruction_Operand operands[2];

} Instruction;

typedef struct {
  u16 loaded_executable_size; // @Todo: Remove
  u16 decoder_cursor;

  u8 memory[MAX_MEMORY];

  u16 ip;
  u16 cs;
  u16 ds;
  u16 ss;
  u16 es;

  Instruction instruction; // current instruction

  u16 flags;

  // Options
  u8 dump_out;

} CPU;

#define REG_ACCUMULATOR 0
Register_Access *register_access(u32 reg, u8 is_wide);
Register_Access *register_access_by_enum(Register reg);

#endif
