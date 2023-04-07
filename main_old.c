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
#define BYTE_SWAP_16BIT(LO, HI) ((HI << 8) & 0xFF00) | (LO & 0x00FF)

typedef char  s8;
typedef short s16;
typedef int   s32;
typedef long  s64;

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef u8 _bool;

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

typedef struct {
    char *buffer;
    size_t max_size;
    size_t index;
} String_Builder;

void create_string_builder(String_Builder *builder, size_t max_size)
{
    builder->buffer = (char *)malloc(max_size+1);
    builder->max_size = max_size;
    builder->index = 0;

    ZERO_MEMORY(builder->buffer, max_size+1);
}

void str_append(String_Builder *builder, char *str)
{
    size_t size = strlen(str);
    assert(builder->index + size<= builder->max_size);

    memcpy(builder->buffer+builder->index, str, size);
    builder->index += size;
    builder->buffer[builder->index] = '\0';
}

void str_sprintf_and_append(String_Builder *builder, char *fmt, ...) 
{
    s64 size = 0;
    va_list ap;

    // @Bug: \n \r \t characters not counted somehow, but if we do let's say 2 \n\n character then it will
    //  count as 1 \n
    // Determine required size
    va_start(ap, fmt);
    size = vsnprintf(NULL, size, fmt, ap);
    va_end(ap);

    assert(size > -1 && builder->index + size <= builder->max_size);

    va_start(ap, fmt);
    size = vsnprintf(builder->buffer+builder->index, size+1, fmt, ap);
    va_end(ap);

    assert(size > -1);

    builder->index += size;
}

// ----------------------------------------------

#define REGISTER_DIRECTION_LEFT  1
#define REGISTER_DIRECTION_RIGHT 0


// Registers (REG) lookup table
const char* const registers[2][8] = {
    // W=0 (8bit)
    { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" },

    // W=1 (16bit)
    { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" }
};

// Register indexes for the lookup table
#define REG_INDEX_ACCUMULATOR 0

//#define REG_BASE_REGISTER     "bx"
//#define REG_COUNT_REGISTER    "cx"
//#define REG_DATA_REGISER      "dx"
//#define REG_STACK_POINTER     "sp"
//#define REG_BASE_POINTER      "bp"
//#define REG_SOURCE_INDEX      "si"
//#define REG_DESTINATION_INDEX "di"

// Mode (MOD) lookup table
const char* const mode[4] = {
    "Memory mode",  // Memory mode, no displacement follows
    "8bit memory mode",  // Memory mode 8 bit
    "16bit memory mode",  // Memory mode 16 bit
    "Register to register",  // Register to (no displacement) 
};

#define ARITHMETIC_OPCODE_LOOKUP(byte, opcode, opcode_name) \
     opcode = (byte >> 3) & 7; \
     switch (opcode) { \
        case 0: { opcode_name = "add"; break; } \
        case 5: { opcode_name = "sub"; break; } \
        case 7: { opcode_name = "cmp"; break; } \
        default: { \
            printf("[WARNING]: This arithmetic instruction is not handled yet!\n"); \
            assert(0); \
        } \
    } \

char *get_address_calc(u8 r_m, u8 mod, s16 displacement) 
{
    // @CleanUp: 
    const char *address = NULL;
    static char address_str[100]; // Cant be multithreaded
    ZERO_MEMORY(address_str, 100);

    // effective address calculation from page 162.
    switch (r_m) {
        case 0x00: {address = "bx + si"; break;}
        case 0x01: {address = "bx + di"; break;}
        case 0x02: {address = "bp + si"; break;}
        case 0x03: {address = "bp + di"; break;}
        case 0x04: {address = "si"; break;}
        case 0x05: {address = "di"; break;}
        case 0x06: {
            if (mod == 0x00) {
                sprintf(address_str, "[%d]", displacement);
                return address_str;
            }
            address = "bp";  
            break;
        }
        case 0x07: {address = "bx"; break;}
        default: assert(0);
    }

    if (displacement == 0) {
        sprintf(address_str, "[%s]", address);
    } else if (displacement > 0) {
        sprintf(address_str, "[%s + %d]", address, displacement);
    } else {
        sprintf(address_str, "[%s - %d]", address, displacement*-1);
    } 

    return address_str;
}

_bool mov_decode(char *buf, size_t *_i, String_Builder *builder)
{
    size_t i = *_i;

    if (((buf[i] >> 4) & 0x0F) == 0x0B) { // mov (imediate to register)
        
        u8 reg = buf[i] & 0x07;
        u8 is_16bit = (buf[i] >> 3) & 0x01;
        
        s16 data = buf[i+1];
        if (is_16bit) {
            data = BYTE_SWAP_16BIT(buf[i+1], buf[i+2]);
            i++;
        }

        printf("[imediate to register]: opcode: %d ; 16bit: %d ; reg: %s (%d) ; data: %d\n", 0x0B, is_16bit, (char*)registers[reg], reg, data);

        str_sprintf_and_append(builder, "%s %s, %d\n", "mov", registers[is_16bit][reg], data);
         
        i+=2;
    }
    else if (((buf[i] >> 2) & 0x3F) == 0x22) { // mov (register/memory to/from register)
    
        // 1. byte
        u8 reg_dir = (buf[i] >> 1) & 0x01;
        u8 is_16bit = buf[i] & 1;

        // 2. byte
        u8 mod = (buf[i+1] >> 6) & 0x03;
        u8 reg = (buf[i+1] >> 3) & 0x07;
        u8 r_m = buf[i+1] & 0x07;

        printf("[reg/memory to/from register]: buf[i]: %d, opcode: %d; 16bit: %d ; mode: %s (%d) ; reg: %d ; r_m: %d ; reg_dir: %d\n", buf[i], 0x22, is_16bit, mode[mod], mod, reg, r_m, reg_dir);
        
        if (mod == 0x00) { // Memory mode, no displacement follows, except when R/M = 110, then 16 bit displacement follows

            s16 displacement_16_bit = 0;
            if (r_m == 0x06) {
                // Direct address
                displacement_16_bit = BYTE_SWAP_16BIT(buf[i+2], buf[i+3]);
                i+=2;
            }

            char *address = get_address_calc(r_m, mod, displacement_16_bit);

            if (reg_dir == REGISTER_DIRECTION_LEFT) {
                str_sprintf_and_append(builder, "%s %s, %s\n", "mov", registers[is_16bit][reg], address);
            } else {
                str_sprintf_and_append(builder, "%s %s, %s\n", "mov", address, registers[is_16bit][reg]);
            }
        }
        else if (mod == 0x01) { // Memory mode, 8-bit displacement follows
           
            s8 displacement_8_bit = buf[i+2];
            i++;

            char *address = get_address_calc(r_m, mod, displacement_8_bit);

            if (reg_dir == REGISTER_DIRECTION_LEFT) {
                str_sprintf_and_append(builder, "%s %s, %s\n", "mov", registers[is_16bit][reg], address);
            } else {
                str_sprintf_and_append(builder, "%s %s, %s\n", "mov", address, registers[is_16bit][reg]);
            }
        }
        else if (mod == 0x02) { // Memory mode, 16-bit displacement follows
            
            s16 displacement_16_bit = BYTE_SWAP_16BIT(buf[i+2], buf[i+3]);
            i+=2;
            
            char *address = get_address_calc(r_m, mod, displacement_16_bit);

            if (reg_dir == REGISTER_DIRECTION_LEFT) {
                str_sprintf_and_append(builder, "%s %s, %s\n", "mov", registers[is_16bit][reg], address);
            } else {
                str_sprintf_and_append(builder, "%s %s, %s\n", "mov", address, registers[is_16bit][reg]);
            }
        }
        else if (mod == 0x03) { // Register mode, no displacement follows
            u8 left = r_m;
            u8 right = reg;
            if (reg_dir == REGISTER_DIRECTION_LEFT) {
                left = r_m;
                right = reg;
            }

            str_sprintf_and_append(builder, "%s %s, %s\n", "mov", registers[is_16bit][left], registers[is_16bit][right]);
        }

        i+=2;
    }
    else if (((buf[i] >> 1) & 0x7F) == 0x63) { // Immediate to register/memory
      }
    else if (((buf[i] >> 1) & 0x7F) == 0x50) { // Memory to accumulator
        u8 is_16bit = buf[i] & 0x01;
    
        printf("[Memory-to-accumulator]: buf[i]: %d, opcode: %d; 16bit: %d\n", buf[i], 0x50, is_16bit);

        s16 address = buf[i+1];
        if (is_16bit) {
            address = BYTE_SWAP_16BIT(buf[i+1], buf[i+2]);
            i++;
        }

        str_sprintf_and_append(builder, "%s %s, [%d]\n", "mov", registers[is_16bit][REG_INDEX_ACCUMULATOR], address);
        
        i+=2;
    }
    else if (((buf[i] >> 1) & 0x7F) == 0x51) { // Accumulator to memory
        u8 is_16bit = buf[i] & 0x01;
    
        printf("[Accumulator-to-Memory]: buf[i]: %d, opcode: %d; 16bit: %d\n", buf[i], 0x50, is_16bit);

        s16 address = buf[i+1];
        if (is_16bit) {
            address = BYTE_SWAP_16BIT(buf[i+1], buf[i+2]);
            i++;
        }

        str_sprintf_and_append(builder, "%s [%d], %s\n", "mov", address, registers[is_16bit][REG_INDEX_ACCUMULATOR]);
        
        i+=2;
    } else {
        // It is not a "mov" instruction
        return 0;
    }

    *_i = i;

    return 1;
}


void decode_reg_memory_to_from_register(char *buf, size_t *_i, String_Builder *builder, const char *opcode_name)
{
    size_t i = *_i;

    u8 reg_dir = (buf[i] >> 1) & 0x01;
    u8 is_16bit = buf[i] & 1;

    u8 mod = (buf[i+1] >> 6) & 0x03;
    u8 reg = (buf[i+1] >> 3) & 0x07;
    u8 r_m = buf[i+1] & 0x07;

    if (mod == 0x00) { // Memory mode, no displacement follows, except when R/M = 110, then 16 bit displacement follows

        s16 displacement_16_bit = 0;
        if (r_m == 0x06) {
            // Direct address
            displacement_16_bit = BYTE_SWAP_16BIT(buf[i+2], buf[i+3]);
            i+=2;
        }

        char *address = get_address_calc(r_m, mod, displacement_16_bit);

        if (reg_dir == REGISTER_DIRECTION_LEFT) {
            str_sprintf_and_append(builder, "%s %s, %s\n", opcode_name, registers[is_16bit][reg], address);
        } else {
            str_sprintf_and_append(builder, "%s %s, %s\n", opcode_name, address, registers[is_16bit][reg]);
        }
    }
    else if (mod == 0x01) { // Memory mode, 8-bit displacement follows

        s8 displacement_8_bit = buf[i+2];
        i++;

        char *address = get_address_calc(r_m, mod, displacement_8_bit);

        if (reg_dir == REGISTER_DIRECTION_LEFT) {
            str_sprintf_and_append(builder, "%s %s, %s\n", opcode_name, registers[is_16bit][reg], address);
        } else {
            str_sprintf_and_append(builder, "%s %s, %s\n", opcode_name, address, registers[is_16bit][reg]);
        }
    }
    else if (mod == 0x02) { // Memory mode, 16-bit displacement follows

        s16 displacement_16_bit = BYTE_SWAP_16BIT(buf[i+2], buf[i+3]);
        i+=2;

        char *address = get_address_calc(r_m, mod, displacement_16_bit);

        if (reg_dir == REGISTER_DIRECTION_LEFT) {
            str_sprintf_and_append(builder, "%s %s, %s\n", opcode_name, registers[is_16bit][reg], address);
        } else {
            str_sprintf_and_append(builder, "%s %s, %s\n", opcode_name, address, registers[is_16bit][reg]);
        }
    }
    else if (mod == 0x03) { // Register mode, no displacement follows
        u8 left = r_m;
        u8 right = reg;
        if (reg_dir == REGISTER_DIRECTION_LEFT) {
            left = r_m;
            right = reg;
        }
        str_sprintf_and_append(builder, "%s %s, %s\n", opcode_name, registers[is_16bit][left], registers[is_16bit][right]);
    }

    i+=2;

    *_i = i;
}

void decode_mov_immediate_to_register_memory(char *buf, size_t *_i, String_Builder *builder)
{
    size_t i = *_i;

    u8 is_16bit = (buf[i] & 1);

    u8 mod = (buf[i] >> 6) & 0x03;
    assert(((buf[i] >> 3) & 0x07) == 0);
    u8 r_m = buf[i] & 0x07;

    if (mod == 0x00) { // Memory mode, no displacement follows, except when R/M = 110, then 16 bit displacement follows
        s16 displacement_16_bit = 0;
        s16 data = 0;
        const char *data_type = NULL;

        if (r_m == 0x06) {
            displacement_16_bit = BYTE_SWAP_16BIT(buf[i+2], buf[i+3]);

            data = buf[i+4];
            data_type = "byte";
            if (is_16bit) {
                data = BYTE_SWAP_16BIT(buf[i+4], buf[i+5]);
                data_type = "word";
                i++;
            }

            i+=3;
        } 
        else {
            // if no displacement, then the data will be at where displacement has to be
            data = buf[i+2];
            data_type = "byte";
            if (is_16bit) {
                data = BYTE_SWAP_16BIT(buf[i+2], buf[i+3]);
                data_type = "word";
                i++;
            }
            i+=1;
        }

        char *address = get_address_calc(r_m, mod, displacement_16_bit);

        str_sprintf_and_append(builder, "%s %s, %s %d\n", "mov", address, data_type, data);
    } 
    else if (mod == 0x01) { // Memory mode, 8-bit displacement follows
        assert(0);
        s8 displacement_8_bit = buf[i+2];
        i++;

        char *address = get_address_calc(r_m, mod, displacement_8_bit);
    }
    else if (mod == 0x02) { // Memory mode, 16-bit displacement follows
        s16 displacement_16_bit = BYTE_SWAP_16BIT(buf[i+2], buf[i+3]);

        s16 data = buf[i+4];
        const char *data_type = "byte";
        if (is_16bit) {
            data = BYTE_SWAP_16BIT(buf[i+4], buf[i+5]);
            data_type = "word";
            i++;
        }

        i+=3;

        char *address = get_address_calc(r_m, mod, displacement_16_bit);

        str_sprintf_and_append(builder, "%s %s, %s %d\n", "mov", address, data_type, data);
    } 

    i+=2;

    *_i = i;
}

void decode_arithmetic__immediate_to_register_memory(char *buf, size_t *_i, String_Builder *builder)
{
    size_t i = *_i;

    u8 opcode = 0;
    const char *opcode_name = NULL;
    ARITHMETIC_OPCODE_LOOKUP(buf[i], opcode, opcode_name);

    u8 is_16bit  = buf[i] & 1;
    u8 is_signed = (buf[i] >> 1) & 1;

    u8 mod = (buf[i+1] >> 6) & 3;
    u8 r_m = buf[i+1] & 7;

    if (mod == 0x00) { // Memory mode, no displacement follows, except when R/M = 110, then 16 bit displacement follows
        s16 displacement_16_bit = 0;
        s16 data = 0;
        const char *data_type = NULL;

        if (r_m == 0x06) {
            displacement_16_bit = BYTE_SWAP_16BIT(buf[i+2], buf[i+3]);

            data = buf[i+4];
            data_type = "byte";
            if (is_16bit && !is_signed) {
                data = BYTE_SWAP_16BIT(buf[i+4], buf[i+5]);
                data_type = "word";
                i++;
            }

            i+=3;
        } 
        else {
            // if no displacement, then the data will be at where displacement has to be
            data = buf[i+2];
            data_type = "byte";
            if (is_16bit && !is_signed) {
                data = BYTE_SWAP_16BIT(buf[i+2], buf[i+3]);
                data_type = "word";
                i++;
            }
            i+=1;
        }

        char *address = get_address_calc(r_m, mod, displacement_16_bit);

        str_sprintf_and_append(builder, "%s %s, %s %d\n", opcode_name, address, data_type, data);
    } 
    else if (mod == 0x01) { // Memory mode, 8-bit displacement follows
        assert(0);
        s8 displacement_8_bit = buf[i+2];
        i++;

        char *address = get_address_calc(r_m, mod, displacement_8_bit);
    }
    else if (mod == 0x02) { // Memory mode, 16-bit displacement follows
        s16 displacement_16_bit = BYTE_SWAP_16BIT(buf[i+2], buf[i+3]);

        s16 data = buf[i+4];
        const char *data_type = "byte";
        if (is_16bit && !is_signed) {
            data = BYTE_SWAP_16BIT(buf[i+4], buf[i+5]);
            data_type = "word";
            i++;
        }

        i+=3;

        char *address = get_address_calc(r_m, mod, displacement_16_bit);

        str_sprintf_and_append(builder, "%s %s, %s %d\n", opcode_name, address, data_type, data);

    } else if (mod == 0x03) {
        const char *data_type = "byte";

        s16 data = buf[i+2];
        if (is_16bit && !is_signed) {
            data = BYTE_SWAP_16BIT(buf[i+2], buf[i+3]);
            data_type = "word";
            i++;
        }
        i++;

        str_sprintf_and_append(builder, "%s %s, %s %d\n", opcode_name, registers[is_16bit][r_m], data_type, data);
    }

    i+=2;

    *_i = i;
}

void decode_arithmetic__immediate_to_accumulator(char *buf, size_t *_i, String_Builder *builder)
{
    size_t i = *_i;
    
    u8 opcode = 0;
    const char *opcode_name = NULL;
    ARITHMETIC_OPCODE_LOOKUP(buf[i], opcode, opcode_name);

    u8 is_16bit = buf[i] & 1;
    s16 data = buf[i+1];
    if (is_16bit) {
        data = BYTE_SWAP_16BIT(buf[i+1], buf[i+2]);
        i++;
    }

    str_sprintf_and_append(builder, "%s %s, %d\n", opcode_name, registers[is_16bit][REG_INDEX_ACCUMULATOR], data);

    i+=2;

    *_i = i;
}

void decode_arithmetic__reg_memory(char *buf, size_t *_i, String_Builder *builder) 
{
    u8 opcode = 0;
    const char *opcode_name = NULL;
    ARITHMETIC_OPCODE_LOOKUP(buf[*_i], opcode, opcode_name);

    decode_reg_memory_to_from_register(buf, _i, builder, opcode_name);
}

int main(int argc, char **argv)
{
    printf("binary filename: %s\n", argv[1]);

    char *buf = NULL;
    size_t fsize = read_entire_file(argv[1], &buf);

    String_Builder builder = {0};
    create_string_builder(&builder, 8096);

    // @Todo: Can we obtain this "bits 16" stuff from somewhere or do we only know it because we are decoding the 8086/8088?
    str_append(&builder, "bits 16\n\n");  

    size_t i = 0;
    _bool success = 0;
    do {
//        if (mov_decode(buf, &i, &builder)) {
//        }

        if (((buf[i] >> 2) & 63) == 34) {
            decode_reg_memory_to_from_register(buf, &i, &builder, "mov");
            continue;
        }
        else if (((buf[i] >> 1) & 127) == 99) {
            decode_mov_immediate_to_register_memory(buf, &i, &builder);
            continue;
        }

        // Check the arithmetic stuffs
        if ((buf[i] >> 2) == 32) {
            decode_arithmetic__immediate_to_register_memory(buf, &i, &builder);
            continue;
        }
        else if (((buf[i] >> 5) & 7) == 0) {
            if (((buf[i] >> 1) & 3) == 2) {
                decode_arithmetic__immediate_to_accumulator(buf, &i, &builder);
                continue;
            }
            else if (((buf[i] >> 2) & 1) == 0) {
                decode_arithmetic__reg_memory(buf, &i, &builder);
                continue;
            } else {
                assert(0);
            }
        }
        else {
            printf("[WARNING] NOT HANDLED YET! Break...\n");
            printf("buf[i] == %d ; buf[i+1] == %d\n", buf[i], buf[i+1]);
            break;
        } 

    } while(i != fsize);

    printf("\n\n--------------\n[OUTPUT]: \n\n%s\n--------------\noutput asm size: %ld\n\n", builder.buffer, builder.index+1);

//    FILE *fp = fopen("./output.asm", "wb");
//    fwrite(out_buf, strlen(out_buf), 1, fp);
//    fclose(fp);

    return 0;
}
