#!bin/bash


default: lab3a.c ext2_fs.h
	@gcc -Wall -Wextra -O2 lab3a.c -o lab3a

clean:
	@rm -f *.tar.gz
	@rm -f log.csv
	@rm -f lab3a

dist:
	@tar -czvf lab3a-104629708.tar.gz ext2_fs.h lab3a.c Makefile README