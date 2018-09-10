#!bin/bash

all: lab2_add lab2_list

lab2_add: lab2_add.c
	gcc -lpthread -Wextra -Wall -lrt -g -o lab2_add lab2_add.c

lab2_list: lab2_list.c
	gcc -lpthread -Wextra -Wall -lrt -g -o lab2_list lab2_list.c SortedList.c

tests:
	@./tests.sh

graphs:
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp

clean:
	@rm -f *.o
	@rm -f *.tar.gz
	@rm -f lab2_add
	@rm -f lab2_list
	#@rm -f *.png
	#@rm -f *.csv

dist: all tests graphs
	tar -cvzf lab2a-104629708.tar.gz *.c Makefile README *.h *.csv *.png *.gp tests.sh
