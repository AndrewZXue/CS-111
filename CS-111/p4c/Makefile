#!bin/bash
default: lab4c_tcp lab4c_tls

lab4c_tcp:
	@gcc -Wall -Wextra -lm -g -o lab4c_tcp lab4c_tcp.c 

lab4c_tls:
	@gcc -Wall -Wextra -lm -lssl -lcrypto -g -o lab4c_tls lab4c_tls.c 

clean:
	@rm -f lab4c_tcp 
	@rm -f lab4c_tls 
	@rm -f *.tar.gz *.txt 

dist:
	@tar -cvzf lab4c-104629708.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README
	