all: filedl_server

filedl_server: filedl_server.o
	gcc -o filedl_server filedl_server.o

filedl_server.o: filedl_server.c
	gcc -c -o filedl_server.o filedl_server.c

clean:
	rm -f filedl_server.o filedl_server