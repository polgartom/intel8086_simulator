#include "sim86.h"
#include "decoder.h"
#include "simulator.h"

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

    load_executable(&ctx, input_filename);
    run(&ctx);

    if (dump_out) {
        FILE *fp = fopen("memory_dump.data", "w");
        assert(fp != NULL);    
        fwrite(ctx.memory, 1, 65556, fp);
    }

    return 0;
}
