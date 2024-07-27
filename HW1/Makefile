all: filedl_client

filedl_client: filedl_client.o
	gcc -o filedl_client filedl_client.o

filedl_client.o: filedl_client.c
	gcc -c -o filedl_client.o filedl_client.c

clean:
	rm -f filedl_client.o filedl_client