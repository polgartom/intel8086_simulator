typedef enum {
    INST_NONE,

    INST_MOVE,
    INST_ARITHMETIC,
    INST_CONTROL,
    INST_STRING,
    INST_IO,
    INST_FLOW,
    INST_STACK,
    INST_LOGICAL,
    
    INST_COUNT,
} Instruction_Type;

typedef struct {
    
    Instruction_Type inst;
    
} Instruction;

typedef struct {
    Instruction instructions[4096]; // @Incomplete    
    int inst_index;
    
    Token *tokens;
} Parser;

Parser parser; // @XXX: Multi-Thread

inline Token *current_token()
{
    // @Todo: Dynamic tokens size
    if (lexer.ti == 4096-1) return NULL;
    return parser.tokens;    
}

inline void eat_token()
{
    parser.tokens += 1;
}

inline Token *peak_next_token()
{
    // @Todo: Proper error message at assertion
    // @Todo: Dynamic tokens size
    if (lexer.ti == 4096-1) return NULL;
    return parser.tokens+1;
}

inline Token *peak_prev_token()
{
    if (parser.tokens == lexer.tokens) return NULL;
    return parser.tokens-1;
}

void parse_tokens()
{
    parser.tokens = lexer.tokens;

    Token *end = lexer.tokens+lexer.ti;
    Token *t = NULL;
    while ((t = current_token()) && t != end) {
        print_token(t);
        
        eat_token();
    };
}