#ifndef _H_SIMULATOR
#define _H_SIMULATOR

#include "sim86.h"

void load_executable(Context *ctx, char *filename);
void run(Context *ctx);

#endif
