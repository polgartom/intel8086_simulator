#include "assembler.h"
#include "new_string.h"
#include "array.h"

#include "lexer.c"
#include "parser.c"
#include "bytecode_builder.c"

int main(int argc, char **argv)
{
    String input = read_entire_file("mock/listing_0039_more_movs.asm");
    tokenize(input);

    parse_tokens();

    build_bytecodes(parser.instructions);

    return 0;
}
