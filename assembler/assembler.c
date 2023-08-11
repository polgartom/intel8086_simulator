#include "assembler.h"
#include "new_string.h"

#include "lexer.c"
#include "parser.c"

int main(int argc, char **argv)
{
    String input = read_entire_file("mock/listing_0039_more_movs.asm");
    tokenize(input);

    // parse_tokens();
    
    return 0;
}
