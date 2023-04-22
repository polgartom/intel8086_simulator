#include "sim86.h"
#include "decoder.h"
#include "printer.h"
#include "simulator.h"

size_t load_binary_file_to_memory(Context *ctx, char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("[ERROR]: Failed to open %s file. Probably it is not exists.\n", filename);
        assert(0);
    }

    fseek(fp, 0, SEEK_END);
    u32 fsize = ftell(fp);
    rewind(fp);

    assert(fsize+1 <= MAX_BINARY_FILE_SIZE);

    ZERO_MEMORY(ctx->loaded_binary, fsize+1);

    fread(ctx->loaded_binary, fsize, 1, fp);
    fclose(fp);

    return fsize;
}

int main(int argc, char **argv)
{
    if (argc < 2 || STR_LEN(argv[1]) == 0) {
        fprintf(stderr, "No binary file specified!\n");
    }
    printf("\nbinary: %s\n\n", argv[1]);

    Context ctx = {};
    ctx.loaded_binary_size = load_binary_file_to_memory(&ctx, argv[1]);

    decode(&ctx);
    run(&ctx);

    return 0;
}
