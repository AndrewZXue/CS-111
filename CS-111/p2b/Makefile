#!bin/bash

TARGET = SortedList.c SortedList.h lab2_list.c Makefile lab2b_list.csv lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png lab2b_list.gp tests.sh profile.out README

default: lab2_list

lab2_list: lab2_list.c
	@gcc -lpthread -Wextra -Wall -lrt -g -o lab2_list lab2_list.c SortedList.c

tests:
	@./tests.sh

profile: default
	@mv lab2_list lab2_list_new
	@gcc -std=gnu11 -pthread -g -L /u/ch/ugrad/andyzhou/lib -lprofiler -o lab2_list SortedList.c lab2_list.c
	@rm -f ./raw.gprof
	@LD_PRELOAD=/u/ch/ugrad/andyzhou/lib/libprofiler.so.0 CPUPROFILE=raw.gprof ./lab2_list --thread=12 --iterations=1000 --sync=s
	@/usr/bin/pprof --text ./lab2_list raw.gprof > profile.out
	@/usr/bin/pprof --list=thd ./lab2_list raw.gprof >> profile.out
	@mv lab2_list_new lab2_list
	@rm -f ./raw.gprof


graphs:
	gnuplot lab2b_list.gp

clean:
	@rm -f *.o
	@rm -f *.tar.gz
	@rm -f lab2_list
	@rm -f *.prof
	#@rm -f *.png
	#@rm -f *.csv
erase:
	@rm -f *.csv
	@rm -f *.png

dist: erase default tests graphs profile
	tar -cvzf lab2b-104629708.tar.gz $(TARGET)