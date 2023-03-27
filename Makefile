.SILENT: run
make: run

build:
	gcc main.c -o main.out -Wall 

build_input_asm:
	nasm ./input/listing_0037_single_register_mov.asm

run:
	make build
	exec ./main.out ./input/listing_0039_more_movs
