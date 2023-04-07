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

typedef struct {
    char *buffer;
    size_t max_size;
    size_t index;
} String_Builder;

void alloc_memory_for_string_builder(String_Builder *builder, size_t max_size)
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

void str_sprintf_and_append(String_Builder *builder, const char *fmt, ...) 
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

#define str_sprintf_and_append_with_space(builder, fmt, ...) \
    str_sprintf_and_append(builder, " "fmt, ##__VA_ARGS__); \

// ----------------------------------------------

typedef struct {
    char *buffer;
    char *start_ptr;
    char *end_ptr;
    size_t size;
} Buffer;

typedef struct {
    Buffer *binary;
    String_Builder *builder;
} ASM_Decoder;

#define ASMD_CURR_BYTE(_d) *(_d->binary->buffer)
// @Performance: I hate this function call but the compiler throw me a warning if I do this with a macro 
s8 ASMD_NEXT_BYTE(ASM_Decoder *_d) { return *(++_d->binary->buffer); }
#define ASMD_NEXT_BYTE_WITHOUT_STEP(_d) *(_d->binary->buffer+1)

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

#define ARITHMETIC_OPCODE_LOOKUP(byte, opcode_name) \
     switch ((byte >> 3) & 7) { \
        case 0: { opcode = "add"; break; } \
        case 5: { opcode = "sub"; break; } \
        case 7: { opcode = "cmp"; break; } \
        default: { \
            printf("[WARNING]: This arithmetic instruction is not handled yet!\n"); \
            goto _debug_parse_end; \
        } \
    } \

char *get_address_calculation(ASM_Decoder *d, u8 r_m, u8 mod, s16 displacement) 
{
    // overkill tmp size
    static char tmp[128];
    ZERO_MEMORY(tmp, 128);

    const char *address = NULL;
    switch (r_m) {
        case 0x00: {address = "bx + si"; break;}
        case 0x01: {address = "bx + di"; break;}
        case 0x02: {address = "bp + si"; break;}
        case 0x03: {address = "bp + di"; break;}
        case 0x04: {address = "si"; break;}
        case 0x05: {address = "di"; break;}
        case 0x06: {
            if (mod == 0x00) {
                // Direct Address
                sprintf(tmp, "[%d]", displacement);
                return tmp;
            }

            address = "bp";  
            break;
        }
        case 0x07: {address = "bx"; break;}
        default: assert(0);
    }

    if (displacement == 0) {
        sprintf(tmp, "[%s]", address);
    } else if (displacement > 0) {
        sprintf(tmp, "[%s + %d]", address, displacement);
    } else {
        sprintf(tmp, "[%s - %d]", address, displacement*-1);
    } 

    return tmp;
}


void register_memory_to_from_decode(ASM_Decoder *d, const char *opcode, u8 reg_dir, u8 is_16bit)
{
    char byte = ASMD_NEXT_BYTE(d);

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

        printf("[%s]: opcode: %s, reg_dir: %d, is16bit: %d ; dest: %s ; src: %s\n", __FUNCTION__, opcode, reg_dir, is_16bit, dest, src);

        str_sprintf_and_append(d->builder, "%s %s, %s\n", opcode, dest, src);
        return;
    }

    s16 displacement = 0;
    if ((mod == 0x00 && r_m == 0x06) || mod == 0x02) {
        displacement = BYTE_SWAP_16BIT(ASMD_NEXT_BYTE(d), ASMD_NEXT_BYTE(d));
    }
    else if (mod == 0x01) { 
        displacement = ASMD_NEXT_BYTE(d);
    }
    
    char *address = get_address_calculation(d, r_m, mod, displacement);

    printf("[%s]: opcode: %s, reg_dir: %d, is16bit: %d ; disp: %d; addr: %s\n", __FUNCTION__, opcode, reg_dir, is_16bit, displacement, address);

    if (reg_dir == REG_IS_DEST) {
        str_sprintf_and_append(d->builder, "%s %s, %s\n", opcode, registers[is_16bit][reg], address);
    } else {
        str_sprintf_and_append(d->builder, "%s %s, %s\n", opcode, address, registers[is_16bit][reg]);
    }
}

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
    
    str_sprintf_and_append(d->builder, "%s %s, %s %d\n", opcode, dest, data_type, data);
}

void immediate_to_register_memory_decode(ASM_Decoder *d, const char *opcode, u8 is_signed, u8 is_16bit)
{
    char byte = ASMD_NEXT_BYTE(d);
    u8 mod = (byte >> 6) & 0x03;
    u8 r_m = byte & 0x07;
    s16 displacement = 0;

    char *dest = 0;

    if (mod == 0x03) {
        dest = (char*)registers[is_16bit][r_m];
    }
    else if (mod == 0x00) { 
        if (r_m == 0x06) {
            displacement = BYTE_SWAP_16BIT(ASMD_NEXT_BYTE(d), ASMD_NEXT_BYTE(d));
        }
        dest = get_address_calculation(d, r_m, mod, displacement);
    } 
    else if (mod == 0x02) {
        displacement = BYTE_SWAP_16BIT(ASMD_NEXT_BYTE(d), ASMD_NEXT_BYTE(d));
        dest = get_address_calculation(d, r_m, mod, displacement);
    }
    else if (mod == 0x01) { 
        printf("NOT HANDLED 8 bit displacement in immediate to register/memory!\n");
        assert(0);
        displacement = ASMD_NEXT_BYTE(d);
        dest = get_address_calculation(d, r_m, mod, displacement);
    }

    printf("[%s]: opcode: %s, is_signed: %d, is16bit: %d ; disp: %d ; dest: %s\n", __FUNCTION__, opcode, is_signed, is_16bit, displacement, dest);

    build_instuction_dest_data(d, opcode, dest, is_signed, is_16bit);
}

// -------------------------------------------

int main(int argc, char **argv)
{
    printf("binary filename: %s\n", argv[1]);

    Buffer bin;
    bin.size = read_entire_file(argv[1], &bin.buffer);
    bin.start_ptr = bin.buffer;
    bin.end_ptr = bin.buffer + bin.size; 

    String_Builder builder = {0};
    alloc_memory_for_string_builder(&builder, 8096);

    ASM_Decoder decoder = {&bin, &builder};

    // @Todo: Can we obtain this "bits 16" stuff from somewhere or do we only know it because we are decoding the 8086/8088?
    str_append(&builder, "bits 16\n\n");  

    u8 reg_dir = 0;
    u8 is_16bit = 0;

    do {
        char byte = ASMD_CURR_BYTE((&decoder));
        
        // ARITHMETIC
        if (((byte >> 6) & 7) == 0) {
            const char *opcode = NULL;
            ARITHMETIC_OPCODE_LOOKUP(byte, opcode);

            if (((byte >> 1) & 3) == 2) {
                // Immediate to accumulator                
                is_16bit = byte & 1;
                char *reg_accumulator = (char *)registers[is_16bit][REG_INDEX_ACCUMULATOR];

                build_instuction_dest_data(&decoder, opcode, reg_accumulator, 0, is_16bit);
            }
            else if (((byte >> 2) & 1) == 0) {
                reg_dir = (byte >> 1) & 1;
                is_16bit = byte & 1;

                register_memory_to_from_decode(&decoder, opcode, reg_dir, is_16bit);
            } else {
                assert(0);
            }
        }
        else if (((byte >> 2) & 63) == 32) {
            is_16bit = byte & 1;
            u8 is_signed = (byte >> 1) & 1;

            const char *opcode = NULL;
            ARITHMETIC_OPCODE_LOOKUP(ASMD_NEXT_BYTE_WITHOUT_STEP((&decoder)), opcode);
           
            immediate_to_register_memory_decode(&decoder, opcode, is_signed, is_16bit);
        }
        // MOV
        else if (((byte >> 2) & 63) == 34) {
            reg_dir = (byte >> 1) & 1;
            is_16bit = byte & 1;

            register_memory_to_from_decode(&decoder, "mov", reg_dir, is_16bit);
        }
        else if (((byte >> 1) & 127) == 99) {
            char second_byte = ASMD_NEXT_BYTE_WITHOUT_STEP((&decoder));
            assert(((second_byte >> 3) & 7) == 0);
            is_16bit = byte & 1;

            immediate_to_register_memory_decode(&decoder, "mov", 0, is_16bit);
        }
        else if (((byte >> 4) & 0x0F) == 0x0B) {
            u8 reg = byte & 0x07;
            is_16bit = (byte >> 3) & 0x01;
            char *dest = (char *)registers[is_16bit][reg];

            build_instuction_dest_data(&decoder, "mov", dest, 0, is_16bit);
        }
        else if (((byte >> 2) & 0x3F) == 0x28) {
            // Memory to/from accumulator
            is_16bit = byte & 1;
            reg_dir = (byte >> 1) & 1;
            char *reg_accumulator = (char *)registers[is_16bit][REG_INDEX_ACCUMULATOR];
            s16 address = ASMD_NEXT_BYTE((&decoder));
            if (is_16bit) {
                address = BYTE_SWAP_16BIT(address, ASMD_NEXT_BYTE((&decoder)));
            }

            if (reg_dir == 0) {
                str_sprintf_and_append(decoder.builder, "%s %s, [%d]\n", "mov", reg_accumulator, address);
            } else {
                str_sprintf_and_append(decoder.builder, "%s [%d], %s\n", "mov", address, reg_accumulator);
            }
        }
        else {
            printf("NOT HANDLED!\n");
            break;
        }

        bin.buffer++;

    } while(bin.buffer != bin.end_ptr);

_debug_parse_end:

    printf("\n\n--------------\n[OUTPUT]: \n\n%s\n--------------\noutput asm size: %ld\n\n", builder.buffer, builder.index+1);

    return 0;
}
