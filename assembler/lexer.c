#include "assembler.h"
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
    UNKNOWN,
    
    IDENTIFIER,
    REGISTER,
    DIRECTIVE,
    LABEL,
    COMMENT,
    STRING_LITERAL,
    NUMERIC_LITERAL,
    
    HASH_SIGN,
    EXCLAMATION_MARK,
    PLUS_OP,
    MINUS_OP,
    MULTIPLY_OP,
    DIVIDE_OP,
    COMMA,
    SEMICOLON,
    COLON,
    EQUAL,
    COMPARE_OP,
    GREATER_OP,
    GREATER_THAN_EQUAL_OP,
    LESS_OP,
    LESS_THAN_EQUAL_OP,
    LEFT_ROUND_BRACKET,
    RIGHT_ROUND_BRACKET,
    LEFT_BLOCK_BRACKET,
    RIGHT_BLOCK_BRACKET,
    LEFT_CURLY_BRACKET,
    RIGHT_CURLY_BRACKET,
  
    LINE_BREAK,
    
} Token_Type;

typedef struct {
    String     value;
    Token_Type type;
} Token;

typedef struct {
    String str;
    int cl;  
    int cr;
    
    Token tokens[4096]; // @Incomplete
    int ti;
} Lexer;

Lexer lexer; // @XXX: Multi-thread

inline void lexer_add_token(String value, Token_Type type)
{
    // printf("ti: %d ; max_tokens: %lld\n", lexer.ti, ARRAY_SIZE(lexer.tokens));
    assert(lexer.ti != ARRAY_SIZE(lexer.tokens));
    
    // printf("[add_token] -> " SFMT " ; %d ; ti: %d\n", SARG(value), type, lexer.ti); 
    
    lexer.tokens[lexer.ti].value = value;
    lexer.tokens[lexer.ti].type  = type;
    
    lexer.ti += 1;
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
        case '#':   return HASH_SIGN;
        case '!':   return EXCLAMATION_MARK;
        case '+':   return PLUS_OP;
        case '-':   return MINUS_OP;
        case '*':   return MULTIPLY_OP;
        case '/':   return DIVIDE_OP;
        case ',':   return COMMA;
        case ';':   return SEMICOLON;
        case ':':   return COLON;
        case '=':   return EQUAL;
        case '>':   return GREATER_OP;
        case '<':   return LESS_OP;
        case '(':   return LEFT_ROUND_BRACKET;
        case ')':   return RIGHT_ROUND_BRACKET;
        case '[':   return LEFT_BLOCK_BRACKET;
        case ']':   return RIGHT_BLOCK_BRACKET;
        case '{':   return LEFT_CURLY_BRACKET;
        case '}':   return RIGHT_CURLY_BRACKET;
    }

    return UNKNOWN;
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

            Token_Type type = IDENTIFIER;
            if (c == ':') {
                type = LABEL;
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
            lexer_add_token(literal, STRING_LITERAL);

            eat_next_char_and_keep_up_left_cursor(); // step from "
            return;
        }

        eat_next_char();
    }
    
    ASSERT(false, "Invalid, unclosed string literal!");
}

void parse_numeric_literal()
{
    char c = current_char();

    bool is_hex = c == '0' && peak_next_char() == 'x';
    bool is_bin = c == '0' && peak_next_char() == 'b';
    
    while ((c = current_char())) {
        if (IS_DIGIT(c)) {
            eat_next_char();
            continue;
        }
        else if (IS_SPACE(c) || decide_single_char_token_type(c) != UNKNOWN) {
            // @Temporary: this is temporary, so at some point we have to decide which chars can separate the numeric literals
            
            String s = get_cursor_range(true);
            keep_up_left_cursor();
            
            // @Incomplete: More validation
            if (is_hex) ASSERT(s.count > 2, "Invalid hex decimal value -> "SFMT, SARG(s));
            if (is_bin) ASSERT(s.count > 2, "Invalid bin decimal value -> "SFMT, SARG(s));
            
            lexer_add_token(s, NUMERIC_LITERAL);
            
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
    ASSERT(type != UNKNOWN, "UNKNOWN token -> %c", c);

    char next_char  = peak_next_char();
    if (type == EQUAL && next_char == '=') {
        eat_next_char();
        type = COMPARE_OP;
    } 
    else if (type == GREATER_OP && next_char == '=') {
        eat_next_char();
        type = GREATER_THAN_EQUAL_OP;
    } 
    else if (type == LESS_OP && next_char == '=') {
        eat_next_char();
        type = LESS_THAN_EQUAL_OP;
    }
    
    String t = get_cursor_range(false);
    lexer_add_token(t, type);
    
    eat_next_char_and_keep_up_left_cursor();
}

const char *token_type_name_as_cstr(Token_Type type) {
    switch (type) {
        case UNKNOWN:               return XSTR(UNKNOWN);
        case IDENTIFIER:            return XSTR(IDENTIFIER);
        case REGISTER:              return XSTR(REGISTER);
        case DIRECTIVE:             return XSTR(DIRECTIVE);
        case LABEL:                 return XSTR(LABEL);
        case COMMENT:               return XSTR(COMMENT);
        case STRING_LITERAL:        return XSTR(STRING_LITERAL);
        case NUMERIC_LITERAL:       return XSTR(NUMERIC_LITERAL);
        
        case HASH_SIGN:             return XSTR(HASH_SIGN);
        case EXCLAMATION_MARK:      return XSTR(EXCLAMATION_MARK);
        case PLUS_OP:               return XSTR(PLUS_OP);
        case MINUS_OP:              return XSTR(MINUS_OP);
        case MULTIPLY_OP:           return XSTR(MULTIPLY_OP);
        case DIVIDE_OP:             return XSTR(DIVIDE_OP);
        case COMMA:                 return XSTR(COMMA);
        case SEMICOLON:             return XSTR(SEMICOLON);
        case COLON:                 return XSTR(COLON);
        case EQUAL:                 return XSTR(EQUAL);
        case COMPARE_OP:            return XSTR(COMPARE_OP);
        case GREATER_OP:            return XSTR(GREATER_OP);
        case GREATER_THAN_EQUAL_OP: return XSTR(GREATER_THAN_EQUAL_OP);
        case LESS_OP:               return XSTR(LESS_OP);
        case LESS_THAN_EQUAL_OP:    return XSTR(LESS_THAN_EQUAL_OP);
        case LEFT_ROUND_BRACKET:    return XSTR(LEFT_ROUND_BRACKET);
        case RIGHT_ROUND_BRACKET:   return XSTR(RIGHT_ROUND_BRACKET);
        case LEFT_BLOCK_BRACKET:    return XSTR(LEFT_BLOCK_BRACKET);
        case RIGHT_BLOCK_BRACKET:   return XSTR(RIGHT_BLOCK_BRACKET);
        case LEFT_CURLY_BRACKET:    return XSTR(LEFT_CURLY_BRACKET);
        case RIGHT_CURLY_BRACKET:   return XSTR(RIGHT_CURLY_BRACKET);
        
        case LINE_BREAK:            return XSTR(LINE_BREAK);
        
        default:                           assert(0);
    }
    
    return "";
}

void print_token(Token *t)
{
    printf("{type: \"%s\", value: \""SFMT"\"}\n", token_type_name_as_cstr(t->type), SARG(t->value));
}

void dump_tokens_out(Lexer *l)
{
    for (int i = 0; i < l->ti; i++) {
        print_token(l->tokens+i);
    }
}

void tokenize(String input)
{
    lexer.str = input;

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
                if (lexer.ti > 0 && lexer.tokens[lexer.ti-1].type != LINE_BREAK) {
                    keep_up_left_cursor();
                    lexer_add_token(string_create(""), LINE_BREAK);
                }
            }
            eat_next_char_and_keep_up_left_cursor();
        }
        else {
            parse_simple_token();
        }
    }
    
    dump_tokens_out(&lexer);
}