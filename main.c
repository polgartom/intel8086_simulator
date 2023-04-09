#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#define ZERO_MEMORY(dest, len) memset(((char*)dest), 0, (len))
#define STR_EQUAL(str1, str2) (strcmp(str1, str2) == 0)
#define STR_LEN(x) (x != NULL ? strlen(x) : 0)
#define LOG(f_, ...) printf(f_ "\n", ##__VA_ARGS__)

// "Intel convention, if the displacement is two bytes, the most-significant 
// byte is stored second in the instruction."
#define BYTE_SWAP_16BIT(LO, HI) ((LO & 0x00FF) | ((HI << 8) & 0xFF00))

typedef char  s8;
typedef short s16;
typedef int   s32;
typedef long  s64;

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

size_t read_entire_file(char *filename, char **buf)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("[ERROR]: Failed to open %s file. Probably it is not exists.\n", filename);
        assert(0);
    }

    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    rewind(fp);

    *buf = (char *)malloc(fsize);
    ZERO_MEMORY(*buf, fsize+1);

    fread(*buf, fsize, 1, fp);
    fclose(fp);

    return fsize;
}

// ----------------------------------------------

typedef struct {
    char *buffer;
    char *start_ptr;
    char *end_ptr;
    size_t size;
} Buffer;

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
        const char *reg;
        Effective_Address_Expression address;
        u16 immediate_u16;
        s16 immediate_s16;
    };

} Instruction_Operand;

#define FLAG_IS_16BIT  0b00000001
#define FLAG_REG_DIR   0b00000010
#define FLAG_IS_SIGNED 0b00000100

typedef struct {
    u16 byte_offset;
    u8  size;
    const char *opcode;

    u32 flags;

    Instruction_Operand operands[2];

} Instruction;

typedef struct {
    Buffer *binary;
    
    Instruction *current_instruction;
    Instruction instructions[1024]; // @Todo: Alloc this dinamically
    u16 number_of_instructions;
} ASM_Decoder;


#define ASMD_CURR_BYTE(_d) *(_d->binary->buffer)
u8 ASMD_NEXT_BYTE(ASM_Decoder *_d) { return *(++_d->binary->buffer); }
#define ASMD_NEXT_BYTE_WITHOUT_STEP(_d) *(_d->binary->buffer+1)
#define ASMD_CURR_BYTE_INDEX(_d) ((_d->binary->buffer) - (_d->binary->start_ptr))

#define REG_IS_DEST 1
#define REG_IS_SRC  0

// Registers (REG) lookup table
const char* const registers[2][8] = {
    // W=0 (8bit)
    { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" },

    // W=1 (16bit)
    { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" }
};

// Register indexes for the lookup table
#define REG_INDEX_ACCUMULATOR 0

// Mode (MOD) lookup table
const char* const mode[4] = {
    "Memory mode",  // Memory mode, no displacement follows
    "8bit memory mode",  // Memory mode 8 bit
    "16bit memory mode",  // Memory mode 16 bit
    "Register to register",  // Register to (no displacement) 
};

#define ARITHMETIC_OPCODE_LOOKUP(__byte, __opcode) \
     switch ((__byte >> 3) & 7) { \
        case 0: { __opcode = "add"; break; } \
        case 5: { __opcode = "sub"; break; } \
        case 7: { __opcode = "cmp"; break; } \
        default: { \
            printf("[WARNING]: This arithmetic instruction is not handled yet!\n"); \
            goto _debug_parse_end; \
        } \
    } \

Effective_Address_Base get_address_base(u8 r_m, u8 mod) 
{
    switch (r_m) {
        case 0x00: {return Effective_Address_bx_si;}
        case 0x01: {return Effective_Address_bx_di;}
        case 0x02: {return Effective_Address_bp_si;}
        case 0x03: {return Effective_Address_bp_di;}
        case 0x04: {return Effective_Address_si;   }
        case 0x05: {return Effective_Address_di;   }
        case 0x06: {
            if (mod == 0x00) {
                return Effective_Address_direct; 
            }
            return Effective_Address_bp;
        }
        case 0x07: {return Effective_Address_bx;}
        default: {
            fprintf(stderr, "[ERROR]: Invalid effective address expression!\n");
            assert(0);
        } 
    }
}

void register_memory_to_from_decode(ASM_Decoder *d, u8 reg_dir, u8 is_16bit)
{
    u8 byte = ASMD_NEXT_BYTE(d);
    Instruction *instruction = d->current_instruction;

    u8 mod = (byte >> 6) & 0x03;
    u8 reg = (byte >> 3) & 0x07;
    u8 r_m = byte & 0x07;
    
    const char *src = NULL;
    const char *dest = NULL;

    if (mod == 0x03) { 
        dest = registers[is_16bit][r_m];
        src = registers[is_16bit][reg];
        if (reg_dir == REG_IS_DEST) {
            dest = registers[is_16bit][reg];
            src = registers[is_16bit][r_m];
        }

        instruction->operands[0].type = Operand_Register;
        instruction->operands[1].type = Operand_Register;
        instruction->operands[0].reg = dest;
        instruction->operands[1].reg = src;

        return;
    }

    s16 displacement = 0;
    if ((mod == 0x00 && r_m == 0x06) || mod == 0x02) {
        displacement = BYTE_SWAP_16BIT(ASMD_NEXT_BYTE(d), ASMD_NEXT_BYTE(d));
    }
    else if (mod == 0x01) { 
        displacement = ASMD_NEXT_BYTE(d);
    }
   
    Instruction_Operand a_operand;
    Instruction_Operand b_operand;

    a_operand.type = Operand_Memory;
    a_operand.address.base = get_address_base(r_m, mod);
    a_operand.address.displacement = displacement;

    b_operand.type = Operand_Register;
    b_operand.reg = registers[is_16bit][reg];

    if (reg_dir == REG_IS_DEST) {
        instruction->operands[0] = b_operand;
        instruction->operands[1] = a_operand;
    } else {
        instruction->operands[0] = a_operand;
        instruction->operands[1] = b_operand;
    }
}

/*
void build_instuction_dest_data(ASM_Decoder *d, const char *opcode, char *dest, u8 is_signed, u8 is_16bit)
{
    // @Todo: if we know that the register is cl then the data type not necessary to specified, because cl is going to be
    //  byte and cx is going to be word and so on...
    s16 data = ASMD_NEXT_BYTE(d);
    const char *data_type = "byte";
    if (is_16bit && !is_signed) {
        data = BYTE_SWAP_16BIT(data, ASMD_NEXT_BYTE(d));
        data_type = "word";
    }
    
    sprintf(d->current_instruction->buf, "%s %s, %s %d\n", opcode, dest, data_type, data);
}
*/

void add_immediate_to_operand(ASM_Decoder *d, Instruction_Operand *operand, u8 is_signed, u8 is_16bit)
{
    operand->type = Operand_Immediate;
    if (is_16bit && !is_signed) {
        operand->immediate_u16 = BYTE_SWAP_16BIT(ASMD_NEXT_BYTE(d), ASMD_NEXT_BYTE(d));
    } else {
        operand->immediate_s16 = ASMD_NEXT_BYTE(d);
    }
}

void immediate_to_register_memory_decode(ASM_Decoder *d, u8 is_signed, u8 is_16bit)
{
    char byte = ASMD_NEXT_BYTE(d);
    Instruction *instruction = d->current_instruction;

    u8 mod = (byte >> 6) & 0x03;
    u8 r_m = byte & 0x07;

    if (mod == 0x03) {
        instruction->operands[0].type = Operand_Register;
        instruction->operands[0].reg  = (char*)registers[is_16bit][r_m];
    } 
    else {
        s16 displacement = 0;

        if (mod == 0x00) { 
            if (r_m == 0x06) {
                displacement = BYTE_SWAP_16BIT(ASMD_NEXT_BYTE(d), ASMD_NEXT_BYTE(d));
            }
        } 
        else if (mod == 0x02) {
            displacement = BYTE_SWAP_16BIT(ASMD_NEXT_BYTE(d), ASMD_NEXT_BYTE(d));
        }
        else if (mod == 0x01) { 
            fprintf(stderr, "[ERROR]: NOT HANDLED 8 bit displacement in immediate to register/memory!\n");
            assert(0);
            displacement = ASMD_NEXT_BYTE(d);
        }

        instruction->operands[0].type = Operand_Memory;
        instruction->operands[0].address.base = get_address_base(r_m, mod);
        instruction->operands[0].address.displacement = displacement;
    }

    add_immediate_to_operand(d, &instruction->operands[1], is_signed, is_16bit);
}

// -------------------------------------------

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "No binary file specified!\n");
    }

    printf("binary filename: %s\n", argv[1]);

    Buffer bin;
    bin.size = read_entire_file(argv[1], &bin.buffer);
    bin.start_ptr = bin.buffer;
    bin.end_ptr = bin.buffer + bin.size; 

    ASM_Decoder decoder = {&bin, .current_instruction = &decoder.instructions[0]};

    do {
        u8 byte = ASMD_CURR_BYTE((&decoder));
        u8 reg_dir = 0;
        u8 is_16bit = 0;

        decoder.current_instruction->byte_offset = ASMD_CURR_BYTE_INDEX((&decoder));
        decoder.number_of_instructions++;

        Instruction *instruction = decoder.current_instruction;
        
        // ARITHMETIC
        if (((byte >> 6) & 7) == 0) {
            ARITHMETIC_OPCODE_LOOKUP(byte, instruction->opcode);

            if (((byte >> 1) & 3) == 2) {
                // Immediate to accumulator                
                is_16bit = byte & 1;
                if (is_16bit) {
                    instruction->flags |= FLAG_IS_16BIT;
                }

                instruction->operands[0].type = Operand_Register;
                instruction->operands[0].reg = registers[is_16bit][REG_INDEX_ACCUMULATOR];

                add_immediate_to_operand(&decoder, &instruction->operands[1], 0, is_16bit);
            }
            else if (((byte >> 2) & 1) == 0) {
                reg_dir = (byte >> 1) & 1;

                is_16bit = byte & 1;
                if (is_16bit) {
                    instruction->flags |= FLAG_IS_16BIT;
                }

                register_memory_to_from_decode(&decoder, reg_dir, is_16bit);
            } else {
                fprintf(stderr, "[ERROR]: Invalid opcode!\n");
                assert(0);
            }
        }
        else if (((byte >> 2) & 63) == 32) {
            ARITHMETIC_OPCODE_LOOKUP(ASMD_NEXT_BYTE_WITHOUT_STEP((&decoder)), instruction->opcode);

            is_16bit = byte & 1;
            if (is_16bit) {
                instruction->flags |= FLAG_IS_16BIT;
            }

            u8 is_signed = (byte >> 1) & 1;
            if (is_signed) {
                instruction->flags |= FLAG_IS_SIGNED;
            }

                       
            immediate_to_register_memory_decode(&decoder, is_signed, is_16bit);
        }
        // MOV
        else if (((byte >> 2) & 63) == 34) {
            instruction->opcode = "mov";
            
            reg_dir = (byte >> 1) & 1;
            is_16bit = byte & 1;
            if (is_16bit) {
                instruction->flags |= FLAG_IS_16BIT;
            }

            register_memory_to_from_decode(&decoder, reg_dir, is_16bit);
        }
        else if (((byte >> 1) & 127) == 99) {
            instruction->opcode = "mov";

            char second_byte = ASMD_NEXT_BYTE_WITHOUT_STEP((&decoder));
            assert(((second_byte >> 3) & 7) == 0);

            is_16bit = byte & 1;
            if (is_16bit) {
                instruction->flags |= FLAG_IS_16BIT;
            }

            immediate_to_register_memory_decode(&decoder, 0, is_16bit);
        }
        else if (((byte >> 4) & 0x0F) == 0x0B) {
            instruction->opcode = "mov";

            u8 reg = byte & 0x07;

            is_16bit = (byte >> 3) & 1;
            if (is_16bit) {
                instruction->flags |= FLAG_IS_16BIT;
            }

            instruction->operands[0].type = Operand_Register;
            instruction->operands[0].reg = registers[is_16bit][reg];

            add_immediate_to_operand(&decoder, &instruction->operands[1], 0, is_16bit);
        }
        else if (((byte >> 2) & 0x3F) == 0x28) {
            instruction->opcode = "mov";

            // Memory to/from accumulator
            is_16bit = byte & 1;
            if (is_16bit) {
                instruction->flags |= FLAG_IS_16BIT;
            }

            decoder.current_instruction->opcode = "mov";

            Instruction_Operand a_operand;
            a_operand.type = Operand_Memory;
            a_operand.address.base = Effective_Address_direct;

            a_operand.address.displacement = ASMD_NEXT_BYTE((&decoder));
            if (is_16bit) {
                a_operand.address.displacement = BYTE_SWAP_16BIT(ASMD_NEXT_BYTE((&decoder)), ASMD_NEXT_BYTE((&decoder)));
            }

            Instruction_Operand b_operand;
            b_operand.type = Operand_Register;
            b_operand.reg = registers[is_16bit][REG_INDEX_ACCUMULATOR];

            reg_dir = (byte >> 1) & 1;
            if (reg_dir == 0) {
                instruction->operands[0] = b_operand;
                instruction->operands[1] = a_operand;
            } else {
                instruction->operands[0] = a_operand;
                instruction->operands[1] = b_operand;
            }
            
        }
        // Return from CALL (jumps)
        else if (((byte >> 4) & 15) == 0x07) {
            s8 ip_inc8 = ASMD_NEXT_BYTE((&decoder)); // 8 bit signed increment to instruction pointer
            const char *opcode = NULL;
            switch (byte & 0x0F) {
                case 0b0000: { opcode = "jo";  break;               }
                case 0b1000: { opcode = "js";  break;               }
                case 0b0010: { opcode = "jb";  break; /* or jnae */ }
                case 0b0100: { opcode = "je";  break; /* or jz */   }
                case 0b0110: { opcode = "jbe"; break; /* or jna */  }
                case 0b1010: { opcode = "jp";  break; /* or jpe */  }
                case 0b0101: { opcode = "jnz"; break; /* or jne */  }
                case 0b1100: { opcode = "jl";  break; /* or jnge */ }
                case 0b1110: { opcode = "jle"; break; /* or jng */  }
                case 0b1101: { opcode = "jnl"; break; /* or jge */  }
                case 0b1111: { opcode = "jg";  break; /* or jnle */ }
                case 0b0011: { opcode = "jae"; break; /* or jnle */ }
                case 0b0111: { opcode = "ja";  break; /* or jnbe */ }
                case 0b1011: { opcode = "jnp"; break; /* or jpo */  }
                case 0b0001: { opcode = "jno"; break;               }
                case 0b1001: { opcode = "jns"; break;               }
                default: {
                    fprintf(stderr, "[ERROR]: This invalid opcode\n");
                    assert(0);
                }
            }

            instruction->opcode = opcode;

            // We have to add +2, because the relative displacement (offset) start from the second byte (ip_inc8)
            //  of instruction. 
            instruction->operands[0].type = Operand_Relative_Immediate;
            instruction->operands[0].immediate_u16 = ip_inc8+2;
            //sprintf(decoder.current_instruction->buf, "%s $%+d\n", opcode, ip_inc8+2);
        }
        else if (((byte >> 4) & 0x0F) == 0b1110) {
            s8 ip_inc8 = ASMD_NEXT_BYTE((&decoder));
            const char *opcode = NULL;
            switch (byte & 0x0F) {
                case 0b0010: { opcode = "loop"; break;                    }
                case 0b0001: { opcode = "loopz"; break;  /* or loope */   }
                case 0b0000: { opcode = "loopnz"; break; /* or loopne */  }
                case 0b0011: { opcode = "jcxz"; break;                    }
                default: {
                    fprintf(stderr, "[ERROR]: This is invalid opcode\n");
                    assert(0);
                }
            }

            instruction->opcode = opcode;

            // We have to add +2, because the relative displacement (offset) start from the second byte (ip_inc8)
            //  of instruction. 
            instruction->operands[0].type = Operand_Relative_Immediate;
            instruction->operands[0].immediate_u16 = ip_inc8+2;
        }
        else {
            fprintf(stderr, "[WARNING]: Instruction is not handled!\n");
            break;
        }

        bin.buffer++;
        decoder.current_instruction++;

    } while(bin.buffer != bin.end_ptr);

_debug_parse_end:

    printf("\n\n----------------\n[OUTPUT]: \n\n");

    fprintf(stdout, "bits 16\n");  
    for (u16 i = 0; i < decoder.number_of_instructions; i++) {
        Instruction *instruction = &decoder.instructions[i];

        fprintf(stdout, "%s", instruction->opcode);
        
        const char *separator = " ";
        for (u8 j = 0; j < 2; j++) {
            Instruction_Operand *op = &instruction->operands[j];
            if (op->type == Operand_None) {
                continue;
            }
            
            fprintf(stdout, "%s", separator);
            separator = ", ";

            switch (op->type) {
                case Operand_None: {
                    break;
                }
                case Operand_Register: {
                    fprintf(stdout, "%s", op->reg);

                    break;
                }
                case Operand_Memory: {
                    if (instruction->operands[0].type != Operand_Register) {
                        fprintf(stdout, "%s ", (instruction->flags & FLAG_IS_16BIT) ? "word" : "byte");
                    }

                    char const *r_m_base[] = {"","bx+si","bx+di","bp+si","bp+di","si","di","bp","bx"};
                    fprintf(stdout, "[%s", r_m_base[op->address.base]);
                    if (op->address.displacement) {
                        fprintf(stdout, "%+d", op->address.displacement);
                    }
                    fprintf(stdout, "]");

                    break;
                }
                case Operand_Immediate: {
                    if ((instruction->flags & FLAG_IS_16BIT) && !(instruction->flags & FLAG_IS_SIGNED)) {
                        fprintf(stdout, "%d", op->immediate_u16);
                    } else {
                        fprintf(stdout, "%d", op->immediate_s16);
                    }

                    break;
                }
                case Operand_Relative_Immediate: {
                    fprintf(stdout, "$%+d", op->immediate_s16);

                    break;
                }
                default: {
                    fprintf(stderr, "[WARNING]: I found a not operand at print out!\n");
                }
            }

        }

        fprintf(stdout, "\n");
    }

    return 0;
}
