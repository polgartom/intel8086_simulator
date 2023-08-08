#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "new_string.h"

typedef char s8;
typedef short s16;
typedef int s32;
typedef long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

typedef char bool;
#define true  1;
#define false 0;

#define ZERO_MEMORY(dest, len) memset(((u8 *)dest), 0, (len))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr)[0])
#define STR_EQUAL(str1, str2) (strcmp(str1, str2) == 0)
#define STR_LEN(x) (x != NULL ? strlen(x) : 0)
#define XSTR(x) #x

#define IS_ALPHA(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define IS_DIGIT(c) (c >= '0' && c <= '9')
#define IS_ALNUM(c) (IS_ALPHA(c) || IS_DIGIT(c) || c == '_')
#define IS_SPACE(c) (c == ' ' || c == '\t' || c == '\r' || c == '\n')

String read_entire_file(char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("\n[ERROR]: Failed to open %s file. Probably it is not exists.\n", filename);
        assert(0);
    }
    
    fseek(fp, 0, SEEK_END);
    u32 fsize = ftell(fp);
    rewind(fp);
    
    String s = str_make_alloc(fsize);
    
    fread(s.data, fsize, 1, fp);
    fclose(fp);
    
    return s;
}

typedef enum {
    UNKNOWN,
    IDENTIFIER,
    REGISTER,
    DIRECTIVE,
    LABEL,
    COMMENT
} Token_Type;

typedef struct {
    String     value;
    Token_Type type;
} Token;

typedef struct {
    String str;
    int cl;  
    int cr;
    Token tokens[1024]; // @XXX
} Lexer;

Lexer lexer; // @XXX: Multi-thread

void inline eat_next_char()
{
    lexer.cr += 1;  
}

char inline peak_next_char()
{
    assert(lexer.cr+1 <= lexer.str.count);
    return lexer.str.data[lexer.cr+1];
}

char inline current_char()
{
    return *lexer.str.data;
}

void inline keep_up_left_cursor()
{
    lexer.cl = lexer.cr;
}

String inline get_lexer_cursor_range(bool closed_interval)
{
    String s = str_advance(lexer.str, lexer.cl);
    s.count = lexer.cl == lexer.cr ? 1 : (lexer.cr - lexer.cl) + (closed_interval ? 0 : 1);
    return s;
}

Token parse_identifier()
{
    char c;
    while ((c = current_char())) {
        if (IS_ALNUM(c)) {
            eat_next_char();
            continue;
        }
        else if (IS_SPACE(c)) {
            String identifier = get_lexer_cursor_range();
            keep_up_left_cursor();
            
            Token t = {};
            t.value = identifier;
            
            c = peak_next_char();
            if (c == ':') {
                t.type = LABEL;
                eat_next_char();
            }
            
            return t;
        }
        assert(false);
    }
    
    assert(false);
}

void parse_string_literal()
{
    
}

void parse_numeric_literal()
{

}

int main(int argc, char **argv)
{
    lexer.str = read_entire_file("mock/rectangle.asm");

    char c;
    while ((c = current_char())) {
        if (IS_ALPHA(c) || c == '_') {
            // identifier, label
            parse_identifier();
        }
        else if (c == '%') {
            // directive
        }
        else if (IS_DIGIT(c)) {
            parse_numeric_literal();
        }
        else if (IS_SPACE(c)) {
            eat_next_char();        
            keep_up_left_cursor();
        }

    }
    
    printf("END!!!\n");
    
    return 0;
}
