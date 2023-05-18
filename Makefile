CC = gcc 
#CCFLAGS = -Wall -g -W
CCFLAGS = -g
OPTS_SDL=`sdl-config --cflags --libs`

.PHONY: build release bios

release: CCFLAGS += -O3
release: build

build: 
	$(CC) $(CCFLAGS) $(OPTS_SDL) $(wildcard ./*.c) -o ./build/sim86.out

bios:
	nasm bios/mybios.asm
	make build
	exec ./build/sim86.out bios/mybios --debug

asm:
	$(CC) $(CCFLAGS) $(wildcard ./*.c) -S

make: build