CC = gcc 
#CCFLAGS = -Wall -g -W
CCFLAGS = -g
OPTS_SDL=`sdl-config --cflags --libs`

.PHONY: build release jura bios biosd jurabmp

release: CCFLAGS += -O3
release: build

build: 
	$(CC) $(CCFLAGS) $(OPTS_SDL) $(wildcard ./*.c) -o ./build/sim86.out

jurabmp:
	python3 demo/bmp_to_asm_bin.py demo/jurassic_park_r5_g6_b5.bmp

jura:
	make jurabmp
	nasm bios/jura.asm
	make release
	exec ./build/sim86.out bios/jura > /dev/null


asm:
	$(CC) $(CCFLAGS) $(wildcard ./*.c) -S

make: build