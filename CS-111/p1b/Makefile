.SILENT:

all: client server

client:
		gcc -Wall -lmcrypt -g -o lab1b-client lab1b-client.c

server:
		gcc -Wall -lmcrypt -g -o lab1b-server lab1b-server.c

clean:
		rm -rf lab1b-client lab1b-server *.tar.gz *.i *.s *.o

dist:
		tar -cvzf lab1b-104629708.tar.gz lab1b-client.c lab1b-server.c Makefile my.key README
