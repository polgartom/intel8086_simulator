.SILENT: run
make: run

build:
	gcc main.c -o main.out -Wall 

run:
	make build
	exec ./main.out ./input/listing_0040_challenge_movs
