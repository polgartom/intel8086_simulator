#include "assembler.h"
#include "array.h"
#include "new_string.h"

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
    
    String s = string_make_alloc(fsize+1); // +1, because we'll insert a line break to deal with the EOF easier
    s.data[s.count-1] = '\n';
    
    fread(s.data, fsize, 1, fp);
    fclose(fp);
    
    return s;
}

typedef enum {
    T_UNKNOWN,
    
    T_IDENTIFIER,
    T_REGISTER,
    T_DIRECTIVE,
    T_LABEL,
    T_COMMENT,
    T_STRING_LITERAL,
    T_NUMERIC_LITERAL,
    
    T_HASH_SIGN,
    T_EXCLAMATION_MARK,
    T_PLUS_OP,
    T_MINUS_OP,
    T_MULTIPLY_OP,
    T_DIVIDE_OP,
    T_COMMA,
    T_SEMICOLON,
    T_COLON,
    T_EQUAL,
    T_COMPARE_OP,
    T_GREATER_OP,
    T_GREATER_THAN_EQUAL_OP,
    T_LESS_OP,
    T_LESS_THAN_EQUAL_OP,
    T_LEFT_ROUND_BRACKET,
    T_RIGHT_ROUND_BRACKET,
    T_LEFT_BLOCK_BRACKET,
    T_RIGHT_BLOCK_BRACKET,
    T_LEFT_CURLY_BRACKET,
    T_RIGHT_CURLY_BRACKET,
  
    T_LINE_BREAK,
    
} Token_Type;

typedef struct {
    String     value;
    Token_Type type;
} Token;

typedef struct {
    String str;
    int cl;  
    int cr;
    
    Array tokens;
    int ti;
} Lexer;

Lexer lexer; // @XXX: Multi-thread

const char *token_type_name_as_cstr(Token_Type type) {
    switch (type) {
        case T_UNKNOWN:               return XSTR(UNKNOWN);
        case T_IDENTIFIER:            return XSTR(IDENTIFIER);
        case T_REGISTER:              return XSTR(REGISTER);
        case T_DIRECTIVE:             return XSTR(DIRECTIVE);
        case T_LABEL:                 return XSTR(LABEL);
        case T_COMMENT:               return XSTR(COMMENT);
        case T_STRING_LITERAL:        return XSTR(STRING_LITERAL);
        case T_NUMERIC_LITERAL:       return XSTR(NUMERIC_LITERAL);
        
        case T_HASH_SIGN:             return XSTR(T_HASH_SIGN);
        case T_EXCLAMATION_MARK:      return XSTR(T_EXCLAMATION_MARK);
        case T_PLUS_OP:               return XSTR(T_PLUS_OP);
        case T_MINUS_OP:              return XSTR(T_MINUS_OP);
        case T_MULTIPLY_OP:           return XSTR(T_MULTIPLY_OP);
        case T_DIVIDE_OP:             return XSTR(T_DIVIDE_OP);
        case T_COMMA:                 return XSTR(T_COMMA);
        case T_SEMICOLON:             return XSTR(T_SEMICOLON);
        case T_COLON:                 return XSTR(T_COLON);
        case T_EQUAL:                 return XSTR(T_EQUAL);
        case T_COMPARE_OP:            return XSTR(T_COMPARE_OP);
        case T_GREATER_OP:            return XSTR(T_GREATER_OP);
        case T_GREATER_THAN_EQUAL_OP: return XSTR(T_GREATER_THAN_EQUAL_OP);
        case T_LESS_OP:               return XSTR(T_LESS_OP);
        case T_LESS_THAN_EQUAL_OP:    return XSTR(T_LESS_THAN_EQUAL_OP);
        case T_LEFT_ROUND_BRACKET:    return XSTR(T_LEFT_ROUND_BRACKET);
        case T_RIGHT_ROUND_BRACKET:   return XSTR(T_RIGHT_ROUND_BRACKET);
        case T_LEFT_BLOCK_BRACKET:    return XSTR(T_LEFT_BLOCK_BRACKET);
        case T_RIGHT_BLOCK_BRACKET:   return XSTR(T_RIGHT_BLOCK_BRACKET);
        case T_LEFT_CURLY_BRACKET:    return XSTR(T_LEFT_CURLY_BRACKET);
        case T_RIGHT_CURLY_BRACKET:   return XSTR(T_RIGHT_CURLY_BRACKET);
        
        case T_LINE_BREAK:            return XSTR(T_LINE_BREAK);
        
        default:                           assert(0);
    }
    
    return "";
}

void lexer_add_token(String value, Token_Type type)
{
    //printf("[add_token] -> " SFMT " ; %s ; #%d\n", SARG(value), token_type_name_as_cstr(type), lexer.tokens.count); 
    
    Token *token = (Token *)malloc(sizeof(Token));
    token->value = value;
    token->type = type;
    array_add(&lexer.tokens, token);
}

inline void eat_next_char()
{
    lexer.cr += 1;  
}

inline char peak_next_char()
{
    // @Todo: Proper error message at assertion
    assert(lexer.cr+1 <= lexer.str.count);
    return lexer.str.data[lexer.cr+1];
}

inline char current_char()
{
    return lexer.str.data[lexer.cr];
}

inline void keep_up_left_cursor()
{
    lexer.cl = lexer.cr;
}

#define eat_next_char_and_keep_up_left_cursor() { eat_next_char(); keep_up_left_cursor(); }

inline String get_cursor_range(bool closed_interval)
{
    String s = string_advance(lexer.str, lexer.cl);
    s.count = lexer.cl == lexer.cr ? 1 : (lexer.cr - lexer.cl) + (closed_interval ? 0 : 1);
    return s;
}

Token_Type decide_single_char_token_type(char c)
{
    switch (c) {
        case '#':   return T_HASH_SIGN;
        case '!':   return T_EXCLAMATION_MARK;
        case '+':   return T_PLUS_OP;
        case '-':   return T_MINUS_OP;
        case '*':   return T_MULTIPLY_OP;
        case '/':   return T_DIVIDE_OP;
        case ',':   return T_COMMA;
        case ';':   return T_SEMICOLON;
        case ':':   return T_COLON;
        case '=':   return T_EQUAL;
        case '>':   return T_GREATER_OP;
        case '<':   return T_LESS_OP;
        case '(':   return T_LEFT_ROUND_BRACKET;
        case ')':   return T_RIGHT_ROUND_BRACKET;
        case '[':   return T_LEFT_BLOCK_BRACKET;
        case ']':   return T_RIGHT_BLOCK_BRACKET;
        case '{':   return T_LEFT_CURLY_BRACKET;
        case '}':   return T_RIGHT_CURLY_BRACKET;
    }

    return T_UNKNOWN;
}

void parse_identifier(bool expect_label)
{
    char c;
    while ((c = current_char())) {
        // @Temporary: this is temporary, so at some point we have to decide which chars can separate the identifiers

        if (IS_ALNUM(c) && peak_next_char()) {
            eat_next_char();
            continue;
        } else {                        
            if (expect_label && !IS_SPACE(c)) {
                ASSERT(false, "Invalid label -> "SFMT, SARG(get_cursor_range(true)));
            }

            String identifier = get_cursor_range(true);

            Token_Type type = T_IDENTIFIER;
            if (c == ':') {
                type = T_LABEL;
                eat_next_char();
            }

            lexer_add_token(identifier, type);
            keep_up_left_cursor();

            return;
        }
        
        ASSERT(false, "Invalid identifier -> "SFMT, SARG(get_cursor_range(true)));
    }
    
}

void parse_string_literal()
{
    eat_next_char_and_keep_up_left_cursor(); // step from "
    
    bool escaped = false;
    
    char c;
    while ((c = current_char())) {
        if (c == '\\') {
            char next_char = peak_next_char(); 
            ASSERT(next_char, "Unclosed string (escaped)!");
            if (next_char == '\"') {
                eat_next_char();
                escaped = !escaped;
            }
            
            // @Incomplete: Parse encoded stuffs
            
        } else if (c == '\"') {
            ASSERT(!escaped, "Unclosed string (escaped)!");
            
            String literal = get_cursor_range(true);
            lexer_add_token(literal, T_STRING_LITERAL);

            eat_next_char_and_keep_up_left_cursor(); // step from "
            return;
        }

        eat_next_char();
    }
    
    ASSERT(false, "Invalid, unclosed string literal!");
}

void parse_numeric_literal()
{
    // @Todo: Parse signed numbers

    char c = current_char();

    bool is_hex = c == '0' && peak_next_char() == 'x';
    bool is_bin = c == '0' && peak_next_char() == 'b';
    
    while ((c = current_char())) {
        if (IS_DIGIT(c)) {
            eat_next_char();
            continue;
        }
        else if (IS_SPACE(c) || decide_single_char_token_type(c) != T_UNKNOWN) {
            // @Temporary: this is temporary, so at some point we have to decide which chars can separate the numeric literals
            
            String s = get_cursor_range(true);
            keep_up_left_cursor();
            
            // @Incomplete: More validation
            if (is_hex) ASSERT(s.count > 2, "Invalid hex decimal value -> "SFMT, SARG(s));
            if (is_bin) ASSERT(s.count > 2, "Invalid bin decimal value -> "SFMT, SARG(s));
            
            lexer_add_token(s, T_NUMERIC_LITERAL);
            
            return;
        }

        ASSERT(false, "Invalid numeric value -> "SFMT, SARG(get_cursor_range(true)));
    }
}

void parse_comment()
{
    eat_next_char_and_keep_up_left_cursor(); // step from ;

    char c;
    while (c = current_char()) {
        if (c == '\n' || c == '\r') {
            String s = get_cursor_range(true); 

            // Skip all of the line breaks after comment until an instruction not appear           
            do {
                eat_next_char();
            } while (current_char() == '\n' || current_char() == '\r');
            
            keep_up_left_cursor();
            
            // lexer_add_token(string_trim_white(s), COMMENT);
            
            return;
        }
        eat_next_char();
    }
}

void parse_simple_token()
{
    keep_up_left_cursor();
    
    char c          = current_char();
    Token_Type type = decide_single_char_token_type(c);
    ASSERT(type != T_UNKNOWN, "UNKNOWN token -> %c", c);

    // char next_char  = peak_next_char();
    // if (type == EQUAL && next_char == '=') {
    //     eat_next_char();
    //     type = T_COMPARE_OP;
    // } 
    // else if (type == GREATER_OP && next_char == '=') {
    //     eat_next_char();
    //     type = T_GREATER_THAN_EQUAL_OP;
    // } 
    // else if (type == LESS_OP && next_char == '=') {
    //     eat_next_char();
    //     type = T_LESS_THAN_EQUAL_OP;
    // }
    
    String t = get_cursor_range(false);
    lexer_add_token(t, type);
    
    eat_next_char_and_keep_up_left_cursor();
}

void print_token(Token *t)
{
    printf("{type: \"%s\", value: \""SFMT"\"}\n", token_type_name_as_cstr(t->type), SARG(t->value));
}

void dump_tokens_out(Lexer *l)
{
    for (int i = 0; i < l->tokens.count; i++) {
        print_token((Token *)l->tokens.data[i]);
    }
}

void tokenize(String input)
{
    lexer.str    = input;
    lexer.tokens = array_create(64);

    char c;
    while ((c = current_char())) {
        if (IS_ALPHA(c) || c == '_') {
            // identifier, label
            parse_identifier(false);
        }
        else if (c == '%') {
            // directive
            parse_identifier(true);
        }
        else if (c == ';') {
            parse_comment();
        }
        else if (IS_DIGIT(c)) {
            parse_numeric_literal();
        }
        else if (IS_SPACE(c)) {
            if (c == '\n') {
                // Instructions separated by at least one new line
                Token *token = (Token *)lexer.tokens.data[lexer.tokens.count-1];
                if (lexer.tokens.count > 0 && token->type != T_LINE_BREAK) {
                    keep_up_left_cursor();
                    lexer_add_token(string_create(""), T_LINE_BREAK);
                }
            }
            eat_next_char_and_keep_up_left_cursor();
        }
        else {
            parse_simple_token();
        }
    }
    
    // dump_tokens_out(&lexer);
}