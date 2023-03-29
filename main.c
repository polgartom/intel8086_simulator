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
const char* const mode[4] = {
    "Memory mode",  // Memory mode, no displacement follows
    "8bit memory mode",  // Memory mode 8 bit
    "16bit memory mode",  // Memory mode 16 bit
    "Register to register",  // Register to (no displacement) 
};

void decode_next_instruction(char *buf, char *out_buf, size_t *out_buf_i) 
{
    u8 r_m = buf[1] & 0x07;
    u8 reg = (buf[1] >> 3) & 0x07;
    u8 mod = (buf[1] >> 6) & 0x03;

    printf("buf[0]: %d; buf[1]: %d\n", buf[0], buf[1]);
    printf("mode: %s (%d) ; reg: %d ; r_m: %d\n", mode[mod], mod, reg, r_m);

    u8 is16bit = ((u8)buf[0]) & 1;
    u8 opcode = 0;
    const char *opcode_name = NULL;

    // TODO: We have to check the opcode first and decide the mod after. See at: page 164.

    if (mod == 0x03) { // register-to-register
        u8 reg_dir = ((u8)buf[0] >> 1) & 1;
        u8 left = r_m;
        u8 right = reg;
        if (reg_dir == 1) {
            left = r_m;
            right = reg;
        }

        opcode = (buf[0] >> 2) & 0x3F;
        switch(opcode) { 
            case 0x22: {opcode_name = "mov";  break;}
            case 0xFF: {opcode_name = "push"; break;}
            case 0x8F: {opcode_name = "pop";  break;}
            default: {
                printf("[WARNING]: INSTRUCTION (%d) IS PROBABLY NOT HANDLED YET!\n", opcode);
                assert(0);
            }
        }
        printf("opcode: %s (%d)\n", opcode_name, opcode);

        printf("%s %s, %s\n", opcode_name, registers[is16bit][left], registers[is16bit][right]);
        *out_buf_i += sprintf(out_buf+(*out_buf_i), "%s %s, %s\n", opcode_name, registers[is16bit][left], registers[is16bit][right]);
    }
    // TODO: We have to check the opcode first and decide the mod after. See at: page 164.
    else if (mod == 0x00) {
        opcode = (buf[0] >> 4) & 0x0F;
        switch(opcode) {
            case 0x0B: {opcode_name = "mov"; break;}
            default: {
                printf("[WARNING]: INSTRUCTION (%d) IS PROBABLY NOT HANDLED YET!\n", opcode);
                assert(0);
            }
        }
        printf("opcode: %s (%d)\n", opcode_name, opcode);
        
        is16bit = (buf[0] >> 3) & 0x01;
        reg = (buf[0]) & 0x07;

        s16 data = buf[1] & 0xFF;
        if (is16bit) {
            data = ((data << 8) & 0xFF00) | buf[2];
        }

        printf("is16bit (wide): %d ; reg: %s (%d)\n", is16bit, registers[is16bit][reg], reg);

        printf("%s %s, %d\n", opcode_name, registers[is16bit][reg], data);
        *out_buf_i += sprintf(out_buf+(*out_buf_i), "%s %s, %d\n", opcode_name, registers[is16bit][reg], data);
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

    // TODO: Must to read all of the bytes first, because we don't know at this point
    //  that if it is will be a 16 bit (wide) or not.

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
