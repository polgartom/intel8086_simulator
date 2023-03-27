#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ZERO_MEMORY(dest, len) memset(((char*)dest), 0, (len))
#define STR_EQUAL(str1, str2) (strcmp(str1, str2) == 0)
#define STR_LEN(x) (x != NULL ? strlen(x) : 0)
#define LOG(f_, ...) printf(f_ "\n", ##__VA_ARGS__)

typedef char  s8;
typedef short s16;
typedef int   s32;

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

size_t open_binary_file(char *filename, FILE **fp)
{
    FILE *_fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("[ERROR]: Failed to open %s file. Probably it is not exists.\n", filename);
        assert(_fp);
    }
    *fp = _fp;

    fseek(*fp, 0, SEEK_END);
    size_t fsize = ftell(*fp);
    rewind(*fp);
    
    return fsize;
}

// Registers (REG) lookup table
const char* const registers[2][8] = {
    // W=0 (8bit)
    { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" },

    // W=1 (dword/16bit)
    { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" }
};

// Mode (MOD) lookup table
u8 mode[4] = {
    0, // Memory mode, no displacement follows
    1, // Memory mode 8 bit
    2, // Memory mode 16 bit
    3  // Register to (no displacement) 
};

void decode_next_instruction(char *buf, char *out_buf, size_t *out_buf_i) 
{
    u8 is16bit = ((u8)buf[0]) & 1;
    u8 reg_dir = ((u8)buf[0] >> 1) & 1;
    printf("is_16bit: %d\nreg_dir: %d\n", is16bit, reg_dir);
    
    u8 instruct = (buf[0] >> 2) & 0x3F;
    const char *instruct_name = NULL;

    u8 r_m = buf[1] & 0x07;
    u8 reg = (buf[1] >> 0x03) & 0x07;
    u8 mod = (buf[1] >> 0x06) & 0x03;

    printf("mode: %d\n", mod);
    
    u8 left = r_m;
    u8 right = reg;
    if (reg_dir == 1) {
        left = r_m;
        right = reg;
    }

    if (mod == 0x00) { // memory mode, no displacement follows
    
    }
    else if (mod == 0x01) { // 8-bit immediate-to-register 
        // @Todo: a lookup table for instructions
        switch(instruct) {
            case 0x33: {instruct_name = "mov"; break;}
            default: {
                printf("[WARNING]: INSTRUCTION (%d) IS PROBABLY NOT HANDLED YET!", instruct);
                assert(0);
            }
        }
 
        printf("%s %s, %d\n", instruct_name, registers[is16bit][left], (s16)right);
        *out_buf_i += sprintf(out_buf+(*out_buf_i), "%s %s, %s\n", instruct_name, registers[is16bit][left], registers[is16bit][right]);
    }
    else if (mod == 0x02) { // 16-bit immediate-to-register

    }
    else if (mod == 0x03) { // register-to-register    
        // @Todo: a lookup table for instructions
        switch(instruct) { 
            case 0x22: {instruct_name = "mov";  break;}
            case 0xFF: {instruct_name = "push"; break;}
            case 0x8F: {instruct_name = "pop";  break;}
            default: {
                printf("[WARNING]: INSTRUCTION (%d) IS PROBABLY NOT HANDLED YET!", instruct);
                assert(0);
            }
        }

        printf("%s %s, %s\n", instruct_name, registers[is16bit][left], registers[is16bit][right]);
        *out_buf_i += sprintf(out_buf+(*out_buf_i), "%s %s, %s\n", instruct_name, registers[is16bit][left], registers[is16bit][right]);
    }

}

int main(int argc, char **argv)
{
    printf("%s\n", argv[1]);

    FILE *fp = NULL;
    open_binary_file(argv[1], &fp);

    char buf[2+1] = {0};
    ZERO_MEMORY(buf, 2+1);

    char out_buf[2048] = {0}; // @Todo: string builder
    size_t out_buf_i = 0;
    ZERO_MEMORY(out_buf, 2048);

    u16 readed_size = 0;
    while ((readed_size = fread(buf, 1, 2, fp))) {
        printf("read %d byte from buffer.\n", readed_size);
        decode_next_instruction(buf, out_buf, &out_buf_i);
        printf("\n\n");
    }

    fclose(fp);

    fp = fopen("./output.asm", "wb");
    fwrite(out_buf, strlen(out_buf), 1, fp);

    return 0;
}
