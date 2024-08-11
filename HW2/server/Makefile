all: snw_server

filedl_server: snw_server.o
	gcc -o snw_server snw_server.o

filedl_server.o: snw_server.c
	gcc -c -o snw_server.o snw_server.c

clean:
	rm -f snw_server.o snw_server
	