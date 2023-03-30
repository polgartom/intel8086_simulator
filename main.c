#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ZERO_MEMORY(dest, len) memset(((char*)dest), 0, (len))
#define STR_EQUAL(str1, str2) (strcmp(str1, str2) == 0)
#define STR_LEN(x) (x != NULL ? strlen(x) : 0)
#define LOG(f_, ...) printf(f_ "\n", ##__VA_ARGS__)

// "Intel convention, if the displacement is two bytes, the most-significant 
// byte is stored second in the instruction."
#define INTEL_16BIT_LOHI_TO_HILO(LO, HI) ((HI << 8) & 0xFF00) | (LO & 0x00FF)

typedef char  s8;
typedef short s16;
typedef int   s32;

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

// Registers (REG) lookup table
const char* const registers[2][8] = {
    // W=0 (8bit)
    { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" },

    // W=1 (16bit)
    { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" }
};

// Mode (MOD) lookup table
const char* const mode[4] = {
    "Memory mode",  // Memory mode, no displacement follows
    "8bit memory mode",  // Memory mode 8 bit
    "16bit memory mode",  // Memory mode 16 bit
    "Register to register",  // Register to (no displacement) 
};

int main(int argc, char **argv)
{
    printf("%s\n", argv[1]);

    char out_buf[2048] = {0}; // @Todo: string builder
    size_t out_buf_i = 0;
    ZERO_MEMORY(out_buf, 2048);
        
    char *buf = NULL;
    size_t fsize = read_entire_file(argv[1], &buf);

    size_t i = 0;
    do {
        // Opcode lookup
        u8 opcode = buf[i];
        const char *opcode_name = NULL;
        
        // printf("byte1: %d; byte2: %d\n", buf[0], buf[1]);

        if (((buf[i] >> 4) & 0x0F) == 0x0B) { // mov (imediate to register)
            
            // 1. byte
            opcode = 0x0B;
            opcode_name = "mov";
            u8 reg = buf[i] & 0x07;
            u8 is_16bit = (buf[i] >> 3) & 0x01;
            
            // 2. byte or if is_16bit, then 2-3. byte
            s16 data = buf[i+1];
            if (is_16bit) {
                data = INTEL_16BIT_LOHI_TO_HILO(buf[i+1], buf[i+2]);
                i++;
            }

            out_buf_i += sprintf(&out_buf[out_buf_i], "%s %s, %d\n", opcode_name, registers[is_16bit][reg], data);
            
            i+=2;
        }
        else if ((buf[i] >> 2) & 0x3F) { // mov (register/memory to/from register)
        
            // 1. byte
            opcode = 0x22;
            opcode_name = "mov";
            u8 reg_dir = (buf[i] >> 1) & 0x01;
            u8 is_16bit = buf[i] & 1;

            // 2. byte
            u8 mod = (buf[i+1] >> 6) & 0x03;
            u8 reg = (buf[i+1] >> 3) & 0x07;
            u8 r_m = buf[i+1] & 0x07;

            // decide register direction
            
            printf("opcode: %d; mode: %s (%d) ; reg: %d ; r_m: %d ; reg_dir: %d\n", opcode, mode[mod], mod, reg, r_m, reg_dir);
            
            const char *effective_address_calc = NULL;

            // Memory mode, no displacement follows, except when R/M = 110, then 16 bit displacement follows
            if (mod == 0x00) {
                // @Redundant
                switch (r_m) {
                    case 0x00: {effective_address_calc = "[bx + si]"; break;}
                    case 0x01: {effective_address_calc = "[bx + di]"; break;}
                    case 0x02: {effective_address_calc = "[bp + si]"; break;}
                    case 0x03: {effective_address_calc = "[bp + di]"; break;}
                    case 0x04: {effective_address_calc = "[si]"; break;}
                    case 0x05: {effective_address_calc = "[di]"; break;}
                    case 0x06: {
                        printf("!!!DIRECT ADDRESS NOT IMPLEMENTED YET!!!\n");
                        assert(0); 
                        // @Todo: 16-bit displacement follows
                        break;
                    }
                    case 0x07: {effective_address_calc = "[bx]"; break;}
                    default: assert(0);
                }
                

                if (reg_dir == 1) {
                    out_buf_i += sprintf(&out_buf[out_buf_i], "%s %s, %s\n", opcode_name, registers[is_16bit][reg], effective_address_calc);
                } else {
                    out_buf_i += sprintf(&out_buf[out_buf_i], "%s %s, %s\n", opcode_name, effective_address_calc, registers[is_16bit][reg]);
                }
            }
            // Memory mode, 8-bit displacement follows
            else if (mod == 0x01) {
                // @Redundant
                switch (r_m) {
                    case 0x00: {effective_address_calc = "[bx + si +"; break;}
                    case 0x01: {effective_address_calc = "[bx + di +"; break;}
                    case 0x02: {effective_address_calc = "[bp + si +"; break;}
                    case 0x03: {effective_address_calc = "[bp + di +"; break;}
                    case 0x04: {effective_address_calc = "[si +"; break;}
                    case 0x05: {effective_address_calc = "[di +"; break;}
                    case 0x06: {effective_address_calc = "[bp +"; break;}
                    case 0x07: {effective_address_calc = "[bx +"; break;}
                    default: assert(0);
                }
                
                s8 displacement_8_bit = buf[i+2];
                i++;
                    
                if (reg_dir == 1) {
                    out_buf_i += sprintf(&out_buf[out_buf_i], "%s %s, %s %d]\n", opcode_name, registers[is_16bit][reg], effective_address_calc, displacement_8_bit);
                } else {
                    out_buf_i += sprintf(&out_buf[out_buf_i], "%s %s %d], %s\n", opcode_name, effective_address_calc, displacement_8_bit, registers[is_16bit][reg]);
                }
            }
            // Memory mode, 16-bit displacement follows
            else if (mod == 0x02) {
                // @Redundant
                switch (r_m) {
                    case 0x00: {effective_address_calc = "[bx + si +"; break;}
                    case 0x01: {effective_address_calc = "[bx + di +"; break;}
                    case 0x02: {effective_address_calc = "[bp + si +"; break;}
                    case 0x03: {effective_address_calc = "[bp + di +"; break;}
                    case 0x04: {effective_address_calc = "[si +"; break;}
                    case 0x05: {effective_address_calc = "[di +"; break;}
                    case 0x06: {effective_address_calc = "[bp +"; break;}
                    case 0x07: {effective_address_calc = "[bx +"; break;}
                    default: assert(0);
                }

                s16 displacement_16_bit = INTEL_16BIT_LOHI_TO_HILO(buf[i+2], buf[i+3]);
                i+=2;
                
                if (reg_dir == 1) {
                    out_buf_i += sprintf(&out_buf[out_buf_i], "%s %s, %s %d]\n", opcode_name, registers[is_16bit][reg], effective_address_calc, displacement_16_bit);
                } else {
                    out_buf_i += sprintf(&out_buf[out_buf_i], "%s %s %d], %s\n", opcode_name, effective_address_calc, displacement_16_bit, registers[is_16bit][reg]);
                }
            }
            // Register mode, no displacement follows
            else if (mod == 0x03) {
                u8 left = r_m;
                u8 right = reg;
                if (reg_dir == 1) {
                    left = r_m;
                    right = reg;
                }
                out_buf_i += sprintf(&out_buf[out_buf_i], "%s %s, %s\n", opcode_name, registers[is_16bit][left], registers[is_16bit][right]);
            }

            i+=2;
        } 

    } while(i != fsize);

    printf("\n\n--------------\n\n[OUTPUT]: \n\n%s\n\n", out_buf);

//    FILE *fp = fopen("./output.asm", "wb");
//    fwrite(out_buf, strlen(out_buf), 1, fp);
//    fclose(fp);

    return 0;
}
