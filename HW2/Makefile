all: snw_client

filedl_server: snw_client.o
	gcc -o snw_client snw_client.o

filedl_server.o: snw_client.c
	gcc -c -o snw_client.o snw_client.c

clean:
	rm -f snw_client.o snw_client
	