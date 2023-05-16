#include "sim86.h"
#include "decoder.h"
#include "simulator.h"

int main(int argc, char **argv)
{
    assert(argc > 1);

    CPU cpu = {0};
    boot(&cpu);

    u8 dump_out = 0;

    char *input_filename = NULL;

    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            if (argv[i][0] == '-') {
                if (STR_EQUAL(argv[i], "--dump")) {
                    dump_out = 1;
                }

                if (STR_EQUAL(argv[i], "--decode")) {
                    cpu.decode_only = 1;
                }

                if (STR_EQUAL(argv[i], "--debug")) {
                    // @Todo: The i8086/88 contains the debug flag so later we simulate this too
                    // instead of this boolean
                    cpu.debug_mode = 1;
                }
            } else {
                input_filename = argv[i];
                continue;
            }
        }
    }

    printf("\nbinary: %s\n\n", input_filename);

    boot(&cpu);
    load_executable(&cpu, input_filename);
    run(&cpu);

    if (dump_out) {
        FILE *fp = fopen("memory_dump.data", "w");
        assert(fp != NULL);
        fwrite(cpu.memory, 1, 65556, fp);
    }

    return 0;
}
