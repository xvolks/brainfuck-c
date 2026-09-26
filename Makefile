all: bf.c
	cc -o bf bf.c
debug: bf.c
	cc -g -o bf bf.c
