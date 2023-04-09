CC = gcc 
CCFLAGS = -Wall -g -W

release: CCFLAGS += -O3
release: build

build: 
	$(CC) $(CCFLAGS) $(wildcard ./*.c) -o sim86.out

make: build

