#!bin/bash


default: lab4b

lab4b: lab4b.c
	@gcc -Wall -lmraa -lm -g -o lab4b lab4b.c

clean:
	@rm lab4b
	@rm -f *.o
	@rm -f *.tar.gz

dist:
	tar -cvzf lab4b-104629708.tar.gz lab4b.c Makefile README