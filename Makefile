all: proj

proj: ece586_proj.c ece586_proj.h
	gcc -g -Wall -o proj ece586_proj.c
