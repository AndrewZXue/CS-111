#!bin/bash

all: lab1a

lab1a: lab1a.c
	@gcc -Wall -g -o lab1a lab1a.c

clean:
	@rm -f *.o
	@rm -f *.tar.gz
	@rm -f lab1a
	@#@rm -f lab1a.dSYM

dist:
	tar -cvzf lab1a-104629708.tar.gz lab1a.c Makefile README
