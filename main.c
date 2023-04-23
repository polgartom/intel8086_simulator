#include "sim86.h"
#include "decoder.h"
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
    assert(argc > 1);

    Context ctx = {};
    ZERO_MEMORY(ctx.memory, 1024*1024);
    char *input_filename = NULL;

    u8 dump_out = 0;

    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            if (argv[i][0] == '-') {
                if (STR_EQUAL(argv[i], "--dump")) {
                    dump_out = 1;
                }
            } else {
                input_filename = argv[i];
                continue;
            }
        }
    }

    printf("\nbinary: %s\n\n", input_filename);

    ctx.loaded_binary_size = load_binary_file_to_memory(&ctx, input_filename);

    decode(&ctx);
    run(&ctx);

    if (dump_out) {
        FILE *fp = fopen("memory_dump.data", "w");
        assert(fp != NULL);    
        fwrite(ctx.memory, 1, 65556, fp);
    }

    return 0;
}
