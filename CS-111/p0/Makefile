#!bin/bash
all: lab0

lab0: lab0.c
	gcc -Wall -g -o lab0 lab0.c	

check: 
	#functionality check
	@echo "functionality check" > input.txt
	@echo "" > output.txt
	@./lab0 --input=input.txt --output=output.txt 
	@diff -q input.txt output.txt
	if [ $$? -ne 0] ; \
	then \
		echo "Functionality Test Failed, Contents not Correctly Copied"; \
	else \
		echo "Functionality Test Passed"; \
	fi
	@rm input.txt
	@rm output.txt
	#input test
	@echo "input check" > input.txt
	@./lab0 --input=input.txt > /dev/null ; \
	if [ $$? -ne 1] ; \
	then \
		echo "Input Test Failed, Cannot Read Input File" ; \
		exit 1 ; \
	fi
	#invalid argument test
	@./lab0 --randomstuff ; \
	if [ $$? -ne 1] ; \
	then \
		echo "Invalid Argument Test Failed. invalid argument!" ; \
		exit 1 ; \
	fi
	#non-exsiting input file test
	@./lab0 --input=non_exsiting_input.txt > output.txt ; \
	if [ $$? -ne 2] ; \
	then \
		echo "non-exisiting input file handled incorrectly" ; \
		exit 1 ; \
	fi
	@rm output.txt
	#unopenable output file test
	@echo "output file test" > output.txt
	@chmod -w output.txt
	@echo "output file test input" > input.txt
	@./lab0 --input=input.txt --output=output.txt ; \
	if [ $$? -ne 3] ; \
	then \
		echo "unopenable output file handled incorrectly" ; \
		exit 1 ; \
	fi
	@chmod +w output.txt
	@rm input.txt
	@rm output.txt
	#SIGSEGV test
	@echo "SIGSEGV test" > input.txt
	@./lab0 --segfault --catch > /dev/null ; \
	if [ $$? -ne 4] ; \
	then \
		echo "SIGSEGV test failed" ; \
		exit 1 ; \
	fi
	@echo "All tests Finished"
clean:
	@rm -f *.o
	@rm -rf input.txt
	@rm -rf output.txt
	@rm -rf lab0
	@rm -f *.tar.gz

dist:
	tar -cvzf lab0-104629708.tar.gz lab0.c README Makefile *.png
