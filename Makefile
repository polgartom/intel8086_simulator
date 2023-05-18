CC = gcc 
#CCFLAGS = -Wall -g -W
CCFLAGS = -g
OPTS_SDL=`sdl-config --cflags --libs`

.PHONY: build release bios biosd

release: CCFLAGS += -O3
release: build

build: 
	$(CC) $(CCFLAGS) $(OPTS_SDL) $(wildcard ./*.c) -o ./build/sim86.out

biosd:
	nasm bios/mybios.asm
	make release
	exec ./build/sim86.out bios/mybios --debug

bios:
	nasm bios/mybios.asm
	make release
	exec ./build/sim86.out bios/mybios > /dev/null


asm:
	$(CC) $(CCFLAGS) $(wildcard ./*.c) -S

make: build