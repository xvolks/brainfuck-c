all: bf.c
	cc -O3 -o bf bf.c
debug: bf.c
	cc -g -o bf bf.c
